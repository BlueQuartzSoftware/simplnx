#include "TriangleUtilities.hpp"

#include <Eigen/Core>
#include <Eigen/Geometry>

using namespace nx::core;

namespace
{
enum WindingType
{
  Clockwise,
  CounterClockwise
};

WindingType DetermineWinding(IGeometry::MeshIndexType triangle, int32 featureId, const Int32AbstractDataStore& faceIds)
{
  if(faceIds[(triangle * 2) + 1] == featureId)
  {
    return WindingType::CounterClockwise;
  }

  return WindingType::Clockwise;
}
} // namespace

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

  INodeGeometry2D::SharedVertexList::value_type determinant =
      (volumeMatrix[MeshingUtilities::detail::k_00] *
       (volumeMatrix[MeshingUtilities::detail::k_11] * volumeMatrix[MeshingUtilities::detail::k_22] - volumeMatrix[MeshingUtilities::detail::k_12] * volumeMatrix[MeshingUtilities::detail::k_21])) -
      (volumeMatrix[MeshingUtilities::detail::k_01] *
       (volumeMatrix[MeshingUtilities::detail::k_10] * volumeMatrix[MeshingUtilities::detail::k_22] - volumeMatrix[MeshingUtilities::detail::k_12] * volumeMatrix[MeshingUtilities::detail::k_20])) +
      (volumeMatrix[MeshingUtilities::detail::k_02] *
       (volumeMatrix[MeshingUtilities::detail::k_10] * volumeMatrix[MeshingUtilities::detail::k_21] - volumeMatrix[MeshingUtilities::detail::k_11] * volumeMatrix[MeshingUtilities::detail::k_20]));
  return determinant / 6.0f;
}

Result<> MeshingUtilities::RepairTriangleWinding(INodeGeometry2D::SharedFaceList::store_type& triangles, const Int32AbstractDataStore& faceLabelsStore, const std::atomic_bool& shouldCancel)
{
  // Get max group (feature id != 0)
  int32 maxFeature = 0;
  for(int32 i = 0; i < faceLabelsStore.getSize(); i++)
  {
    if(faceLabelsStore[i] > maxFeature)
    {
      maxFeature = faceLabelsStore[i];
    }
  }

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
  for(int32 feature = 1; feature < maxFeature + 1; feature++)
  {
    std::set<std::pair<IGeometry::MeshIndexType, IGeometry::MeshIndexType>> edgeList = {};
    for(usize i = 0; i < faceLabelsStore.getNumberOfTuples(); i++)
    {
      if(shouldCancel)
      {
        return {};
      }

      if(faceLabelsStore[i * 2] != feature && faceLabelsStore[(i * 2) + 1] != feature)
      {
        continue;
      }

      const WindingType winding = DetermineWinding(i, feature, faceLabelsStore);

      std::pair<IGeometry::MeshIndexType, IGeometry::MeshIndexType> edge1;
      std::pair<IGeometry::MeshIndexType, IGeometry::MeshIndexType> edge2;
      std::pair<IGeometry::MeshIndexType, IGeometry::MeshIndexType> edge3;
      if(winding == WindingType::Clockwise)
      {
        edge1 = std::make_pair(triangles[(i * 3) + 0], triangles[(i * 3) + 1]);
        edge2 = std::make_pair(triangles[(i * 3) + 1], triangles[(i * 3) + 2]);
        edge3 = std::make_pair(triangles[(i * 3) + 2], triangles[(i * 3) + 0]);
      }
      else // CounterClockwise
      {
        edge1 = std::make_pair(triangles[(i * 3) + 2], triangles[(i * 3) + 1]);
        edge2 = std::make_pair(triangles[(i * 3) + 1], triangles[(i * 3) + 0]);
        edge3 = std::make_pair(triangles[(i * 3) + 0], triangles[(i * 3) + 2]);
      }

      // This is computationally heavy
      if(edgeList.find(edge1) != edgeList.end() || edgeList.find(edge2) != edgeList.end() || edgeList.find(edge3) != edgeList.end()) // If true it contains a conflicting edge
      {
        if(faceLabelsStore[(i * 2) + 1] < feature) // already visited
        {
          count++;
          continue;
        }

        // Flip it
        const IGeometry::MeshIndexType tempValue = triangles[(i * 3) + 0];
        triangles[(i * 3) + 0] = triangles[(i * 3) + 2];
        triangles[(i * 3) + 2] = tempValue;

        if(winding == WindingType::Clockwise)
        {
          edge1 = std::make_pair(triangles[(i * 3) + 0], triangles[(i * 3) + 1]);
          edge2 = std::make_pair(triangles[(i * 3) + 1], triangles[(i * 3) + 2]);
          edge3 = std::make_pair(triangles[(i * 3) + 2], triangles[(i * 3) + 0]);
        }
        else // CounterClockwise
        {
          edge1 = std::make_pair(triangles[(i * 3) + 2], triangles[(i * 3) + 1]);
          edge2 = std::make_pair(triangles[(i * 3) + 1], triangles[(i * 3) + 0]);
          edge3 = std::make_pair(triangles[(i * 3) + 0], triangles[(i * 3) + 2]);
        }
      }

      // Edges are unique
      edgeList.emplace(std::move(edge1));
      edgeList.emplace(std::move(edge2));
      edgeList.emplace(std::move(edge3));
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
