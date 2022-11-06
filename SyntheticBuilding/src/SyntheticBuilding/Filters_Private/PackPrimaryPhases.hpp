#pragma once

#include "SyntheticBuilding/SyntheticBuilding_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class PackPrimaryPhases
 * @brief This filter will ....
 */
class SYNTHETICBUILDING_EXPORT PackPrimaryPhases : public IFilter
{
public:
  PackPrimaryPhases() = default;
  ~PackPrimaryPhases() noexcept override = default;

  PackPrimaryPhases(const PackPrimaryPhases&) = delete;
  PackPrimaryPhases(PackPrimaryPhases&&) noexcept = delete;

  PackPrimaryPhases& operator=(const PackPrimaryPhases&) = delete;
  PackPrimaryPhases& operator=(PackPrimaryPhases&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_PeriodicBoundaries_Key = "periodic_boundaries";
  static inline constexpr StringLiteral k_UseMask_Key = "use_mask";
  static inline constexpr StringLiteral k_OutputCellAttributeMatrixPath_Key = "output_cell_attribute_matrix_path";
  static inline constexpr StringLiteral k_MaskArrayPath_Key = "mask_array_path";
  static inline constexpr StringLiteral k_InputStatsArrayPath_Key = "input_stats_array_path";
  static inline constexpr StringLiteral k_InputPhaseTypesArrayPath_Key = "input_phase_types_array_path";
  static inline constexpr StringLiteral k_InputPhaseNamesArrayPath_Key = "input_phase_names_array_path";
  static inline constexpr StringLiteral k_InputShapeTypesArrayPath_Key = "input_shape_types_array_path";
  static inline constexpr StringLiteral k_FeatureIdsArrayName_Key = "feature_ids_array_name";
  static inline constexpr StringLiteral k_CellPhasesArrayName_Key = "cell_phases_array_name";
  static inline constexpr StringLiteral k_OutputCellFeatureAttributeMatrixName_Key = "output_cell_feature_attribute_matrix_name";
  static inline constexpr StringLiteral k_FeaturePhasesArrayName_Key = "feature_phases_array_name";
  static inline constexpr StringLiteral k_OutputCellEnsembleAttributeMatrixName_Key = "output_cell_ensemble_attribute_matrix_name";
  static inline constexpr StringLiteral k_NumFeaturesArrayName_Key = "num_features_array_name";
  static inline constexpr StringLiteral k_FeatureGeneration_Key = "feature_generation";
  static inline constexpr StringLiteral k_FeatureInputFile_Key = "feature_input_file";
  static inline constexpr StringLiteral k_SaveGeometricDescriptions_Key = "save_geometric_descriptions";
  static inline constexpr StringLiteral k_NewAttributeMatrixPath_Key = "new_attribute_matrix_path";
  static inline constexpr StringLiteral k_SelectedAttributeMatrixPath_Key = "selected_attribute_matrix_path";

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

COMPLEX_DEF_FILTER_TRAITS(complex, PackPrimaryPhases, "9b786e15-e01a-458d-82cd-d6483b22d13c");
