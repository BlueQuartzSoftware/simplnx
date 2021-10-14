#include "ITKDanielssonDistanceMapImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
std::string ITKDanielssonDistanceMapImage::name() const
{
  return FilterTraits<ITKDanielssonDistanceMapImage>::name.str();
}

std::string ITKDanielssonDistanceMapImage::className() const
{
  return FilterTraits<ITKDanielssonDistanceMapImage>::className;
}

Uuid ITKDanielssonDistanceMapImage::uuid() const
{
  return FilterTraits<ITKDanielssonDistanceMapImage>::uuid;
}

std::string ITKDanielssonDistanceMapImage::humanName() const
{
  return "ITK::Danielsson Distance Map Image Filter";
}

Parameters ITKDanielssonDistanceMapImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<BoolParameter>(k_InputIsBinary_Key, "InputIsBinary", "", false));
  params.insert(std::make_unique<BoolParameter>(k_SquaredDistance_Key, "SquaredDistance", "", false));
  params.insert(std::make_unique<BoolParameter>(k_UseImageSpacing_Key, "UseImageSpacing", "", false));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

IFilter::UniquePointer ITKDanielssonDistanceMapImage::clone() const
{
  return std::make_unique<ITKDanielssonDistanceMapImage>();
}

Result<OutputActions> ITKDanielssonDistanceMapImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pInputIsBinaryValue = filterArgs.value<bool>(k_InputIsBinary_Key);
  auto pSquaredDistanceValue = filterArgs.value<bool>(k_SquaredDistance_Key);
  auto pUseImageSpacingValue = filterArgs.value<bool>(k_UseImageSpacing_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ITKDanielssonDistanceMapImageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ITKDanielssonDistanceMapImage::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputIsBinaryValue = filterArgs.value<bool>(k_InputIsBinary_Key);
  auto pSquaredDistanceValue = filterArgs.value<bool>(k_SquaredDistance_Key);
  auto pUseImageSpacingValue = filterArgs.value<bool>(k_UseImageSpacing_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
