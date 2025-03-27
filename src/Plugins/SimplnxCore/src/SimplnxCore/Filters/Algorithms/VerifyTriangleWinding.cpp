#include "VerifyTriangleWinding.hpp"

#include "SimplnxCore/Filters/ReverseTriangleWindingFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/Meshing/VertexUtilities.hpp"

#include "Eigen/Core"

using namespace nx::core;

namespace
{
std::pair<usize, bool> FindValidSeed(const int32 targetFeatureId, const TriangleGeom::SharedFaceList::store_type& triangles, const TriangleGeom::SharedVertexList::store_type& verts, const Int32AbstractDataStore& faceLabels)
{
  usize seedFaceIdx = 0;
  float32 xMax = std::numeric_limits<float32>::min();
  float32 avgX = 0.0f;

  // walk the nodes within targetFeature
  const usize triComp = triangles.getNumberOfComponents();
  for(usize i = 0; i++ < faceLabels.getNumberOfTuples(); i++)
  {
    if(faceLabels[(i * 2) + 0] == targetFeatureId || faceLabels[(i * 2) + 1] == targetFeatureId)
    {
      avgX = static_cast<float32>(verts[triangles[i * triComp]] + verts[triangles[(i * triComp) + 1]] + verts[triangles[(i * triComp) + 2]]) / 3.0; // Get X value of all vertices
      if(avgX > xMax)
      {
        xMax = avgX;
        seedFaceIdx = i;
      }
    }
  }
  // Now we have the "right most" triangle based on x component of the centroid of the triangles for this label.

  // Let's now figure out if the normal points generally in the positive or negative X direction.
  Eigen::Vector3f normal;
  const usize vertexIndex = triangles[seedFaceIdx * 3];
  if(faceLabels[(seedFaceIdx * 2) + 0] == targetFeatureId)
  {
    normal = Eigen::Vector3f(verts[vertexIndex + 0], verts[vertexIndex + 1], verts[vertexIndex + 2]).normalized();
  }
  else
  {
    normal = Eigen::Vector3f(verts[vertexIndex + 2], verts[vertexIndex + 1], verts[vertexIndex + 0]).normalized();
  }

  return std::make_pair(seedFaceIdx, normal[0] < 0.0f); // X value of normal
}

Result<> ImplementReversal(const std::vector<bool> reversalVotes, const DataPath& geomPath, const std::function<Result<>(const DataPath&)>& reversalFunction)
{
  if(std::count(reversalVotes.begin(), reversalVotes.end(), true) >= (reversalVotes.size() / 2))
  {
    return reversalFunction(geomPath);
  }

  return{};
}
} // namespace

// -----------------------------------------------------------------------------
VerifyTriangleWinding::VerifyTriangleWinding(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             VerifyTriangleWindingInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
const std::atomic_bool& VerifyTriangleWinding::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> VerifyTriangleWinding::operator()()
{
  {
    // Sort Vertices For Merging
    MeshingUtilities::SortedVerticesList sortedVerticesList = MeshingUtilities::OrderSharedVertices(m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TargetGeometryPath));
    auto& triGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TargetGeometryPath);
    if(MeshingUtilities::HasDuplicateVertices(triGeom.getVertices()->getDataStoreRef(), sortedVerticesList))
    {
      // Remove duplicates
      MeshingUtilities::RemoveDuplicateVertices(triGeom, sortedVerticesList);
    }
    else
    {
      // Sorting here to make ordering implicit rather than maintaining a mapping, feature parity with duplicate removal
      MeshingUtilities::SortVertices(triGeom, sortedVerticesList);
    }
  }

  // Load container, node list, and node data respectively;
  auto& triGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TargetGeometryPath);

  TriangleGeom::SharedFaceList::store_type& triangles = triGeom.getFaces()->getDataStoreRef();
  const TriangleGeom::SharedVertexList::store_type& verts = triGeom.getVertices()->getDataStoreRef();

  // Load double-sided mesh grouping
  Int32AbstractDataStore& faceLabelsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FaceLabelsPath)->getDataStoreRef();

  // Get max group (feature id != 0)
  int32 maxFeature = 0;
  for(int32 i = 0; i < faceLabelsStore.getSize(); i++)
  {
    if(faceLabelsStore[i] > maxFeature)
    {
      maxFeature = faceLabelsStore[i];
    }
  }

  // TODO:
  //  - Revisit reversal system to see if its worth going cluster by cluster rather than overall
  //  - Assess viability of implementing an internal voting system within the cluster

  // Define a voting system for full reversal
  std::vector<bool> reversalVote(maxFeature + 1);

  /**
   * This works by making a map of the edges since a properly wound mesh
   * should have unique edges. The KEY assumption here is that there are NO
   * DUPLICATE VERTICES IN THE MESH, hence the earlier cleanup.
   *
   * This assumption breaks down if more than two triangles share an edge,
   * so we will be going feature by feature to avoid running into "corner-edges"
   * (where three or more features meet).
   *
   * NOTE: no duplicate vertices, means no duplicate edges
   */

  // TODO:
  //  - Add a hint function to `FindValidSeed`, since we may already walk across a valid triangle for the next feature label

  // Walk the features repairing the graph group by group
  usize count = 0;
  for(int32 feature = 1; feature < maxFeature + 1; feature++)
  {
    std::set<std::pair<IGeometry::MeshIndexType, IGeometry::MeshIndexType>> edgeList = {};
    for(usize i = 0; i < faceLabelsStore.getNumberOfTuples(); i++)
    {
      if(faceLabelsStore[i * 2] == feature)
      {
        std::pair<IGeometry::MeshIndexType, IGeometry::MeshIndexType> edge1 = std::make_pair(triangles[(i * 3) + 0], triangles[(i * 3) + 1]);
        std::pair<IGeometry::MeshIndexType, IGeometry::MeshIndexType> edge2 = std::make_pair(triangles[(i * 3) + 1], triangles[(i * 3) + 2]);
        std::pair<IGeometry::MeshIndexType, IGeometry::MeshIndexType> edge3 = std::make_pair(triangles[(i * 3) + 2], triangles[(i * 3) + 0]);

        if(edgeList.find(edge1) != edgeList.end())
        {
          if(faceLabelsStore[(i * 2) + 1] < feature) // already visited
          {
            count++;
            continue;
          }

          // Flip it
          int32 tempValue = faceLabelsStore[i * 2];
          faceLabelsStore[i * 2] = faceLabelsStore[(i * 2) + 1];
          faceLabelsStore[(i * 2) + 1] = tempValue;

          // Edges are now unique by definition
          edgeList.emplace(triangles[(i * 3) + 0], triangles[(i * 3) + 2]);
          edgeList.emplace(triangles[(i * 3) + 2], triangles[(i * 3) + 1]);
          edgeList.emplace(triangles[(i * 3) + 1], triangles[(i * 3) + 0]);

          continue;
        }
        if(edgeList.find(edge2) != edgeList.end())
        {
          if(faceLabelsStore[(i * 2) + 1] < feature) // already visited
          {
            count++;
            continue;
          }

          // Flip it
          int32 tempValue = faceLabelsStore[i * 2];
          faceLabelsStore[i * 2] = faceLabelsStore[(i * 2) + 1];
          faceLabelsStore[(i * 2) + 1] = tempValue;

          // Edges are now unique by definition
          edgeList.emplace(triangles[(i * 3) + 0], triangles[(i * 3) + 2]);
          edgeList.emplace(triangles[(i * 3) + 2], triangles[(i * 3) + 1]);
          edgeList.emplace(triangles[(i * 3) + 1], triangles[(i * 3) + 0]);
          continue;
        }
        if(edgeList.find(edge3) != edgeList.end())
        {
          if(faceLabelsStore[(i * 2) + 1] < feature) // already visited
          {
            count++;
            continue;
          }

          // Flip it
          int32 tempValue = faceLabelsStore[i * 2];
          faceLabelsStore[i * 2] = faceLabelsStore[(i * 2) + 1];
          faceLabelsStore[(i * 2) + 1] = tempValue;

          // Edges are now unique by definition
          edgeList.emplace(triangles[(i * 3) + 0], triangles[(i * 3) + 2]);
          edgeList.emplace(triangles[(i * 3) + 2], triangles[(i * 3) + 1]);
          edgeList.emplace(triangles[(i * 3) + 1], triangles[(i * 3) + 0]);
          continue;
        }

        edgeList.emplace(std::move(edge1));
        edgeList.emplace(std::move(edge2));
        edgeList.emplace(std::move(edge3));

        continue;
      }
      if(faceLabelsStore[(i * 2) + 1] == feature)
      {
        std::pair<IGeometry::MeshIndexType, IGeometry::MeshIndexType> edge1 = std::make_pair(triangles[(i * 3) + 0], triangles[(i * 3) + 2]);
        std::pair<IGeometry::MeshIndexType, IGeometry::MeshIndexType> edge2 = std::make_pair(triangles[(i * 3) + 2], triangles[(i * 3) + 1]);
        std::pair<IGeometry::MeshIndexType, IGeometry::MeshIndexType> edge3 = std::make_pair(triangles[(i * 3) + 1], triangles[(i * 3) + 0]);
        if(edgeList.find(edge1) != edgeList.end())
        {
          if(faceLabelsStore[(i * 2) + 1] < feature) // already visited
          {
            count++;
            continue;
          }

          // Flip it
          int32 tempValue = faceLabelsStore[i * 2];
          faceLabelsStore[i * 2] = faceLabelsStore[(i * 2) + 1];
          faceLabelsStore[(i * 2) + 1] = tempValue;

          // Edges are now unique by definition
          edgeList.emplace(triangles[(i * 3) + 0], triangles[(i * 3) + 1]);
          edgeList.emplace(triangles[(i * 3) + 1], triangles[(i * 3) + 2]);
          edgeList.emplace(triangles[(i * 3) + 2], triangles[(i * 3) + 0]);
          continue;
        }
        if(edgeList.find(edge2) != edgeList.end())
        {
          if(faceLabelsStore[(i * 2) + 1] < feature) // already visited
          {
            count++;
            continue;
          }

          // Flip it
          int32 tempValue = faceLabelsStore[i * 2];
          faceLabelsStore[i * 2] = faceLabelsStore[(i * 2) + 1];
          faceLabelsStore[(i * 2) + 1] = tempValue;

          // Edges are now unique by definition
          edgeList.emplace(triangles[(i * 3) + 0], triangles[(i * 3) + 1]);
          edgeList.emplace(triangles[(i * 3) + 1], triangles[(i * 3) + 2]);
          edgeList.emplace(triangles[(i * 3) + 2], triangles[(i * 3) + 0]);
          continue;
        }
        if(edgeList.find(edge3) != edgeList.end())
        {
          if(faceLabelsStore[(i * 2) + 1] < feature) // already visited
          {
            count++;
            continue;
          }

          // Flip it
          int32 tempValue = faceLabelsStore[i * 2];
          faceLabelsStore[i * 2] = faceLabelsStore[(i * 2) + 1];
          faceLabelsStore[(i * 2) + 1] = tempValue;

          // Edges are now unique by definition
          edgeList.emplace(triangles[(i * 3) + 0], triangles[(i * 3) + 1]);
          edgeList.emplace(triangles[(i * 3) + 1], triangles[(i * 3) + 2]);
          edgeList.emplace(triangles[(i * 3) + 2], triangles[(i * 3) + 0]);
          continue;
        }

        edgeList.emplace(std::move(edge1));
        edgeList.emplace(std::move(edge2));
        edgeList.emplace(std::move(edge3));

        continue;
      }
    }

    // Find baseline/seed node (correct winding)
   std::pair<usize, bool> seedResult = ::FindValidSeed(feature, triangles, verts, faceLabelsStore);

    reversalVote[feature] = seedResult.second;
  }

  // TODO:
  //  - Add a warning for count here

  // Define a capturing lambda to execute filter without passing member variables to free functions
  const std::function<Result<>(const DataPath&)> f_ExecuteReverseTriangleWinding = [this](const DataPath& triGeomPath) -> Result<> {
    const ReverseTriangleWindingFilter filter;

    Arguments args;

    args.insertOrAssign(ReverseTriangleWindingFilter::k_TriGeomPath_Key, std::make_any<DataPath>(triGeomPath));

    auto preflightResult = filter.preflight(m_DataStructure, args, m_MessageHandler, m_ShouldCancel);
    if(preflightResult.outputActions.invalid())
    {
      return ConvertResult(std::move(preflightResult.outputActions));
    }

    auto executeResult = filter.execute(m_DataStructure, args, nullptr, m_MessageHandler, m_ShouldCancel);
    if(executeResult.result.invalid())
    {
      return executeResult.result;
    }

    return {};
  };

  Result<> result = ::ImplementReversal(reversalVote, m_InputValues->TargetGeometryPath, f_ExecuteReverseTriangleWinding);
  if(result.invalid())
  {
    return result;
  }

  // TODO:
  //  Verify all labels have been visited by here

  return {};
}
