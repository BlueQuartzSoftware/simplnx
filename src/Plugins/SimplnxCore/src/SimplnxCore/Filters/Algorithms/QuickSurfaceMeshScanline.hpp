#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <memory>

namespace nx::core
{
/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */

struct QuickSurfaceMeshInputValues;

/**
 * @class QuickSurfaceMeshScanline
 * @brief Generates QuickSurfaceMesh output with bounded scanline storage.
 *
 * DispatchAlgorithm normally selects this implementation when any target uses
 * out-of-core (OOC) storage. Tests can force the path with in-memory targets.
 * Random element access can repeatedly load and evict disk-backed chunks. The
 * scanline reads Feature IDs in bulk.
 *
 * Each cell compares +X, +Y, and +Z neighbors. Two Feature ID Z slices therefore
 * suffice. Two rolling node planes replace a volume-sized node-ID map. In OOC
 * execution, temporary records retain mesh-sized node ownership. A fixed page cache
 * bounds resident record memory. Triangle connectivity, face labels, and transfers
 * flush per Z slice.
 *
 * The implementation preserves QuickSurfaceMeshDirect correction choices, label
 * order, and topology. Winding repair uses external sorting and temporary records.
 * The in-memory fallback uses transient adjacency only after all targets are in memory.
 *
 * @warning Concurrent instances are not safe. Construction and correction use
 * shared translation-unit random-number state.
 *
 * @see QuickSurfaceMeshDirect for the in-memory reference implementation.
 */
class SIMPLNXCORE_EXPORT QuickSurfaceMeshScanline
{
public:
  /**
   * @brief Names the scalar store for mesh vertex coordinates.
   */
  using VertexStore = AbstractDataStore<IGeometry::SharedVertexList::value_type>;

  /**
   * @brief Names the scalar store for mesh triangle connectivity.
   */
  using TriStore = AbstractDataStore<IGeometry::SharedTriList::value_type>;

  /**
   * @brief Names the integer type used for mesh indices.
   */
  using MeshIndexType = IGeometry::MeshIndexType;

  /**
   * @brief Constructs the scanline meshing algorithm.
   * @param dataStructure DataStructure that must outlive the algorithm.
   * @param mesgHandler Message callback that must outlive the algorithm.
   * @param shouldCancel Atomic cancellation flag that must outlive the algorithm.
   * @param inputValues Non-null algorithm inputs that must outlive the algorithm.
   * @pre inputValues is not null.
   *
   * The object borrows all arguments. It reseeds the problem-voxel random-number generator.
   */
  QuickSurfaceMeshScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const QuickSurfaceMeshInputValues* inputValues);

  /**
   * @brief Destroys the scanline meshing algorithm.
   */
  ~QuickSurfaceMeshScanline() noexcept;

  QuickSurfaceMeshScanline(const QuickSurfaceMeshScanline&) = delete;
  QuickSurfaceMeshScanline(QuickSurfaceMeshScanline&&) noexcept = delete;
  QuickSurfaceMeshScanline& operator=(const QuickSurfaceMeshScanline&) = delete;
  QuickSurfaceMeshScanline& operator=(QuickSurfaceMeshScanline&&) noexcept = delete;

  /**
   * @brief Runs correction, counting, meshing, and optional winding repair.
   * @return Valid result on completion or cancellation. Returns an error from bulk I/O,
   * record storage, tuple transfer, or winding repair otherwise.
   *
   * Cancellation can leave corrected Feature IDs or resized, partially written mesh output.
   */
  Result<> operator()();

private:
  /**
   * @brief Resolves diagonal voxel conflicts in two buffered Z slices.
   * @return Valid result on completion or cancellation. Returns a Feature ID bulk-I/O error otherwise.
   *
   * QuickSurfaceMeshDirect-compatible random choices preserve mesh topology.
   * Dirty flags write only changed Feature ID slices.
   */
  Result<> correctProblemVoxels();

  /**
   * @brief Counts mesh vertices and triangles with rolling node planes.
   * @param[out] nodeCount Receives the number of unique mesh vertices.
   * @param[out] triangleCount Receives the number of generated triangles.
   * @param[out] numFeatures Receives the greatest observed Feature ID.
   * @return Valid result on completion or cancellation. Returns a Feature ID bulk-I/O error otherwise.
   *
   * Later cells cannot reference a retired Z plane. The rolling planes avoid a
   * volume-sized node-ID map.
   */
  Result<> countActiveNodesAndTriangles(MeshIndexType& nodeCount, MeshIndexType& triangleCount, usize& numFeatures, MeshIndexType& suppressedFaceCount);

  /**
   * @brief Generates mesh output with external node records and per-slice buffers.
   * @param nodeCount Exact vertex count from the counting pass.
   * @param triangleCount Exact triangle count from the counting pass.
   * @param numFeatures Greatest Feature ID from the counting pass.
   * @return Valid result on completion or cancellation. Returns a temporary-record,
   * bulk-I/O, or tuple-transfer error otherwise.
   *
   * OOC execution keeps mesh-sized node ownership in temporary records. Per-slice
   * output buffers avoid per-triangle storage I/O.
   */
  Result<> createNodesAndTriangles(MeshIndexType nodeCount, MeshIndexType triangleCount, usize numFeatures);

  DataStructure& m_DataStructure;
  const QuickSurfaceMeshInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
