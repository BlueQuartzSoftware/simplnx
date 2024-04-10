#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class FindFeatureClusteringFilter
 * @brief This filter determines the radial distribution function (RDF), as a histogram, of a given set of Features.
 */
class SIMPLNXCORE_EXPORT FindFeatureClusteringFilter : public IFilter
{
public:
  FindFeatureClusteringFilter() = default;
  ~FindFeatureClusteringFilter() noexcept override = default;

  FindFeatureClusteringFilter(const FindFeatureClusteringFilter&) = delete;
  FindFeatureClusteringFilter(FindFeatureClusteringFilter&&) noexcept = delete;

  FindFeatureClusteringFilter& operator=(const FindFeatureClusteringFilter&) = delete;
  FindFeatureClusteringFilter& operator=(FindFeatureClusteringFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedImageGeometryPath_Key = "selected_image_geometry_path";
  static inline constexpr StringLiteral k_NumberOfBins_Key = "number_of_bins";
  static inline constexpr StringLiteral k_PhaseNumber_Key = "phase_number";
  static inline constexpr StringLiteral k_RemoveBiasedFeatures_Key = "remove_biased_features";
  static inline constexpr StringLiteral k_SetRandomSeed_Key = "set_random_seed";
  static inline constexpr StringLiteral k_SeedValue_Key = "seed_value";
  static inline constexpr StringLiteral k_SeedArrayName_Key = "seed_array_name";
  static inline constexpr StringLiteral k_EquivalentDiametersArrayPath_Key = "equivalent_diameters_array_path";
  static inline constexpr StringLiteral k_FeaturePhasesArrayPath_Key = "feature_phases_array_path";
  static inline constexpr StringLiteral k_CentroidsArrayPath_Key = "centroids_array_path";
  static inline constexpr StringLiteral k_BiasedFeaturesArrayPath_Key = "biased_features_array_path";
  static inline constexpr StringLiteral k_CellEnsembleAttributeMatrixPath_Key = "cell_ensemble_attribute_matrix_path";
  static inline constexpr StringLiteral k_ClusteringListArrayName_Key = "clustering_list_array_name";
  static inline constexpr StringLiteral k_RDFArrayName_Key = "rdf_array_name";
  static inline constexpr StringLiteral k_MaxMinArrayName_Key = "max_min_array_name";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

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
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, FindFeatureClusteringFilter, "d6e01678-3a03-433f-89ad-4e9adf1f9a45");
/* LEGACY UUID FOR THIS FILTER a1e9cf6d-2d1b-573e-98b8-0314c993d2b6 */
