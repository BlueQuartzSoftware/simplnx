#include "RegularGridSampleSurfaceMesh.hpp"

#include "SimplnxCore/Filters/Algorithms/SliceTriangleGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

using namespace nx::core;

namespace
{

// ----------------------------------------------------------------------------
//
inline std::array<nx::core::Point3Df, 2> GetEdgeCoordinates(usize edgeId, const INodeGeometry0D::SharedVertexList& verts, const INodeGeometry1D::SharedEdgeList& edges)
{
  usize v0Idx = edges[edgeId * 2];
  usize v1Idx = edges[edgeId * 2 + 1];
  return {Point3Df{verts[v0Idx * 3], verts[v0Idx * 3 + 1], verts[v0Idx * 3 + 2]}, Point3Df{verts[v1Idx * 3], verts[v1Idx * 3 + 1], verts[v1Idx * 3 + 2]}};
}

// ----------------------------------------------------------------------------
// Helper function to check if a point lies inside a polygon using ray-casting
bool pointInPolygon(const EdgeGeom& edgeGeom, const std::vector<usize>& edgeIndices, const Point3Df& point, const INodeGeometry0D::SharedVertexList& verts,
                    const INodeGeometry1D::SharedEdgeList& edges)
{
  size_t intersections = 0;
  size_t numEdges = edgeIndices.size();
  std::array<nx::core::Point3Df, 2> edgeVertices;

  for(size_t i = 0; i < numEdges; ++i)
  {

    edgeVertices = GetEdgeCoordinates(edgeIndices[i], verts, edges);
    // edgeGeom.getEdgeCoordinates(edgeIndices[i], edgeVertices);

    Point3Df& p1 = edgeVertices[0];
    p1[2] = 0.0f; // Force down to the zero plane
    Point3Df& p2 = edgeVertices[1];
    p2[2] = 0.0f; // Force down to the zero plane

    if(p1[1] > p2[1])
    {
      std::swap(p1, p2);
    }

    // Check if the ray intersects the edge
    if(point[1] > p1[1] && point[1] <= p2[1] && point[0] <= std::max(p1[0], p2[0]))
    {
      float xIntersection = (point[1] - p1[1]) * (p2[0] - p1[0]) / (p2[1] - p1[1]) + p1[0];
      if(point[0] <= xIntersection)
      {
        intersections++;
      }
    }
  }
  return (intersections % 2) == 1;
}

// ----------------------------------------------------------------------------
//
class SampleSurfaceMeshSliceImpl
{
public:
  SampleSurfaceMeshSliceImpl() = delete;
  SampleSurfaceMeshSliceImpl(const SampleSurfaceMeshSliceImpl&) = default;

  SampleSurfaceMeshSliceImpl(RegularGridSampleSurfaceMesh* filterAlg, const EdgeGeom& edgeGeom, int32 currentSliceId, usize imageGeomIdx, const ImageGeom& imageGeom, const Int32Array& sliceIds,
                             Int32Array& featureIds, const std::atomic_bool& shouldCancel)
  : m_FilterAlg(filterAlg)
  , m_EdgeGeom(edgeGeom)
  , m_CurrentSliceId(currentSliceId)
  , m_ImageGeomIdx(imageGeomIdx)
  , m_ImageGeom(imageGeom)
  , m_SliceIds(sliceIds)
  , m_FeatureIds(featureIds)
  , m_ShouldCancel(shouldCancel)
  {
  }
  SampleSurfaceMeshSliceImpl(SampleSurfaceMeshSliceImpl&&) = default;                // Move Constructor Not Implemented
  SampleSurfaceMeshSliceImpl& operator=(const SampleSurfaceMeshSliceImpl&) = delete; // Copy Assignment Not Implemented
  SampleSurfaceMeshSliceImpl& operator=(SampleSurfaceMeshSliceImpl&&) = delete;      // Move Assignment Not Implemented

  ~SampleSurfaceMeshSliceImpl() = default;

  void operator()() const
  {
    auto start = std::chrono::steady_clock::now();
    usize numEdges = m_EdgeGeom.getNumberOfEdges();
    std::vector<usize> edgeIndices;
    edgeIndices.reserve(1024); // Reserve some space in the vector. This is just a guess.
    SizeVec3 dimensions = m_ImageGeom.getDimensions();
    size_t cellsPerSlice = dimensions[0] * dimensions[1];
    const INodeGeometry0D::SharedVertexList& verts = m_EdgeGeom.getVerticesRef();
    const INodeGeometry1D::SharedEdgeList& edges = m_EdgeGeom.getEdgesRef();

    // Loop over all edges and find the edges that are just for the current Slice Id
    for(usize edgeIdx = 0; edgeIdx < numEdges; edgeIdx++)
    {
      int32 sliceIndex = m_SliceIds[edgeIdx];
      if(m_CurrentSliceId == sliceIndex)
      {
        edgeIndices.push_back(edgeIdx);
      }
    }

    if(m_ShouldCancel)
    {
      return;
    }

    std::vector<int32> featureIds(cellsPerSlice, 0);

    // Now that we have the edges that are on this slice, iterate over all
    // voxels on this slice
    for(size_t planeIdx = 0; planeIdx < cellsPerSlice; planeIdx++)
    {
      Point3Df imagePoint = m_ImageGeom.getCoordsf(m_ImageGeomIdx + planeIdx);
      imagePoint[2] = 0.0f; // Force this down to the zero plane.

      if(pointInPolygon(m_EdgeGeom, edgeIndices, imagePoint, verts, edges))
      {
        // featureIds[m_ImageGeomIdx + planeIdx] = 1;
        featureIds[planeIdx] = 1; // Parallel version
      }

      if(m_ShouldCancel)
      {
        return;
      }
    }

    m_FilterAlg->sendThreadSafeUpdate(m_FeatureIds, featureIds, m_ImageGeomIdx);
  }

private:
  RegularGridSampleSurfaceMesh* m_FilterAlg = nullptr;
  ;
  const EdgeGeom& m_EdgeGeom;
  int32 m_CurrentSliceId;
  usize m_ImageGeomIdx;
  const ImageGeom m_ImageGeom;
  const Int32Array& m_SliceIds;
  Int32Array& m_FeatureIds;
  const std::atomic_bool& m_ShouldCancel;
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
const std::atomic_bool& RegularGridSampleSurfaceMesh::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
void RegularGridSampleSurfaceMesh::generatePoints(std::vector<Point3Df>& points)
{
  auto dims = m_InputValues->Dimensions;
  auto spacing = m_InputValues->Spacing;
  auto origin = m_InputValues->Origin;

  points.reserve(dims[0] * dims[1] * dims[2]);

  for(int32 k = 0; k < dims[2]; k++)
  {
    float32 f_k = static_cast<float32>(k) + 0.5f;
    for(int32 j = 0; j < dims[1]; j++)
    {
      float32 f_j = static_cast<float32>(j) + 0.5f;
      for(int32 i = 0; i < dims[0]; i++)
      {
        float32 f_i = static_cast<float32>(i) + 0.5f;
        points.emplace_back(f_i * spacing[0] + origin[0], f_j * spacing[1] + origin[1], f_k * spacing[2] + origin[2]);
      }
    }
  }
}

// -----------------------------------------------------------------------------
Result<> RegularGridSampleSurfaceMesh::operator()()
{
  const ChoicesParameter::ValueType k_UserDefinedRange = 1;
  /////////////////////////////////////////////////////////////////////////////
  // Slice the Triangle Geometry
  SliceTriangleGeometryInputValues inputValues;
  inputValues.SliceRange = k_UserDefinedRange;
  inputValues.Zstart = m_InputValues->Origin[2] + (m_InputValues->Spacing[2] * 0.5);
  inputValues.Zend = m_InputValues->Origin[2] + (m_InputValues->Dimensions[2] * m_InputValues->Spacing[2]) + (m_InputValues->Spacing[2] * 0.5);
  inputValues.SliceResolution = m_InputValues->Spacing[2];
  inputValues.HaveRegionIds = false;
  inputValues.CADDataContainerName = m_InputValues->TriangleGeometryPath;
  // inputValues.RegionIdArrayPath;
  DataPath edgeDataPath({fmt::format(".{}_sliced", m_InputValues->TriangleGeometryPath.getTargetName())});
  inputValues.SliceDataContainerName = edgeDataPath;
  inputValues.EdgeAttributeMatrixName = "EdgeAttributeMatrix";
  inputValues.SliceIdArrayName = "SliceIds";
  inputValues.SliceAttributeMatrixName = "SliceAttributeMatrix";

  Result<> result = nx::core::SliceTriangleGeometry(m_DataStructure, m_MessageHandler, m_ShouldCancel, &inputValues)();
  if(result.invalid())
  {
    return result;
  }

  /////////////////////////////////////////////////////////////////////////////
  // RASTER THE PIXELS BASED ON POINT IN POLYGON
  DataPath edgeAmPath = edgeDataPath.createChildPath(inputValues.EdgeAttributeMatrixName);
  DataPath sliceIdDataPath = edgeAmPath.createChildPath(inputValues.SliceIdArrayName);
  auto& edgeGeom = m_DataStructure.getDataRefAs<EdgeGeom>(edgeDataPath);
  auto& sliceId = m_DataStructure.getDataRefAs<Int32Array>(sliceIdDataPath);

  // Get the Image Geometry that is the sampling Grid
  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryOutputPath);
  FloatVec3 origin = imageGeom.getOrigin();
  FloatVec3 spacing = imageGeom.getSpacing();
  SizeVec3 dimensions = imageGeom.getDimensions();

  // Get the Feature Ids array
  auto featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath); //->getDataStoreRef();

  ParallelTaskAlgorithm taskRunner;
  taskRunner.setParallelizationEnabled(true);

  int32 currentSliceId = 0;
  int32 totalSlices = static_cast<int32>((inputValues.Zend - inputValues.Zstart) / inputValues.SliceResolution);
  // Loop over each slice that generated a polygon for the outline of the mesh
  for(float zValue = inputValues.Zstart; zValue <= inputValues.Zend; zValue += inputValues.SliceResolution)
  {
    if(m_ShouldCancel)
    {
      break;
    }
    m_MessageHandler({IFilter::Message::Type::Info, fmt::format("Raster {}/{}", currentSliceId, totalSlices)});

    // Compute the raw index into the ImageGeometry Cell Data
    nx::core::Point3Df coord = {origin[0] + spacing[0] * 0.5f, origin[1] + spacing[1] * 0.5f, zValue};
    auto possibleIndex = imageGeom.getIndex(coord[0], coord[1], coord[2]);
    if(!possibleIndex.has_value())
    {
      // fmt::print("{} NO Index into Image Geometry for coord {}\n", currentSliceId, fmt::join(coord, ","));
      currentSliceId++;
      continue;
    }

    taskRunner.execute(SampleSurfaceMeshSliceImpl(this, edgeGeom, currentSliceId, possibleIndex.value(), imageGeom, sliceId, featureIds, m_ShouldCancel));

    currentSliceId++;
  }

  taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.

  return {};
}

// -----------------------------------------------------------------------------
void RegularGridSampleSurfaceMesh::sendThreadSafeUpdate(Int32Array& featureIds, const std::vector<int32>& rasterBuffer, usize offset)
{
  // We lock access to the DataArray since I don't think DataArray is thread safe.
  std::lock_guard<std::mutex> lock(m_ProgressMessage_Mutex);
  auto& dataStore = featureIds.getDataStoreRef();
  for(usize idx = 0; idx < rasterBuffer.size(); idx++)
  {
    dataStore[offset + idx] = rasterBuffer[idx];
  }
}
