#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ComputeBoundingBoxStatsFilter
 * @brief This filter will ....
 */
class SIMPLNXCORE_EXPORT ComputeBoundingBoxStatsFilter : public IFilter
{
public:
  ComputeBoundingBoxStatsFilter() = default;
  ~ComputeBoundingBoxStatsFilter() noexcept override = default;

  ComputeBoundingBoxStatsFilter(const ComputeBoundingBoxStatsFilter&) = delete;
  ComputeBoundingBoxStatsFilter(ComputeBoundingBoxStatsFilter&&) noexcept = delete;

  ComputeBoundingBoxStatsFilter& operator=(const ComputeBoundingBoxStatsFilter&) = delete;
  ComputeBoundingBoxStatsFilter& operator=(ComputeBoundingBoxStatsFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteralType k_CalculateLength_Key = "calculate_length";
  static constexpr StringLiteralType k_CalculateMin_Key = "calculate_min";
  static constexpr StringLiteralType k_CalculateMax_Key = "calculate_max";
  static constexpr StringLiteralType k_CalculateSummation_Key = "calculate_summation";
  static constexpr StringLiteralType k_CalculateMean_Key = "calculate_mean";
  static constexpr StringLiteralType k_CalculateMedian_Key = "calculate_median";
  static constexpr StringLiteralType k_CalculateMode_Key = "calculate_mode";
  static constexpr StringLiteralType k_CalculateUniqueValues_Key = "calculate_num_unique_values";
  static constexpr StringLiteralType k_CalculateStandardDeviation_Key = "calculate_standard_deviation";

  static constexpr StringLiteralType k_GeometryPath_Key = "geometry_path";
  static constexpr StringLiteralType k_UnifiedBoundsPath_Key = "unified_bounds_path";
  static constexpr StringLiteralType k_InputArrayPath_Key = "input_array_path";

  static constexpr StringLiteralType k_CreateNewAM_Key = "create_new_am";
  static constexpr StringLiteralType k_OutputAMPath_Key = "output_am_path";
  static constexpr StringLiteralType k_NewAMName_Key = "new_am_name";

  static constexpr StringLiteralType k_BoundsHasDataName_Key = "bounds_has_data_name";
  static constexpr StringLiteralType k_LengthName_Key = "length_name";
  static constexpr StringLiteralType k_MinName_Key = "min_name";
  static constexpr StringLiteralType k_MaxName_Key = "max_name";
  static constexpr StringLiteralType k_SummationName_Key = "summation_name";
  static constexpr StringLiteralType k_MeanName_Key = "mean_name";
  static constexpr StringLiteralType k_MedianName_Key = "median_name";
  static constexpr StringLiteralType k_ModeName_Key = "mode_name";
  static constexpr StringLiteralType k_NumUniqueValuesName_Key = "num_unique_values_name";
  static constexpr StringLiteralType k_StdDevName_Key = "std_dev_name";

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
   * @brief Returns the human-readable name of the filter.
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
   * @brief Returns parameters version integer.
   * Initial version should always be 1.
   * Should be incremented everytime the parameters change.
   * @return VersionType
   */
  VersionType parametersVersion() const override;

  /**
   * @brief Returns a copy of the filter.
   * @return
   */
  UniquePointerType clone() const override;

protected:
  /**
   * @brief Takes in a DataStructure and checks that the filter can be run on it with the given arguments.
   * Returns any warnings/errors. Also returns the changes that would be applied to the DataStructure.
   * Some parts of the actions may not be completely filled out if all the required information is not available at preflight time.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @param shouldCancel Atomic boolean value that can be checked to cancel the filter
   * @param executionContext The ExecutionContext that can be used to determine the correct absolute path from a relative path
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param pipelineNode The node in the pipeline that is being executed
   * @param messageHandler The MessageHandler object
   * @param shouldCancel Atomic boolean value that can be checked to cancel the filter
   * @param executionContext The ExecutionContext that can be used to determine the correct absolute path from a relative path
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ComputeBoundingBoxStatsFilter, "6b1e0fc9-0365-4a13-a353-91ec34941cea");
