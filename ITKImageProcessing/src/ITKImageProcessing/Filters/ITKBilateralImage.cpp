#include "ITKBilateralImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKBilateralImage::name() const
{
  return FilterTraits<ITKBilateralImage>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKBilateralImage::className() const
{
  return FilterTraits<ITKBilateralImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKBilateralImage::uuid() const
{
  return FilterTraits<ITKBilateralImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKBilateralImage::humanName() const
{
  return "ITK::Bilateral Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKBilateralImage::defaultTags() const
{
  return {"#ITK Image Processing", "#ITK Smoothing"};
}

//------------------------------------------------------------------------------
Parameters ITKBilateralImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float64Parameter>(k_DomainSigma_Key, "DomainSigma", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_RangeSigma_Key, "RangeSigma", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_NumberOfRangeGaussianSamples_Key, "NumberOfRangeGaussianSamples", "", 2.3456789));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKBilateralImage::clone() const
{
  return std::make_unique<ITKBilateralImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKBilateralImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pDomainSigmaValue = filterArgs.value<float64>(k_DomainSigma_Key);
  auto pRangeSigmaValue = filterArgs.value<float64>(k_RangeSigma_Key);
  auto pNumberOfRangeGaussianSamplesValue = filterArgs.value<float64>(k_NumberOfRangeGaussianSamples_Key);
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
  auto action = std::make_unique<ITKBilateralImageAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ITKBilateralImage::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pDomainSigmaValue = filterArgs.value<float64>(k_DomainSigma_Key);
  auto pRangeSigmaValue = filterArgs.value<float64>(k_RangeSigma_Key);
  auto pNumberOfRangeGaussianSamplesValue = filterArgs.value<float64>(k_NumberOfRangeGaussianSamples_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
