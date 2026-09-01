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
 * @brief Stores size, phase, geometry, and array selections.
 */
struct SIMPLNXCORE_EXPORT RequireMinimumSizeFeaturesInputValues
{
  BoolParameter::ValueType ApplySinglePhase;
  ArraySelectionParameter::ValueType FeatureIdsPath;
  ArraySelectionParameter::ValueType FeaturePhasesPath;
  GeometrySelectionParameter::ValueType InputImageGeometryPath;
  Int64Parameter::ValueType MinAllowedFeaturesSize;
  ArraySelectionParameter::ValueType FeatureNumCellsPath;
  Int32Parameter::ValueType PhaseNumber;
};

/**
 * @class RequireMinimumSizeFeatures
 * @brief Removes features below a cell-count threshold and fills their cells.
 *
 * Single-phase mode removes only small features in the selected phase. Surviving
 * Feature IDs are compacted during the same 65,536-cell pass that marks removed
 * cells as negative. This avoids a second complete Feature ID renumbering pass.
 *
 * FillBadVoxels processes all cell IDataArray siblings sequentially through
 * rolling slices and updates Feature IDs last. This order gives each array the
 * same assignment snapshot. Cancellation between arrays can leave a sibling
 * array ahead of Feature IDs, so canceled output must be discarded.
 *
 * Resident scratch is feature-scale active and renumber state, one fixed Feature
 * ID chunk, and rolling slices for one cell array. The marking pass discards its
 * bulk-I/O results. The final feature-compaction return value is also discarded.
 * Either condition can make this method return success with partial output.
 */
class SIMPLNXCORE_EXPORT RequireMinimumSizeFeatures
{
public:
  /**
   * @brief Initializes the minimum-size removal algorithm.
   * @param dataStructure Contains image, cell, and feature data.
   * @param mesgHandler Receives removal and fill messages.
   * @param shouldCancel Signals cancellation between phases, chunks, and slices.
   * @param inputValues Selects the threshold, phase, and paths.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  RequireMinimumSizeFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RequireMinimumSizeFeaturesInputValues* inputValues);
  /**
   * @brief Destroys the minimum-size removal algorithm.
   */
  ~RequireMinimumSizeFeatures() noexcept;

  RequireMinimumSizeFeatures(const RequireMinimumSizeFeatures&) = delete;
  RequireMinimumSizeFeatures(RequireMinimumSizeFeatures&&) noexcept = delete;
  RequireMinimumSizeFeatures& operator=(const RequireMinimumSizeFeatures&) = delete;
  RequireMinimumSizeFeatures& operator=(RequireMinimumSizeFeatures&&) noexcept = delete;

  /**
   * @brief Removes selected features, fills their cells, and compacts feature data.
   * @return Phase, all-removed, or fill result.
   * @pre Feature IDs are negative or index FeatureNumCellsPath.
   * @pre Cell arrays match the ImageGeom cell dimensions.
   * @pre FeaturePhasesPath is valid when ApplySinglePhase is true.
   *
   * An out-of-range nonnegative Feature ID causes unchecked active-flag access.
   * Cancellation returns success without rollback. The final feature compaction
   * can fail or cancel without changing this method's success result.
   */
  Result<> operator()();

protected:
  /**
   * @brief Fills negative Feature IDs from face-neighbor majority votes.
   * @param dimensions ImageGeom dimensions in X, Y, and Z order.
   * @return Feature ID, unfillable-region, or bulk-transfer result.
   *
   * Sibling arrays update sequentially. Feature IDs update last.
   */
  Result<> assignBadVoxels(SizeVec3 dimensions);

  /**
   * @brief Selects active features and applies their compacted IDs to cells.
   * @param featureIdsStoreRef Feature IDs modified in place.
   * @param featureNumCellsStoreRef Supplies one cell count per feature.
   * @param featurePhases Supplies phases, or null when phase filtering is off.
   * @param phaseNumber Selected phase when phase filtering is on.
   * @param applyToSinglePhase True to restrict removal to phaseNumber.
   * @param minAllowedFeatureSize Minimum retained cell count.
   * @param errorReturn Receives the all-removed error.
   * @return Active flags indexed by original Feature ID. Returns empty after cancellation.
   * @pre featurePhases is not null when applyToSinglePhase is true.
   * @pre Nonnegative Feature IDs index the returned active flags.
   *
   * Bulk-I/O results are not inspected. Cancellation or a storage failure can
   * leave earlier Feature ID chunks compacted.
   */
  std::vector<bool> removeSmallFeatures(Int32AbstractDataStore& featureIdsStoreRef, const Int32AbstractDataStore& featureNumCellsStoreRef, const Int32AbstractDataStore* featurePhases,
                                        int32_t phaseNumber, bool applyToSinglePhase, int64 minAllowedFeatureSize, Error& errorReturn);

private:
  DataStructure& m_DataStructure;
  const RequireMinimumSizeFeaturesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
