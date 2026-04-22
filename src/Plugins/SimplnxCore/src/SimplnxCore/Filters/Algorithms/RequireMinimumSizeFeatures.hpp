#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

namespace nx::core
{

/**
 * @struct RequireMinimumSizeFeaturesInputValues
 * @brief Holds all user-configured parameters for the RequireMinimumSizeFeatures algorithm.
 */
struct SIMPLNXCORE_EXPORT RequireMinimumSizeFeaturesInputValues
{
  BoolParameter::ValueType ApplySinglePhase;                    ///< If true, only remove small features in one phase.
  ArraySelectionParameter::ValueType FeatureIdsPath;            ///< Per-cell Feature ID array (int32).
  ArraySelectionParameter::ValueType FeaturePhasesPath;         ///< Per-feature phase array (for single-phase mode).
  GeometrySelectionParameter::ValueType InputImageGeometryPath; ///< Input ImageGeom.
  Int64Parameter::ValueType MinAllowedFeaturesSize;             ///< Minimum voxel count threshold.
  ArraySelectionParameter::ValueType FeatureNumCellsPath;       ///< Per-feature voxel count array.
  Int32Parameter::ValueType PhaseNumber;                        ///< Phase to filter (when ApplySinglePhase is true).
};

/**
 * @class RequireMinimumSizeFeatures
 * @brief Removes features with fewer voxels than a user-specified minimum threshold,
 * then iteratively fills the resulting gaps by voting among face-neighbor feature IDs.
 *
 * @section ooc_optimization Out-of-Core Optimization
 * Two operations were optimized:
 *
 * **removeSmallFeatures()**: The original per-element setValue(-1) loop for marking
 * removed features caused a chunk operation per voxel. The optimized version reads
 * FeatureIds in 64K-tuple chunks via copyIntoBuffer(), modifies the buffer in-place,
 * and writes back only modified chunks via copyFromBuffer().
 *
 * **assignBadVoxels()**: The original per-element getValue() in the voting loop caused
 * chunk thrashing across the entire volume on every iteration. The optimized version
 * uses a rolling 3-slice buffer: for each Z-slice, the current slice and its Z-neighbors
 * are in memory so all 6 face-neighbor reads come from local buffers. The buffer slides
 * forward one slice at a time. Changed voxels and their chosen neighbors are tracked in
 * compact parallel sparse vectors (O(bad_voxels), not O(n_cells)) — the earlier dense
 * per-voxel index vector has been removed. The transfer phase dispatches one
 * ChunkedTransferWorker per cell-level array via ParallelTaskAlgorithm; each worker
 * does Z-batched bulk I/O (copyIntoBuffer + in-memory edits + copyFromBuffer) to
 * replace per-voxel copyTuple() with a small number of HDF5 chunk operations.
 */
class SIMPLNXCORE_EXPORT RequireMinimumSizeFeatures
{
public:
  RequireMinimumSizeFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RequireMinimumSizeFeaturesInputValues* inputValues);
  ~RequireMinimumSizeFeatures() noexcept;

  RequireMinimumSizeFeatures(const RequireMinimumSizeFeatures&) = delete;
  RequireMinimumSizeFeatures(RequireMinimumSizeFeatures&&) noexcept = delete;
  RequireMinimumSizeFeatures& operator=(const RequireMinimumSizeFeatures&) = delete;
  RequireMinimumSizeFeatures& operator=(RequireMinimumSizeFeatures&&) noexcept = delete;

  /**
   * @brief Executes the minimum-size filter: removes small features, then fills gaps.
   * @return Result<> indicating success or error.
   */
  Result<> operator()();

protected:
  /**
   * @brief Iteratively fills voxels belonging to removed features (featureId < 0)
   * by voting among their 6 face-neighbors. Uses a rolling 3-slice buffer to avoid
   * per-element OOC access during the voting scan.
   * @param dimensions XYZ dimensions of the ImageGeom.
   * @param featureNumCellsStoreRef Per-feature voxel count array (for vote counter sizing).
   */
  void assignBadVoxels(SizeVec3 dimensions, const Int32AbstractDataStore& featureNumCellsStoreRef);

  /**
   * @brief Marks features below the minimum size as inactive and sets their voxels'
   * Feature IDs to -1 using chunked bulk I/O.
   * @param featureIdsStoreRef Per-cell Feature ID DataStore (modified in-place).
   * @param featureNumCellsStoreRef Per-feature voxel count array.
   * @param featurePhases Per-feature phase array (may be nullptr).
   * @param phaseNumber Target phase number (when applyToSinglePhase is true).
   * @param applyToSinglePhase If true, only remove features in the specified phase.
   * @param minAllowedFeatureSize Minimum voxel count threshold.
   * @param errorReturn Output: receives error details if all features would be removed.
   * @return Vector of booleans indicating which features remain active.
   */
  std::vector<bool> removeSmallFeatures(Int32AbstractDataStore& featureIdsStoreRef, const Int32AbstractDataStore& featureNumCellsStoreRef, const Int32AbstractDataStore* featurePhases,
                                        int32_t phaseNumber, bool applyToSinglePhase, int64 minAllowedFeatureSize, Error& errorReturn);

private:
  DataStructure& m_DataStructure;                                       ///< Reference to the DataStructure.
  const RequireMinimumSizeFeaturesInputValues* m_InputValues = nullptr; ///< User-configured parameters.
  const std::atomic_bool& m_ShouldCancel;                               ///< Cancellation flag.
  const IFilter::MessageHandler& m_MessageHandler;                      ///< Message handler for progress.
};

} // namespace nx::core
