#include "FindNRingNeighbors.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"

#include <sstream>

namespace nx::core
{
// -----------------------------------------------------------------------------
FindNRingNeighbors::FindNRingNeighbors(FindNRingNeighborsInputValues* inputValues)
: m_InputValues(inputValues)
{
}

// -----------------------------------------------------------------------------
FindNRingNeighbors::~FindNRingNeighbors() noexcept = default;

// -----------------------------------------------------------------------------
Result<> FindNRingNeighbors::operator()(const IFilter::MessageHandler& /*mesgHandler*/, const std::atomic_bool& shouldCancel)
{
  auto* triangleGeomPtr = m_InputValues->triangle_geom_ptr;
  const auto& triangleId = m_InputValues->triangle_id;

  auto* trianglesArrayPtr = triangleGeomPtr->getFaces();
  auto& triangles = trianglesArrayPtr->getDataStoreRef();
  int32_t err = 0;

  // Get the AbstractDataStore reference for the FaceLabels DataArray
  auto& faceLabels = m_InputValues->face_labels_array->getDataStoreRef();

  // Clear out all the previous triangles.
  m_NRingTriangles.clear();

  // Make sure we have the proper connectivity built
  Result<> result = triangleGeomPtr->findElementsContainingVert(false);
  if(result.invalid())
  {
    return result;
  }
  const INodeGeometry1D::ElementDynamicListType* node2TrianglePtr = triangleGeomPtr->getElementsContainingVert();

  // Figure out these boolean values for a sanity check
  bool check0 = faceLabels[triangleId * 2] == m_InputValues->region_id0 && faceLabels[triangleId * 2 + 1] == m_InputValues->region_id1;
  bool check1 = faceLabels[triangleId * 2 + 1] == m_InputValues->region_id0 && faceLabels[triangleId * 2] == m_InputValues->region_id1;

  if(!check0 && !check1)
  {
    std::stringstream ss;
    ss << "FindNRingNeighbors Seed triangle ID does not have a matching Region ID for " << m_InputValues->region_id0 << " & " << m_InputValues->region_id1 << "\n";
    ss << "Region Ids are: " << faceLabels[m_InputValues->triangle_id * 2] << " & " << faceLabels[m_InputValues->triangle_id * 2 + 1] << "\n";
    return MakeErrorResult(err, ss.str());
  }

  // Add our seed triangle
  m_NRingTriangles.insert(triangleId);

  for(int64_t ring = 0; ring < m_InputValues->ring; ++ring)
  {
    if(shouldCancel)
    {
      return {};
    }

    // Make a copy of the 1 Ring Triangles that we just found so that we can use those triangles as the
    // seed triangles for the 2 Ring triangles
    UniqueFaceIdsTType lcvTriangles(m_NRingTriangles);

    // Now that we have the 1 ring triangles, get the 2 Ring neighbors from that list
    for(auto triangleIdx : lcvTriangles)
    {
      // For each node, get the triangle ids that the node belongs to
      for(int32_t i = 0; i < 3; ++i)
      {
        // Get all the triangles for this Node id
        uint16_t tCount = node2TrianglePtr->getNumberOfElements(triangles[triangleIdx * 3 + i]);
        IGeometry::MeshIndexType* dataPtr = node2TrianglePtr->getElementListPointer(triangles[triangleIdx * 3 + i]);

        // Copy all the triangles into our "2Ring" set which will be the unique set of triangle ids
        for(uint16_t t = 0; t < tCount; ++t)
        {
          IGeometry::MeshIndexType tid = dataPtr[t];
          check0 = faceLabels[tid * 2] == m_InputValues->region_id0 && faceLabels[tid * 2 + 1] == m_InputValues->region_id1;
          check1 = faceLabels[tid * 2 + 1] == m_InputValues->region_id0 && faceLabels[tid * 2] == m_InputValues->region_id1;

          if(check0 || check1)
          {
            m_NRingTriangles.insert(tid);
          }
        }
      }
    }
  }
  return {};
}

// -----------------------------------------------------------------------------
FindNRingNeighbors::UniqueFaceIdsTType FindNRingNeighbors::getNRingTriangles() const
{
  return m_NRingTriangles;
}
} // namespace nx::core
