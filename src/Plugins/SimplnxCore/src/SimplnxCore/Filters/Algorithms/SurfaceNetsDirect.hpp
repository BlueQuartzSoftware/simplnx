#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct SurfaceNetsInputValues;

/**
 * @class SurfaceNetsDirect
 * @brief In-core algorithm for SurfaceNets that delegates to the MMSurfaceNet
 * library for cell classification and mesh generation.
 *
 * Selected by DispatchAlgorithm when all input arrays are backed by in-memory
 * DataStore. This is the original algorithm implementation and serves as the
 * reference for correctness.
 *
 * The algorithm runs in six phases:
 *   1. **Build surface net** -- MMSurfaceNet classifies every cell in a padded
 *      grid (dimX+2, dimY+2, dimZ+2), identifying surface cells where the 8
 *      corner labels are not all identical. Each surface cell gets a vertex.
 *      Uses operator[] to read FeatureIds, which is fast for in-memory stores.
 *
 *   2. **Smooth surface net** (optional) -- Iterative relaxation moves vertices
 *      toward the average of their face-connected neighbors, clamped to stay
 *      within MaxDistanceFromVoxel of their cell center.
 *
 *   3. **Transform vertices** -- Converts local cell-relative positions to
 *      world coordinates using the ImageGeom origin and spacing.
 *
 *   4. **Count triangles** -- Iterates surface vertices, checking 3 edges per
 *      cell (BackBottom, LeftBottom, LeftBack) for crossings that produce quads.
 *      Each quad becomes 2 triangles.
 *
 *   5. **Generate triangles** -- Second pass that writes triangle connectivity,
 *      face labels, and runs TupleTransfer for cell/feature data.
 *
 *   6. **Winding repair** (optional) -- Fixes inconsistent triangle orientations.
 *
 * Memory: O(volume) for the MMCellMap internal data structure (one Cell per
 * padded voxel). This is the main reason the Scanline variant exists.
 *
 * @see SurfaceNetsScanline for the OOC-optimized variant
 */
class SIMPLNXCORE_EXPORT SurfaceNetsDirect
{
public:
  /**
   * @brief Constructs the in-core algorithm.
   * @param dataStructure The DataStructure containing all input/output objects
   * @param mesgHandler Callback for progress and status messages
   * @param shouldCancel Atomic flag checked periodically for user cancellation
   * @param inputValues Pointer to the parameter struct (must outlive this object)
   */
  SurfaceNetsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const SurfaceNetsInputValues* inputValues);
  ~SurfaceNetsDirect() noexcept;

  SurfaceNetsDirect(const SurfaceNetsDirect&) = delete;
  SurfaceNetsDirect(SurfaceNetsDirect&&) noexcept = delete;
  SurfaceNetsDirect& operator=(const SurfaceNetsDirect&) = delete;
  SurfaceNetsDirect& operator=(SurfaceNetsDirect&&) noexcept = delete;

  /**
   * @brief Executes the full in-core Surface Nets pipeline: cell classification,
   * optional smoothing, vertex transformation, triangle generation, and optional
   * winding repair.
   * @return Result<> indicating success or an error from allocation or winding repair
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;                        ///< Reference to the active DataStructure
  const SurfaceNetsInputValues* m_InputValues = nullptr; ///< User parameters and created array paths
  const std::atomic_bool& m_ShouldCancel;                ///< User cancellation flag
  const IFilter::MessageHandler& m_MessageHandler;       ///< Progress message callback
};

} // namespace nx::core
