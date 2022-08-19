#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class TiDwellFatigueCrystallographicAnalysis
 * @brief This filter will ....
 */
class DREAM3DREVIEW_EXPORT TiDwellFatigueCrystallographicAnalysis : public IFilter
{
public:
  TiDwellFatigueCrystallographicAnalysis() = default;
  ~TiDwellFatigueCrystallographicAnalysis() noexcept override = default;

  TiDwellFatigueCrystallographicAnalysis(const TiDwellFatigueCrystallographicAnalysis&) = delete;
  TiDwellFatigueCrystallographicAnalysis(TiDwellFatigueCrystallographicAnalysis&&) noexcept = delete;

  TiDwellFatigueCrystallographicAnalysis& operator=(const TiDwellFatigueCrystallographicAnalysis&) = delete;
  TiDwellFatigueCrystallographicAnalysis& operator=(TiDwellFatigueCrystallographicAnalysis&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_AlphaGlobPhasePresent_Key = "AlphaGlobPhasePresent";
  static inline constexpr StringLiteral k_AlphaGlobPhase_Key = "AlphaGlobPhase";
  static inline constexpr StringLiteral k_MTRPhase_Key = "MTRPhase";
  static inline constexpr StringLiteral k_LatticeParameterA_Key = "LatticeParameterA";
  static inline constexpr StringLiteral k_LatticeParameterC_Key = "LatticeParameterC";
  static inline constexpr StringLiteral k_StressAxis_Key = "StressAxis";
  static inline constexpr StringLiteral k_SubsurfaceDistance_Key = "SubsurfaceDistance";
  static inline constexpr StringLiteral k_ConsiderationFraction_Key = "ConsiderationFraction";
  static inline constexpr StringLiteral k_DoNotAssumeInitiatorPresence_Key = "DoNotAssumeInitiatorPresence";
  static inline constexpr StringLiteral k_InitiatorLowerThreshold_Key = "InitiatorLowerThreshold";
  static inline constexpr StringLiteral k_InitiatorUpperThreshold_Key = "InitiatorUpperThreshold";
  static inline constexpr StringLiteral k_HardFeatureLowerThreshold_Key = "HardFeatureLowerThreshold";
  static inline constexpr StringLiteral k_HardFeatureUpperThreshold_Key = "HardFeatureUpperThreshold";
  static inline constexpr StringLiteral k_SoftFeatureLowerThreshold_Key = "SoftFeatureLowerThreshold";
  static inline constexpr StringLiteral k_SoftFeatureUpperThreshold_Key = "SoftFeatureUpperThreshold";
  static inline constexpr StringLiteral k_DataContainerName_Key = "DataContainerName";
  static inline constexpr StringLiteral k_FeatureIdsArrayPath_Key = "FeatureIdsArrayPath";
  static inline constexpr StringLiteral k_CellFeatureAttributeMatrixPath_Key = "CellFeatureAttributeMatrixPath";
  static inline constexpr StringLiteral k_FeatureEulerAnglesArrayPath_Key = "FeatureEulerAnglesArrayPath";
  static inline constexpr StringLiteral k_FeaturePhasesArrayPath_Key = "FeaturePhasesArrayPath";
  static inline constexpr StringLiteral k_NeighborListArrayPath_Key = "NeighborListArrayPath";
  static inline constexpr StringLiteral k_CentroidsArrayPath_Key = "CentroidsArrayPath";
  static inline constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "CrystalStructuresArrayPath";
  static inline constexpr StringLiteral k_CellParentIdsArrayName_Key = "CellParentIdsArrayName";
  static inline constexpr StringLiteral k_NewCellFeatureAttributeMatrixName_Key = "NewCellFeatureAttributeMatrixName";
  static inline constexpr StringLiteral k_SelectedFeaturesArrayName_Key = "SelectedFeaturesArrayName";
  static inline constexpr StringLiteral k_InitiatorsArrayName_Key = "InitiatorsArrayName";
  static inline constexpr StringLiteral k_HardFeaturesArrayName_Key = "HardFeaturesArrayName";
  static inline constexpr StringLiteral k_SoftFeaturesArrayName_Key = "SoftFeaturesArrayName";
  static inline constexpr StringLiteral k_HardSoftGroupsArrayName_Key = "HardSoftGroupsArrayName";
  static inline constexpr StringLiteral k_FeatureParentIdsArrayName_Key = "FeatureParentIdsArrayName";

  /**
   * @brief Returns the name of the filter.
   * @return
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return
   */
  std::string className() const override;

  /**
   * @brief Returns the uuid of the filter.
   * @return
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the human readable name of the filter.
   * @return
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns the parameters of the filter (i.e. its inputs)
   * @return
   */
  Parameters parameters() const override;

  /**
   * @brief Returns a copy of the filter.
   * @return
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief Takes in a DataStructure and checks that the filter can be run on it with the given arguments.
   * Returns any warnings/errors. Also returns the changes that would be applied to the DataStructure.
   * Some parts of the actions may not be completely filled out if all the required information is not available at preflight time.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, TiDwellFatigueCrystallographicAnalysis, "98fb00fe-de43-439b-a875-bfdbe3d93412");
