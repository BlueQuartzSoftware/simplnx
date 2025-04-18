#include "TriangleUtilities.hpp"

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <algorithm>
#include <chrono>
#include <queue>

using namespace nx::core;

INodeGeometry2D::SharedVertexList::value_type MeshingUtilities::detail::FindTetrahedronVolume(const std::array<usize, 3>& vertIndices, const INodeGeometry2D::SharedVertexList::store_type& vertices)
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
                                                 const Int32AbstractDataStore& faceLabelsStore, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler)
{
  // Get max group and feature count (feature id != 0)
  std::vector<usize> featureCount(1, 0);
  for(int32 i = 0; i < faceLabelsStore.getSize(); i++)
  {
    const int32 feature = faceLabelsStore[i];
    while(feature >= featureCount.size())
    {
      featureCount.push_back(0);
    }

    featureCount[feature]++;
  }
  featureCount.shrink_to_fit();

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
  for(int32 feature = 1; feature < featureCount.size(); feature++)
  {
    std::set<std::pair<IGeometry::MeshIndexType, IGeometry::MeshIndexType>> edgeList = {};

    usize next = 0;
    usize acceptableZeros = 0;

    std::vector<usize> indices(featureCount[feature], 0);

    std::queue<IGeometry::MeshIndexType> searchTargets = {};
    for(usize i = 0; i < numTuples; i++)
    {
      if(faceLabelsStore[i * 2] != feature && faceLabelsStore[(i * 2) + 1] != feature)
      {
        continue;
      }

      searchTargets.push(i);
      break;
    }

    while(std::count(indices.begin(), indices.end(), 0) > acceptableZeros)
    {
      if(shouldCancel)
      {
        return {};
      }

      if(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() > 1000)
      {
        mesgHandler(fmt::format("Current Feature: {} | Total Progress : {:2f}%", feature, 100.0f * static_cast<float>(feature) / static_cast<float>(featureCount.size())));
        start = std::chrono::steady_clock::now();
      }

      while(!searchTargets.empty())
      {
        if(shouldCancel)
        {
          return {};
        }

        // Dequeue a vertex from queue and store it
        const IGeometry::MeshIndexType i = searchTargets.front();
        searchTargets.pop();

        if(faceLabelsStore[i * 2] != feature && faceLabelsStore[(i * 2) + 1] != feature)
        {
          continue;
        }

        auto pos = std::find(indices.begin(), indices.end(), i);
        if(pos != indices.end())
        {
          if(i != 0 || acceptableZeros == 1)
          {
            continue;
          }

          acceptableZeros = 1;
        }

        auto numElem = neighbors.getNumberOfElements(i);
        const IGeometry::MeshIndexType* neighborListPtr = neighbors.getElementListPointer(i);

        for(uint16 element = 0; element < numElem; element++)
        {
          searchTargets.push(neighborListPtr[element]);
        }

        indices[next] = i;
        next++;

        std::pair<IGeometry::MeshIndexType, IGeometry::MeshIndexType> edge1 = std::make_pair(triangles[(i * 3) + 0], triangles[(i * 3) + 1]);
        std::pair<IGeometry::MeshIndexType, IGeometry::MeshIndexType> edge2 = std::make_pair(triangles[(i * 3) + 1], triangles[(i * 3) + 2]);
        std::pair<IGeometry::MeshIndexType, IGeometry::MeshIndexType> edge3 = std::make_pair(triangles[(i * 3) + 2], triangles[(i * 3) + 0]);

        // This is computationally heavy
        if(edgeList.find(edge1) != edgeList.end() || edgeList.find(edge2) != edgeList.end() || edgeList.find(edge3) != edgeList.end()) // If true it contains a conflicting edge
        {
          // check if previously visited
          const usize offset = faceLabelsStore[i * 2] == feature ? 1 : 0;
          const int32 alternateLabel = faceLabelsStore[(i * 2) + offset];
          if(alternateLabel != 0 && alternateLabel < feature)
          {
            count++;
          }
          else
          {
            // Flip it
            const IGeometry::MeshIndexType tempValue = triangles[(i * 3) + 0];
            triangles[(i * 3) + 0] = triangles[(i * 3) + 2];
            triangles[(i * 3) + 2] = tempValue;

            edge1 = std::make_pair(triangles[(i * 3) + 0], triangles[(i * 3) + 1]);
            edge2 = std::make_pair(triangles[(i * 3) + 1], triangles[(i * 3) + 2]);
            edge3 = std::make_pair(triangles[(i * 3) + 2], triangles[(i * 3) + 0]);
          }
        }

        // Edges are unique
        edgeList.emplace(std::move(edge1));
        edgeList.emplace(std::move(edge2));
        edgeList.emplace(std::move(edge3));
      }

      // Find triangles in the feature that haven't been evaluated, if any
      // Zero already considered by this point
      for(usize i = 1; i < numTuples; i++)
      {
        if(faceLabelsStore[i * 2] != feature && faceLabelsStore[(i * 2) + 1] != feature)
        {
          continue;
        }

        if(std::find(indices.begin(), indices.end(), i) != indices.end())
        {
            // No extra checks because we don't evaluate zero here
            continue;
        }

        searchTargets.push(i);
        break;
      }
    }
  }

  if(count > 0)
  {
    return MakeWarningVoidResult(-56730, fmt::format("{} triangles cold not be made consistent, due to the nature of mesh implementation.", count));
  }

  return {};
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
