#include "CombineStlFiles.hpp"

#include "SimplnxCore/Filters/ReadStlFileFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <algorithm>
#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
class CombineStlImpl
{
public:
  CombineStlImpl(UInt64Array& destTriArray, Float32Array& destVerticesArray, Float64Array& destFaceNormalsArray, const UInt64Array& inputTriArray, const Float32Array& inputVerticesArray,
                 const Float64Array& inputFaceNormalsArray, usize triTupleOffset, usize vertexTupleOffset, usize faceNormalsTupleOffset)
  : m_DestTriangles(destTriArray.getDataStoreRef())
  , m_DestVertices(destVerticesArray.getDataStoreRef())
  , m_DestFaceNormals(destFaceNormalsArray.getDataStoreRef())
  , m_InputTriangles(inputTriArray.getDataStoreRef())
  , m_InputVertices(inputVerticesArray.getDataStoreRef())
  , m_InputFaceNormals(inputFaceNormalsArray.getDataStoreRef())
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
  AbstractDataStore<uint64>& m_DestTriangles;
  AbstractDataStore<float32>& m_DestVertices;
  AbstractDataStore<float64>& m_DestFaceNormals;
  const AbstractDataStore<uint64>& m_InputTriangles;
  const AbstractDataStore<float32>& m_InputVertices;
  const AbstractDataStore<float64>& m_InputFaceNormals;
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
  std::vector<fs::path> paths;
  const std::string ext(".stl");

  // Just count up the stl files in the directory
  size_t index = 0;
  for(const auto& entry : std::filesystem::directory_iterator{m_InputValues->StlFilesPath})
  {
    if(fs::is_regular_file(entry) && StringUtilities::toLower(entry.path().extension().string()) == ext)
    {
      paths.emplace_back(entry);
    }
  }

  // Sort the paths for something sort of reasonable.
  std::sort(paths.begin(), paths.end());

  auto pCellFeatureAttributeMatrixPath = m_InputValues->TriangleDataContainerName.createChildPath(m_InputValues->CellFeatureAttributeMatrixName);
  auto activeArrayPath = pCellFeatureAttributeMatrixPath.createChildPath(m_InputValues->ActiveArrayName);
  auto fileListPath = pCellFeatureAttributeMatrixPath.createChildPath(m_InputValues->FileListArrayName);

  auto activeArray = m_DataStructure.getDataRefAs<UInt8Array>(activeArrayPath);
  activeArray[0] = 0;
  auto fileListStrArray = m_DataStructure.getDataRefAs<StringArray>(fileListPath);

  int32 currentIndex = 1;
  for(const auto& filePath : paths)
  {
    std::string stlFilePath = filePath.string();
    if(getCancel())
    {
      return {};
    }

    fileListStrArray[currentIndex] = stlFilePath;
    activeArray[currentIndex] = 1;
    m_MessageHandler(IFilter::Message::Type::Info, fmt::format("({}/{}) Reading {}", currentIndex, paths.size(), stlFilePath));
    currentIndex++;

    ReadStlFileFilter stlFileReader;
    Arguments args;
    args.insertOrAssign(ReadStlFileFilter::k_StlFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(stlFilePath));
    args.insertOrAssign(ReadStlFileFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(DataPath({filePath.stem().string()})));
    auto executeResult = stlFileReader.execute(tempDataStructure, args);
    if(executeResult.result.invalid())
    {
      return executeResult.result;
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

  // Sort the Geometries in lexicographical order based on the name of the geometry which should
  // match the file name. We do this because some file systems do not iterate through the directory
  // contents in lexicographical order (GitHub CI)
  std::sort(stlGeometries.begin(), stlGeometries.end(), [](const auto& lhs, const auto& rhs) { return lhs->getName() < rhs->getName(); });

  auto& combinedGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleDataContainerName);
  auto* combinedFaceAM = combinedGeom.getFaceAttributeMatrix();
  auto& combinedFaceNormals = m_DataStructure.getDataRefAs<Float64Array>(m_InputValues->FaceNormalsArrayName);
  auto* combinedVertexAM = combinedGeom.getVertexAttributeMatrix();

  // Make sure all arrays and Attribute Matrix are sized correctly.
  combinedGeom.resizeFaceList(totalTriangles);
  combinedGeom.resizeVertexList(totalVertices);
  combinedFaceAM->resizeTuples(std::vector<usize>{totalTriangles});
  combinedVertexAM->resizeTuples(std::vector<usize>{totalVertices});

  usize triOffset = 0;
  usize vertexOffset = 0;
  usize triCounter = 0;
  usize faceNormalsOffset = 0;
  INodeGeometry2D::SharedFaceList& triangles = combinedGeom.getFacesRef();
  INodeGeometry0D::SharedVertexList& vertices = combinedGeom.getVerticesRef();
  ParallelTaskAlgorithm taskRunner;
  int32 fileIndex = 1;
  usize faceLabelOffset = 0;
  usize vertexLabelOffset = 0;

  m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Moving final triangle geometry data..."));

  // Loop over each temp geometry and copy the data into the destination geometry
  for(auto* currentGeometry : stlGeometries)
  {
    if(getCancel())
    {
      return {};
    }

    INodeGeometry2D::SharedFaceList& currentSharedFaceList = currentGeometry->getFacesRef();
    usize currentGeomNumTriangles = currentGeometry->getNumberOfFaces();
    usize currentGeomNumVertices = currentGeometry->getNumberOfVertices();
    {
      auto& currentSFLStore = currentSharedFaceList.getDataStoreRef();
      for(usize triIndex = 0; triIndex < currentGeomNumTriangles; triIndex++)
      {
        currentSFLStore[3 * triIndex + 0] += triCounter;
        currentSFLStore[3 * triIndex + 1] += triCounter;
        currentSFLStore[3 * triIndex + 2] += triCounter;
      }
      triCounter += currentGeomNumVertices;
    }

    if(m_InputValues->LabelFaces)
    {
      // Type checked in preflight; Unsafe acceptable; pointer for speed
      auto* faceLabelsStore = m_DataStructure.getDataAsUnsafe<Int32Array>(m_InputValues->FaceFileIndexArrayPath)->getDataStore();
      std::fill(faceLabelsStore->begin() + faceLabelOffset, faceLabelsStore->begin() + faceLabelOffset + currentGeomNumTriangles, fileIndex);
    }

    faceLabelOffset += currentGeomNumTriangles;

    if(m_InputValues->LabelVertices)
    {
      // Type checked in preflight; Unsafe acceptable; pointer for speed
      auto* vertexLabels = m_DataStructure.getDataAsUnsafe<Int32Array>(m_InputValues->VertexFileIndexArrayPath)->getDataStore();
      std::fill(vertexLabels->begin() + vertexLabelOffset, vertexLabels->begin() + vertexLabelOffset + currentGeomNumVertices, fileIndex);
    }
    vertexLabelOffset += currentGeomNumVertices;

    INodeGeometry0D::SharedVertexList& curVertices = currentGeometry->getVerticesRef();
    auto& curFaceNormals = tempDataStructure.getDataRefAs<Float64Array>(currentGeometry->getFaceAttributeMatrixDataPath().createChildPath("Face Normals"));

    taskRunner.execute(CombineStlImpl{triangles, vertices, combinedFaceNormals, currentSharedFaceList, curVertices, curFaceNormals, triOffset, vertexOffset, faceNormalsOffset});

    triOffset += currentGeomNumTriangles * 3;
    vertexOffset += currentGeomNumVertices * 3;
    faceNormalsOffset += curFaceNormals.getSize();
    fileIndex++;
  }
  taskRunner.wait(); // This will spill over if the number of geometries to processes does not divide evenly by the number of threads.

  return {};
}
