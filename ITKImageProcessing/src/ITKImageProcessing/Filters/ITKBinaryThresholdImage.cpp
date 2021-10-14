#include "ITKBinaryThresholdImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
std::string ITKBinaryThresholdImage::name() const
{
  return FilterTraits<ITKBinaryThresholdImage>::name.str();
}

std::string ITKBinaryThresholdImage::className() const
{
  return FilterTraits<ITKBinaryThresholdImage>::className;
}

Uuid ITKBinaryThresholdImage::uuid() const
{
  return FilterTraits<ITKBinaryThresholdImage>::uuid;
}

std::string ITKBinaryThresholdImage::humanName() const
{
  return "ITK::Binary Threshold Image Filter";
}

Parameters ITKBinaryThresholdImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float64Parameter>(k_LowerThreshold_Key, "LowerThreshold", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_UpperThreshold_Key, "UpperThreshold", "", 2.3456789));
  params.insert(std::make_unique<Int32Parameter>(k_InsideValue_Key, "InsideValue", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_OutsideValue_Key, "OutsideValue", "", 1234356));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

IFilter::UniquePointer ITKBinaryThresholdImage::clone() const
{
  return std::make_unique<ITKBinaryThresholdImage>();
}

Result<OutputActions> ITKBinaryThresholdImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pLowerThresholdValue = filterArgs.value<float64>(k_LowerThreshold_Key);
  auto pUpperThresholdValue = filterArgs.value<float64>(k_UpperThreshold_Key);
  auto pInsideValueValue = filterArgs.value<int32>(k_InsideValue_Key);
  auto pOutsideValueValue = filterArgs.value<int32>(k_OutsideValue_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ITKBinaryThresholdImageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ITKBinaryThresholdImage::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pLowerThresholdValue = filterArgs.value<float64>(k_LowerThreshold_Key);
  auto pUpperThresholdValue = filterArgs.value<float64>(k_UpperThreshold_Key);
  auto pInsideValueValue = filterArgs.value<int32>(k_InsideValue_Key);
  auto pOutsideValueValue = filterArgs.value<int32>(k_OutsideValue_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
