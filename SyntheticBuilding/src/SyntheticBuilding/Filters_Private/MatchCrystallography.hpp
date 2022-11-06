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
  static inline constexpr StringLiteral k_MaxIterations_Key = "max_iterations";
  static inline constexpr StringLiteral k_CellFeatureIdsArrayPath_Key = "feature_ids_path";
  static inline constexpr StringLiteral k_FeaturePhasesArrayPath_Key = "feature_phases_array_path";
  static inline constexpr StringLiteral k_SurfaceFeaturesArrayPath_Key = "surface_features_array_path";
  static inline constexpr StringLiteral k_NeighborListArrayPath_Key = "neighbor_list_array_path";
  static inline constexpr StringLiteral k_SharedSurfaceAreaListArrayPath_Key = "shared_surface_area_list_array_path";
  static inline constexpr StringLiteral k_InputStatsArrayPath_Key = "input_stats_array_path";
  static inline constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "crystal_structures_array_path";
  static inline constexpr StringLiteral k_PhaseTypesArrayPath_Key = "phase_types_array_path";
  static inline constexpr StringLiteral k_NumFeaturesArrayPath_Key = "num_features_array_path";
  static inline constexpr StringLiteral k_CellEulerAnglesArrayName_Key = "cell_euler_angles_array_name";
  static inline constexpr StringLiteral k_VolumesArrayName_Key = "volumes_array_name";
  static inline constexpr StringLiteral k_FeatureEulerAnglesArrayName_Key = "feature_euler_angles_array_name";
  static inline constexpr StringLiteral k_AvgQuatsArrayName_Key = "avg_quats_array_name";

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

COMPLEX_DEF_FILTER_TRAITS(complex, MatchCrystallography, "47de904e-6108-40cb-9c1b-a885e2e2ca8d");
