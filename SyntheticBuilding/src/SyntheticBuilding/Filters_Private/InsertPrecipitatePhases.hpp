#pragma once

#include "SyntheticBuilding/SyntheticBuilding_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class InsertPrecipitatePhases
 * @brief This filter will ....
 */
class SYNTHETICBUILDING_EXPORT InsertPrecipitatePhases : public IFilter
{
public:
  InsertPrecipitatePhases() = default;
  ~InsertPrecipitatePhases() noexcept override = default;

  InsertPrecipitatePhases(const InsertPrecipitatePhases&) = delete;
  InsertPrecipitatePhases(InsertPrecipitatePhases&&) noexcept = delete;

  InsertPrecipitatePhases& operator=(const InsertPrecipitatePhases&) = delete;
  InsertPrecipitatePhases& operator=(InsertPrecipitatePhases&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_PeriodicBoundaries_Key = "PeriodicBoundaries";
  static inline constexpr StringLiteral k_MatchRDF_Key = "MatchRDF";
  static inline constexpr StringLiteral k_UseMask_Key = "UseMask";
  static inline constexpr StringLiteral k_FeatureIdsArrayPath_Key = "FeatureIdsArrayPath";
  static inline constexpr StringLiteral k_CellPhasesArrayPath_Key = "CellPhasesArrayPath";
  static inline constexpr StringLiteral k_BoundaryCellsArrayPath_Key = "BoundaryCellsArrayPath";
  static inline constexpr StringLiteral k_MaskArrayPath_Key = "MaskArrayPath";
  static inline constexpr StringLiteral k_FeaturePhasesArrayPath_Key = "FeaturePhasesArrayPath";
  static inline constexpr StringLiteral k_InputStatsArrayPath_Key = "InputStatsArrayPath";
  static inline constexpr StringLiteral k_InputPhaseTypesArrayPath_Key = "InputPhaseTypesArrayPath";
  static inline constexpr StringLiteral k_InputShapeTypesArrayPath_Key = "InputShapeTypesArrayPath";
  static inline constexpr StringLiteral k_NumFeaturesArrayPath_Key = "NumFeaturesArrayPath";
  static inline constexpr StringLiteral k_FeatureGeneration_Key = "FeatureGeneration";
  static inline constexpr StringLiteral k_PrecipInputFile_Key = "PrecipInputFile";
  static inline constexpr StringLiteral k_SaveGeometricDescriptions_Key = "SaveGeometricDescriptions";
  static inline constexpr StringLiteral k_NewAttributeMatrixPath_Key = "NewAttributeMatrixPath";
  static inline constexpr StringLiteral k_SelectedAttributeMatrixPath_Key = "SelectedAttributeMatrixPath";

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

COMPLEX_DEF_FILTER_TRAITS(complex, InsertPrecipitatePhases, "ba3c79c0-0e4c-40d2-8a98-bbff04066a3e");
