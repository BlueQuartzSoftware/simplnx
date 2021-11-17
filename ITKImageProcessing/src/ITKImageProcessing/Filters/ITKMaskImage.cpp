#include "ITKMaskImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKMaskImage::name() const
{
  return FilterTraits<ITKMaskImage>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKMaskImage::className() const
{
  return FilterTraits<ITKMaskImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKMaskImage::uuid() const
{
  return FilterTraits<ITKMaskImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKMaskImage::humanName() const
{
  return "ITK::Mask Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKMaskImage::defaultTags() const
{
  return {"#ITK Image Processing", "#ITK IntensityTransformation"};
}

//------------------------------------------------------------------------------
Parameters ITKMaskImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float64Parameter>(k_OutsideValue_Key, "OutsideValue", "", 2.3456789));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskCellArrayPath_Key, "Mask Array", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKMaskImage::clone() const
{
  return std::make_unique<ITKMaskImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKMaskImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pOutsideValue = filterArgs.value<float64>(k_OutsideValue_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pMaskCellArrayPathValue = filterArgs.value<DataPath>(k_MaskCellArrayPath_Key);
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
  auto action = std::make_unique<ITKMaskImageAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ITKMaskImage::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pOutsideValue = filterArgs.value<float64>(k_OutsideValue_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pMaskCellArrayPathValue = filterArgs.value<DataPath>(k_MaskCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
