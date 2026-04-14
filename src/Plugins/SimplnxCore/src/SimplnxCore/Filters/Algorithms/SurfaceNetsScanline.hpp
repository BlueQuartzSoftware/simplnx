#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "SimplnxCore/SurfaceNets/MMCellFlag.h"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <array>
#include <unordered_map>
#include <vector>

namespace nx::core
{
struct SurfaceNetsInputValues;

/**
 * @class SurfaceNetsScanline
 * @brief Out-of-core (OOC) optimized algorithm for SurfaceNets.
 *
 * Selected by DispatchAlgorithm when any input array is backed by chunked
 * (OOC) storage. Produces identical output to SurfaceNetsDirect but avoids
 * the O(volume) MMCellMap allocation and per-element FeatureIds access.
 *
 * ## OOC Strategy
 *
 * The key insight is that the Surface Nets cell classification only needs
 * the 8 corner labels of each cell, which span at most 2 adjacent Z-slices.
 * The scanline variant exploits this by:
 *
 *   - **Bulk I/O**: Reading FeatureIds two Z-slices at a time via
 *     copyIntoBuffer() with a rolling ping-pong buffer. Each cell's 8
 *     corner labels are resolved from the two buffered slices.
 *
 *   - **O(surface) storage**: Instead of allocating one Cell per padded
 *     voxel (the O(volume) MMCellMap), this algorithm stores only the
 *     surface cells -- those where not all 8 corner labels are identical.
 *     Surface cells are stored in m_Vertices (a vector of VertexInfo) and
 *     m_CellToVertex (a hash map from padded cell coordinate to vertex index).
 *     For typical datasets, surface is O(n^{2/3}) vs O(n) for volume.
 *
 *   - **Self-contained smoothing**: The relaxation is performed entirely on
 *     the O(surface) vertex data using neighbor lookups through m_CellToVertex,
 *     without needing the full MMCellMap.
 *
 *   - **Buffered output writes**: Triangle connectivity, face labels, and
 *     vertex coordinates are accumulated in local buffers and flushed via
 *     copyFromBuffer() in bulk.
 *
 * ## Phases
 *
 *   1. **Cell classification** (operator() main loop) -- Iterates padded cells
 *      in Z-slice order, reads 8 corner labels from rolling slice buffers,
 *      computes MMCellFlag for each cell, stores surface cells in m_Vertices.
 *
 *   2A. **Smoothing** (optional) -- Iterative relaxation using face-connected
 *       neighbor positions looked up from m_CellToVertex. Same convergence
 *       behavior as MMSurfaceNet::relax() but operates on O(surface) data.
 *
 *   2B. **Vertex transform** -- Converts local cell-relative positions to
 *       world coordinates and assigns node types from MMCellFlag junction counts.
 *
 *   3A. **Triangle counting** -- Iterates surface vertices checking 3 edges
 *       per cell for crossings. Each crossing produces a quad = 2 triangles.
 *
 *   3B-3E. **Triangle generation** -- Second pass writes triangle connectivity,
 *       face labels, and runs TupleTransfer. All output is buffered and flushed
 *       in bulk via copyFromBuffer().
 *
 *   3F. **Winding repair** (optional) -- Same as SurfaceNetsDirect.
 *
 * Memory: O(surface_cells) for m_Vertices + O(surface_cells) for m_CellToVertex
 * + O(triangle_count * 3) for output buffers.
 *
 * @see SurfaceNetsDirect for the in-core reference implementation
 */
class SIMPLNXCORE_EXPORT SurfaceNetsScanline
{
public:
  /**
   * @brief Constructs the OOC-optimized algorithm.
   * @param dataStructure The DataStructure containing all input/output objects
   * @param mesgHandler Callback for progress and status messages
   * @param shouldCancel Atomic flag checked periodically for user cancellation
   * @param inputValues Pointer to the parameter struct (must outlive this object)
   */
  SurfaceNetsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const SurfaceNetsInputValues* inputValues);
  ~SurfaceNetsScanline() noexcept;

  SurfaceNetsScanline(const SurfaceNetsScanline&) = delete;
  SurfaceNetsScanline(SurfaceNetsScanline&&) noexcept = delete;
  SurfaceNetsScanline& operator=(const SurfaceNetsScanline&) = delete;
  SurfaceNetsScanline& operator=(SurfaceNetsScanline&&) noexcept = delete;

  /**
   * @brief Executes the full OOC Surface Nets pipeline: cell classification,
   * optional smoothing, vertex transformation, triangle generation, and optional
   * winding repair.
   * @return Result<> indicating success or an error from winding repair
   */
  Result<> operator()();

  /**
   * @brief Per-vertex information stored only for surface cells.
   *
   * This struct replaces the full MMCellMap::Cell for surface cells. It stores
   * the padded grid coordinates and the MMCellFlag that encodes which edges
   * and faces of the cell are crossed by the feature boundary.
   */
  struct VertexInfo
  {
    std::array<int32, 3> cellIndex; ///< (i,j,k) position in the padded coordinate system
    MMCellFlag flag;                ///< Cell classification flags (vertex type, edge crossings, face crossings)
  };

private:
  DataStructure& m_DataStructure;                        ///< Reference to the active DataStructure
  const SurfaceNetsInputValues* m_InputValues = nullptr; ///< User parameters and created array paths
  const std::atomic_bool& m_ShouldCancel;                ///< User cancellation flag
  const IFilter::MessageHandler& m_MessageHandler;       ///< Progress message callback

  /// O(surface) vertex storage -- populated during cell classification (Phase 1).
  /// Each entry corresponds to one surface cell that will become a mesh vertex.
  std::vector<VertexInfo> m_Vertices;

  /// Hash map from packed padded-cell coordinate to index in m_Vertices.
  /// Used for O(1) neighbor lookups during smoothing and edge-quad generation.
  std::unordered_map<uint64, usize> m_CellToVertex;
};

} // namespace nx::core
