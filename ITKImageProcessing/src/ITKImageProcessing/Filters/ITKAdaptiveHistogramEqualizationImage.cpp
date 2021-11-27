#include "ITKAdaptiveHistogramEqualizationImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKAdaptiveHistogramEqualizationImage::name() const
{
  return FilterTraits<ITKAdaptiveHistogramEqualizationImage>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKAdaptiveHistogramEqualizationImage::className() const
{
  return FilterTraits<ITKAdaptiveHistogramEqualizationImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKAdaptiveHistogramEqualizationImage::uuid() const
{
  return FilterTraits<ITKAdaptiveHistogramEqualizationImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKAdaptiveHistogramEqualizationImage::humanName() const
{
  return "ITK::Adaptive Histogram Equalization Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKAdaptiveHistogramEqualizationImage::defaultTags() const
{
  return {"#ITK Image Processing", "#ITK ImageStatistics"};
}

//------------------------------------------------------------------------------
Parameters ITKAdaptiveHistogramEqualizationImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Radius_Key, "Radius", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<Float32Parameter>(k_Alpha_Key, "Alpha", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_Beta_Key, "Beta", "", 1.23345f));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKAdaptiveHistogramEqualizationImage::clone() const
{
  return std::make_unique<ITKAdaptiveHistogramEqualizationImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKAdaptiveHistogramEqualizationImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pRadiusValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Radius_Key);
  auto pAlphaValue = filterArgs.value<float32>(k_Alpha_Key);
  auto pBetaValue = filterArgs.value<float32>(k_Beta_Key);
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
  // None found in this filter based on the filter parameters

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ITKAdaptiveHistogramEqualizationImage::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pRadiusValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Radius_Key);
  auto pAlphaValue = filterArgs.value<float32>(k_Alpha_Key);
  auto pBetaValue = filterArgs.value<float32>(k_Beta_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
