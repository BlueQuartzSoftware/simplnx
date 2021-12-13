#pragma once

#include "SyntheticBuilding/SyntheticBuilding_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class MatchCrystallography
 * @brief This filter will ....
 */
class SYNTHETICBUILDING_EXPORT MatchCrystallography : public IFilter
{
public:
  MatchCrystallography() = default;
  ~MatchCrystallography() noexcept override = default;

  MatchCrystallography(const MatchCrystallography&) = delete;
  MatchCrystallography(MatchCrystallography&&) noexcept = delete;

  MatchCrystallography& operator=(const MatchCrystallography&) = delete;
  MatchCrystallography& operator=(MatchCrystallography&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_MaxIterations_Key = "MaxIterations";
  static inline constexpr StringLiteral k_FeatureIdsArrayPath_Key = "FeatureIdsArrayPath";
  static inline constexpr StringLiteral k_FeaturePhasesArrayPath_Key = "FeaturePhasesArrayPath";
  static inline constexpr StringLiteral k_SurfaceFeaturesArrayPath_Key = "SurfaceFeaturesArrayPath";
  static inline constexpr StringLiteral k_NeighborListArrayPath_Key = "NeighborListArrayPath";
  static inline constexpr StringLiteral k_SharedSurfaceAreaListArrayPath_Key = "SharedSurfaceAreaListArrayPath";
  static inline constexpr StringLiteral k_InputStatsArrayPath_Key = "InputStatsArrayPath";
  static inline constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "CrystalStructuresArrayPath";
  static inline constexpr StringLiteral k_PhaseTypesArrayPath_Key = "PhaseTypesArrayPath";
  static inline constexpr StringLiteral k_NumFeaturesArrayPath_Key = "NumFeaturesArrayPath";
  static inline constexpr StringLiteral k_CellEulerAnglesArrayName_Key = "CellEulerAnglesArrayName";
  static inline constexpr StringLiteral k_VolumesArrayName_Key = "VolumesArrayName";
  static inline constexpr StringLiteral k_FeatureEulerAnglesArrayName_Key = "FeatureEulerAnglesArrayName";
  static inline constexpr StringLiteral k_AvgQuatsArrayName_Key = "AvgQuatsArrayName";

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

COMPLEX_DEF_FILTER_TRAITS(complex, MatchCrystallography, "7bfb6e4a-6075-56da-8006-b262d99dff30");
