#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class FindNumFeaturesFilter
 * @brief This filter will create a new array containing the number of features
 */
class COMPLEXCORE_EXPORT FindNumFeaturesFilter : public IFilter
{
public:
  FindNumFeaturesFilter() = default;
  ~FindNumFeaturesFilter() noexcept override = default;

  FindNumFeaturesFilter(const FindNumFeaturesFilter&) = delete;
  FindNumFeaturesFilter(FindNumFeaturesFilter&&) noexcept = delete;

  FindNumFeaturesFilter& operator=(const FindNumFeaturesFilter&) = delete;
  FindNumFeaturesFilter& operator=(FindNumFeaturesFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_FeaturePhasesArrayPath_Key = "feature_phases_array_path";
  static inline constexpr StringLiteral k_NumFeaturesArrayPath_Key = "num_features_array_path";

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
   * @param dataGraph The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataGraph, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataGraph The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, FindNumFeaturesFilter, "ec3388de-d7a7-4cf6-94f4-81061b0863c8");
/* LEGACY UUID FOR THIS FILTER 529743cf-d5d5-5d5a-a79f-95c84a5ddbb5 */
