#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class KMedoidsFilter
 * @brief This filter will ....
 */
class COMPLEXCORE_EXPORT KMedoidsFilter : public IFilter
{
public:
  KMedoidsFilter() = default;
  ~KMedoidsFilter() noexcept override = default;

  KMedoidsFilter(const KMedoidsFilter&) = delete;
  KMedoidsFilter(KMedoidsFilter&&) noexcept = delete;

  KMedoidsFilter& operator=(const KMedoidsFilter&) = delete;
  KMedoidsFilter& operator=(KMedoidsFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InitClusters_Key = "init_clusters";
  static inline constexpr StringLiteral k_DistanceMetric_Key = "distance_metric";
  static inline constexpr StringLiteral k_UseMask_Key = "use_mask";
  static inline constexpr StringLiteral k_SelectedArrayPath_Key = "selected_array_path";
  static inline constexpr StringLiteral k_MaskArrayPath_Key = "mask_array_path";
  static inline constexpr StringLiteral k_FeatureIdsArrayName_Key = "feature_ids_array_name";
  static inline constexpr StringLiteral k_FeatureAMPath_Key = "feature_attribute_matrix_path";
  static inline constexpr StringLiteral k_MedoidsArrayName_Key = "medoids_array_name";
  static inline constexpr StringLiteral k_UseSeed_Key = "use_seed";
  static inline constexpr StringLiteral k_SeedValue_Key = "seed_value";

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

COMPLEX_DEF_FILTER_TRAITS(complex, KMedoidsFilter, "7643cb64-bcb9-4867-b85a-bf158cd0f54a");
/* LEGACY UUID FOR THIS FILTER f7486aa6-3049-5be7-8511-ae772b70c90b */
