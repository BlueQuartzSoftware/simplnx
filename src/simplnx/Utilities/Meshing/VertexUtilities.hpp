#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"

namespace nx::core::MeshingUtilities
{
enum AxialAlignment : uint8
{
  X = 0,
  Y = 1,
  Z = 2
};

struct SortedVerticesList
{
  AxialAlignment axis;
  std::vector<IGeometry::MeshIndexType> ordering;
};

/**
 * @brief This function sorts the vertices along the axis with the widest range of values. It doesn't modify the input mesh, but instead returns an object containing the relevant information
 * @param geom The input mesh
 * @returns SortedVerticesList which is a struct containing the sorted indices and the axis they were sorted on
 */
SortedVerticesList OrderSharedVertices(const INodeGeometry2D& geom);

/**
 * @brief This function sorts the vertices along the given axis. It doesn't modify the input mesh, but instead returns an object containing the relevant information
 * @param axis The axis to sort them by
 * @param geom The input mesh
 * @returns std::vector<IGeometry::MeshIndexType> which is the sorted indices along the given axis
 */
std::vector<IGeometry::MeshIndexType> OrderSharedVerticesAlongAxis(nx::core::MeshingUtilities::AxialAlignment axis, const INodeGeometry2D::SharedVertexList::store_type& vertexList);

/**
 * @brief This function attempts to find duplicate vertices. Vertices are all unique if `false`
 * @param verts The SharedVertexList the sorted vertices are based on
 * @param sortedVertices The object containing a sorted list of vertices and the axis it was sorted on
 * @returns bool false means vertices are unique
 */
bool HasDuplicateVertices(const IGeometry::SharedVertexList::store_type& verts, const SortedVerticesList& sortedVertices);
} // namespace nx::core::MeshingUtilities
