#include "ITKDoubleThresholdImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
std::string ITKDoubleThresholdImage::name() const
{
  return FilterTraits<ITKDoubleThresholdImage>::name.str();
}

Uuid ITKDoubleThresholdImage::uuid() const
{
  return FilterTraits<ITKDoubleThresholdImage>::uuid;
}

std::string ITKDoubleThresholdImage::humanName() const
{
  return "ITK::Double Threshold Image Filter";
}

Parameters ITKDoubleThresholdImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float64Parameter>(k_Threshold1_Key, "Threshold1", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_Threshold2_Key, "Threshold2", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_Threshold3_Key, "Threshold3", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_Threshold4_Key, "Threshold4", "", 2.3456789));
  params.insert(std::make_unique<Int32Parameter>(k_InsideValue_Key, "InsideValue", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_OutsideValue_Key, "OutsideValue", "", 1234356));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "FullyConnected", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

IFilter::UniquePointer ITKDoubleThresholdImage::clone() const
{
  return std::make_unique<ITKDoubleThresholdImage>();
}

Result<OutputActions> ITKDoubleThresholdImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pThreshold1Value = filterArgs.value<float64>(k_Threshold1_Key);
  auto pThreshold2Value = filterArgs.value<float64>(k_Threshold2_Key);
  auto pThreshold3Value = filterArgs.value<float64>(k_Threshold3_Key);
  auto pThreshold4Value = filterArgs.value<float64>(k_Threshold4_Key);
  auto pInsideValueValue = filterArgs.value<int32>(k_InsideValue_Key);
  auto pOutsideValueValue = filterArgs.value<int32>(k_OutsideValue_Key);
  auto pFullyConnectedValue = filterArgs.value<bool>(k_FullyConnected_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ITKDoubleThresholdImageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ITKDoubleThresholdImage::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pThreshold1Value = filterArgs.value<float64>(k_Threshold1_Key);
  auto pThreshold2Value = filterArgs.value<float64>(k_Threshold2_Key);
  auto pThreshold3Value = filterArgs.value<float64>(k_Threshold3_Key);
  auto pThreshold4Value = filterArgs.value<float64>(k_Threshold4_Key);
  auto pInsideValueValue = filterArgs.value<int32>(k_InsideValue_Key);
  auto pOutsideValueValue = filterArgs.value<int32>(k_OutsideValue_Key);
  auto pFullyConnectedValue = filterArgs.value<bool>(k_FullyConnected_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
