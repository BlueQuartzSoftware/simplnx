#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ComputeArrayHistogram
 * @brief This filter calculate the frequency histogram of a data structure
 */
class SIMPLNXCORE_EXPORT ComputeArrayHistogramFilter : public IFilter
{
public:
  ComputeArrayHistogramFilter() = default;
  ~ComputeArrayHistogramFilter() noexcept override = default;

  ComputeArrayHistogramFilter(const ComputeArrayHistogramFilter&) = delete;
  ComputeArrayHistogramFilter(ComputeArrayHistogramFilter&&) noexcept = delete;

  ComputeArrayHistogramFilter& operator=(const ComputeArrayHistogramFilter&) = delete;
  ComputeArrayHistogramFilter& operator=(ComputeArrayHistogramFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_NumberOfBins_Key = "number_of_bins";
  static inline constexpr StringLiteral k_UserDefinedRange_Key = "user_defined_range";
  static inline constexpr StringLiteral k_MinRange_Key = "min_range";
  static inline constexpr StringLiteral k_MaxRange_Key = "max_range";
  static inline constexpr StringLiteral k_CreateNewDataGroup_Key = "create_new_data_group";
  static inline constexpr StringLiteral k_SelectedArrayPaths_Key = "selected_array_paths";
  static inline constexpr StringLiteral k_NewDataGroupPath_Key = "new_data_group_path";
  static inline constexpr StringLiteral k_DataGroupPath_Key = "output_data_group_path";
  static inline constexpr StringLiteral k_HistoBinCountName_Key = "histogram_bin_count_suffix";
  static inline constexpr StringLiteral k_HistoBinRangeName_Key = "histogram_bin_range_suffix";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ComputeArrayHistogramFilter, "c6b6d9e5-301d-4767-abf7-530f5ef5007d");
