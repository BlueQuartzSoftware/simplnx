#include "ITKOtsuMultipleThresholdsImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKOtsuMultipleThresholdsImage::name() const
{
  return FilterTraits<ITKOtsuMultipleThresholdsImage>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKOtsuMultipleThresholdsImage::className() const
{
  return FilterTraits<ITKOtsuMultipleThresholdsImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKOtsuMultipleThresholdsImage::uuid() const
{
  return FilterTraits<ITKOtsuMultipleThresholdsImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKOtsuMultipleThresholdsImage::humanName() const
{
  return "ITK::Otsu Multiple Thresholds Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKOtsuMultipleThresholdsImage::defaultTags() const
{
  return {"#ITK Image Processing", "#ITK Thresholding"};
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKOtsuMultipleThresholdsImage::clone() const
{
  return std::make_unique<ITKOtsuMultipleThresholdsImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKOtsuMultipleThresholdsImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pNumberOfThresholdsValue = filterArgs.value<int32>(k_NumberOfThresholds_Key);
  auto pLabelOffsetValue = filterArgs.value<int32>(k_LabelOffset_Key);
  auto pNumberOfHistogramBinsValue = filterArgs.value<float64>(k_NumberOfHistogramBins_Key);
  auto pValleyEmphasisValue = filterArgs.value<bool>(k_ValleyEmphasis_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

#if 0
  // Define the OutputActions Object that will hold the actions that would take
  // place if the filter were to execute. This is mainly what would happen to the
  // DataStructure during this filter, i.e., what modificationst to the DataStructure
  // would take place.
  OutputActions actions;
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ITKOtsuMultipleThresholdsImageAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ITKOtsuMultipleThresholdsImage::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
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
