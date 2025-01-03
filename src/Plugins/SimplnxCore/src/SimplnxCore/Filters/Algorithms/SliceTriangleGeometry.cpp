#include "SliceTriangleGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/GeometryUtilities.hpp"

using namespace nx::core;

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

  auto& edgeGeom = m_DataStructure.getDataRefAs<EdgeGeom>(m_InputValues->SliceDataContainerName);
  edgeGeom.resizeVertexList(numVerts);
  edgeGeom.resizeEdgeList(numEdges);
  INodeGeometry0D::SharedVertexList& verts = edgeGeom.getVerticesRef();
  INodeGeometry1D::SharedEdgeList& edges = edgeGeom.getEdgesRef();
  edgeGeom.getVertexAttributeMatrix()->resizeTuples({numVerts});
  edgeGeom.getEdgeAttributeMatrix()->resizeTuples({numEdges});
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

  Result<> result = GeometryUtilities::EliminateDuplicateNodes<EdgeGeom>(edgeGeom);
  if(result.invalid())
  {
    return result;
  }

  // REMOVE DUPLICATE EDGES FROM THE GENERATED EDGE GEOMETRY
  // Remember to also fix up the sliceIds and regionIds arrays
  using UniqueEdges = std::set<std::pair<uint64, uint64>>;
  UniqueEdges uniqueEdges;
  usize currentEdgeListSize = 0;
  for(usize edgeIdx = 0; edgeIdx < numEdges; edgeIdx++)
  {
    uniqueEdges.insert(std::minmax(edges[edgeIdx * 2], edges[edgeIdx * 2 + 1]));
    // Did something get inserted
    if(currentEdgeListSize < uniqueEdges.size())
    {
      edges[currentEdgeListSize * 2] = edges[edgeIdx * 2];
      edges[currentEdgeListSize * 2 + 1] = edges[edgeIdx * 2 + 1];
      sliceId[currentEdgeListSize] = sliceId[edgeIdx];
      if(m_InputValues->HaveRegionIds)
      {
        (*triRegionIds)[currentEdgeListSize] = (*triRegionIds)[edgeIdx];
      }
      currentEdgeListSize++;
    }
  }
  if(numEdges != uniqueEdges.size())
  {
    edgeGeom.resizeEdgeList(uniqueEdges.size());
    edgeGeom.getEdgeAttributeMatrix()->resizeTuples({uniqueEdges.size()});
  }

  return result;
}
