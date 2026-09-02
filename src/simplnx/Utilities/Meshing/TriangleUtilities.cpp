#include "TriangleUtilities.hpp"

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <algorithm>
#include <chrono>
#include <queue>

using namespace nx::core;

namespace
{
using EdgeListT = std::set<std::pair<IGeometry::MeshIndexType, IGeometry::MeshIndexType>>;

Result<> ProcessWindingsWithLabels(INodeGeometry2D::SharedFaceList::store_type& triangles, const DynamicListArray<uint16, IGeometry::MeshIndexType>& neighbors,
                                   const Int32AbstractDataStore& faceLabelsStore, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler, int32 maxFeature)
{
  /**
   * This works by making a map of the edges since a properly wound mesh
   * should have unique edges. The KEY assumption here is that there are NO
   * DUPLICATE VERTICES IN THE MESH, hence the earlier validation.
   *
   * This assumption breaks down if more than two triangles share an edge,
   * so we will be going feature by feature to avoid running into "corner-edges"
   * (where three or more features meet).
   *
   * NOTE: no duplicate vertices, means no duplicate edges
   */

  // Walk the features repairing the graph group by group
  usize count = 0;
  auto start = std::chrono::steady_clock::now();
  const usize numTuples = faceLabelsStore.getNumberOfTuples();
  std::vector<bool> visited(faceLabelsStore.getNumberOfTuples(), false);
  std::vector<bool> unmodified(faceLabelsStore.getNumberOfTuples(), false);

  // Precompute the first triangle carrying each feature label in a single pass. The original code
  // re-scanned every triangle for each feature to find a seed, which is O(features * triangles) and
  // dominates the runtime on large meshes; this makes seeding O(1) per feature.
  constexpr IGeometry::MeshIndexType k_NoSeed = std::numeric_limits<IGeometry::MeshIndexType>::max();
  std::vector<IGeometry::MeshIndexType> firstTriOfFeature(static_cast<usize>(maxFeature) + 1, k_NoSeed);
  for(usize i = 0; i < numTuples; i++)
  {
    for(usize c = 0; c < 2; c++)
    {
      const int32 lbl = faceLabelsStore[(i * 2) + c];
      if(lbl >= 1 && lbl <= maxFeature && firstTriOfFeature[static_cast<usize>(lbl)] == k_NoSeed)
      {
        firstTriOfFeature[static_cast<usize>(lbl)] = i;
      }
    }
  }

  for(int32 feature = 1; feature < maxFeature + 1; feature++)
  {
    std::queue<IGeometry::MeshIndexType> searchTargets = {};

    // process base case: seed from the first triangle carrying this feature (O(1) lookup)
    const IGeometry::MeshIndexType seedTri = firstTriOfFeature[static_cast<usize>(feature)];
    if(seedTri != k_NoSeed)
    {
      auto numElem = neighbors.getNumberOfElements(seedTri);
      const IGeometry::MeshIndexType* neighborListPtr = neighbors.getElementListPointer(seedTri);

      for(uint16 element = 0; element < numElem; element++)
      {
        const usize neighbor = neighborListPtr[element];
        if(faceLabelsStore[neighbor * 2] != feature && faceLabelsStore[(neighbor * 2) + 1] != feature)
        {
          continue;
        }
        searchTargets.push(neighbor);
      }

      visited[seedTri] = true;
    }

    // begin mass search
    while(!searchTargets.empty())
    {
      if(shouldCancel)
      {
        return {};
      }

      // Dequeue a vertex from queue and store it
      const IGeometry::MeshIndexType triangle = searchTargets.front();
      searchTargets.pop();

      if(visited[triangle])
      {
        continue;
      }

      if(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() > 1000)
      {
        mesgHandler.sendInfoMessage(fmt::format("Current Feature: {}/{} | Progress : {:2.2f}%", feature, maxFeature, 100.0f * static_cast<float>(feature) / static_cast<float>(maxFeature + 1)));
        start = std::chrono::steady_clock::now();
      }

      auto numElem = neighbors.getNumberOfElements(triangle);
      const IGeometry::MeshIndexType* neighborListPtr = neighbors.getElementListPointer(triangle);

      std::set<usize> localNeighbors = {};

      for(uint16 element = 0; element < numElem; element++)
      {
        const usize neighbor = neighborListPtr[element];
        if(faceLabelsStore[neighbor * 2] != feature && faceLabelsStore[(neighbor * 2) + 1] != feature)
        {
          continue;
        }

        searchTargets.push(neighbor);
        localNeighbors.emplace(neighbor);
      }

      visited[triangle] = true;

      // Load valid adjacent triangle's edges into a list
      EdgeListT edgeList = {};
      for(const usize neighbor : localNeighbors)
      {
        if(!visited[neighbor])
        {
          continue;
        }

        std::pair<IGeometry::MeshIndexType, IGeometry::MeshIndexType> edge1 = std::make_pair(triangles[(neighbor * 3) + 0], triangles[(neighbor * 3) + 1]);
        std::pair<IGeometry::MeshIndexType, IGeometry::MeshIndexType> edge2 = std::make_pair(triangles[(neighbor * 3) + 1], triangles[(neighbor * 3) + 2]);
        std::pair<IGeometry::MeshIndexType, IGeometry::MeshIndexType> edge3 = std::make_pair(triangles[(neighbor * 3) + 2], triangles[(neighbor * 3) + 0]);

        if(unmodified[neighbor])
        {
          // synthetic flip to maintain homogeneity
          edge1 = std::make_pair(triangles[(neighbor * 3) + 0], triangles[(neighbor * 3) + 2]);
          edge2 = std::make_pair(triangles[(neighbor * 3) + 2], triangles[(neighbor * 3) + 1]);
          edge3 = std::make_pair(triangles[(neighbor * 3) + 1], triangles[(neighbor * 3) + 0]);
        }

        // Edges are unique
        edgeList.emplace(std::move(edge1));
        edgeList.emplace(std::move(edge2));
        edgeList.emplace(std::move(edge3));
      }

      // This is computationally heavy
      if(edgeList.find(std::make_pair(triangles[(triangle * 3) + 0], triangles[(triangle * 3) + 1])) != edgeList.end() ||
         edgeList.find(std::make_pair(triangles[(triangle * 3) + 1], triangles[(triangle * 3) + 2])) != edgeList.end() ||
         edgeList.find(std::make_pair(triangles[(triangle * 3) + 2], triangles[(triangle * 3) + 0])) != edgeList.end()) // If true it contains a conflicting edge
      {
        // check if previously visited
        const usize offset = faceLabelsStore[triangle * 2] == feature ? 1 : 0;
        const int32 alternateLabel = faceLabelsStore[(triangle * 2) + offset];
        if(alternateLabel != 0 && alternateLabel < feature)
        {
          unmodified[triangle] = true;
          count++;
        }
        else
        {
          // Flip it
          const IGeometry::MeshIndexType tempValue = triangles[(triangle * 3) + 0];
          triangles[(triangle * 3) + 0] = triangles[(triangle * 3) + 2];
          triangles[(triangle * 3) + 2] = tempValue;
        }
      }
    }
  }

  if(count > 0)
  {
    return MakeWarningVoidResult(-56730, fmt::format("{} triangles cold not be made consistent, due to the nature of mesh implementation.", count));
  }

  return {};
}

Result<> ProcessWindingsWithRegions(INodeGeometry2D::SharedFaceList::store_type& triangles, const DynamicListArray<uint16, IGeometry::MeshIndexType>& neighbors,
                                    const Int32AbstractDataStore& regionsStore, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler, int32 maxFeature)
{
  /**
   * This works by making a map of the edges since a properly wound mesh
   * should have unique edges. The KEY assumption here is that there are NO
   * DUPLICATE VERTICES IN THE MESH, hence the earlier validation.
   *
   * This assumption breaks down if more than two triangles share an edge,
   * so we will be going feature by feature to avoid running into "corner-edges"
   * (where three or more features meet).
   *
   * NOTE: no duplicate vertices, means no duplicate edges
   */

  // Walk the features repairing the graph group by group
  auto start = std::chrono::steady_clock::now();
  const usize numTuples = regionsStore.getNumberOfTuples();
  std::vector<bool> visited(regionsStore.getNumberOfTuples(), false);

  // Precompute the first triangle carrying each region id in a single pass (see the note in
  // ProcessWindingsWithLabels): avoids the O(features * triangles) per-feature seed rescan.
  constexpr IGeometry::MeshIndexType k_NoSeed = std::numeric_limits<IGeometry::MeshIndexType>::max();
  std::vector<IGeometry::MeshIndexType> firstTriOfFeature(static_cast<usize>(maxFeature) + 1, k_NoSeed);
  for(usize i = 0; i < numTuples; i++)
  {
    const int32 rid = regionsStore[i];
    if(rid >= 1 && rid <= maxFeature && firstTriOfFeature[static_cast<usize>(rid)] == k_NoSeed)
    {
      firstTriOfFeature[static_cast<usize>(rid)] = i;
    }
  }

  for(int32 feature = 1; feature < maxFeature + 1; feature++)
  {
    std::queue<IGeometry::MeshIndexType> searchTargets = {};

    // process base case: seed from the first triangle carrying this region (O(1) lookup)
    const IGeometry::MeshIndexType seedTri = firstTriOfFeature[static_cast<usize>(feature)];
    if(seedTri != k_NoSeed)
    {
      auto numElem = neighbors.getNumberOfElements(seedTri);
      const IGeometry::MeshIndexType* neighborListPtr = neighbors.getElementListPointer(seedTri);

      for(uint16 element = 0; element < numElem; element++)
      {
        const usize neighbor = neighborListPtr[element];
        if(regionsStore[neighbor] != feature)
        {
          continue;
        }
        searchTargets.push(neighbor);
      }

      visited[seedTri] = true;
    }

    // begin mass search
    while(!searchTargets.empty())
    {
      if(shouldCancel)
      {
        return {};
      }

      // Dequeue a vertex from queue and store it
      const IGeometry::MeshIndexType triangle = searchTargets.front();
      searchTargets.pop();

      if(visited[triangle])
      {
        continue;
      }

      if(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() > 1000)
      {
        mesgHandler.sendInfoMessage(fmt::format("Current Feature: {}/{} | Progress : {:2.2f}%", feature, maxFeature, 100.0f * static_cast<float>(feature) / static_cast<float>(maxFeature + 1)));
        start = std::chrono::steady_clock::now();
      }

      auto numElem = neighbors.getNumberOfElements(triangle);
      const IGeometry::MeshIndexType* neighborListPtr = neighbors.getElementListPointer(triangle);

      std::set<usize> localNeighbors = {};

      for(uint16 element = 0; element < numElem; element++)
      {
        const usize neighbor = neighborListPtr[element];
        if(regionsStore[neighbor] != feature)
        {
          continue;
        }

        searchTargets.push(neighbor);
        localNeighbors.emplace(neighbor);
      }

      visited[triangle] = true;

      // Load valid adjacent triangle's edges into a list
      EdgeListT edgeList = {};
      for(const usize neighbor : localNeighbors)
      {
        if(!visited[neighbor])
        {
          continue;
        }

        // Edges are unique
        edgeList.emplace(triangles[(neighbor * 3) + 0], triangles[(neighbor * 3) + 1]);
        edgeList.emplace(triangles[(neighbor * 3) + 1], triangles[(neighbor * 3) + 2]);
        edgeList.emplace(triangles[(neighbor * 3) + 2], triangles[(neighbor * 3) + 0]);
      }

      // This is computationally heavy
      if(edgeList.find(std::make_pair(triangles[(triangle * 3) + 0], triangles[(triangle * 3) + 1])) != edgeList.end() ||
         edgeList.find(std::make_pair(triangles[(triangle * 3) + 1], triangles[(triangle * 3) + 2])) != edgeList.end() ||
         edgeList.find(std::make_pair(triangles[(triangle * 3) + 2], triangles[(triangle * 3) + 0])) != edgeList.end()) // If true it contains a conflicting edge
      {
        // Flip it
        const IGeometry::MeshIndexType tempValue = triangles[(triangle * 3) + 0];
        triangles[(triangle * 3) + 0] = triangles[(triangle * 3) + 2];
        triangles[(triangle * 3) + 2] = tempValue;
      }
    }
  }

  return {};
}
} // namespace

INodeGeometry2D::SharedVertexList::value_type MeshingUtilities::detail::FindTriangleVolume(const std::array<usize, 3>& vertIndices, const INodeGeometry2D::SharedVertexList::store_type& vertices)
{
  const usize vertAIndex = vertIndices[0] * 3;
  const usize vertBIndex = vertIndices[1] * 3;
  const usize vertCIndex = vertIndices[2] * 3;

  // This is a 3x3 matrix laid out in typical "C" order where the columns raster the fastest, then the rows
  std::array<INodeGeometry2D::SharedVertexList::value_type, 9> volumeMatrix = {
      vertices[vertBIndex + 0] - vertices[vertAIndex + 0], vertices[vertCIndex + 0] - vertices[vertAIndex + 0], 0.0f - vertices[vertAIndex + 0],
      vertices[vertBIndex + 1] - vertices[vertAIndex + 1], vertices[vertCIndex + 1] - vertices[vertAIndex + 1], 0.0f - vertices[vertAIndex + 1],
      vertices[vertBIndex + 2] - vertices[vertAIndex + 2], vertices[vertCIndex + 2] - vertices[vertAIndex + 2], 0.0f - vertices[vertAIndex + 2]};

  const INodeGeometry2D::SharedVertexList::value_type determinant =
      (volumeMatrix[MeshingUtilities::detail::k_00] *
       (volumeMatrix[MeshingUtilities::detail::k_11] * volumeMatrix[MeshingUtilities::detail::k_22] - volumeMatrix[MeshingUtilities::detail::k_12] * volumeMatrix[MeshingUtilities::detail::k_21])) -
      (volumeMatrix[MeshingUtilities::detail::k_01] *
       (volumeMatrix[MeshingUtilities::detail::k_10] * volumeMatrix[MeshingUtilities::detail::k_22] - volumeMatrix[MeshingUtilities::detail::k_12] * volumeMatrix[MeshingUtilities::detail::k_20])) +
      (volumeMatrix[MeshingUtilities::detail::k_02] *
       (volumeMatrix[MeshingUtilities::detail::k_10] * volumeMatrix[MeshingUtilities::detail::k_21] - volumeMatrix[MeshingUtilities::detail::k_11] * volumeMatrix[MeshingUtilities::detail::k_20]));
  return determinant / 6.0f;
}

Result<> MeshingUtilities::RepairTriangleWinding(INodeGeometry2D::SharedFaceList::store_type& triangles, const DynamicListArray<uint16, IGeometry::MeshIndexType>& neighbors,
                                                 const Int32AbstractDataStore& idsStore, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler)
{
  usize numComp = idsStore.getNumberOfComponents();
  if(numComp > 2 || numComp == 0)
  {
    return MakeErrorResult(-65770,
                           fmt::format("MeshingUtilities::RepairTriangleWinding: invalid ID array supplied. The ID array must have 1 or 2 components, supplied array components: {}.", numComp));
  }

  // Get max group and (feature id != 0)
  int32 maxFeature = 0;
  for(int32 i = 0; i < idsStore.getSize(); i++)
  {
    maxFeature = std::max(idsStore[i], maxFeature);
  }

  if(numComp == 2)
  {
    return ::ProcessWindingsWithLabels(triangles, neighbors, idsStore, shouldCancel, mesgHandler, maxFeature);
  }

  // numComp == 1
  return ::ProcessWindingsWithRegions(triangles, neighbors, idsStore, shouldCancel, mesgHandler, maxFeature);
}

MeshingUtilities::CalculateNormalsImpl::CalculateNormalsImpl(const INodeGeometry2D::SharedFaceList::store_type& triangles, const INodeGeometry2D::SharedVertexList::store_type& verts,
                                                             nx::core::Float64AbstractDataStore& normals, const std::atomic_bool& shouldCancel)
: m_Triangles(triangles)
, m_Vertices(verts)
, m_Normals(normals)
, m_ShouldCancel(shouldCancel)
{
}

void MeshingUtilities::CalculateNormalsImpl::generate(nx::core::types::usize start, nx::core::types::usize end) const
{
  for(usize triangle = start; triangle < end; triangle++)
  {
    if(m_ShouldCancel)
    {
      break;
    }

    const usize triangleIndex = triangle * 3;

    const usize vertAIndex = m_Triangles[triangleIndex] * 3;
    const Eigen::Vector3d vertA = Eigen::Vector3d{m_Vertices[vertAIndex], m_Vertices[vertAIndex + 1], m_Vertices[vertAIndex + 2]};
    const usize vertBIndex = m_Triangles[triangleIndex + 1] * 3;
    const Eigen::Vector3d vertB = Eigen::Vector3d{m_Vertices[vertBIndex], m_Vertices[vertBIndex + 1], m_Vertices[vertBIndex + 2]};
    const usize vertCIndex = m_Triangles[triangleIndex + 2] * 3;
    const Eigen::Vector3d vertC = Eigen::Vector3d{m_Vertices[vertCIndex], m_Vertices[vertCIndex + 1], m_Vertices[vertCIndex + 2]};

    const Eigen::Vector3d vecA = vertB - vertA;
    const Eigen::Vector3d vecB = vertC - vertA;

    Eigen::Vector3d normal = vecA.cross(vecB);
    normal.normalize();

    m_Normals[triangleIndex] = normal[0];
    m_Normals[triangleIndex + 1] = normal[1];
    m_Normals[triangleIndex + 2] = normal[2];
  }
}

void MeshingUtilities::CalculateNormalsImpl::operator()(const nx::core::Range& range) const
{
  generate(range.min(), range.max());
}

Result<> MeshingUtilities::MakeEmptyMeshWarning(const DataPath& triangleGeomPath, usize numCells, usize numVertices)
{
  return MakeWarningVoidResult(k_EmptyMeshAfterSkinRemovalWarning,
                               fmt::format("The 'Bounding Box Skin' option's 'Background-Backed Walls Only' mode removed every face of geometry '{}'. All {} cells of the input have Feature Id 0 "
                                           "(background), so there is no internal interface and no Feature to cap. The Triangle Geometry now has 0 faces and {} vertices remaining.",
                                           triangleGeomPath.toString(), numCells, numVertices));
}

Result<> MeshingUtilities::MakeNoFacesPrunedWarning(const DataPath& triangleGeomPath)
{
  return MakeWarningVoidResult(k_NoFacesPrunedWarning,
                               fmt::format("The 'Bounding Box Skin' option's 'Background-Backed Walls Only' mode removed 0 faces of geometry '{}': no bounding-box wall face is backed by "
                                           "background (Feature Id 0). This says nothing about the volume's interior -- a volume whose background is fully enclosed as interior porosity "
                                           "reaches this same warning, because none of that background borders a wall. There is nothing for this option to prune on this input; the output "
                                           "is identical to leaving it off.",
                                           triangleGeomPath.toString()));
}

Result<> MeshingUtilities::ValidateFeatureIdsAgainstSentinels(const Int32AbstractDataStore& featureIdsStore, const DataPath& featureIdsPath, bool rejectMaxInt32, const std::atomic_bool& shouldCancel,
                                                              const IFilter::MessageHandler& mesgHandler)
{
  const usize numTuples = featureIdsStore.getNumberOfTuples();
  mesgHandler.sendInfoMessage(fmt::format("Validating {} Feature Ids against internal sentinel values...", numTuples));

  // Polled every k_CancelCheckInterval tuples rather than every tuple: at 512^3 (~134M tuples) this
  // loop is a full streaming pass under the out-of-core backend, and a per-tuple cancel check would
  // add overhead to what is otherwise a tight, uncontested read loop.
  constexpr usize k_CancelCheckInterval = 1'000'000;
  for(usize i = 0; i < numTuples; i++)
  {
    if(i % k_CancelCheckInterval == 0 && shouldCancel)
    {
      return {};
    }

    const int32 featureId = featureIdsStore[i];
    if(featureId < 0)
    {
      return MakeErrorResult(k_InvalidFeatureIdError, fmt::format("Feature Ids array '{}' contains a negative value ({}) at tuple index {}. This mesher reserves negative Feature Ids for an "
                                                                  "internal ghost/exterior sentinel convention; relabel this input so every Feature Id is >= 0. (Mitigation for the "
                                                                  "sentinel-collision design tracked separately as simplnx#1705.)",
                                                                  featureIdsPath.toString(), featureId, i));
    }
    if(rejectMaxInt32 && featureId == std::numeric_limits<int32>::max())
    {
      return MakeErrorResult(k_InvalidFeatureIdError, fmt::format("Feature Ids array '{}' contains the value {} at tuple index {}, which collides with an internal 'outside the volume' "
                                                                  "sentinel used by this mesher. Relabel this input so no Feature Id equals INT32_MAX. (Mitigation for the sentinel-collision "
                                                                  "design tracked separately as simplnx#1705.)",
                                                                  featureIdsPath.toString(), featureId, i));
    }
  }

  return {};
}
