#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class FindSchmidsFilter
 * @brief This filter will ....
 */
class ORIENTATIONANALYSIS_EXPORT FindSchmidsFilter : public IFilter
{
public:
  FindSchmidsFilter() = default;
  ~FindSchmidsFilter() noexcept override = default;

  FindSchmidsFilter(const FindSchmidsFilter&) = delete;
  FindSchmidsFilter(FindSchmidsFilter&&) noexcept = delete;

  FindSchmidsFilter& operator=(const FindSchmidsFilter&) = delete;
  FindSchmidsFilter& operator=(FindSchmidsFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_LoadingDirection_Key = "loading_direction";
  static inline constexpr StringLiteral k_StoreAngleComponents_Key = "store_angle_components";
  static inline constexpr StringLiteral k_OverrideSystem_Key = "override_system";
  static inline constexpr StringLiteral k_SlipPlane_Key = "slip_plane";
  static inline constexpr StringLiteral k_SlipDirection_Key = "slip_direction";
  static inline constexpr StringLiteral k_FeaturePhasesArrayPath_Key = "feature_phases_array_path";
  static inline constexpr StringLiteral k_AvgQuatsArrayPath_Key = "avg_quats_array_path";
  static inline constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "crystal_structures_array_path";
  static inline constexpr StringLiteral k_SchmidsArrayName_Key = "schmids_array_name";
  static inline constexpr StringLiteral k_SlipSystemsArrayName_Key = "slip_systems_array_name";
  static inline constexpr StringLiteral k_PolesArrayName_Key = "poles_array_name";
  static inline constexpr StringLiteral k_PhisArrayName_Key = "phis_array_name";
  static inline constexpr StringLiteral k_LambdasArrayName_Key = "lambdas_array_name";

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
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, FindSchmidsFilter, "b4681855-0a3d-4237-97f2-5aec509115c4");
