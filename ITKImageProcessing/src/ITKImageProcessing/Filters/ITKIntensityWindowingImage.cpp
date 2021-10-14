#include "ITKIntensityWindowingImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
std::string ITKIntensityWindowingImage::name() const
{
  return FilterTraits<ITKIntensityWindowingImage>::name.str();
}

Uuid ITKIntensityWindowingImage::uuid() const
{
  return FilterTraits<ITKIntensityWindowingImage>::uuid;
}

std::string ITKIntensityWindowingImage::humanName() const
{
  return "ITK::Intensity Windowing Image Filter";
}

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

IFilter::UniquePointer ITKIntensityWindowingImage::clone() const
{
  return std::make_unique<ITKIntensityWindowingImage>();
}

Result<OutputActions> ITKIntensityWindowingImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pWindowMinimumValue = filterArgs.value<float64>(k_WindowMinimum_Key);
  auto pWindowMaximumValue = filterArgs.value<float64>(k_WindowMaximum_Key);
  auto pOutputMinimumValue = filterArgs.value<float64>(k_OutputMinimum_Key);
  auto pOutputMaximumValue = filterArgs.value<float64>(k_OutputMaximum_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ITKIntensityWindowingImageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ITKIntensityWindowingImage::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
