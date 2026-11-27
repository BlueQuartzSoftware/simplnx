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
  CombineStlImpl(UInt64ArrayType& destTriArray, Float32ArrayType& destVerticesArray, Float64ArrayType& destFaceNormalsArray, const UInt64ArrayType& inputTriArray, const Float32ArrayType& inputVerticesArray,
                 const Float64ArrayType& inputFaceNormalsArray, usize triTupleOffset, usize vertexTupleOffset, usize faceNormalsTupleOffset)
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
  for(const auto& entry : std::filesystem::directory_iterator{m_InputValues->stl_files_path})
  {
    if(fs::is_regular_file(entry) && string_utilities::ToLower(entry.path().extension().string()) == ext)
    {
      paths.emplace_back(entry);
    }
  }

  // Sort paths so file lists and geometry labels are independent of directory iteration order.
  std::sort(paths.begin(), paths.end());

  auto pCellFeatureAttributeMatrixPath = m_InputValues->triangle_data_container_name.createChildPath(m_InputValues->cell_feature_attribute_matrix_name);
  auto activeArrayPath = pCellFeatureAttributeMatrixPath.createChildPath(m_InputValues->active_array_name);
  auto fileListPath = pCellFeatureAttributeMatrixPath.createChildPath(m_InputValues->file_list_array_name);

  auto activeArray = m_DataStructure.getDataRefAs<UInt8ArrayType>(activeArrayPath);
  activeArray[0] = 0;
  auto fileListStrArray = m_DataStructure.getDataRefAs<StringArray>(fileListPath);

  std::vector<TriangleGeom*> stlGeometries;
  int32 currentIndex = 1;
  m_MessageHandler.sendProgressCount("Reading STL Files", 0, paths.size());
  for(const auto& filePath : paths)
  {
    std::string stlFilePath = filePath.string();
    if(getCancel())
    {
      return {};
    }

    fileListStrArray[currentIndex] = stlFilePath;
    activeArray[currentIndex] = 1;

    ReadStlFileFilter stlFileReader;
    Arguments args;
    args.insertOrAssign(ReadStlFileFilter::k_StlFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(stlFilePath));
    args.insertOrAssign(ReadStlFileFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(DataPath({filePath.stem().string()})));
    auto executeResult = stlFileReader.execute(tempDataStructure, args);
    if(executeResult.result.invalid())
    {
      return executeResult.result;
    }
    stlGeometries.push_back(&tempDataStructure.getDataRefAs<TriangleGeom>(DataPath({filePath.stem().string()})));
    m_MessageHandler.sendProgressCount("Reading STL Files", static_cast<usize>(currentIndex), paths.size(), stlFilePath);
    currentIndex++;
  }

  usize totalTriangles = 0;
  usize totalVertices = 0;
  // Preserve the sorted file read order used by File List.
  for(const auto* triangleGeomPtr : stlGeometries)
  {
    totalTriangles += triangleGeomPtr->getNumberOfFaces();
    totalVertices += triangleGeomPtr->getNumberOfVertices();
  }

  auto& combinedGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->triangle_data_container_name);
  auto* combinedFaceAmPtr = combinedGeom.getFaceAttributeMatrix();
  auto& combinedFaceNormals = m_DataStructure.getDataRefAs<Float64ArrayType>(m_InputValues->face_normals_array_name);
  auto* combinedVertexAmPtr = combinedGeom.getVertexAttributeMatrix();

  // Make sure all arrays and Attribute Matrix are sized correctly.
  Result<> resizeResult = combinedGeom.resizeFaceList(totalTriangles);
  if(resizeResult.invalid())
  {
    return resizeResult;
  }
  resizeResult = combinedGeom.resizeVertexList(totalVertices);
  if(resizeResult.invalid())
  {
    return resizeResult;
  }
  resizeResult = combinedFaceAmPtr->resizeTuples(std::vector<usize>{totalTriangles});
  if(resizeResult.invalid())
  {
    return resizeResult;
  }
  resizeResult = combinedVertexAmPtr->resizeTuples(std::vector<usize>{totalVertices});
  if(resizeResult.invalid())
  {
    return resizeResult;
  }

  usize triOffset = 0;
  usize vertexOffset = 0;
  usize triCounter = 0;
  usize faceNormalsOffset = 0;
  INodeGeometry2D::SharedFaceListType& triangles = combinedGeom.getFacesRef();
  INodeGeometry0D::SharedVertexListType& vertices = combinedGeom.getVerticesRef();
  ParallelTaskAlgorithm taskRunner;
  // Concurrent writes to one out-of-core store are not safe.
  taskRunner.requireArraysInMemory({&triangles, &vertices, &combinedFaceNormals});
  int32 fileIndex = 1;
  usize faceLabelOffset = 0;
  usize vertexLabelOffset = 0;

  m_MessageHandler.sendInfoMessage(fmt::format("Moving final triangle geometry data..."));

  // Loop over each temp geometry and copy the data into the destination geometry
  for(auto* currentGeometryPtr : stlGeometries)
  {
    if(getCancel())
    {
      return {};
    }

    INodeGeometry2D::SharedFaceListType& currentSharedFaceList = currentGeometryPtr->getFacesRef();
    usize currentGeomNumTriangles = currentGeometryPtr->getNumberOfFaces();
    usize currentGeomNumVertices = currentGeometryPtr->getNumberOfVertices();
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

    if(m_InputValues->create_part_numbers)
    {
      // Type checked in preflight; Unsafe acceptable; pointer for speed
      auto* partNumberDataStorePtr = m_DataStructure.getDataAsUnsafe<Int32ArrayType>(m_InputValues->part_number_index_array_path)->getDataStore();
      std::fill(partNumberDataStorePtr->begin() + faceLabelOffset, partNumberDataStorePtr->begin() + faceLabelOffset + currentGeomNumTriangles, fileIndex);
    }

    faceLabelOffset += currentGeomNumTriangles;

    if(m_InputValues->label_vertices)
    {
      // Type checked in preflight; Unsafe acceptable; pointer for speed
      auto* vertexLabelsPtr = m_DataStructure.getDataAsUnsafe<Int32ArrayType>(m_InputValues->vertex_file_index_array_path)->getDataStore();
      std::fill(vertexLabelsPtr->begin() + vertexLabelOffset, vertexLabelsPtr->begin() + vertexLabelOffset + currentGeomNumVertices, fileIndex);
    }
    vertexLabelOffset += currentGeomNumVertices;

    INodeGeometry0D::SharedVertexListType& curVertices = currentGeometryPtr->getVerticesRef();
    auto& curFaceNormals = tempDataStructure.getDataRefAs<Float64ArrayType>(currentGeometryPtr->getFaceAttributeMatrixDataPath().createChildPath("Face Normals"));

    taskRunner.execute(CombineStlImpl{triangles, vertices, combinedFaceNormals, currentSharedFaceList, curVertices, curFaceNormals, triOffset, vertexOffset, faceNormalsOffset});

    triOffset += currentGeomNumTriangles * 3;
    vertexOffset += currentGeomNumVertices * 3;
    faceNormalsOffset += curFaceNormals.getSize();
    fileIndex++;
  }
  taskRunner.wait(); // This will spill over if the number of geometries to processes does not divide evenly by the number of threads.

  // Create the Face Labels Array if the user asked for it.
  if(m_InputValues->create_face_labels)
  {
    auto* faceLabelDataStorePtr = m_DataStructure.getDataRefAs<Int32ArrayType>(m_InputValues->face_label_index_array_path).getDataStore();
    usize numTuples = faceLabelDataStorePtr->getNumberOfTuples();
    AbstractDataStore<int32>* partNumberDataStorePtr = nullptr;
    if(m_InputValues->create_part_numbers)
    {
      partNumberDataStorePtr = m_DataStructure.getDataRefAs<Int32ArrayType>(m_InputValues->part_number_index_array_path).getDataStore();
    }

    for(usize idx = 0; idx < numTuples; idx++)
    {
      faceLabelDataStorePtr->setComponent(idx, 0, 0);
      if(partNumberDataStorePtr != nullptr)
      {
        faceLabelDataStorePtr->setComponent(idx, 1, (*partNumberDataStorePtr)[idx]);
      }
      else
      {
        faceLabelDataStorePtr->setComponent(idx, 1, 1);
      }
    }
  }

  return {};
}
