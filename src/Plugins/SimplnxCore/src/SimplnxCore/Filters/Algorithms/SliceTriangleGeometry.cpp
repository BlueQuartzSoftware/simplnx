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

// -----------------------------------------------------------------------------
const std::atomic_bool& SliceTriangleGeometry::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> SliceTriangleGeometry::operator()()
{
  auto& triangle = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->cad_data_container_name);
  Result<> edgesResult = triangle.findEdges(true);
  if(edgesResult.invalid())
  {
    return edgesResult;
  }

  AbstractDataStore<int32>* triRegionIdPtr = nullptr;
  if(m_InputValues->have_region_ids)
  {
    triRegionIdPtr = m_DataStructure.getDataAs<Int32ArrayType>(m_InputValues->region_id_array_path)->getDataStore();
  }
  float zStart = m_InputValues->zstart;
  float zEnd = m_InputValues->zend;

  // The majority of the algorithm to slice the triangle geometry is in this function
  geometry_utilities::SliceTriangleReturnType sliceTriangleResult =
      geometry_utilities::SliceTriangleGeometry(triangle, m_ShouldCancel, m_InputValues->slice_range, zStart, zEnd, m_InputValues->slice_resolution, triRegionIdPtr);

  // Now convert the slicing results into actual SIMPLNX Geometries.
  usize numVerts = sliceTriangleResult.slice_verts.size() / 3;
  usize numEdges = sliceTriangleResult.slice_verts.size() / 6;

  if(numVerts != (2 * numEdges))
  {
    return MakeErrorResult(-62102, fmt::format("Number of sectioned vertices and edges do not make sense.  Number of Vertices: {} and Number of Edges: {}", numVerts, numEdges));
  }

  auto& edgeGeom = m_DataStructure.getDataRefAs<EdgeGeom>(m_InputValues->slice_data_container_name);
  Result<> resizeResult = edgeGeom.resizeVertexList(numVerts);
  if(resizeResult.invalid())
  {
    return resizeResult;
  }
  resizeResult = edgeGeom.resizeEdgeList(numEdges);
  if(resizeResult.invalid())
  {
    return resizeResult;
  }
  INodeGeometry0D::SharedVertexListType& verts = edgeGeom.getVerticesRef();
  INodeGeometry1D::SharedEdgeListType& edges = edgeGeom.getEdgesRef();
  resizeResult = edgeGeom.getVertexAttributeMatrix()->resizeTuples({numVerts});
  if(resizeResult.invalid())
  {
    return resizeResult;
  }
  resizeResult = edgeGeom.getEdgeAttributeMatrix()->resizeTuples({numEdges});
  if(resizeResult.invalid())
  {
    return resizeResult;
  }
  auto& sliceAM = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->slice_data_container_name.createChildPath(m_InputValues->slice_attribute_matrix_name));
  resizeResult = sliceAM.resizeTuples({sliceTriangleResult.number_of_slices});
  if(resizeResult.invalid())
  {
    return resizeResult;
  }

  DataPath edgeAmPath = m_InputValues->slice_data_container_name.createChildPath(m_InputValues->edge_attribute_matrix_name);
  auto& sliceId = m_DataStructure.getDataRefAs<Int32ArrayType>(edgeAmPath.createChildPath(m_InputValues->slice_id_array_name));
  sliceId.fill(0);
  Int32ArrayType* triRegionIdsPtr = nullptr;
  if(m_InputValues->have_region_ids)
  {
    triRegionIdsPtr = m_DataStructure.getDataAs<Int32ArrayType>(edgeAmPath.createChildPath(m_InputValues->region_id_array_path.getTargetName()));
    triRegionIdsPtr->fill(0);
  }

  for(usize i = 0; i < numEdges; i++)
  {
    edges[2 * i] = 2 * i;
    edges[2 * i + 1] = 2 * i + 1;
    verts[3 * (2 * i)] = sliceTriangleResult.slice_verts[3 * (2 * i)];
    verts[3 * (2 * i) + 1] = sliceTriangleResult.slice_verts[3 * (2 * i) + 1];
    verts[3 * (2 * i) + 2] = sliceTriangleResult.slice_verts[3 * (2 * i) + 2];
    verts[3 * (2 * i + 1)] = sliceTriangleResult.slice_verts[3 * (2 * i + 1)];
    verts[3 * (2 * i + 1) + 1] = sliceTriangleResult.slice_verts[3 * (2 * i + 1) + 1];
    verts[3 * (2 * i + 1) + 2] = sliceTriangleResult.slice_verts[3 * (2 * i + 1) + 2];
    sliceId[i] = sliceTriangleResult.slice_ids[i];
    if(m_InputValues->have_region_ids)
    {
      (*triRegionIdsPtr)[i] = sliceTriangleResult.region_ids[i];
    }
  }

  Result<> result = geometry_utilities::EliminateDuplicateNodes<EdgeGeom>(edgeGeom);
  if(result.invalid())
  {
    return result;
  }

  // REMOVE DUPLICATE EDGES FROM THE GENERATED EDGE GEOMETRY
  // Remember to also fix up the sliceIds and regionIds arrays
  using UniqueEdgesType = std::set<std::pair<uint64, uint64>>;
  UniqueEdgesType uniqueEdges;
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
      if(m_InputValues->have_region_ids)
      {
        (*triRegionIdsPtr)[currentEdgeListSize] = (*triRegionIdsPtr)[edgeIdx];
      }
      currentEdgeListSize++;
    }
  }
  if(numEdges != uniqueEdges.size())
  {
    resizeResult = edgeGeom.resizeEdgeList(uniqueEdges.size());
    if(resizeResult.invalid())
    {
      return resizeResult;
    }
    resizeResult = edgeGeom.getEdgeAttributeMatrix()->resizeTuples({uniqueEdges.size()});
    if(resizeResult.invalid())
    {
      return resizeResult;
    }
  }

  return result;
}
