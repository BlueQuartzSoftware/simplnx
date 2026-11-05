#include "IdentifyDuplicateVertices.hpp"

#include "SimplnxCore/Filters/ReverseTriangleWindingFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/Meshing/VertexUtilities.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
IdentifyDuplicateVertices::IdentifyDuplicateVertices(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                     IdentifyDuplicateVerticesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
const std::atomic_bool& IdentifyDuplicateVertices::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> IdentifyDuplicateVertices::operator()()
{
  const auto& geom = m_DataStructure.getDataRefAs<INodeGeometry0D>(m_InputValues->TargetGeometryPath);
  const INodeGeometry2D::SharedVertexList::store_type& verts = geom.getVertices()->getDataStoreRef();

  // Sort Vertices
  MeshingUtilities::SortedVerticesList sortedVerticesList = MeshingUtilities::OrderSharedVertices(m_DataStructure.getDataRefAs<INodeGeometry0D>(m_InputValues->TargetGeometryPath), m_ShouldCancel);
  if(m_ShouldCancel)
  {
    return {};
  }
  if(!MeshingUtilities::HasDuplicateVertices(geom.getVertices()->getDataStoreRef(), sortedVerticesList))
  {
    // no duplicates found
    return {};
  }

  auto& duplicatesMask = m_DataStructure.getDataAs<UInt8Array>(m_InputValues->DuplicatesMaskPath)->getDataStoreRef();
  duplicatesMask.fill(0);
  for(usize i = 1; i < sortedVerticesList.ordering.size(); ++i)
  {
    const IGeometry::MeshIndexType previous = sortedVerticesList.ordering[i - 1] * 3;
    const IGeometry::MeshIndexType current = sortedVerticesList.ordering[i] * 3;
    if(verts[previous] == verts[current] && verts[previous + 1] == verts[current + 1] && verts[previous + 2] == verts[current + 2])
    {
      duplicatesMask[sortedVerticesList.ordering[i]] = 1;
    }
  }

  return {};
}
