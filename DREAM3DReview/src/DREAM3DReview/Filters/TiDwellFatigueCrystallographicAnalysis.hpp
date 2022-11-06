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
  static inline constexpr StringLiteral k_AlphaGlobPhasePresent_Key = "alpha_glob_phase_present";
  static inline constexpr StringLiteral k_AlphaGlobPhase_Key = "alpha_glob_phase";
  static inline constexpr StringLiteral k_MTRPhase_Key = "m_tr_phase";
  static inline constexpr StringLiteral k_LatticeParameterA_Key = "lattice_parameter_a";
  static inline constexpr StringLiteral k_LatticeParameterC_Key = "lattice_parameter_c";
  static inline constexpr StringLiteral k_StressAxis_Key = "stress_axis";
  static inline constexpr StringLiteral k_SubsurfaceDistance_Key = "subsurface_distance";
  static inline constexpr StringLiteral k_ConsiderationFraction_Key = "consideration_fraction";
  static inline constexpr StringLiteral k_DoNotAssumeInitiatorPresence_Key = "do_not_assume_initiator_presence";
  static inline constexpr StringLiteral k_InitiatorLowerThreshold_Key = "initiator_lower_threshold";
  static inline constexpr StringLiteral k_InitiatorUpperThreshold_Key = "initiator_upper_threshold";
  static inline constexpr StringLiteral k_HardFeatureLowerThreshold_Key = "hard_feature_lower_threshold";
  static inline constexpr StringLiteral k_HardFeatureUpperThreshold_Key = "hard_feature_upper_threshold";
  static inline constexpr StringLiteral k_SoftFeatureLowerThreshold_Key = "soft_feature_lower_threshold";
  static inline constexpr StringLiteral k_SoftFeatureUpperThreshold_Key = "soft_feature_upper_threshold";
  static inline constexpr StringLiteral k_DataContainerName_Key = "data_container_name";
  static inline constexpr StringLiteral k_CellFeatureIdsArrayPath_Key = "feature_ids_path";
  static inline constexpr StringLiteral k_CellFeatureAttributeMatrixPath_Key = "cell_feature_attribute_matrix_path";
  static inline constexpr StringLiteral k_FeatureEulerAnglesArrayPath_Key = "feature_euler_angles_array_path";
  static inline constexpr StringLiteral k_FeaturePhasesArrayPath_Key = "feature_phases_array_path";
  static inline constexpr StringLiteral k_NeighborListArrayPath_Key = "neighbor_list_array_path";
  static inline constexpr StringLiteral k_CentroidsArrayPath_Key = "centroids_array_path";
  static inline constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "crystal_structures_array_path";
  static inline constexpr StringLiteral k_CellParentIdsArrayName_Key = "cell_parent_ids_array_name";
  static inline constexpr StringLiteral k_NewCellFeatureAttributeMatrixName_Key = "new_cell_feature_attribute_matrix_name";
  static inline constexpr StringLiteral k_SelectedFeaturesArrayName_Key = "selected_features_array_name";
  static inline constexpr StringLiteral k_InitiatorsArrayName_Key = "initiators_array_name";
  static inline constexpr StringLiteral k_HardFeaturesArrayName_Key = "hard_features_array_name";
  static inline constexpr StringLiteral k_SoftFeaturesArrayName_Key = "soft_features_array_name";
  static inline constexpr StringLiteral k_HardSoftGroupsArrayName_Key = "hard_soft_groups_array_name";
  static inline constexpr StringLiteral k_FeatureParentIdsArrayName_Key = "feature_parent_ids_array_name";

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
