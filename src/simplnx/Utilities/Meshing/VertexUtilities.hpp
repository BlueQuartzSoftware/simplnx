#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"

namespace nx::core::MeshingUtilities
{
enum SIMPLNX_EXPORT AxialAlignment : uint8
{
  X = 0,
  Y = 1,
  Z = 2
};

struct SIMPLNX_EXPORT SortedVerticesList
{
  AxialAlignment axis;
  std::vector<IGeometry::MeshIndexType> ordering;
};

/**
 * @brief Sorts vertex indices by X, Y, Z, then input index. NaNs follow numbers on each axis.
 * @param geom Input mesh. The function does not modify the mesh.
 * @param shouldCancel Skips sorting if cancellation is already requested.
 * @return Sorted indices with X as the primary axis.
 */
SIMPLNX_EXPORT SortedVerticesList OrderSharedVertices(const INodeGeometry0D& geom, const std::atomic_bool& shouldCancel);

/**
 * @brief Sorts indices by the given axis, the next two axes cyclically, then input index. NaNs follow numbers on each axis.
 * @param axis Primary sort axis.
 * @param vertexList Input coordinates. The function does not modify the coordinates.
 * @param shouldCancel Skips sorting if cancellation is already requested.
 * @return Sorted vertex indices.
 */
SIMPLNX_EXPORT std::vector<IGeometry::MeshIndexType> OrderSharedVerticesAlongAxis(nx::core::MeshingUtilities::AxialAlignment axis, const INodeGeometry0D::SharedVertexList::store_type& vertexList,
                                                                                  const std::atomic_bool& shouldCancel);

/**
 * @brief Finds adjacent vertices with exactly equal coordinates. Signed zeros are equal; NaNs are never equal.
 * @param verts Input coordinates.
 * @param sortedVertices Indices sorted by OrderSharedVertices or OrderSharedVerticesAlongAxis, without cancellation.
 * @return True if any two vertices have exactly equal X, Y, and Z coordinates.
 */
SIMPLNX_EXPORT bool HasDuplicateVertices(const IGeometry::SharedVertexList::store_type& verts, const SortedVerticesList& sortedVertices);
} // namespace nx::core::MeshingUtilities
