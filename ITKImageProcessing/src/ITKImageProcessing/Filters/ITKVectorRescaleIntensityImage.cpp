#include "ITKVectorRescaleIntensityImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
std::string ITKVectorRescaleIntensityImage::name() const
{
  return FilterTraits<ITKVectorRescaleIntensityImage>::name.str();
}

Uuid ITKVectorRescaleIntensityImage::uuid() const
{
  return FilterTraits<ITKVectorRescaleIntensityImage>::uuid;
}

std::string ITKVectorRescaleIntensityImage::humanName() const
{
  return "ITK::Vector Rescale Intensity Image Filter";
}

Parameters ITKVectorRescaleIntensityImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ChoicesParameter>(k_OutputType_Key, "Output Component Type", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insert(std::make_unique<Float64Parameter>(k_OutputMaximumMagnitude_Key, "OutputMaximumMagnitude", "", 2.3456789));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

IFilter::UniquePointer ITKVectorRescaleIntensityImage::clone() const
{
  return std::make_unique<ITKVectorRescaleIntensityImage>();
}

Result<OutputActions> ITKVectorRescaleIntensityImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pOutputTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_OutputType_Key);
  auto pOutputMaximumMagnitudeValue = filterArgs.value<float64>(k_OutputMaximumMagnitude_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ITKVectorRescaleIntensityImageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ITKVectorRescaleIntensityImage::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pOutputTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_OutputType_Key);
  auto pOutputMaximumMagnitudeValue = filterArgs.value<float64>(k_OutputMaximumMagnitude_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
