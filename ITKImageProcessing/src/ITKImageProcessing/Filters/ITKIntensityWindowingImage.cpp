#include "ITKIntensityWindowingImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKIntensityWindowingImage::name() const
{
  return FilterTraits<ITKIntensityWindowingImage>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKIntensityWindowingImage::className() const
{
  return FilterTraits<ITKIntensityWindowingImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKIntensityWindowingImage::uuid() const
{
  return FilterTraits<ITKIntensityWindowingImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKIntensityWindowingImage::humanName() const
{
  return "ITK::Intensity Windowing Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKIntensityWindowingImage::defaultTags() const
{
  return {"#ITK Image Processing", "#ITK IntensityTransformation"};
}

//------------------------------------------------------------------------------
Parameters ITKIntensityWindowingImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float64Parameter>(k_WindowMinimum_Key, "WindowMinimum", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_WindowMaximum_Key, "WindowMaximum", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_OutputMinimum_Key, "OutputMinimum", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_OutputMaximum_Key, "OutputMaximum", "", 2.3456789));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKIntensityWindowingImage::clone() const
{
  return std::make_unique<ITKIntensityWindowingImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKIntensityWindowingImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pWindowMinimumValue = filterArgs.value<float64>(k_WindowMinimum_Key);
  auto pWindowMaximumValue = filterArgs.value<float64>(k_WindowMaximum_Key);
  auto pOutputMinimumValue = filterArgs.value<float64>(k_OutputMinimum_Key);
  auto pOutputMaximumValue = filterArgs.value<float64>(k_OutputMaximum_Key);
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
  auto action = std::make_unique<ITKIntensityWindowingImageAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ITKIntensityWindowingImage::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pWindowMinimumValue = filterArgs.value<float64>(k_WindowMinimum_Key);
  auto pWindowMaximumValue = filterArgs.value<float64>(k_WindowMaximum_Key);
  auto pOutputMinimumValue = filterArgs.value<float64>(k_OutputMinimum_Key);
  auto pOutputMaximumValue = filterArgs.value<float64>(k_OutputMaximum_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
