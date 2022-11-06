#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class EstablishFoamMorphology
 * @brief This filter will ....
 */
class DREAM3DREVIEW_EXPORT EstablishFoamMorphology : public IFilter
{
public:
  EstablishFoamMorphology() = default;
  ~EstablishFoamMorphology() noexcept override = default;

  EstablishFoamMorphology(const EstablishFoamMorphology&) = delete;
  EstablishFoamMorphology(EstablishFoamMorphology&&) noexcept = delete;

  EstablishFoamMorphology& operator=(const EstablishFoamMorphology&) = delete;
  EstablishFoamMorphology& operator=(EstablishFoamMorphology&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_PeriodicBoundaries_Key = "periodic_boundaries";
  static inline constexpr StringLiteral k_DataContainerName_Key = "data_container_name";
  static inline constexpr StringLiteral k_InputStatsArrayPath_Key = "input_stats_array_path";
  static inline constexpr StringLiteral k_InputPhaseTypesArrayPath_Key = "input_phase_types_array_path";
  static inline constexpr StringLiteral k_InputPhaseNamesArrayPath_Key = "input_phase_names_array_path";
  static inline constexpr StringLiteral k_InputShapeTypesArrayPath_Key = "input_shape_types_array_path";
  static inline constexpr StringLiteral k_InputCellFeatureIdsArrayPath_Key = "input_cell_feature_ids_array_path";
  static inline constexpr StringLiteral k_OutputCellEnsembleAttributeMatrixName_Key = "output_cell_ensemble_attribute_matrix_name";
  static inline constexpr StringLiteral k_NumFeaturesArrayName_Key = "num_features_array_name";
  static inline constexpr StringLiteral k_OutputCellFeatureAttributeMatrixName_Key = "output_cell_feature_attribute_matrix_name";
  static inline constexpr StringLiteral k_FeatureIdsArrayName_Key = "feature_ids_array_name";
  static inline constexpr StringLiteral k_MaskArrayName_Key = "mask_array_name";
  static inline constexpr StringLiteral k_CellPhasesArrayName_Key = "cell_phases_array_name";
  static inline constexpr StringLiteral k_QPEuclideanDistancesArrayName_Key = "q_peuclidean_distances_array_name";
  static inline constexpr StringLiteral k_TJEuclideanDistancesArrayName_Key = "t_jeuclidean_distances_array_name";
  static inline constexpr StringLiteral k_GBEuclideanDistancesArrayName_Key = "g_beuclidean_distances_array_name";
  static inline constexpr StringLiteral k_WriteGoalAttributes_Key = "write_goal_attributes";
  static inline constexpr StringLiteral k_CsvOutputFile_Key = "csv_output_file";
  static inline constexpr StringLiteral k_HaveFeatures_Key = "have_features";
  static inline constexpr StringLiteral k_MinStrutThickness_Key = "min_strut_thickness";
  static inline constexpr StringLiteral k_StrutThicknessVariability_Key = "strut_thickness_variability";
  static inline constexpr StringLiteral k_StrutShapeVariability_Key = "strut_shape_variability";

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

COMPLEX_DEF_FILTER_TRAITS(complex, EstablishFoamMorphology, "bd76bba1-00d4-4acd-88e0-05578525a8cb");
