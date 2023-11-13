#include "CombineStlFiles.hpp"

#include "ComplexCore/Filters/ReadStlFileFilter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/ParallelTaskAlgorithm.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace
{
class CombineStlImpl
{
public:
  CombineStlImpl(UInt64Array& destTriArray, Float32Array& destVerticesArray, Float64Array& destFaceNormalsArray, const UInt64Array& inputTriArray, const Float32Array& inputVerticesArray,
                 const Float64Array& inputFaceNormalsArray, usize triTupleOffset, usize vertexTupleOffset, usize faceNormalsTupleOffset)
  : m_DestTriangles(destTriArray)
  , m_DestVertices(destVerticesArray)
  , m_DestFaceNormals(destFaceNormalsArray)
  , m_InputTriangles(inputTriArray)
  , m_InputVertices(inputVerticesArray)
  , m_InputFaceNormals(inputFaceNormalsArray)
  , m_TriTupleOffset(triTupleOffset)
  , m_VerticesTupleOffset(vertexTupleOffset)
  , m_FaceNormalsTupleOffset(faceNormalsTupleOffset)
  {
  }

  ~CombineStlImpl() = default;
  CombineStlImpl(const CombineStlImpl&) = default;
  CombineStlImpl(CombineStlImpl&&) noexcept = default;
  CombineStlImpl& operator=(const CombineStlImpl&) = delete;
  CombineStlImpl& operator=(CombineStlImpl&&) noexcept = delete;

  void operator()() const
  {
    std::copy(m_InputTriangles.begin(), m_InputTriangles.end(), m_DestTriangles.begin() + m_TriTupleOffset);
    std::copy(m_InputVertices.begin(), m_InputVertices.end(), m_DestVertices.begin() + m_VerticesTupleOffset);
    std::copy(m_InputFaceNormals.begin(), m_InputFaceNormals.end(), m_DestFaceNormals.begin() + m_FaceNormalsTupleOffset);
  }

private:
  UInt64Array& m_DestTriangles;
  Float32Array& m_DestVertices;
  Float64Array& m_DestFaceNormals;
  const UInt64Array& m_InputTriangles;
  const Float32Array& m_InputVertices;
  const Float64Array& m_InputFaceNormals;
  usize m_TriTupleOffset;
  usize m_VerticesTupleOffset;
  usize m_FaceNormalsTupleOffset;
};

} // namespace

// -----------------------------------------------------------------------------
CombineStlFiles::CombineStlFiles(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CombineStlFilesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
CombineStlFiles::~CombineStlFiles() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& CombineStlFiles::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> CombineStlFiles::operator()()
{
  DataStructure tempDataStructure;
  for(const auto& dirEntry : std::filesystem::directory_iterator{m_InputValues->StlFilesPath})
  {
    if(getCancel())
    {
      return {};
    }

    const fs::path& stlFilePath = dirEntry.path();
    if(fs::is_regular_file(stlFilePath) && StringUtilities::toLower(stlFilePath.extension().string()) == ".stl")
    {
      ReadStlFileFilter stlFileReader;
      Arguments args;
      args.insertOrAssign(ReadStlFileFilter::k_StlFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(stlFilePath));
      args.insertOrAssign(ReadStlFileFilter::k_TriangleGeometryName_Key, std::make_any<DataPath>(DataPath({stlFilePath.stem().string()})));
      auto executeResult = stlFileReader.execute(tempDataStructure, args);
      if(executeResult.result.invalid())
      {
        return executeResult.result;
      }
    }
  }

  usize totalTriangles = 0;
  usize totalVertices = 0;
  std::vector<TriangleGeom*> stlGeometries;
  for(auto* stlGeom : tempDataStructure.getTopLevelData())
  {
    if(auto* triangleGeom = dynamic_cast<TriangleGeom*>(stlGeom); triangleGeom != nullptr)
    {
      totalTriangles += triangleGeom->getNumberOfFaces();
      totalVertices += triangleGeom->getNumberOfVertices();
      stlGeometries.push_back(triangleGeom);
    }
  }

  // Sort the Goemetries in lexigraphical order based on the name of the geometry which should
  // match the file name. We do this because some file systems do not iterate through the directory
  // contents in lexigraphical order (GitHub CI)
  std::sort(stlGeometries.begin(), stlGeometries.end(), [](const auto& lhs, const auto& rhs) { return lhs->getName() < rhs->getName(); });

  auto& combinedGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleDataContainerName);
  auto* combinedFaceAM = combinedGeom.getFaceAttributeMatrix();
  auto& combinedFaceNormals = m_DataStructure.getDataRefAs<Float64Array>(m_InputValues->FaceNormalsArrayName);
  combinedGeom.resizeFaceList(totalTriangles);
  combinedGeom.resizeVertexList(totalVertices);
  combinedFaceAM->resizeTuples(std::vector<usize>{totalTriangles});

  usize triOffset = 0;
  usize vertexOffset = 0;
  usize triCounter = 0;
  usize faceNormalsOffset = 0;
  INodeGeometry2D::SharedFaceList& triangles = combinedGeom.getFacesRef();
  INodeGeometry0D::SharedVertexList& vertices = combinedGeom.getVerticesRef();
  ParallelTaskAlgorithm taskRunner;
  for(auto* geom : stlGeometries)
  {
    if(getCancel())
    {
      return {};
    }

    INodeGeometry2D::SharedFaceList& curTriangles = geom->getFacesRef();
    for(usize t = 0; t < geom->getNumberOfFaces(); t++)
    {
      curTriangles[3 * t + 0] += triCounter;
      curTriangles[3 * t + 1] += triCounter;
      curTriangles[3 * t + 2] += triCounter;
    }
    triCounter += geom->getNumberOfVertices();
    INodeGeometry0D::SharedVertexList& curVertices = geom->getVerticesRef();
    auto& curFaceNormals = tempDataStructure.getDataRefAs<Float64Array>(geom->getFaceAttributeMatrixDataPath().createChildPath("Face Normals"));

    taskRunner.execute(CombineStlImpl{triangles, vertices, combinedFaceNormals, curTriangles, curVertices, curFaceNormals, triOffset, vertexOffset, faceNormalsOffset});

    triOffset += geom->getNumberOfFaces() * 3;
    vertexOffset += geom->getNumberOfVertices() * 3;
    faceNormalsOffset += curFaceNormals.getSize();
  }
  taskRunner.wait(); // This will spill over if the number of geometries to processes does not divide evenly by the number of threads.

  return {};
}
