#include "TriangleUtilities.hpp"

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <nonstd/span.hpp>

#include <algorithm>
#include <chrono>
#include <memory>
#include <queue>

using namespace nx::core;

namespace
{
using EdgeListT = std::set<std::pair<IGeometry::MeshIndexType, IGeometry::MeshIndexType>>;

Result<> ProcessWindingsWithLabels(IGeometry::MeshIndexType* triangles, usize numTris, const DynamicListArray<uint16, IGeometry::MeshIndexType>& neighbors, const int32* faceLabels,
                                   const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler, int32 maxFeature)
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
  std::vector<bool> visited(numTris, false);
  std::vector<bool> unmodified(numTris, false);
  for(int32 feature = 1; feature < maxFeature + 1; feature++)
  {
    std::queue<IGeometry::MeshIndexType> searchTargets = {};

    // process base case
    for(usize i = 0; i < numTris; i++)
    {
      if(faceLabels[i * 2] != feature && faceLabels[(i * 2) + 1] != feature)
      {
        continue;
      }

      auto numElem = neighbors.getNumberOfElements(i);
      const IGeometry::MeshIndexType* neighborListPtr = neighbors.getElementListPointer(i);

      for(uint16 element = 0; element < numElem; element++)
      {
        const usize neighbor = neighborListPtr[element];
        if(faceLabels[neighbor * 2] != feature && faceLabels[(neighbor * 2) + 1] != feature)
        {
          continue;
        }
        searchTargets.push(neighbor);
      }

      visited[i] = true;
      break;
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
        mesgHandler(fmt::format("Current Feature: {} | Total Progress : {:2.2f}%", feature, 100.0f * static_cast<float>(feature) / static_cast<float>(maxFeature + 1)));
        start = std::chrono::steady_clock::now();
      }

      auto numElem = neighbors.getNumberOfElements(triangle);
      const IGeometry::MeshIndexType* neighborListPtr = neighbors.getElementListPointer(triangle);

      std::set<usize> localNeighbors = {};

      for(uint16 element = 0; element < numElem; element++)
      {
        const usize neighbor = neighborListPtr[element];
        if(faceLabels[neighbor * 2] != feature && faceLabels[(neighbor * 2) + 1] != feature)
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
        const usize offset = faceLabels[triangle * 2] == feature ? 1 : 0;
        const int32 alternateLabel = faceLabels[(triangle * 2) + offset];
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

Result<> ProcessWindingsWithRegions(IGeometry::MeshIndexType* triangles, usize numTris, const DynamicListArray<uint16, IGeometry::MeshIndexType>& neighbors, const int32* regions,
                                    const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler, int32 maxFeature)
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
  std::vector<bool> visited(numTris, false);
  for(int32 feature = 1; feature < maxFeature + 1; feature++)
  {
    std::queue<IGeometry::MeshIndexType> searchTargets = {};

    // process base case
    for(usize i = 0; i < numTris; i++)
    {
      if(regions[i] != feature)
      {
        continue;
      }

      auto numElem = neighbors.getNumberOfElements(i);
      const IGeometry::MeshIndexType* neighborListPtr = neighbors.getElementListPointer(i);

      for(uint16 element = 0; element < numElem; element++)
      {
        const usize neighbor = neighborListPtr[element];
        if(regions[neighbor] != feature)
        {
          continue;
        }
        searchTargets.push(neighbor);
      }

      visited[i] = true;
      break;
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
        mesgHandler(fmt::format("Current Feature: {} | Total Progress : {:2.2f}%", feature, 100.0f * static_cast<float>(feature) / static_cast<float>(maxFeature + 1)));
        start = std::chrono::steady_clock::now();
      }

      auto numElem = neighbors.getNumberOfElements(triangle);
      const IGeometry::MeshIndexType* neighborListPtr = neighbors.getElementListPointer(triangle);

      std::set<usize> localNeighbors = {};

      for(uint16 element = 0; element < numElem; element++)
      {
        const usize neighbor = neighborListPtr[element];
        if(regions[neighbor] != feature)
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

  const usize numTris = triangles.getNumberOfTuples();
  const usize idsSize = idsStore.getSize(); // numTris * numComp

  // Bulk-read triangles into local buffer to avoid per-element OOC overhead
  auto triBuf = std::make_unique<IGeometry::MeshIndexType[]>(numTris * 3);
  triangles.copyIntoBuffer(0, nonstd::span<IGeometry::MeshIndexType>(triBuf.get(), numTris * 3));

  // Bulk-read ids into local buffer
  auto idsBuf = std::make_unique<int32[]>(idsSize);
  idsStore.copyIntoBuffer(0, nonstd::span<int32>(idsBuf.get(), idsSize));

  // Find max feature from local buffer
  int32 maxFeature = 0;
  for(usize i = 0; i < idsSize; i++)
  {
    maxFeature = std::max(idsBuf[i], maxFeature);
  }

  Result<> result;
  if(numComp == 2)
  {
    result = ::ProcessWindingsWithLabels(triBuf.get(), numTris, neighbors, idsBuf.get(), shouldCancel, mesgHandler, maxFeature);
  }
  else
  {
    result = ::ProcessWindingsWithRegions(triBuf.get(), numTris, neighbors, idsBuf.get(), shouldCancel, mesgHandler, maxFeature);
  }

  // Bulk-write modified triangles back
  triangles.copyFromBuffer(0, nonstd::span<const IGeometry::MeshIndexType>(triBuf.get(), numTris * 3));

  return result;
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
