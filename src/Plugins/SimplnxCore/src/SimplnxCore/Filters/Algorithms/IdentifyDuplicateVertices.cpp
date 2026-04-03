#include "IdentifyDuplicateVertices.hpp"

#include "SimplnxCore/Filters/ReverseTriangleWindingFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/Meshing/VertexUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

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
  if(!MeshingUtilities::HasDuplicateVertices(geom.getVertices()->getDataStoreRef(), sortedVerticesList))
  {
    // no duplicates found
    return {};
  }

  // Leverage ordering assumptions to speed up duplicate checks
  std::array<IGeometry::MeshIndexType, 3> offset;
  switch(sortedVerticesList.axis)
  {
  case MeshingUtilities::AxialAlignment::X: {
    offset = {0, 1, 2};
    break;
  }
  case MeshingUtilities::AxialAlignment::Y: {
    offset = {1, 2, 0};
    break;
  }
  case MeshingUtilities::AxialAlignment::Z: {
    offset = {2, 1, 0};
    break;
  }
  }

  auto& duplicatesMask = m_DataStructure.getDataAs<UInt8Array>(m_InputValues->DuplicatesMaskPath)->getDataStoreRef();
  duplicatesMask.fill(0);

  MessageHelper messageHelper(m_MessageHandler);
  auto progressHelper = messageHelper.createProgressMessageHelper();
  usize totalVertices = sortedVerticesList.ordering.size();
  progressHelper.setMaxProgresss(totalVertices);
  progressHelper.setProgressMessageTemplate("Identify Duplicate Vertices: {:.1f}% Complete");
  auto progressMessenger = progressHelper.createProgressMessenger(std::chrono::milliseconds(1000));

  using VertT = INodeGeometry0D::SharedVertexList::value_type;
  for(usize i = 1; i < sortedVerticesList.ordering.size(); i++)
  {
    const IGeometry::MeshIndexType prevIndex = sortedVerticesList.ordering[i - 1] * 3;
    const IGeometry::MeshIndexType currentIndex = sortedVerticesList.ordering[i] * 3;
    if(std::numeric_limits<VertT>::epsilon() < std::fabs(verts[prevIndex + offset[0]] - verts[currentIndex + offset[0]]))
    {
      // value on first axis is different; proceed to next iteration
      progressMessenger.sendProgressMessage(1);
      continue;
    }
    if(std::numeric_limits<VertT>::epsilon() < std::fabs(verts[prevIndex + offset[1]] - verts[currentIndex + offset[1]]))
    {
      // value on second axis is different; proceed to next iteration
      progressMessenger.sendProgressMessage(1);
      continue;
    }
    if(std::numeric_limits<VertT>::epsilon() < std::fabs(verts[prevIndex + offset[2]] - verts[currentIndex + offset[2]]))
    {
      // value on third axis is different; proceed to next iteration
      progressMessenger.sendProgressMessage(1);
      continue;
    }

    // Duplicate found flag true
    duplicatesMask[sortedVerticesList.ordering[i]] = 1;
    progressMessenger.sendProgressMessage(1);
  }

  return {};
}
