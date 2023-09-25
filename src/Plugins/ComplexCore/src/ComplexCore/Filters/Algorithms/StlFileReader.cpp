#include "StlFileReader.hpp"

#include "ComplexCore/utils/StlUtilities.hpp"

#include "complex/Common/Range.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

#include <cstdio>
#include <utility>

using namespace complex;

namespace
{

/**
 * @brief The FindUniqueIdsImpl class implements a threaded algorithm that determines the set of
 * unique vertices in the triangle geometry
 */
class FindUniqueIdsImpl
{
public:
  FindUniqueIdsImpl(IGeometry::SharedVertexList& vertex, const std::vector<std::vector<size_t>>& nodesInBin, complex::Int64DataStore& uniqueIds)
  : m_Vertex(vertex)
  , m_NodesInBin(nodesInBin)
  , m_UniqueIds(uniqueIds)
  {
  }

  // -----------------------------------------------------------------------------
  void convert(size_t start, size_t end) const
  {
    for(size_t i = start; i < end; i++)
    {
      for(size_t j = 0; j < m_NodesInBin[i].size(); j++)
      {
        size_t node1 = m_NodesInBin[i][j];
        if(m_UniqueIds[node1] == static_cast<int64_t>(node1))
        {
          for(size_t k = j + 1; k < m_NodesInBin[i].size(); k++)
          {
            size_t node2 = m_NodesInBin[i][k];
            if(m_Vertex[node1 * 3] == m_Vertex[node2 * 3] && m_Vertex[node1 * 3 + 1] == m_Vertex[node2 * 3 + 1] && m_Vertex[node1 * 3 + 2] == m_Vertex[node2 * 3 + 2])
            {
              m_UniqueIds[node2] = node1;
            }
          }
        }
      }
    }
  }

  // -----------------------------------------------------------------------------
  void operator()(const Range& range) const
  {
    convert(range.min(), range.max());
  }

private:
  const IGeometry::SharedVertexList& m_Vertex;
  const std::vector<std::vector<size_t>>& m_NodesInBin;
  complex::Int64DataStore& m_UniqueIds;
};

class StlFileSentinel
{
public:
  StlFileSentinel(FILE* file)
  : m_File(file)
  {
  }
  ~StlFileSentinel()
  {
    std::ignore = std::fclose(m_File);
  }
  StlFileSentinel(const StlFileSentinel&) = delete;            // Copy Constructor Not Implemented
  StlFileSentinel(StlFileSentinel&&) = delete;                 // Move Constructor Not Implemented
  StlFileSentinel& operator=(const StlFileSentinel&) = delete; // Copy Assignment Not Implemented
  StlFileSentinel& operator=(StlFileSentinel&&) = delete;      // Move Assignment Not Implemented

private:
  FILE* m_File = nullptr;
};

std::array<float, 6> CreateMinMaxCoords()
{
  return {std::numeric_limits<float>::max(),  -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
          -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),  -std::numeric_limits<float>::max()};
}

} // End anonymous namespace

StlFileReader::StlFileReader(DataStructure& data, fs::path stlFilePath, const DataPath& geometryPath, const DataPath& faceGroupPath, const DataPath& faceNormalsDataPath, bool scaleOutput,
                             float32 scaleFactor, const std::atomic_bool& shouldCancel)
: m_DataStructure(data)
, m_FilePath(std::move(stlFilePath))
, m_GeometryDataPath(geometryPath)
, m_FaceGroupPath(faceGroupPath)
, m_FaceNormalsDataPath(faceNormalsDataPath)
, m_ScaleOutput(scaleOutput)
, m_ScaleFactor(scaleFactor)
, m_ShouldCancel(shouldCancel)
{
}

StlFileReader::~StlFileReader() noexcept = default;

Result<> StlFileReader::operator()()
{

  m_MinMaxCoords = ::CreateMinMaxCoords();

  // Open File
  FILE* f = std::fopen(m_FilePath.string().c_str(), "rb");
  if(nullptr == f)
  {
    return MakeErrorResult(complex::StlConstants::k_ErrorOpeningFile, "Error opening STL file");
  }
  StlFileSentinel fileSentinel(f); // Will ensure that the file is closed when this method returns

  // Read Header
  std::array<char, complex::StlConstants::k_STL_HEADER_LENGTH> stlHeader = {0};
  int32_t triCount = 0;
  if(std::fread(stlHeader.data(), complex::StlConstants::k_STL_HEADER_LENGTH, 1, f) != 1)
  {
    return MakeErrorResult(complex::StlConstants::k_StlHeaderParseError, "Error reading first 8 bytes of STL header. This can't be good.");
  }

  // Look for the tell-tale signs that the file was written from Magics Materialise
  // If the file was written by Magics as a "Color STL" file then the 2byte int
  // values between each triangle will be NON Zero which will screw up the reading.
  // This NON Zero value does NOT indicate a length but is some sort of color
  // value encoded into the file. Instead of being normal like everyone else and
  // using the STL spec they went off and did their own thing.
  std::string stlHeaderStr(stlHeader.data(), complex::StlConstants::k_STL_HEADER_LENGTH);

  bool magicsFile = false;
  static const std::string k_ColorHeader("COLOR=");
  static const std::string k_MaterialHeader("MATERIAL=");
  if(stlHeaderStr.find(k_ColorHeader) != std::string::npos && stlHeaderStr.find(k_MaterialHeader) != std::string::npos)
  {
    magicsFile = true;
  }
  // Read the number of triangles in the file.
  if(std::fread(&triCount, sizeof(int32_t), 1, f) != 1)
  {
    return MakeErrorResult(complex::StlConstants::k_TriangleCountParseError, "Error reading number of triangles from file. This is bad.");
  }

  TriangleGeom& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_GeometryDataPath);

  triangleGeom.resizeFaceList(triCount);
  triangleGeom.resizeVertexList(triCount * 3);

  using SharedTriList = IGeometry::MeshIndexArrayType;
  using SharedVertList = IGeometry::SharedVertexList;

  SharedTriList& triangles = *(triangleGeom.getFaces());
  SharedVertList& nodes = *(triangleGeom.getVertices());

  Float64Array& faceNormals = m_DataStructure.getDataRefAs<Float64Array>(m_FaceNormalsDataPath);

  // Read the triangles
  constexpr size_t k_StlElementCount = 12;
  std::array<float, k_StlElementCount> fileVert = {0.0F};
  uint16_t attr = 0;
  std::vector<uint8_t> triangleAttributeBuffer(std::numeric_limits<uint16_t>::max()); // Just allocate a buffer of max UINT16 elements
  for(int32_t t = 0; t < triCount; ++t)
  {
    size_t objsRead = std::fread(fileVert.data(), sizeof(float), k_StlElementCount, f); // Read the Triangle
    if(k_StlElementCount != objsRead)
    {
      std::string msg = fmt::format("Error reading Triangle '{}}'. Object Count was {} and should have been {}", t, objsRead, k_StlElementCount);
      return MakeErrorResult(complex::StlConstants::k_TriangleParseError, msg);
    }

    objsRead = std::fread(&attr, sizeof(uint16_t), 1, f); // Read the Triangle Attribute Data length
    if(objsRead != 1)
    {
      std::string msg = fmt::format("Error reading Number of attributes for triangle '{}'. Object Count was {} and should have been 1", t, objsRead);
      return MakeErrorResult(complex::StlConstants::k_AttributeParseError, msg);
    }
    if(attr > 0 && !magicsFile)
    {
      std::ignore = std::fseek(f, static_cast<size_t>(attr), SEEK_CUR); // Skip past the Triangle Attribute data since we don't know how to read it anyways
    }
    if(fileVert[3] < m_MinMaxCoords[0])
    {
      m_MinMaxCoords[0] = fileVert[3];
    }
    if(fileVert[3] > m_MinMaxCoords[1])
    {
      m_MinMaxCoords[1] = fileVert[3];
    }
    if(fileVert[4] < m_MinMaxCoords[2])
    {
      m_MinMaxCoords[2] = fileVert[4];
    }
    if(fileVert[4] > m_MinMaxCoords[3])
    {
      m_MinMaxCoords[3] = fileVert[4];
    }
    if(fileVert[5] < m_MinMaxCoords[4])
    {
      m_MinMaxCoords[4] = fileVert[5];
    }
    if(fileVert[5] > m_MinMaxCoords[5])
    {
      m_MinMaxCoords[5] = fileVert[5];
    }
    if(fileVert[6] < m_MinMaxCoords[0])
    {
      m_MinMaxCoords[0] = fileVert[6];
    }
    if(fileVert[6] > m_MinMaxCoords[1])
    {
      m_MinMaxCoords[1] = fileVert[6];
    }
    if(fileVert[7] < m_MinMaxCoords[2])
    {
      m_MinMaxCoords[2] = fileVert[7];
    }
    if(fileVert[7] > m_MinMaxCoords[3])
    {
      m_MinMaxCoords[3] = fileVert[7];
    }
    if(fileVert[8] < m_MinMaxCoords[4])
    {
      m_MinMaxCoords[4] = fileVert[8];
    }
    if(fileVert[8] > m_MinMaxCoords[5])
    {
      m_MinMaxCoords[5] = fileVert[8];
    }
    if(fileVert[9] < m_MinMaxCoords[0])
    {
      m_MinMaxCoords[0] = fileVert[9];
    }
    if(fileVert[9] > m_MinMaxCoords[1])
    {
      m_MinMaxCoords[1] = fileVert[9];
    }
    if(fileVert[10] < m_MinMaxCoords[2])
    {
      m_MinMaxCoords[2] = fileVert[10];
    }
    if(fileVert[10] > m_MinMaxCoords[3])
    {
      m_MinMaxCoords[3] = fileVert[10];
    }
    if(fileVert[11] < m_MinMaxCoords[4])
    {
      m_MinMaxCoords[4] = fileVert[11];
    }
    if(fileVert[11] > m_MinMaxCoords[5])
    {
      m_MinMaxCoords[5] = fileVert[11];
    }
    faceNormals[3 * t + 0] = static_cast<double>(fileVert[0]);
    faceNormals[3 * t + 1] = static_cast<double>(fileVert[1]);
    faceNormals[3 * t + 2] = static_cast<double>(fileVert[2]);
    nodes[3 * (3 * t + 0) + 0] = fileVert[3];
    nodes[3 * (3 * t + 0) + 1] = fileVert[4];
    nodes[3 * (3 * t + 0) + 2] = fileVert[5];
    nodes[3 * (3 * t + 1) + 0] = fileVert[6];
    nodes[3 * (3 * t + 1) + 1] = fileVert[7];
    nodes[3 * (3 * t + 1) + 2] = fileVert[8];
    nodes[3 * (3 * t + 2) + 0] = fileVert[9];
    nodes[3 * (3 * t + 2) + 1] = fileVert[10];
    nodes[3 * (3 * t + 2) + 2] = fileVert[11];
    triangles[t * 3] = 3 * t + 0;
    triangles[t * 3 + 1] = 3 * t + 1;
    triangles[t * 3 + 2] = 3 * t + 2;
    if(m_ShouldCancel)
    {
      return {};
    }
  }

  return eliminate_duplicate_nodes();
  // The fileSentinel will ensure the FILE* is closed.
}

Result<> StlFileReader::eliminate_duplicate_nodes()
{
  TriangleGeom& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_GeometryDataPath);

  using SharedTriList = IGeometry::MeshIndexArrayType;
  using SharedVertList = IGeometry::SharedVertexList;

  SharedTriList& triangles = *(triangleGeom.getFaces());
  SharedVertList& vertices = *(triangleGeom.getVertices());

  IGeometry::MeshIndexType nNodesAll = triangleGeom.getNumberOfVertices();
  IGeometry::MeshIndexType nTriangles = triangleGeom.getNumberOfFaces();
  size_t nNodes = 0;
  if(nNodesAll > 0)
  {
    nNodes = static_cast<size_t>(nNodesAll);
  }
  float stepX = (m_MinMaxCoords[1] - m_MinMaxCoords[0]) / 100.0f;
  float stepY = (m_MinMaxCoords[3] - m_MinMaxCoords[2]) / 100.0f;
  float stepZ = (m_MinMaxCoords[5] - m_MinMaxCoords[4]) / 100.0f;

  std::vector<std::vector<size_t>> nodesInBin(100 * 100 * 100);

  // determine (xyz) bin each node falls in - used to speed up node comparison
  int32_t bin = 0, xBin = 0, yBin = 0, zBin = 0;
  for(size_t i = 0; i < nNodes; i++)
  {
    if(stepX != 0.0)
    {
      xBin = static_cast<int32_t>((vertices[i * 3] - m_MinMaxCoords[0]) / stepX);
    }
    if(stepY != 0.0)
    {
      yBin = static_cast<int32_t>((vertices[i * 3 + 1] - m_MinMaxCoords[2]) / stepY);
    }
    if(zBin != 0.0)
    {
      zBin = static_cast<int32_t>((vertices[i * 3 + 2] - m_MinMaxCoords[4]) / stepZ);
    }
    if(xBin == 100)
    {
      xBin = 99;
    }
    if(yBin == 100)
    {
      yBin = 99;
    }
    if(zBin == 100)
    {
      zBin = 99;
    }
    bin = (zBin * 10000) + (yBin * 100) + xBin;
    nodesInBin[bin].push_back(i);
  }

  // Create array to hold unique node numbers
  Int64DataStore uniqueIds(IDataStore::ShapeType{nNodes}, IDataStore::ShapeType{1}, {});
  for(IGeometry::MeshIndexType i = 0; i < nNodesAll; i++)
  {
    uniqueIds[i] = static_cast<int64_t>(i);
  }

  // Parallel algorithm to find duplicate nodes
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0ULL, static_cast<size_t>(100 * 100 * 100));
  dataAlg.execute(::FindUniqueIdsImpl(vertices, nodesInBin, uniqueIds));

  // renumber the unique nodes
  int64_t uniqueCount = 0;
  for(size_t i = 0; i < nNodes; i++)
  {
    if(uniqueIds[i] == static_cast<int64_t>(i))
    {
      uniqueIds[i] = uniqueCount;
      uniqueCount++;
    }
    else
    {
      uniqueIds[i] = uniqueIds[uniqueIds[i]];
    }
  }

  float scaleFactor = 1.0F;
  if(m_ScaleOutput)
  {
    scaleFactor = m_ScaleFactor;
  }

  // Move nodes to unique Id and then resize nodes array and apply optional scaling
  for(size_t i = 0; i < nNodes; i++)
  {
    vertices[uniqueIds[i] * 3] = vertices[i * 3] * m_ScaleFactor;
    vertices[uniqueIds[i] * 3 + 1] = vertices[i * 3 + 1] * m_ScaleFactor;
    vertices[uniqueIds[i] * 3 + 2] = vertices[i * 3 + 2] * m_ScaleFactor;
  }
  triangleGeom.resizeVertexList(uniqueCount);

  // Update the triangle nodes to reflect the unique ids
  int64_t node1 = 0, node2 = 0, node3 = 0;
  for(size_t i = 0; i < static_cast<size_t>(nTriangles); i++)
  {
    node1 = triangles[i * 3];
    node2 = triangles[i * 3 + 1];
    node3 = triangles[i * 3 + 2];

    triangles[i * 3] = uniqueIds[node1];
    triangles[i * 3 + 1] = uniqueIds[node2];
    triangles[i * 3 + 2] = uniqueIds[node3];
  }

  triangleGeom.getFaceAttributeMatrix()->resizeTuples({triangleGeom.getNumberOfFaces()});
  triangleGeom.getVertexAttributeMatrix()->resizeTuples({triangleGeom.getNumberOfVertices()});

  return {};
}
