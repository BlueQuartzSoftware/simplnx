#include "FindNRingNeighbors.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"

#include <fmt/format.h>

#include <sstream>
#include <string>

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
Result<> FindNRingNeighbors::operator()(const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel)
{
  auto* triangleGeom = m_InputValues->TriangleGeomPtr;
  auto& triangleId = m_InputValues->TriangleId;

  auto* trianglesArray = triangleGeom->getFaces();
  auto& triangles = trianglesArray->getDataStoreRef();
  int32_t err = 0;

  // Get the AbstractDataStore reference for the FaceLabels DataArray
  auto& faceLabels = m_InputValues->FaceLabelsArray->getDataStoreRef();

  // Clear out all the previous triangles.
  m_NRingTriangles.clear();

  // Make sure we have the proper connectivity built
  err = triangleGeom->findElementsContainingVert(false);
  if(err < 0)
  {
    return MakeErrorResult(err, "Failed to find elements containing vert");
  }
  const INodeGeometry1D::ElementDynamicList* node2TrianglePtr = triangleGeom->getElementsContainingVert();

  // Figure out these boolean values for a sanity check
  bool check0 = faceLabels[triangleId * 2] == m_InputValues->RegionId0 && faceLabels[triangleId * 2 + 1] == m_InputValues->RegionId1;
  bool check1 = faceLabels[triangleId * 2 + 1] == m_InputValues->RegionId0 && faceLabels[triangleId * 2] == m_InputValues->RegionId1;

#if 1
  if(!check0 && !check1)
  {
    std::stringstream ss;
    ss << "FindNRingNeighbors Seed triangle ID does not have a matching Region ID for " << m_InputValues->RegionId0 << " & " << m_InputValues->RegionId1 << "\n";
    ss << "Region Ids are: " << faceLabels[m_InputValues->TriangleId * 2] << " & " << faceLabels[m_InputValues->TriangleId * 2 + 1] << "\n";
    return MakeErrorResult(err, ss.str());
  }
#endif

  // Add our seed triangle
  m_NRingTriangles.insert(triangleId);

  for(int64_t ring = 0; ring < m_InputValues->Ring; ++ring)
  {
    // Make a copy of the 1 Ring Triangles that we just found so that we can use those triangles as the
    // seed triangles for the 2 Ring triangles
    UniqueFaceIds_t lcvTriangles(m_NRingTriangles);

    // Now that we have the 1 ring triangles, get the 2 Ring neighbors from that list
    for(UniqueFaceIds_t::iterator triIter = lcvTriangles.begin(); triIter != lcvTriangles.end(); ++triIter)
    {
      int64_t triangleIdx = *triIter;
      // For each node, get the triangle ids that the node belongs to
      for(int32_t i = 0; i < 3; ++i)
      {
        // Get all the triangles for this Node id
        uint16_t tCount = node2TrianglePtr->getNumberOfElements(triangles[triangleIdx * 3 + i]);
        IGeometry::MeshIndexType* data = node2TrianglePtr->getElementListPointer(triangles[triangleIdx * 3 + i]);

        // Copy all the triangles into our "2Ring" set which will be the unique set of triangle ids
        for(uint16_t t = 0; t < tCount; ++t)
        {
          int64_t tid = data[t];
          check0 = faceLabels[tid * 2] == m_InputValues->RegionId0 && faceLabels[tid * 2 + 1] == m_InputValues->RegionId1;
          check1 = faceLabels[tid * 2 + 1] == m_InputValues->RegionId0 && faceLabels[tid * 2] == m_InputValues->RegionId1;

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
FindNRingNeighbors::UniqueFaceIds_t FindNRingNeighbors::getNRingTriangles() const
{
  return m_NRingTriangles;
}
} // namespace nx::core
