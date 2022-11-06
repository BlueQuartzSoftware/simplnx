#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class FindNeighborListStatistics
 * @brief This filter will ....
 */
class DREAM3DREVIEW_EXPORT FindNeighborListStatistics : public IFilter
{
public:
  FindNeighborListStatistics() = default;
  ~FindNeighborListStatistics() noexcept override = default;

  FindNeighborListStatistics(const FindNeighborListStatistics&) = delete;
  FindNeighborListStatistics(FindNeighborListStatistics&&) noexcept = delete;

  FindNeighborListStatistics& operator=(const FindNeighborListStatistics&) = delete;
  FindNeighborListStatistics& operator=(FindNeighborListStatistics&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_FindLength_Key = "find_length";
  static inline constexpr StringLiteral k_FindMin_Key = "find_min";
  static inline constexpr StringLiteral k_FindMax_Key = "find_max";
  static inline constexpr StringLiteral k_FindMean_Key = "find_mean";
  static inline constexpr StringLiteral k_FindMedian_Key = "find_median";
  static inline constexpr StringLiteral k_FindStdDeviation_Key = "find_std_deviation";
  static inline constexpr StringLiteral k_FindSummation_Key = "find_summation";
  static inline constexpr StringLiteral k_SelectedArrayPath_Key = "selected_array_path";
  static inline constexpr StringLiteral k_DestinationAttributeMatrix_Key = "destination_attribute_matrix";
  static inline constexpr StringLiteral k_LengthArrayName_Key = "length_array_name";
  static inline constexpr StringLiteral k_MinimumArrayName_Key = "minimum_array_name";
  static inline constexpr StringLiteral k_MaximumArrayName_Key = "maximum_array_name";
  static inline constexpr StringLiteral k_MeanArrayName_Key = "mean_array_name";
  static inline constexpr StringLiteral k_MedianArrayName_Key = "median_array_name";
  static inline constexpr StringLiteral k_StdDeviationArrayName_Key = "std_deviation_array_name";
  static inline constexpr StringLiteral k_SummationArrayName_Key = "summation_array_name";

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

COMPLEX_DEF_FILTER_TRAITS(complex, FindNeighborListStatistics, "81c0cad6-378f-4462-adf1-32f542ce65c9");
