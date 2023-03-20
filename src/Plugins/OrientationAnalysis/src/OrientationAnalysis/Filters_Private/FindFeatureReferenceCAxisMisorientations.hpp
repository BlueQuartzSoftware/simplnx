#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class FindFeatureReferenceCAxisMisorientations
 * @brief This filter will ....
 */
class ORIENTATIONANALYSIS_EXPORT FindFeatureReferenceCAxisMisorientations : public IFilter
{
public:
  FindFeatureReferenceCAxisMisorientations() = default;
  ~FindFeatureReferenceCAxisMisorientations() noexcept override = default;

  FindFeatureReferenceCAxisMisorientations(const FindFeatureReferenceCAxisMisorientations&) = delete;
  FindFeatureReferenceCAxisMisorientations(FindFeatureReferenceCAxisMisorientations&&) noexcept = delete;

  FindFeatureReferenceCAxisMisorientations& operator=(const FindFeatureReferenceCAxisMisorientations&) = delete;
  FindFeatureReferenceCAxisMisorientations& operator=(FindFeatureReferenceCAxisMisorientations&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_CellFeatureIdsArrayPath_Key = "feature_ids_path";
  static inline constexpr StringLiteral k_CellPhasesArrayPath_Key = "cell_phases_array_path";
  static inline constexpr StringLiteral k_QuatsArrayPath_Key = "quats_array_path";
  static inline constexpr StringLiteral k_AvgCAxesArrayPath_Key = "avg_caxes_array_path";
  static inline constexpr StringLiteral k_FeatureAvgCAxisMisorientationsArrayName_Key = "feature_avg_caxis_misorientations_array_name";
  static inline constexpr StringLiteral k_FeatureStdevCAxisMisorientationsArrayName_Key = "feature_stdev_caxis_misorientations_array_name";
  static inline constexpr StringLiteral k_FeatureReferenceCAxisMisorientationsArrayName_Key = "feature_reference_caxis_misorientations_array_name";

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
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, FindFeatureReferenceCAxisMisorientations, "16c487d2-8f99-4fb5-a4df-d3f70a8e6b25");
