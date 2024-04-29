#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class FindFeaturePhasesFilter
 * @brief This Filter determines the Ensemble of each Feature by querying the
 * Ensemble of the Elements that belong to the Feature. Note that it is assumed
 * that all Elements belonging to a Feature are of the same Feature, and thus
 * any Element can be used to determine the Ensemble of the Feature that owns
 * that Element.
 */
class SIMPLNXCORE_EXPORT FindFeaturePhasesFilter : public IFilter
{
public:
  FindFeaturePhasesFilter() = default;
  ~FindFeaturePhasesFilter() noexcept override = default;

  FindFeaturePhasesFilter(const FindFeaturePhasesFilter&) = delete;
  FindFeaturePhasesFilter(FindFeaturePhasesFilter&&) noexcept = delete;

  FindFeaturePhasesFilter& operator=(const FindFeaturePhasesFilter&) = delete;
  FindFeaturePhasesFilter& operator=(FindFeaturePhasesFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_CellPhasesArrayPath_Key = "cell_phases_array_path";
  static inline constexpr StringLiteral k_CellFeatureIdsArrayPath_Key = "feature_ids_path";
  static inline constexpr StringLiteral k_FeaturePhasesArrayName_Key = "feature_phases_array_name";
  static inline constexpr StringLiteral k_CellFeaturesAttributeMatrixPath_Key = "cell_features_attribute_matrix_path";

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
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, FindFeaturePhasesFilter, "da5bb20e-4a8e-49d9-9434-fbab7bc434fc");
