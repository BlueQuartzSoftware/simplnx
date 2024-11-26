#include "RegularGridSampleSurfaceMesh.hpp"

#include "SimplnxCore/Filters/Algorithms/SliceTriangleGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"

using namespace nx::core;

namespace
{
// ----------------------------------------------------------------------------
//
inline std::array<nx::core::Point3Df, 2> GetEdgeCoordinates(usize edgeId, INodeGeometry0D::SharedVertexList& verts, INodeGeometry1D::SharedEdgeList& edges)
{
  usize v0Idx = edges[edgeId * 2];
  usize v1Idx = edges[edgeId * 2 + 1];
  return {Point3Df{verts[v0Idx * 3], verts[v0Idx * 3 + 1], verts[v0Idx * 3 + 2]}, Point3Df{verts[v1Idx * 3], verts[v1Idx * 3 + 1], verts[v1Idx * 3 + 2]}};
}

// ----------------------------------------------------------------------------
// Helper function to check if a point lies inside a polygon using ray-casting
bool pointInPolygon(const EdgeGeom& edgeGeom, const std::vector<usize>& edgeIndices, const Point3Df& point, INodeGeometry0D::SharedVertexList& verts, INodeGeometry1D::SharedEdgeList& edges)
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

  //  SampleSurfaceMeshInputValues inputs;
  //  inputs.TriangleGeometryPath = m_InputValues->TriangleGeometryPath;
  //  inputs.SurfaceMeshFaceLabelsArrayPath = m_InputValues->SurfaceMeshFaceLabelsArrayPath;
  //  inputs.FeatureIdsArrayPath = m_InputValues->FeatureIdsArrayPath;
  //  return execute(inputs);

  // Slice the Triangle Geometry
  SliceTriangleGeometryInputValues inputValues;
  inputValues.SliceRange = 1;
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

  DataPath edgeAmPath = edgeDataPath.createChildPath(inputValues.EdgeAttributeMatrixName);
  DataPath sliceIdDataPath = edgeAmPath.createChildPath(inputValues.SliceIdArrayName);
  auto& edgeGeom = m_DataStructure.getDataRefAs<EdgeGeom>(edgeDataPath);
  auto& sliceId = m_DataStructure.getDataRefAs<Int32Array>(sliceIdDataPath);
  INodeGeometry0D::SharedVertexList& verts = edgeGeom.getVerticesRef();
  INodeGeometry1D::SharedEdgeList& edges = edgeGeom.getEdgesRef();
  usize numEdges = edgeGeom.getNumberOfEdges();

  // Get the Image Geometry that is the sampling Grid
  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryOutputPath);
  FloatVec3 origin = imageGeom.getOrigin();
  FloatVec3 spacing = imageGeom.getSpacing();
  SizeVec3 dimensions = imageGeom.getDimensions();
  size_t cellsPerSlice = dimensions[0] * dimensions[1];

  // Get the Feature Ids array
  auto& featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();

  std::vector<usize> edgeIndices;
  edgeIndices.reserve(1024); // Reserve some space in the vector. This is just a guess.

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

    // We should probably parallelize over each slice
    // Loop over all edges and find the edges that are just for the current Slice Id
    for(usize edgeIdx = 0; edgeIdx < numEdges; edgeIdx++)
    {
      int32 sliceIndex = sliceId[edgeIdx];
      if(currentSliceId == sliceIndex)
      {
        edgeIndices.push_back(edgeIdx);
      }
    }

    // Now that we have the edges that are on this slice, iterate over all
    // voxels on this slice
    size_t imageGeomIdx = possibleIndex.value();
    int32 hitCount = 0;
    for(size_t planeIdx = 0; planeIdx < cellsPerSlice; planeIdx++)
    {
      Point3Df imagePoint = imageGeom.getCoordsf(imageGeomIdx + planeIdx);
      imagePoint[2] = 0.0f; // Force this down to the zero plane.

      if(pointInPolygon(edgeGeom, edgeIndices, imagePoint, verts, edges))
      {
        featureIds[imageGeomIdx + planeIdx] = 1;
        hitCount++;
      }
    }
    // fmt::print("[{}]    edgeIndices.size(): {}  Voxels in Polygon: {}\n", currentSliceId, edgeIndices.size(), hitCount);
    edgeIndices.clear();
    edgeIndices.reserve(1024);
    currentSliceId++;
  }
  return {};
}
