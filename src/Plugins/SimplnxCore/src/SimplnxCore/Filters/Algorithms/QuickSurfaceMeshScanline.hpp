#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <memory>

namespace nx::core
{
struct QuickSurfaceMeshInputValues;

/**
 * @class QuickSurfaceMeshScanline
 * @brief Out-of-core (OOC) optimized algorithm for QuickSurfaceMesh.
 *
 * Selected by DispatchAlgorithm when any input array is backed by chunked
 * (OOC) storage. Produces identical output to QuickSurfaceMeshDirect but
 * avoids random-access element reads that would cause chunk thrashing on
 * disk-backed DataStores.
 *
 * ## OOC Strategy
 *
 * The key insight is that the QuickSurfaceMesh algorithm only compares each
 * voxel with its +X, +Y, and +Z neighbors. This means at most two adjacent
 * Z-slices of FeatureIds are needed at any time. The scanline variant
 * exploits this by:
 *
 *   - **Bulk I/O**: Reading FeatureIds one Z-slice at a time via
 *     copyIntoBuffer() instead of per-element operator[]. This converts
 *     O(volume) random reads into O(zP) sequential bulk reads.
 *
 *   - **Rolling node-plane buffers**: Instead of the O((xP+1)*(yP+1)*(zP+1))
 *     nodeIds array used by the Direct variant, this algorithm maintains two
 *     node-plane buffers of size O((xP+1)*(yP+1)) that roll forward as each
 *     Z-slice is processed. This reduces memory from O(volume) to O(slice).
 *
 *   - **Buffered output writes**: Triangle connectivity, face labels, and
 *     vertex coordinates are accumulated in per-slice buffers and flushed
 *     via copyFromBuffer() in one bulk write per Z-slice, avoiding
 *     per-element OOC writes.
 *
 * ## Phases
 *
 *   1. **correctProblemVoxels** -- Same diagonal-voxel fix as the Direct
 *      variant, but reads/writes Z-slice pairs via copyIntoBuffer/copyFromBuffer.
 *      Uses dirty flags to skip write-back for unmodified slices.
 *
 *   2. **countActiveNodesAndTriangles** -- Counting pass using rolling
 *      node-plane buffers and double-buffered FeatureId slices.
 *
 *   3. **createNodesAndTriangles** -- Generation pass that writes mesh data.
 *      Vertex coordinates are buffered in a single allocation of size
 *      O(nodeCount * 3) and flushed once at the end. Triangle connectivity
 *      and face labels are flushed per-slice.
 *
 * Memory: O((xP+1)*(yP+1)) for node planes + O(xP*yP) for FeatureId slices
 * + O(nodeCount * 3) for vertex coordinate buffer.
 *
 * @see QuickSurfaceMeshDirect for the in-core reference implementation
 */
class SIMPLNXCORE_EXPORT QuickSurfaceMeshScanline
{
public:
  using VertexStore = AbstractDataStore<IGeometry::SharedVertexList::value_type>;
  using TriStore = AbstractDataStore<IGeometry::SharedTriList::value_type>;
  using MeshIndexType = IGeometry::MeshIndexType;

  /**
   * @brief Constructs the OOC-optimized algorithm. Seeds the RNG for problem-voxel correction.
   * @param dataStructure The DataStructure containing all input/output objects
   * @param mesgHandler Callback for progress and status messages
   * @param shouldCancel Atomic flag checked periodically for user cancellation
   * @param inputValues Pointer to the parameter struct (must outlive this object)
   */
  QuickSurfaceMeshScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const QuickSurfaceMeshInputValues* inputValues);
  ~QuickSurfaceMeshScanline() noexcept;

  QuickSurfaceMeshScanline(const QuickSurfaceMeshScanline&) = delete;
  QuickSurfaceMeshScanline(QuickSurfaceMeshScanline&&) noexcept = delete;
  QuickSurfaceMeshScanline& operator=(const QuickSurfaceMeshScanline&) = delete;
  QuickSurfaceMeshScanline& operator=(QuickSurfaceMeshScanline&&) noexcept = delete;

  /**
   * @brief Executes the full OOC meshing pipeline: problem voxel correction,
   * node/triangle counting, mesh generation, and optional winding repair.
   * @return Result<> indicating success or an error from winding repair
   */
  Result<> operator()();

private:
  /**
   * @brief OOC problem-voxel correction using double-buffered Z-slice pairs.
   *
   * Reads two adjacent Z-slices at a time via copyIntoBuffer(), applies the
   * same 2x2x2 diagonal-conflict resolution as the Direct variant, then
   * writes back only the slices that were actually modified (dirty flags).
   * This avoids per-element read/write through the OOC DataStore.
   */
  void correctProblemVoxels();

  /**
   * @brief Counting pass: determines total node and triangle counts using
   * rolling 2-plane node buffers and double-buffered FeatureId Z-slices.
   * @param[out] nodeCount Total number of unique mesh vertices
   * @param[out] triangleCount Total number of triangles to generate
   * @param[out] numFeatures Maximum FeatureId value found (used for feature array sizing)
   */
  void countActiveNodesAndTriangles(MeshIndexType& nodeCount, MeshIndexType& triangleCount, usize& numFeatures);

  /**
   * @brief Generation pass: creates vertices, triangles, face labels, node types,
   * and runs TupleTransfer for cell/feature data arrays.
   *
   * Uses rolling node-plane buffers to assign vertex IDs, a single bulk
   * vertex-coordinate buffer flushed at the end, and per-slice buffers for
   * triangle connectivity and face labels flushed via copyFromBuffer().
   *
   * @param nodeCount Number of vertices from counting pass (for buffer allocation)
   * @param triangleCount Number of triangles from counting pass (for resizing)
   * @param numFeatures Maximum FeatureId (for feature array sizing)
   */
  void createNodesAndTriangles(MeshIndexType nodeCount, MeshIndexType triangleCount, usize numFeatures);

  DataStructure& m_DataStructure;                             ///< Reference to the active DataStructure
  const QuickSurfaceMeshInputValues* m_InputValues = nullptr; ///< User parameters and created array paths
  const std::atomic_bool& m_ShouldCancel;                     ///< User cancellation flag
  const IFilter::MessageHandler& m_MessageHandler;            ///< Progress message callback
};

} // namespace nx::core
