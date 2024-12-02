#include "SliceTriangleGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/GeometryUtilities.hpp"
#include "simplnx/Utilities/IntersectionUtilities.hpp"

using namespace nx::core;

namespace
{
// -----------------------------------------------------------------------------
char RayIntersectsPlane(const float32 d, const nx::core::Point3Df& q, const nx::core::Point3Df& r, nx::core::Point3Df& p)
{
  const float64 rqDelZ = r[2] - q[2];
  const float64 dqDelZ = d - q[2];
  const float64 t = dqDelZ / rqDelZ;
  for(int i = 0; i < 3; i++)
  {
    p[i] = q[i] + (t * (r[i] - q[i]));
  }
  if(t > 0.0 && t < 1.0)
  {
    return '1';
  }
  if(t == 0.0)
  {
    return 'q';
  }
  if(t == 1.0)
  {
    return 'r';
  }

  return '0';
}
} // namespace

// -----------------------------------------------------------------------------
SliceTriangleGeometry::SliceTriangleGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             SliceTriangleGeometryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
SliceTriangleGeometry::~SliceTriangleGeometry() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& SliceTriangleGeometry::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> SliceTriangleGeometry::operator()()
{
  auto& triangle = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->CADDataContainerName);
  int32 err = triangle.findEdges(true);
  if(err < 0)
  {
    return MakeErrorResult(-62101, "Error retrieving the shared edge list");
  }

  AbstractDataStore<int32>* triRegionIdPtr = nullptr;
  if(m_InputValues->HaveRegionIds)
  {
    triRegionIdPtr = m_DataStructure.getDataAs<Int32Array>(m_InputValues->RegionIdArrayPath)->getDataStore();
  }
  float zStart = m_InputValues->Zstart;
  float zEnd = m_InputValues->Zend;

  if(m_InputValues->SliceRange == slice_triangle_geometry::constants::k_FullRange)
  {
    auto boundingBox = triangle.getBoundingBox();
    zStart = boundingBox.getMinPoint()[2];
    zEnd = boundingBox.getMaxPoint()[2];
  }

  // The majority of the algorithm to slice the triangle geometry is in this function
  GeometryUtilities::SliceTriangleReturnType sliceTriangleResult =
      GeometryUtilities::SliceTriangleGeometry(triangle, m_ShouldCancel, m_InputValues->SliceRange, zStart, zEnd, m_InputValues->SliceResolution, triRegionIdPtr);

  // Now convert the slicing results into actual SIMPLNX Geometries.
  usize numVerts = sliceTriangleResult.SliceVerts.size() / 3;
  usize numEdges = sliceTriangleResult.SliceVerts.size() / 6;

  if(numVerts != (2 * numEdges))
  {
    return MakeErrorResult(-62102, fmt::format("Number of sectioned vertices and edges do not make sense.  Number of Vertices: {} and Number of Edges: {}", numVerts, numEdges));
  }

  auto& edge = m_DataStructure.getDataRefAs<EdgeGeom>(m_InputValues->SliceDataContainerName);
  edge.resizeVertexList(numVerts);
  edge.resizeEdgeList(numEdges);
  INodeGeometry0D::SharedVertexList& verts = edge.getVerticesRef();
  INodeGeometry1D::SharedEdgeList& edges = edge.getEdgesRef();
  edge.getVertexAttributeMatrix()->resizeTuples({numVerts});
  edge.getEdgeAttributeMatrix()->resizeTuples({numEdges});
  auto& sliceAM = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->SliceDataContainerName.createChildPath(m_InputValues->SliceAttributeMatrixName));
  sliceAM.resizeTuples({sliceTriangleResult.NumberOfSlices});

  DataPath edgeAmPath = m_InputValues->SliceDataContainerName.createChildPath(m_InputValues->EdgeAttributeMatrixName);
  auto& sliceId = m_DataStructure.getDataRefAs<Int32Array>(edgeAmPath.createChildPath(m_InputValues->SliceIdArrayName));
  sliceId.fill(0);
  Int32Array* triRegionIds = nullptr;
  if(m_InputValues->HaveRegionIds)
  {
    triRegionIds = m_DataStructure.getDataAs<Int32Array>(edgeAmPath.createChildPath(m_InputValues->RegionIdArrayPath.getTargetName()));
    triRegionIds->fill(0);
  }

  for(usize i = 0; i < numEdges; i++)
  {
    edges[2 * i] = 2 * i;
    edges[2 * i + 1] = 2 * i + 1;
    verts[3 * (2 * i)] = sliceTriangleResult.SliceVerts[3 * (2 * i)];
    verts[3 * (2 * i) + 1] = sliceTriangleResult.SliceVerts[3 * (2 * i) + 1];
    verts[3 * (2 * i) + 2] = sliceTriangleResult.SliceVerts[3 * (2 * i) + 2];
    verts[3 * (2 * i + 1)] = sliceTriangleResult.SliceVerts[3 * (2 * i + 1)];
    verts[3 * (2 * i + 1) + 1] = sliceTriangleResult.SliceVerts[3 * (2 * i + 1) + 1];
    verts[3 * (2 * i + 1) + 2] = sliceTriangleResult.SliceVerts[3 * (2 * i + 1) + 2];
    sliceId[i] = sliceTriangleResult.SliceIds[i];
    if(m_InputValues->HaveRegionIds)
    {
      (*triRegionIds)[i] = sliceTriangleResult.RegionIds[i];
    }
  }

  return GeometryUtilities::EliminateDuplicateNodes<EdgeGeom>(edge);
}
