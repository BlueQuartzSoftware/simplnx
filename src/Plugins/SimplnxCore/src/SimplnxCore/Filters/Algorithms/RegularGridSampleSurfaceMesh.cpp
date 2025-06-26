#include "RegularGridSampleSurfaceMesh.hpp"

#include "SimplnxCore/Filters/Algorithms/SliceTriangleGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

using namespace nx::core;

namespace
{
const std::string k_EdgeAttributeMatrixName = "EdgeAttributeMatrix";
const std::string k_SliceIdsArrayName = "SliceIds";
const std::string k_SliceAttributeMatrixName = "SliceAttributeMatrix";

using BoundingBoxType = std::array<nx::core::Point3Df, 2>;

// ----------------------------------------------------------------------------
// Helper function to check if a point lies inside a polygon using ray-casting
bool pointInPolygon(const EdgeGeom& edgeGeom, const std::vector<IGeometry::MeshIndexType>& edgeIndices, const Point3Df& point, const INodeGeometry0D::SharedVertexList& verts,
                    const INodeGeometry1D::SharedEdgeList& edges)
{
  size_t intersections = 0;
  size_t numEdges = edgeIndices.size();

  std::array<float32, 3> v0 = {0.0f, 0.0f, 0.0f};
  std::array<float32, 3> v1 = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < numEdges; ++i)
  {
    usize edgeId = edgeIndices[i];
    usize v0Idx = edges[edgeId * 2];
    usize v1Idx = edges[edgeId * 2 + 1];

    if(verts[v0Idx * 3 + 1] > verts[v1Idx * 3 + 1])
    {
      v1[0] = verts[v0Idx * 3];
      v1[1] = verts[v0Idx * 3 + 1];
      v1[2] = 0.0f;

      v0[0] = verts[v1Idx * 3];
      v0[1] = verts[v1Idx * 3 + 1];
      v0[2] = 0.0f;
    }
    else
    {
      v0[0] = verts[v0Idx * 3];
      v0[1] = verts[v0Idx * 3 + 1];
      v0[2] = 0.0f;

      v1[0] = verts[v1Idx * 3];
      v1[1] = verts[v1Idx * 3 + 1];
      v1[2] = 0.0f;
    }

    // Check if the ray intersects the edge
    if(point[1] > v0[1] && point[1] <= v1[1] && point[0] <= std::max(v0[0], v1[0]))
    {
      float xIntersection = (point[1] - v0[1]) * (v1[0] - v0[0]) / (v1[1] - v0[1]) + v0[0];
      if(point[0] <= xIntersection)
      {
        intersections++;
      }
    }
  }
  return (intersections % 2) == 1;
}

std::map<int32, BoundingBoxType> ComputeBoundingBox(const std::map<int32_t, std::vector<IGeometry::MeshIndexType>>& partEdgeIndicesMap, const INodeGeometry0D::SharedVertexList& verts,
                                                    const INodeGeometry1D::SharedEdgeList& edges)
{
  std::array<nx::core::Point3Df, 2> edgeVertices;

  std::map<int32, BoundingBoxType> boundingBoxMap;
  for(const auto& entry : partEdgeIndicesMap)
  {
    BoundingBoxType boundingBox = {Point3Df{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()},
                                   Point3Df{std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min()}};

    const auto& edgeIndices = entry.second;
    size_t numEdges = edgeIndices.size();
    for(size_t i = 0; i < numEdges; ++i)
    {
      const IGeometry::MeshIndexType edgeId = edgeIndices[i];
      const IGeometry::MeshIndexType v0Idx = edges[edgeId * 2];
      const IGeometry::MeshIndexType v1Idx = edges[edgeId * 2 + 1];

      edgeVertices[0][0] = verts[v0Idx * 3];
      edgeVertices[0][1] = verts[v0Idx * 3 + 1];
      edgeVertices[0][2] = 0.0f;

      edgeVertices[1][0] = verts[v1Idx * 3];
      edgeVertices[1][1] = verts[v1Idx * 3 + 1];
      edgeVertices[1][2] = 0.0f;

      if(verts[v0Idx * 3] < boundingBox[0][0])
      {
        boundingBox[0][0] = verts[v0Idx * 3];
      }
      if(verts[v0Idx * 3 + 1] < boundingBox[0][1])
      {
        boundingBox[0][1] = verts[v0Idx * 3 + 1];
      }
      if(verts[v0Idx * 3 + 2] < boundingBox[0][2])
      {
        boundingBox[0][2] = verts[v0Idx * 3 + 2];
      }

      if(verts[v1Idx * 3] > boundingBox[1][0])
      {
        boundingBox[1][0] = verts[v1Idx * 3];
      }
      if(verts[v1Idx * 3 + 1] > boundingBox[1][1])
      {
        boundingBox[1][1] = verts[v1Idx * 3 + 1];
      }
      if(verts[v1Idx * 3 + 2] > boundingBox[1][2])
      {
        boundingBox[1][2] = verts[v1Idx * 3 + 2];
      }
    }
    boundingBoxMap.insert(std::make_pair(entry.first, boundingBox));
  }

  return boundingBoxMap;
}

/**
 * @brief
 */
class ProcessSliceImpl
{
public:
  ProcessSliceImpl() = delete;
  ProcessSliceImpl(const ProcessSliceImpl&) = default;

  ProcessSliceImpl(RegularGridSampleSurfaceMesh* filterAlg, const EdgeGeom& edgeGeom, const ImageGeom& imageGeom, int32_t partId, const std::vector<IGeometry::MeshIndexType>& edgeIndices,
                   const std::array<Vec3<float>, 2>& boundingBox, const usize imageGeomIdx)

  : m_FilterAlg(filterAlg)
  , m_EdgeIndices(edgeIndices)
  , m_BoundingBox(boundingBox)
  , m_ImageGeom(imageGeom)
  , m_PartId(partId)
  , m_ImageGeomIdx(imageGeomIdx)
  , m_EdgeGeom(edgeGeom)
  {
  }

  ProcessSliceImpl(ProcessSliceImpl&&) = default;                // Move Constructor is not implemented
  ProcessSliceImpl& operator=(const ProcessSliceImpl&) = delete; // Copy Assignment is not implemented
  ProcessSliceImpl& operator=(ProcessSliceImpl&&) = delete;      // Move Assignment is not implemented

  ~ProcessSliceImpl() = default;

  void operator()() const
  {
    const INodeGeometry0D::SharedVertexList& verts = m_EdgeGeom.getVerticesRef();
    const INodeGeometry1D::SharedEdgeList& edges = m_EdgeGeom.getEdgesRef();

    SizeVec3 minVoxel;
    SizeVec3 maxVoxel;
    m_ImageGeom.computeCellIndex(m_BoundingBox[0], minVoxel);
    m_ImageGeom.computeCellIndex(m_BoundingBox[1], maxVoxel);
    SizeVec3 dimensions = m_ImageGeom.getDimensions();

    std::vector<int32> featureIds(dimensions[0] * dimensions[1]); // Allocate enough for a single slice.

    // Loop ONLY in the bounding box area voxels
    for(usize y = minVoxel[1]; y <= maxVoxel[1]; y++)
    {
      for(usize x = minVoxel[0]; x <= maxVoxel[0]; x++)
      {
        const usize planeIdx = dimensions[0] * y + x;
        Point3Df imagePoint = m_ImageGeom.getCoordsf(m_ImageGeomIdx + planeIdx);
        imagePoint[2] = 0.0f; // Force this plane down to the z=0.0f plane.

        if(imagePoint[0] >= m_BoundingBox[0][0] && imagePoint[0] <= m_BoundingBox[1][0] && imagePoint[1] >= m_BoundingBox[0][1] && imagePoint[1] <= m_BoundingBox[1][1])
        {
          if(pointInPolygon(m_EdgeGeom, m_EdgeIndices, imagePoint, verts, edges))
          {
            featureIds[planeIdx] = m_PartId;
          }
        }
      }
    }
    m_FilterAlg->sendThreadSafeUpdate(featureIds, m_ImageGeomIdx);
  }

private:
  RegularGridSampleSurfaceMesh* m_FilterAlg = nullptr;
  const std::vector<IGeometry::MeshIndexType>& m_EdgeIndices;
  const std::array<Vec3<float>, 2>& m_BoundingBox;
  const ImageGeom m_ImageGeom;
  usize m_ImageGeomIdx;
  int32_t m_PartId = 0;
  const EdgeGeom& m_EdgeGeom;
};

} // namespace

// -----------------------------------------------------------------------------
RegularGridSampleSurfaceMesh::RegularGridSampleSurfaceMesh(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                           RegularGridSampleSurfaceMeshInputValues* inputValues)
: SampleSurfaceMesh(dataStructure, shouldCancel, mesgHandler)
, m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
RegularGridSampleSurfaceMesh::~RegularGridSampleSurfaceMesh() noexcept = default;

// -----------------------------------------------------------------------------
void RegularGridSampleSurfaceMesh::generatePoints(std::vector<Point3Df>& points)
{
  const auto dims = m_InputValues->Dimensions;
  const auto spacing = m_InputValues->Spacing;
  const auto origin = m_InputValues->Origin;

  points.reserve(dims[0] * dims[1] * dims[2]);

  for(int32 k = 0; k < dims[2]; k++)
  {
    const float32 f_k = static_cast<float32>(k) + 0.5f;
    for(int32 j = 0; j < dims[1]; j++)
    {
      const float32 f_j = static_cast<float32>(j) + 0.5f;
      for(int32 i = 0; i < dims[0]; i++)
      {
        const float32 f_i = static_cast<float32>(i) + 0.5f;
        points.emplace_back(f_i * spacing[0] + origin[0], f_j * spacing[1] + origin[1], f_k * spacing[2] + origin[2]);
      }
    }
  }
}

// -----------------------------------------------------------------------------
Result<> RegularGridSampleSurfaceMesh::operator()()
{
  constexpr ChoicesParameter::ValueType k_UserDefinedRange = 1;
  /////////////////////////////////////////////////////////////////////////////
  // Slice the Triangle Geometry
  SliceTriangleGeometryInputValues inputValues;
  inputValues.SliceRange = k_UserDefinedRange;
  inputValues.Zstart = m_InputValues->Origin[2] + (m_InputValues->Spacing[2] * 0.5);
  inputValues.Zend = m_InputValues->Origin[2] + (m_InputValues->Dimensions[2] * m_InputValues->Spacing[2]) + (m_InputValues->Spacing[2] * 0.5);
  inputValues.SliceResolution = m_InputValues->Spacing[2];
  inputValues.HaveRegionIds = true;
  inputValues.CADDataContainerName = m_InputValues->TriangleGeometryPath;
  inputValues.RegionIdArrayPath = m_InputValues->SurfaceMeshPartIdsArrayPath;
  DataPath edgeDataPath({fmt::format(".{}_sliced", m_InputValues->TriangleGeometryPath.getTargetName())});
  inputValues.SliceDataContainerName = edgeDataPath;
  inputValues.EdgeAttributeMatrixName = k_EdgeAttributeMatrixName;   // "EdgeAttributeMatrix";
  inputValues.SliceIdArrayName = k_SliceIdsArrayName;                //"SliceIds";
  inputValues.SliceAttributeMatrixName = k_SliceAttributeMatrixName; //"SliceAttributeMatrix";

  if(Result<> result = nx::core::SliceTriangleGeometry(m_DataStructure, m_MessageHandler, m_ShouldCancel, &inputValues)(); result.invalid())
  {
    return result;
  }

  /////////////////////////////////////////////////////////////////////////////
  // RASTER THE PIXELS BASED ON POINT IN POLYGON
  // Get the Image Geometry that is the sampling Grid
  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryOutputPath);
  FloatVec3 origin = imageGeom.getOrigin();
  FloatVec3 spacing = imageGeom.getSpacing();
  SizeVec3 dimensions = imageGeom.getDimensions();

  // Get the output Feature Ids array
  auto featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath); //->getDataStoreRef();

  DataPath edgeAmPath = edgeDataPath.createChildPath(k_EdgeAttributeMatrixName);
  DataPath partIdsDataPath = edgeAmPath.createChildPath(m_InputValues->SurfaceMeshPartIdsArrayPath.getTargetName());
  const auto& m_PartIds = m_DataStructure.getDataRefAs<Int32Array>(partIdsDataPath);
  std::set<int32> uniquePartIds(m_PartIds.begin(), m_PartIds.end());

  int32 currentSliceId = 0;
  m_TotalSlices = static_cast<int32>((inputValues.Zend - inputValues.Zstart) / inputValues.SliceResolution);
  m_InitialTime = std::chrono::steady_clock::now();
  m_LayerCompleted = 0;

  // Loop over each slice that generated a polygon for the outline of the mesh
  for(float zValue = inputValues.Zstart; zValue <= inputValues.Zend; zValue += inputValues.SliceResolution)
  {
    if(m_ShouldCancel)
    {
      break;
    }
    // m_MessageHandler({IFilter::Message::Type::Info, fmt::format("Raster {}/{}", currentSliceId, totalSlices)});

    // Compute the raw index into the ImageGeometry Cell Data
    nx::core::Point3Df coord = {origin[0] + spacing[0] * 0.5f, origin[1] + spacing[1] * 0.5f, zValue};
    auto possibleIndex = imageGeom.getIndex(coord[0], coord[1], coord[2]);
    if(!possibleIndex.has_value())
    {
      // fmt::print("{} NO Index into Image Geometry for coord {}\n", currentSliceId, fmt::join(coord, ","));
      currentSliceId++;
      continue;
    }
    processSlice(currentSliceId, possibleIndex.value(), uniquePartIds);
    currentSliceId++;

    // Estimate the total time remaining
    const auto now = std::chrono::steady_clock::now();
    m_LayerCompleted++;
    // Compute time/layer ratio
    const auto rate = static_cast<float>(std::chrono::duration_cast<std::chrono::seconds>(now - m_InitialTime).count()) / static_cast<float>(m_LayerCompleted);
    const auto remainingParents = m_TotalSlices - m_LayerCompleted;
    auto minutesRemain = (remainingParents * rate) / 60; // Convert to minutes
    const std::string message = fmt::format("{}/{} Estimated Time Remain: {:.2f} Minutes", m_LayerCompleted, m_TotalSlices, minutesRemain);
    m_MessageHandler({IFilter::Message::Type::Info, message});
  }

  return {};
}

void RegularGridSampleSurfaceMesh::processSlice(int32 m_CurrentSliceId, usize m_ImageGeomIdx, const std::set<int32>& uniquePartIds)
{
  auto& m_ImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryOutputPath);

  m_FeatureIdsDataStorePtr = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);

  DataPath edgeDataPath({fmt::format(".{}_sliced", m_InputValues->TriangleGeometryPath.getTargetName())});

  DataPath edgeAmPath = edgeDataPath.createChildPath(k_EdgeAttributeMatrixName);
  DataPath sliceIdDataPath = edgeAmPath.createChildPath(k_SliceIdsArrayName);
  DataPath partIdsDataPath = edgeAmPath.createChildPath(m_InputValues->SurfaceMeshPartIdsArrayPath.getTargetName());

  const auto& m_EdgeGeom = m_DataStructure.getDataRefAs<EdgeGeom>(edgeDataPath);
  const auto& m_SliceIds = m_DataStructure.getDataRefAs<Int32Array>(sliceIdDataPath);
  const auto& m_PartIds = m_DataStructure.getDataRefAs<Int32Array>(partIdsDataPath);

  usize numEdges = m_EdgeGeom.getNumberOfEdges();

  std::map<int32_t, std::vector<IGeometry::MeshIndexType>> partEdgeIndicesMap;
  // Reserve all polygons for each part
  for(const auto& partId : uniquePartIds)
  {
    partEdgeIndicesMap.insert(std::make_pair(partId, std::vector<IGeometry::MeshIndexType>()));
    partEdgeIndicesMap[partId].reserve(1024);
  }

  SizeVec3 dimensions = m_ImageGeom.getDimensions();
  size_t cellsPerSlice = dimensions[0] * dimensions[1];
  const INodeGeometry0D::SharedVertexList& verts = m_EdgeGeom.getVerticesRef();
  const INodeGeometry1D::SharedEdgeList& edges = m_EdgeGeom.getEdgesRef();

  // Loop over all edges and find the edges that are just for the current SliceId
  // Also sort them into separate containers for PartId.
  for(usize edgeIdx = 0; edgeIdx < numEdges; edgeIdx++)
  {
    if(int32 sliceIndex = m_SliceIds[edgeIdx]; m_CurrentSliceId == sliceIndex)
    {
      int32 partIndex = m_PartIds[edgeIdx];
      partEdgeIndicesMap[partIndex].push_back(edgeIdx);
    }
  }

  if(m_ShouldCancel)
  {
    return;
  }

  std::map<int32, BoundingBoxType> boundingBoxMap = ComputeBoundingBox(partEdgeIndicesMap, verts, edges);

  std::vector<int32> featureIds(cellsPerSlice, 0);

  ParallelTaskAlgorithm taskRunner;
  taskRunner.setParallelizationEnabled(true);
  for(const auto& partId : uniquePartIds)
  {
    taskRunner.execute(ProcessSliceImpl(this, m_EdgeGeom, m_ImageGeom, partId, partEdgeIndicesMap[partId], boundingBoxMap[partId], m_ImageGeomIdx));
  }
  taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.
}

// -----------------------------------------------------------------------------
void RegularGridSampleSurfaceMesh::sendThreadSafeUpdate(const std::vector<int32>& rasterBuffer, usize offset)
{
  // We lock access to the DataArray since I don't think DataArray is thread safe.
  std::lock_guard<std::mutex> lock(m_ProgressMessage_Mutex);
  auto& dataStore = m_FeatureIdsDataStorePtr->getDataStoreRef();
  for(usize idx = 0; idx < rasterBuffer.size(); idx++)
  {
    // Since we have multiple threads updating parts of the current slice, only update if the value is NOT zero.
    if(rasterBuffer[idx] != 0)
    {
      dataStore[offset + idx] = rasterBuffer[idx];
    }
  }
}
