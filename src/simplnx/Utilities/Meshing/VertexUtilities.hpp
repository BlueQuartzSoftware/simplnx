#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"

#include <nonstd/span.hpp>

#include <atomic>
#include <vector>

namespace nx::core
{
class ITemporaryRecordStore;
}

namespace nx::core::MeshingUtilities
{
namespace detail
{
/**
 * @brief Reads requested XYZ tuples through caller-owned bounded buffers.
 * @param vertices Supplies three-component coordinates.
 * @param indexes Specifies vertex tuples in the required output order.
 * @param order Supplies one permutation element per request.
 * @param gathered Receives three coordinates per request, in the original order.
 * @param page Supplies a nonempty whole-XYZ read page when requests exist.
 * @return Shape, index, capacity, or read error, or success with backend warnings.
 * @note Sorts only read requests. The caller owns admission and cancellation checkpoints.
 */
SIMPLNX_EXPORT Result<> GatherVertexCoordinates(const Float32AbstractDataStore& vertices, nonstd::span<const IGeometry::MeshIndexType> indexes, nonstd::span<usize> order,
                                                nonstd::span<float32> gathered, nonstd::span<float32> page);

/** @brief Reads temporary XYZ records with the same caller-buffer and ordering contract. */
SIMPLNX_EXPORT Result<> GatherVertexCoordinates(const ITemporaryRecordStore& vertices, nonstd::span<const IGeometry::MeshIndexType> indexes, nonstd::span<usize> order, nonstd::span<float32> gathered,
                                                nonstd::span<float32> page);
} // namespace detail

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
 * @brief This function sorts the vertices along the axis with the widest range of values. It doesn't modify the input mesh, but instead returns an object containing the relevant information
 * @param geom The input mesh
 * @returns SortedVerticesList which is a struct containing the sorted indices and the axis they were sorted on
 */
SIMPLNX_EXPORT SortedVerticesList OrderSharedVertices(const INodeGeometry0D& geom, const std::atomic_bool& shouldCancel);

/**
 * @brief This function sorts the vertices along the given axis. It doesn't modify the input mesh, but instead returns an object containing the relevant information
 * @param axis The axis to sort them by
 * @param geom The input mesh
 * @returns std::vector<IGeometry::MeshIndexType> which is the sorted indices along the given axis
 */
SIMPLNX_EXPORT std::vector<IGeometry::MeshIndexType> OrderSharedVerticesAlongAxis(nx::core::MeshingUtilities::AxialAlignment axis, const INodeGeometry0D::SharedVertexList::store_type& vertexList,
                                                                                  const std::atomic_bool& shouldCancel);

/**
 * @brief This function attempts to find duplicate vertices. Vertices are all unique if `false`
 * @param verts The SharedVertexList the sorted vertices are based on
 * @param sortedVertices The object containing a sorted list of vertices and the axis it was sorted on
 * @returns bool false means vertices are unique
 */
SIMPLNX_EXPORT bool HasDuplicateVertices(const IGeometry::SharedVertexList::store_type& verts, const SortedVerticesList& sortedVertices);
} // namespace nx::core::MeshingUtilities
