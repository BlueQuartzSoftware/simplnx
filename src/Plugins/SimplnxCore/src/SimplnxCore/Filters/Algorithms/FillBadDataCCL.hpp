#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/UnionFind.hpp"

namespace nx::core
{

// Forward declarations
template <typename T>
class DataArray;
using Int32Array = DataArray<int32>;

template <typename T>
class AbstractDataStore;
using Int32AbstractDataStore = AbstractDataStore<int32>;

struct FillBadDataInputValues;

/**
 * @class FillBadDataCCL
 * @brief Out-of-core optimized algorithm for filling bad data regions using
 * scanline Connected Component Labeling (CCL) with Union-Find.
 *
 * This is the OOC-optimized implementation of the FillBadData algorithm. It
 * replaces the BFS flood-fill approach with a four-phase pipeline that processes
 * data in strict Z-slice sequential order, avoiding the random access pattern
 * that causes chunk thrashing when data is stored in compressed HDF5 chunks.
 *
 * ## Why CCL Instead of BFS for Out-of-Core
 *
 * BFS flood-fill expands outward from a seed voxel, visiting neighbors in a
 * wavefront pattern. This wavefront crosses Z-slice (and therefore chunk)
 * boundaries unpredictably, causing the HDF5 chunk cache to repeatedly load
 * and evict the same chunks. For a typical 300x300x300 volume in 64x64x64
 * chunks, a single BFS can trigger millions of chunk I/O operations.
 *
 * CCL avoids this by scanning voxels in a fixed Z-Y-X order, only looking at
 * backward neighbors (x-1, y-1, z-1). This means each Z-slice is read exactly
 * once during the forward scan. Cross-slice connectivity is tracked via a
 * Union-Find data structure (O(labels) memory), and a rolling 2-slice label
 * buffer provides backward neighbor lookups using only O(dimX * dimY) memory
 * instead of O(volume).
 *
 * ## Four-Phase Algorithm
 *
 * **Phase 1 -- Scanline CCL (Z-slice sequential):**
 * Processes one Z-slice at a time using copyIntoBuffer/copyFromBuffer for bulk
 * I/O. For each bad-data voxel (FeatureId == 0), checks three backward neighbors
 * (x-1, y-1, z-1) in a rolling 2-slice label buffer. Assigns provisional labels
 * and records equivalences in a UnionFind structure. Writes provisional labels
 * back to the FeatureIds store for use in Phase 3. Accumulates per-label voxel
 * counts for size classification.
 *
 * **Phase 2 -- Global resolution:**
 * Flattens the UnionFind structure so every label points directly to its root,
 * and accumulates per-label sizes to root labels. After this phase, querying
 * the size of any label gives the total voxel count of its connected component.
 *
 * **Phase 3 -- Region classification and relabeling:**
 * Reads provisional labels back from the FeatureIds store (one Z-slice at a
 * time) and classifies each component:
 * - Small regions (< minAllowedDefectSize): Relabeled to -1 for filling
 * - Large regions (>= minAllowedDefectSize): Relabeled to 0 (optionally
 *   assigned to a new phase for visualization)
 * Original good-feature labels (in [1, startLabel)) are left unchanged.
 *
 * **Phase 4 -- Iterative morphological fill (temp-file deferred):**
 * Each iteration has two passes:
 * - Pass 1 (Vote): Scans voxels using a 3-slice rolling window. For each -1
 *   voxel, finds the best positive-FeatureId neighbor via majority vote. Writes
 *   (dest, src) index pairs to a temporary file. FeatureIds are read-only during
 *   this pass, ensuring all votes see the pre-iteration state.
 * - Pass 2 (Apply): Reads pairs back from the temp file and applies fills using
 *   a 3-slice rolling buffer per cell array, converting per-tuple random accesses
 *   into bulk slice I/O operations.
 * Iterations repeat until no -1 voxels remain.
 *
 * ## Memory Usage
 *
 * - Phase 1: O(dimX * dimY) for the 2-slice label buffer + O(labels) for UnionFind
 * - Phase 2: O(labels) for flatten
 * - Phase 3: O(dimX * dimY) for slice buffers + O(labels) for classification
 * - Phase 4: O(numFeatures) for vote counter + O(dimX * dimY) for 3-slice windows
 *   + temp file I/O for deferred fill pairs
 *
 * No O(N) memory allocations are made at any point (where N = total voxels).
 *
 * @see FillBadDataBFS for the in-core-optimized alternative.
 * @see FillBadData for the dispatcher that selects between BFS and CCL.
 * @see AlgorithmDispatch.hpp for the dispatch mechanism.
 * @see UnionFind for the disjoint-set data structure used in Phases 1-3.
 */
class SIMPLNXCORE_EXPORT FillBadDataCCL
{
public:
  /**
   * @brief Constructs the CCL fill algorithm with the required context.
   * @param dataStructure The data structure containing the arrays to process.
   * @param mesgHandler Handler for progress and informational messages.
   * @param shouldCancel Cancellation flag checked during execution.
   * @param inputValues Filter parameter values controlling fill behavior.
   */
  FillBadDataCCL(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const FillBadDataInputValues* inputValues);
  ~FillBadDataCCL() noexcept;

  FillBadDataCCL(const FillBadDataCCL&) = delete;
  FillBadDataCCL(FillBadDataCCL&&) noexcept = delete;
  FillBadDataCCL& operator=(const FillBadDataCCL&) = delete;
  FillBadDataCCL& operator=(FillBadDataCCL&&) noexcept = delete;

  /**
   * @brief Executes the four-phase CCL algorithm to identify and fill bad data regions.
   *
   * Orchestrates Phases 1-4 sequentially, emitting progress messages between
   * phases. The algorithm modifies the FeatureIds array and all non-ignored cell
   * data arrays in place, producing results identical to FillBadDataBFS.
   *
   * @return Result indicating success or an error with a descriptive message.
   */
  Result<> operator()();

  /**
   * @brief Returns the cancellation flag reference.
   * @return Reference to the atomic cancellation flag.
   */
  const std::atomic_bool& getCancel() const;

private:
  /**
   * @brief Phase 1: Z-slice sequential scanline CCL for bad-data voxels.
   *
   * Scans the volume one Z-slice at a time, assigning provisional labels to
   * bad-data voxels (FeatureId == 0) and recording equivalences in the
   * Union-Find structure. Uses a rolling 2-slice label buffer for backward
   * neighbor lookups (O(dimX * dimY) memory). Writes provisional labels back
   * to the FeatureIds store for Phase 3 to read.
   *
   * @param featureIdsStore The FeatureIds data store (potentially out-of-core).
   * @param unionFind Union-Find structure for tracking label equivalences.
   * @param nextLabel Next label to assign; incremented as new labels are created.
   * @param dims Image dimensions [X, Y, Z].
   */
  static void phaseOneCCL(Int32AbstractDataStore& featureIdsStore, UnionFind& unionFind, int32& nextLabel, const std::array<int64, 3>& dims);

  /**
   * @brief Phase 2: Flatten the Union-Find to resolve all equivalences.
   *
   * After this call, every label points directly to its root and all per-label
   * sizes are accumulated at root labels. Subsequent find() calls are O(1).
   *
   * @param unionFind Union-Find structure to flatten.
   */
  static void phaseTwoGlobalResolution(UnionFind& unionFind);

  /**
   * @brief Phase 3: Classify regions by size and relabel the FeatureIds store.
   *
   * Reads provisional labels (written during Phase 1) back from the FeatureIds
   * store one Z-slice at a time. Each CCL label is resolved to its root and
   * classified: small regions become -1 (for filling), large regions become 0
   * (optionally assigned to a new phase).
   *
   * @param featureIdsStore The FeatureIds data store to relabel.
   * @param cellPhasesPtr Optional cell phases array (used when storeAsNewPhase is true).
   * @param startLabel First provisional CCL label (= maxExistingFeatureId + 1).
   * @param nextLabel One past the last provisional CCL label assigned.
   * @param unionFind Flattened Union-Find for root resolution.
   * @param maxPhase Maximum existing phase value (large regions get maxPhase + 1).
   */
  void phaseThreeRelabeling(Int32AbstractDataStore& featureIdsStore, Int32Array* cellPhasesPtr, int32 startLabel, int32 nextLabel, UnionFind& unionFind, usize maxPhase) const;

  /**
   * @brief Phase 4: Iterative morphological fill using temp-file deferred I/O.
   *
   * Each iteration scans voxels via a 3-slice rolling window (Pass 1), performs
   * majority voting among face neighbors, and writes (dest, src) pairs to a
   * temporary file. Pass 2 reads pairs back and applies fills using slice-buffered
   * bulk I/O. Repeats until no -1 voxels remain.
   *
   * @param featureIdsStore The FeatureIds data store to fill.
   * @param dims Image dimensions [X, Y, Z].
   * @param numFeatures Maximum feature ID (used to size the vote counter).
   * @return Result indicating success or an error (e.g., temp file creation failure).
   */
  Result<> phaseFourIterativeFill(Int32AbstractDataStore& featureIdsStore, const std::array<int64, 3>& dims, usize numFeatures) const;

  DataStructure& m_DataStructure;                        ///< Reference to the DataStructure containing all arrays
  const FillBadDataInputValues* m_InputValues = nullptr; ///< Non-owning pointer to filter parameter values
  const std::atomic_bool& m_ShouldCancel;                ///< Cancellation flag checked during long-running phases
  const IFilter::MessageHandler& m_MessageHandler;       ///< Handler for emitting progress/informational messages
};

} // namespace nx::core
