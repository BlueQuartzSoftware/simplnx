#include "ITKOtsuMultipleThresholdsImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
std::string ITKOtsuMultipleThresholdsImage::name() const
{
  return FilterTraits<ITKOtsuMultipleThresholdsImage>::name.str();
}

std::string ITKOtsuMultipleThresholdsImage::className() const
{
  return FilterTraits<ITKOtsuMultipleThresholdsImage>::className;
}

Uuid ITKOtsuMultipleThresholdsImage::uuid() const
{
  return FilterTraits<ITKOtsuMultipleThresholdsImage>::uuid;
}

std::string ITKOtsuMultipleThresholdsImage::humanName() const
{
  return "ITK::Otsu Multiple Thresholds Image Filter";
}

Parameters ITKOtsuMultipleThresholdsImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Int32Parameter>(k_NumberOfThresholds_Key, "NumberOfThresholds", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_LabelOffset_Key, "LabelOffset", "", 1234356));
  params.insert(std::make_unique<Float64Parameter>(k_NumberOfHistogramBins_Key, "NumberOfHistogramBins", "", 2.3456789));
  params.insert(std::make_unique<BoolParameter>(k_ValleyEmphasis_Key, "ValleyEmphasis", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

IFilter::UniquePointer ITKOtsuMultipleThresholdsImage::clone() const
{
  return std::make_unique<ITKOtsuMultipleThresholdsImage>();
}

Result<OutputActions> ITKOtsuMultipleThresholdsImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pNumberOfThresholdsValue = filterArgs.value<int32>(k_NumberOfThresholds_Key);
  auto pLabelOffsetValue = filterArgs.value<int32>(k_LabelOffset_Key);
  auto pNumberOfHistogramBinsValue = filterArgs.value<float64>(k_NumberOfHistogramBins_Key);
  auto pValleyEmphasisValue = filterArgs.value<bool>(k_ValleyEmphasis_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ITKOtsuMultipleThresholdsImageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ITKOtsuMultipleThresholdsImage::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pNumberOfThresholdsValue = filterArgs.value<int32>(k_NumberOfThresholds_Key);
  auto pLabelOffsetValue = filterArgs.value<int32>(k_LabelOffset_Key);
  auto pNumberOfHistogramBinsValue = filterArgs.value<float64>(k_NumberOfHistogramBins_Key);
  auto pValleyEmphasisValue = filterArgs.value<bool>(k_ValleyEmphasis_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
