#include "ITKShiftScaleImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
std::string ITKShiftScaleImage::name() const
{
  return FilterTraits<ITKShiftScaleImage>::name.str();
}

std::string ITKShiftScaleImage::className() const
{
  return FilterTraits<ITKShiftScaleImage>::className;
}

Uuid ITKShiftScaleImage::uuid() const
{
  return FilterTraits<ITKShiftScaleImage>::uuid;
}

std::string ITKShiftScaleImage::humanName() const
{
  return "ITK::Shift Scale Image Filter";
}

Parameters ITKShiftScaleImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float64Parameter>(k_Shift_Key, "Shift", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_Scale_Key, "Scale", "", 2.3456789));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

IFilter::UniquePointer ITKShiftScaleImage::clone() const
{
  return std::make_unique<ITKShiftScaleImage>();
}

Result<OutputActions> ITKShiftScaleImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pShiftValue = filterArgs.value<float64>(k_Shift_Key);
  auto pScaleValue = filterArgs.value<float64>(k_Scale_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ITKShiftScaleImageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ITKShiftScaleImage::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pShiftValue = filterArgs.value<float64>(k_Shift_Key);
  auto pScaleValue = filterArgs.value<float64>(k_Scale_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
