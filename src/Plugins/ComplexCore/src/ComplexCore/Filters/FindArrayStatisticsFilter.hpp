#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class FindArrayStatisticsFilter
 * @brief This filter will ....
 */
class COMPLEXCORE_EXPORT FindArrayStatisticsFilter : public IFilter
{
public:
  FindArrayStatisticsFilter() = default;
  ~FindArrayStatisticsFilter() noexcept override = default;

  FindArrayStatisticsFilter(const FindArrayStatisticsFilter&) = delete;
  FindArrayStatisticsFilter(FindArrayStatisticsFilter&&) noexcept = delete;

  FindArrayStatisticsFilter& operator=(const FindArrayStatisticsFilter&) = delete;
  FindArrayStatisticsFilter& operator=(FindArrayStatisticsFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_FindHistogram_Key = "FindHistogram";
  static inline constexpr StringLiteral k_MinRange_Key = "MinRange";
  static inline constexpr StringLiteral k_MaxRange_Key = "MaxRange";
  static inline constexpr StringLiteral k_UseFullRange_Key = "UseFullRange";
  static inline constexpr StringLiteral k_NumBins_Key = "NumBins";
  static inline constexpr StringLiteral k_FindLength_Key = "FindLength";
  static inline constexpr StringLiteral k_FindMin_Key = "FindMin";
  static inline constexpr StringLiteral k_FindMax_Key = "FindMax";
  static inline constexpr StringLiteral k_FindMean_Key = "FindMean";
  static inline constexpr StringLiteral k_FindMedian_Key = "FindMedian";
  static inline constexpr StringLiteral k_FindStdDeviation_Key = "FindStdDeviation";
  static inline constexpr StringLiteral k_FindSummation_Key = "FindSummation";
  static inline constexpr StringLiteral k_UseMask_Key = "UseMask";
  static inline constexpr StringLiteral k_ComputeByIndex_Key = "ComputeByIndex";
  static inline constexpr StringLiteral k_StandardizeData_Key = "StandardizeData";
  static inline constexpr StringLiteral k_SelectedArrayPath_Key = "SelectedArrayPath";
  static inline constexpr StringLiteral k_FeatureIdsArrayPath_Key = "FeatureIdsArrayPath";
  static inline constexpr StringLiteral k_MaskArrayPath_Key = "MaskArrayPath";
  static inline constexpr StringLiteral k_DestinationAttributeMatrix_Key = "DestinationAttributeMatrix";
  static inline constexpr StringLiteral k_HistogramArrayName_Key = "HistogramArrayName";
  static inline constexpr StringLiteral k_LengthArrayName_Key = "LengthArrayName";
  static inline constexpr StringLiteral k_MinimumArrayName_Key = "MinimumArrayName";
  static inline constexpr StringLiteral k_MaximumArrayName_Key = "MaximumArrayName";
  static inline constexpr StringLiteral k_MeanArrayName_Key = "MeanArrayName";
  static inline constexpr StringLiteral k_MedianArrayName_Key = "MedianArrayName";
  static inline constexpr StringLiteral k_StdDeviationArrayName_Key = "StdDeviationArrayName";
  static inline constexpr StringLiteral k_SummationArrayName_Key = "SummationArrayName";
  static inline constexpr StringLiteral k_StandardizedArrayName_Key = "StandardizedArrayName";

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
  OutputActions createCompatibleArrays(const DataStructure& data, const Arguments& args, usize numBins) const;

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

COMPLEX_DEF_FILTER_TRAITS(complex, FindArrayStatisticsFilter, "645ecae2-cb30-4b53-8165-c9857dfa754f");
