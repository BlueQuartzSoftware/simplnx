#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class FindBiasedFeaturesFilter
 * @brief This filter determines which Features are biased by the outer surfaces of the sample.
 */
class COMPLEXCORE_EXPORT FindBiasedFeaturesFilter : public IFilter
{
public:
  FindBiasedFeaturesFilter() = default;
  ~FindBiasedFeaturesFilter() noexcept override = default;

  FindBiasedFeaturesFilter(const FindBiasedFeaturesFilter&) = delete;
  FindBiasedFeaturesFilter(FindBiasedFeaturesFilter&&) noexcept = delete;

  FindBiasedFeaturesFilter& operator=(const FindBiasedFeaturesFilter&) = delete;
  FindBiasedFeaturesFilter& operator=(FindBiasedFeaturesFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_CalcByPhase_Key = "calc_by_phase";
  static inline constexpr StringLiteral k_GeometryPath_Key = "image_geometry_path";
  static inline constexpr StringLiteral k_CentroidsArrayPath_Key = "centroids_array_path";
  static inline constexpr StringLiteral k_SurfaceFeaturesArrayPath_Key = "surface_features_array_path";
  static inline constexpr StringLiteral k_PhasesArrayPath_Key = "phases_array_path";
  static inline constexpr StringLiteral k_BiasedFeaturesArrayName_Key = "biased_features_array_name";

  /**
   * @brief Reads SIMPL json and converts it complex Arguments.
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
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, FindBiasedFeaturesFilter, "d46f2fd7-dc68-4b57-bca3-693016512b2f");
/* LEGACY UUID FOR THIS FILTER 450c2f00-9ddf-56e1-b4c1-0e74e7ad2349 */
