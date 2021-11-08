#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class FindSchmids
 * @brief This filter will ....
 */
class ORIENTATIONANALYSIS_EXPORT FindSchmids : public IFilter
{
public:
  FindSchmids() = default;
  ~FindSchmids() noexcept override = default;

  FindSchmids(const FindSchmids&) = delete;
  FindSchmids(FindSchmids&&) noexcept = delete;

  FindSchmids& operator=(const FindSchmids&) = delete;
  FindSchmids& operator=(FindSchmids&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_LoadingDirection_Key = "LoadingDirection";
  static inline constexpr StringLiteral k_StoreAngleComponents_Key = "StoreAngleComponents";
  static inline constexpr StringLiteral k_OverrideSystem_Key = "OverrideSystem";
  static inline constexpr StringLiteral k_SlipPlane_Key = "SlipPlane";
  static inline constexpr StringLiteral k_SlipDirection_Key = "SlipDirection";
  static inline constexpr StringLiteral k_FeaturePhasesArrayPath_Key = "FeaturePhasesArrayPath";
  static inline constexpr StringLiteral k_AvgQuatsArrayPath_Key = "AvgQuatsArrayPath";
  static inline constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "CrystalStructuresArrayPath";
  static inline constexpr StringLiteral k_SchmidsArrayName_Key = "SchmidsArrayName";
  static inline constexpr StringLiteral k_SlipSystemsArrayName_Key = "SlipSystemsArrayName";
  static inline constexpr StringLiteral k_PolesArrayName_Key = "PolesArrayName";
  static inline constexpr StringLiteral k_PhisArrayName_Key = "PhisArrayName";
  static inline constexpr StringLiteral k_LambdasArrayName_Key = "LambdasArrayName";

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
  PreflightResult preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, FindSchmids, "89c291a9-1c4d-59c2-ba69-b47beb4de594");
