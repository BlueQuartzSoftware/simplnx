#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct FillBadDataInputValues;

/**
 * @class FillBadDataBFS
 * @brief In-core BFS flood-fill algorithm for filling bad data regions in an
 * image geometry.
 *
 * This is the in-core-optimized implementation of the FillBadData algorithm.
 * It uses breadth-first search (BFS) to discover connected components of bad
 * data (voxels with FeatureId == 0), classifies them by size, and iteratively
 * fills small regions by copying cell data from the best neighboring good feature
 * determined by majority vote.
 *
 * ## Algorithm Overview
 *
 * The algorithm proceeds in three steps:
 *
 * **Step 1 -- Scan for maximum feature ID (and optionally maximum phase).**
 * A linear scan finds the largest FeatureId value, which is used to size the
 * vote counter array in Step 3.
 *
 * **Step 2 -- BFS flood-fill to classify bad-data regions.**
 * Starting from each unvisited voxel with FeatureId == 0, BFS expands to all
 * face-adjacent bad-data neighbors. The resulting connected component is then
 * classified by voxel count:
 * - **Large regions** (>= minAllowedDefectSize): Kept as FeatureId = 0 (voids).
 *   Optionally assigned to a new phase (maxPhase + 1) for visualization.
 * - **Small regions** (< minAllowedDefectSize): Marked with FeatureId = -1,
 *   indicating they should be filled in Step 3.
 *
 * **Step 3 -- Iterative morphological dilation (fill).**
 * Each iteration scans all voxels with FeatureId == -1. For each, the 6
 * face-adjacent neighbors are tallied by feature ID (majority vote). The
 * neighbor with the highest vote count becomes the "source" for that voxel.
 * After the vote scan, all cell data arrays are updated by copying every
 * component from the source neighbor to the target voxel. FeatureIds are
 * updated LAST to prevent a freshly filled voxel from becoming a vote source
 * before its other arrays are copied. Iterations repeat until no -1 voxels
 * remain (the good-data boundary dilates inward by one voxel layer per
 * iteration).
 *
 * ## Memory Usage
 *
 * This algorithm allocates O(N) temporary buffers:
 * - `neighbors`: int32 per voxel, mapping each voxel to its best source neighbor
 * - `alreadyChecked`: 1 bit per voxel (std::vector<bool>), tracking BFS visited state
 * - `featureNumber`: int32 per feature (O(numFeatures)), used as a vote counter
 *
 * These O(N) allocations are efficient when data fits in RAM because all accesses
 * are to contiguous in-memory arrays with O(1) random access.
 *
 * ## Why This is Not Suitable for Out-of-Core
 *
 * BFS expands outward from a seed in a wavefront pattern, visiting neighbors in
 * an unpredictable order relative to the on-disk chunk layout. When data is stored
 * in compressed HDF5 chunks, each neighbor access may trigger a chunk load/evict
 * cycle. For a 300x300x300 volume stored in 64x64x64 chunks, a single BFS that
 * spans the volume boundary crosses chunk boundaries thousands of times, causing
 * catastrophic I/O amplification ("chunk thrashing"). The CCL variant avoids this
 * by processing data in strict Z-slice sequential order.
 *
 * @see FillBadDataCCL for the out-of-core-optimized alternative.
 * @see FillBadData for the dispatcher that selects between BFS and CCL.
 * @see AlgorithmDispatch.hpp for the dispatch mechanism.
 */
class SIMPLNXCORE_EXPORT FillBadDataBFS
{
public:
  /**
   * @brief Constructs the BFS fill algorithm with the required context.
   * @param dataStructure The data structure containing the arrays to process.
   * @param mesgHandler Handler for progress and informational messages.
   * @param shouldCancel Cancellation flag checked during execution.
   * @param inputValues Filter parameter values controlling fill behavior.
   */
  FillBadDataBFS(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const FillBadDataInputValues* inputValues);
  ~FillBadDataBFS() noexcept;

  FillBadDataBFS(const FillBadDataBFS&) = delete;
  FillBadDataBFS(FillBadDataBFS&&) noexcept = delete;
  FillBadDataBFS& operator=(const FillBadDataBFS&) = delete;
  FillBadDataBFS& operator=(FillBadDataBFS&&) noexcept = delete;

  /**
   * @brief Executes the BFS flood-fill algorithm to identify and fill bad data regions.
   *
   * This is the main entry point. It performs all three steps (scan, BFS classify,
   * iterative fill) sequentially in a single call. The algorithm modifies the
   * FeatureIds array and all non-ignored cell data arrays in place.
   *
   * @return Result indicating success or an error with a descriptive message.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;                        ///< Reference to the DataStructure containing all arrays
  const FillBadDataInputValues* m_InputValues = nullptr; ///< Non-owning pointer to filter parameter values
  const std::atomic_bool& m_ShouldCancel;                ///< Cancellation flag checked during long-running loops
  const IFilter::MessageHandler& m_MessageHandler;       ///< Handler for emitting progress/informational messages
};

} // namespace nx::core
