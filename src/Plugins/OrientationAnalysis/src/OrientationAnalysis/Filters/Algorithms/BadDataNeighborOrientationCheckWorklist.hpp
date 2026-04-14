#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct BadDataNeighborOrientationCheckInputValues;

/**
 * @class BadDataNeighborOrientationCheckWorklist
 * @brief In-core (Worklist) algorithm for the bad-data neighbor orientation check.
 *
 * This algorithm is selected by the dispatcher when all relevant arrays reside in
 * contiguous in-memory DataStores. It operates in two phases:
 *
 * **Phase 1 -- Initial neighbor counting** (single linear scan):
 *   For every bad voxel, count how many of its 6 face-neighbors are good and have
 *   a crystallographic misorientation within the tolerance. Store this count in a
 *   per-voxel neighborCount[N] array.
 *
 * **Phase 2 -- Worklist-driven propagation** (per level, 6 down to NumberOfNeighbors):
 *   1. Seed a deque with all bad voxels whose neighborCount >= currentLevel.
 *   2. Pop the front voxel. If it is still bad and still eligible, flip its mask
 *      to true.
 *   3. For each still-bad face-neighbor of the newly-flipped voxel: if the neighbor
 *      has matching orientation (same phase, misorientation < tolerance), increment
 *      its neighborCount. If the count now meets the threshold, enqueue the neighbor.
 *   4. Repeat until the deque is empty, then move to the next level.
 *
 * This worklist approach has O(flipped) amortized cost because each voxel is
 * processed at most once per level, and neighbors are only re-examined when a
 * neighboring voxel actually flips. In contrast, the Scanline variant must re-scan
 * the entire volume on every pass.
 *
 * **Memory footprint**: O(N) for the neighborCount array (one int32 per voxel) plus
 * O(worklist size) for the deque.
 *
 * **Why this is not suitable for OOC**: The random-access pattern (deque pops voxels
 * in arbitrary order, then accesses their neighbors) would trigger catastrophic chunk
 * thrashing on disk-backed stores.
 *
 * @see BadDataNeighborOrientationCheckScanline for the OOC-optimized variant.
 */
class ORIENTATIONANALYSIS_EXPORT BadDataNeighborOrientationCheckWorklist
{
public:
  /**
   * @brief Constructs the in-core worklist neighbor orientation check algorithm.
   * @param dataStructure The DataStructure containing all input/output arrays.
   * @param mesgHandler Message handler for progress/info messages.
   * @param shouldCancel Atomic cancellation flag.
   * @param inputValues Pointer to the shared parameter struct; must outlive this object.
   */
  BadDataNeighborOrientationCheckWorklist(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                          const BadDataNeighborOrientationCheckInputValues* inputValues);
  ~BadDataNeighborOrientationCheckWorklist() noexcept;

  BadDataNeighborOrientationCheckWorklist(const BadDataNeighborOrientationCheckWorklist&) = delete;
  BadDataNeighborOrientationCheckWorklist(BadDataNeighborOrientationCheckWorklist&&) noexcept = delete;
  BadDataNeighborOrientationCheckWorklist& operator=(const BadDataNeighborOrientationCheckWorklist&) = delete;
  BadDataNeighborOrientationCheckWorklist& operator=(BadDataNeighborOrientationCheckWorklist&&) noexcept = delete;

  /**
   * @brief Flips bad voxels to good using two-phase worklist propagation.
   * @return Result<> with any errors (e.g., invalid mask path).
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;                                            ///< Reference to the live DataStructure.
  const BadDataNeighborOrientationCheckInputValues* m_InputValues = nullptr; ///< Borrowed pointer to input parameters.
  const std::atomic_bool& m_ShouldCancel;                                    ///< Cancellation flag.
  const IFilter::MessageHandler& m_MessageHandler;                           ///< Message handler for user-facing messages.
};

} // namespace nx::core
