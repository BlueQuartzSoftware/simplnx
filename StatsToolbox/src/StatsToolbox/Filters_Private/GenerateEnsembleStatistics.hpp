#pragma once

#include "StatsToolbox/StatsToolbox_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class GenerateEnsembleStatistics
 * @brief This filter will ....
 */
class STATSTOOLBOX_EXPORT GenerateEnsembleStatistics : public IFilter
{
public:
  GenerateEnsembleStatistics() = default;
  ~GenerateEnsembleStatistics() noexcept override = default;

  GenerateEnsembleStatistics(const GenerateEnsembleStatistics&) = delete;
  GenerateEnsembleStatistics(GenerateEnsembleStatistics&&) noexcept = delete;

  GenerateEnsembleStatistics& operator=(const GenerateEnsembleStatistics&) = delete;
  GenerateEnsembleStatistics& operator=(GenerateEnsembleStatistics&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_PhaseTypeArray_Key = "phase_type_array";
  static inline constexpr StringLiteral k_SizeCorrelationResolution_Key = "size_correlation_resolution";
  static inline constexpr StringLiteral k_FeaturePhasesArrayPath_Key = "feature_phases_array_path";
  static inline constexpr StringLiteral k_NeighborListArrayPath_Key = "neighbor_list_array_path";
  static inline constexpr StringLiteral k_CalculateMorphologicalStats_Key = "calculate_morphological_stats";
  static inline constexpr StringLiteral k_SizeDistributionFitType_Key = "size_distribution_fit_type";
  static inline constexpr StringLiteral k_BiasedFeaturesArrayPath_Key = "biased_features_array_path";
  static inline constexpr StringLiteral k_EquivalentDiametersArrayPath_Key = "equivalent_diameters_array_path";
  static inline constexpr StringLiteral k_AspectRatioDistributionFitType_Key = "aspect_ratio_distribution_fit_type";
  static inline constexpr StringLiteral k_AspectRatiosArrayPath_Key = "aspect_ratios_array_path";
  static inline constexpr StringLiteral k_Omega3DistributionFitType_Key = "omega3_distribution_fit_type";
  static inline constexpr StringLiteral k_Omega3sArrayPath_Key = "omega3s_array_path";
  static inline constexpr StringLiteral k_NeighborhoodDistributionFitType_Key = "neighborhood_distribution_fit_type";
  static inline constexpr StringLiteral k_NeighborhoodsArrayPath_Key = "neighborhoods_array_path";
  static inline constexpr StringLiteral k_AxisEulerAnglesArrayPath_Key = "axis_euler_angles_array_path";
  static inline constexpr StringLiteral k_CalculateCrystallographicStats_Key = "calculate_crystallographic_stats";
  static inline constexpr StringLiteral k_SurfaceFeaturesArrayPath_Key = "surface_features_array_path";
  static inline constexpr StringLiteral k_VolumesArrayPath_Key = "volumes_array_path";
  static inline constexpr StringLiteral k_FeatureEulerAnglesArrayPath_Key = "feature_euler_angles_array_path";
  static inline constexpr StringLiteral k_AvgQuatsArrayPath_Key = "avg_quats_array_path";
  static inline constexpr StringLiteral k_SharedSurfaceAreaListArrayPath_Key = "shared_surface_area_list_array_path";
  static inline constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "crystal_structures_array_path";
  static inline constexpr StringLiteral k_PhaseTypesArrayName_Key = "phase_types_array_name";
  static inline constexpr StringLiteral k_StatisticsArrayName_Key = "statistics_array_name";
  static inline constexpr StringLiteral k_IncludeRadialDistFunc_Key = "include_radial_dist_func";
  static inline constexpr StringLiteral k_RDFArrayPath_Key = "r_df_array_path";
  static inline constexpr StringLiteral k_MaxMinRDFArrayPath_Key = "max_min_rd_farray_path";

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

COMPLEX_DEF_FILTER_TRAITS(complex, GenerateEnsembleStatistics, "643e29d3-5af4-4d51-be57-060ae9f0ba37");
