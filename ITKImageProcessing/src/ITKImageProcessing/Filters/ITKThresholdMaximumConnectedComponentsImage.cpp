#include "ITKThresholdMaximumConnectedComponentsImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKThresholdMaximumConnectedComponentsImage::name() const
{
  return FilterTraits<ITKThresholdMaximumConnectedComponentsImage>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKThresholdMaximumConnectedComponentsImage::className() const
{
  return FilterTraits<ITKThresholdMaximumConnectedComponentsImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKThresholdMaximumConnectedComponentsImage::uuid() const
{
  return FilterTraits<ITKThresholdMaximumConnectedComponentsImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKThresholdMaximumConnectedComponentsImage::humanName() const
{
  return "ITK::Threshold Maximum Connected Components Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKThresholdMaximumConnectedComponentsImage::defaultTags() const
{
  return {"#ITK Image Processing", "#ITK SegmentationPostProcessing"};
}

//------------------------------------------------------------------------------
Parameters ITKThresholdMaximumConnectedComponentsImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float64Parameter>(k_MinimumObjectSizeInPixels_Key, "MinimumObjectSizeInPixels", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_UpperBoundary_Key, "UpperBoundary", "", 2.3456789));
  params.insert(std::make_unique<Int32Parameter>(k_InsideValue_Key, "InsideValue", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_OutsideValue_Key, "OutsideValue", "", 1234356));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKThresholdMaximumConnectedComponentsImage::clone() const
{
  return std::make_unique<ITKThresholdMaximumConnectedComponentsImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKThresholdMaximumConnectedComponentsImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pMinimumObjectSizeInPixelsValue = filterArgs.value<float64>(k_MinimumObjectSizeInPixels_Key);
  auto pUpperBoundaryValue = filterArgs.value<float64>(k_UpperBoundary_Key);
  auto pInsideValue = filterArgs.value<int32>(k_InsideValue_Key);
  auto pOutsideValue = filterArgs.value<int32>(k_OutsideValue_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.

  // Assuming this filter did make some structural changes to the DataStructure then store
  // the outputAction into the resultOutputActions object via a std::move().
  // NOTE: That using std::move() means that you can *NOT* use the outputAction variable
  // past this point so let us scope this part which will stop stupid subtle bugs
  // from being introduced. If you have multiple `Actions` classes that you are
  // using such as a CreateDataGroupAction followed by a CreateArrayAction you might
  // want to consider scoping each of those bits of code into their own section of code
  {
    // Replace the "EmptyAction" with one of the prebuilt actions that apply changes
    // to the DataStructure. If none are available then create a new custom Action subclass.
    // If your filter does not make any structural modifications to the DataStructure then
    // you can skip this code.

    auto outputAction = std::make_unique<EmptyAction>();
    resultOutputActions.value().actions.push_back(std::move(outputAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ITKThresholdMaximumConnectedComponentsImage::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMinimumObjectSizeInPixelsValue = filterArgs.value<float64>(k_MinimumObjectSizeInPixels_Key);
  auto pUpperBoundaryValue = filterArgs.value<float64>(k_UpperBoundary_Key);
  auto pInsideValue = filterArgs.value<int32>(k_InsideValue_Key);
  auto pOutsideValue = filterArgs.value<int32>(k_OutsideValue_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
