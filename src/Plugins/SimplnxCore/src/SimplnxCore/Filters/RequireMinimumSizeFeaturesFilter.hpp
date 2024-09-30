#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class RequireMinimumSizeFeaturesFilter
 * @brief
 */
class SIMPLNXCORE_EXPORT RequireMinimumSizeFeaturesFilter : public IFilter
{
public:
  RequireMinimumSizeFeaturesFilter() = default;
  ~RequireMinimumSizeFeaturesFilter() noexcept override = default;

  RequireMinimumSizeFeaturesFilter(const RequireMinimumSizeFeaturesFilter&) = delete;
  RequireMinimumSizeFeaturesFilter(RequireMinimumSizeFeaturesFilter&&) noexcept = delete;

  RequireMinimumSizeFeaturesFilter& operator=(const RequireMinimumSizeFeaturesFilter&) = delete;
  RequireMinimumSizeFeaturesFilter& operator=(RequireMinimumSizeFeaturesFilter&&) noexcept = delete;

  static inline constexpr StringLiteral k_FeaturePhasesPath_Key = "feature_phases_path";
  static inline constexpr StringLiteral k_NumCellsPath_Key = "num_cells_path";
  static inline constexpr StringLiteral k_FeatureIdsPath_Key = "feature_ids_path";
  static inline constexpr StringLiteral k_ImageGeomPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_ApplySinglePhase_Key = "apply_single_phase";
  static inline constexpr StringLiteral k_MinAllowedFeaturesSize_Key = "min_allowed_features_size";
  static inline constexpr StringLiteral k_PhaseNumber_Key = "phase_number";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  /**
   * @brief
   * @return
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return
   */
  std::string className() const override;

  /**
   * @brief
   * @return
   */
  Uuid uuid() const override;

  /**
   * @brief
   * @return
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief
   * @return
   */
  Parameters parameters() const override;

  /**
   * @brief Returns parameters version integer.
   * Initial version should always be 1.
   * Should be incremented everytime the parameters change.
   * @return VersionType
   */
  VersionType parametersVersion() const override;

  /**
   * @brief
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
   * @param shouldCancel The atomic boolean that holds if the filter should be canceled
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param pipelineNode The PipelineNode object that called this filter
   * @param messageHandler The MessageHandler object
   * @param shouldCancel The atomic boolean that holds if the filter should be canceled
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, RequireMinimumSizeFeaturesFilter, "074472d3-ba8d-4a1a-99f2-2d56a0d082a0");
