#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ComputeArrayStatisticsFilter
 * @brief This filter will ....
 */
class SIMPLNXCORE_EXPORT ComputeArrayStatisticsFilter : public IFilter
{
public:
  ComputeArrayStatisticsFilter() = default;
  ~ComputeArrayStatisticsFilter() noexcept override = default;

  ComputeArrayStatisticsFilter(const ComputeArrayStatisticsFilter&) = delete;
  ComputeArrayStatisticsFilter(ComputeArrayStatisticsFilter&&) noexcept = delete;

  ComputeArrayStatisticsFilter& operator=(const ComputeArrayStatisticsFilter&) = delete;
  ComputeArrayStatisticsFilter& operator=(ComputeArrayStatisticsFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_FindHistogram_Key = "find_histogram";
  static inline constexpr StringLiteral k_MinRange_Key = "min_range";
  static inline constexpr StringLiteral k_MaxRange_Key = "max_range";
  static inline constexpr StringLiteral k_UseFullRange_Key = "use_full_range";
  static inline constexpr StringLiteral k_NumBins_Key = "num_bins";
  static inline constexpr StringLiteral k_FindLength_Key = "find_length";
  static inline constexpr StringLiteral k_FindMin_Key = "find_min";
  static inline constexpr StringLiteral k_FindMax_Key = "find_max";
  static inline constexpr StringLiteral k_FindMean_Key = "find_mean";
  static inline constexpr StringLiteral k_FindMedian_Key = "find_median";
  static inline constexpr StringLiteral k_FindMode_Key = "find_mode";
  static inline constexpr StringLiteral k_FindModalBinRanges_Key = "find_modal_bin_ranges";
  static inline constexpr StringLiteral k_FindStdDeviation_Key = "find_std_deviation";
  static inline constexpr StringLiteral k_FindSummation_Key = "find_summation";
  static inline constexpr StringLiteral k_FindUniqueValues_Key = "find_unique_values";
  static inline constexpr StringLiteral k_UseMask_Key = "use_mask";
  static inline constexpr StringLiteral k_ComputeByIndex_Key = "compute_by_index";
  static inline constexpr StringLiteral k_StandardizeData_Key = "standardize_data";
  static inline constexpr StringLiteral k_SelectedArrayPath_Key = "selected_array_path";
  static inline constexpr StringLiteral k_CellFeatureIdsArrayPath_Key = "feature_ids_path";
  static inline constexpr StringLiteral k_FeatureHasDataArrayName_Key = "feature_has_data_array_name";
  static inline constexpr StringLiteral k_MaskArrayPath_Key = "mask_array_path";
  static inline constexpr StringLiteral k_DestinationAttributeMatrixPath_Key = "destination_attribute_matrix_path";
  static inline constexpr StringLiteral k_HistoBinCountName_Key = "histogram_bin_count_name";
  static inline constexpr StringLiteral k_HistoBinRangeName_Key = "histogram_bin_range_name";
  static inline constexpr StringLiteral k_MostPopulatedBinArrayName_Key = "most_populated_bin_array_name";
  static inline constexpr StringLiteral k_ModalBinArrayName_Key = "modal_bin_array_name";
  static inline constexpr StringLiteral k_LengthArrayName_Key = "length_array_name";
  static inline constexpr StringLiteral k_MinimumArrayName_Key = "minimum_array_name";
  static inline constexpr StringLiteral k_MaximumArrayName_Key = "maximum_array_name";
  static inline constexpr StringLiteral k_MeanArrayName_Key = "mean_array_name";
  static inline constexpr StringLiteral k_MedianArrayName_Key = "median_array_name";
  static inline constexpr StringLiteral k_ModeArrayName_Key = "mode_array_name";
  static inline constexpr StringLiteral k_StdDeviationArrayName_Key = "std_deviation_array_name";
  static inline constexpr StringLiteral k_SummationArrayName_Key = "summation_array_name";
  static inline constexpr StringLiteral k_StandardizedArrayName_Key = "standardized_array_name";
  static inline constexpr StringLiteral k_NumUniqueValuesName_Key = "number_unique_values_name";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ComputeArrayStatisticsFilter, "645ecae2-cb30-4b53-8165-c9857dfa754f");
