#pragma once

#include "Core/Core_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class CalculateArrayHistogram
 * @brief This filter calculate the frequency histogram of a data structure
 */
class CORE_EXPORT CalculateArrayHistogramFilter : public IFilter
{
public:
  CalculateArrayHistogramFilter() = default;
  ~CalculateArrayHistogramFilter() noexcept override = default;

  CalculateArrayHistogramFilter(const CalculateArrayHistogramFilter&) = delete;
  CalculateArrayHistogramFilter(CalculateArrayHistogramFilter&&) noexcept = delete;

  CalculateArrayHistogramFilter& operator=(const CalculateArrayHistogramFilter&) = delete;
  CalculateArrayHistogramFilter& operator=(CalculateArrayHistogramFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_NumberOfBins_Key = "number_of_bins";
  static inline constexpr StringLiteral k_UserDefinedRange_Key = "user_defined_range";
  static inline constexpr StringLiteral k_MinRange_Key = "min_range";
  static inline constexpr StringLiteral k_MaxRange_Key = "max_range";
  static inline constexpr StringLiteral k_NewDataGroup_Key = "new_data_group";
  static inline constexpr StringLiteral k_SelectedArrayPaths_Key = "selected_array_paths";
  static inline constexpr StringLiteral k_NewDataGroupName_Key = "new_data_group_name";
  static inline constexpr StringLiteral k_DataGroupName_Key = "data_group_name";
  static inline constexpr StringLiteral k_HistoName_Key = "histogram_suffix";

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

COMPLEX_DEF_FILTER_TRAITS(complex, CalculateArrayHistogramFilter, "c6b6d9e5-301d-4767-abf7-530f5ef5007d");
