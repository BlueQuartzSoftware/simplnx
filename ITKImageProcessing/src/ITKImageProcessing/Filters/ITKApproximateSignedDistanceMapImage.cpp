#include "ITKApproximateSignedDistanceMapImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
std::string ITKApproximateSignedDistanceMapImage::name() const
{
  return FilterTraits<ITKApproximateSignedDistanceMapImage>::name.str();
}

Uuid ITKApproximateSignedDistanceMapImage::uuid() const
{
  return FilterTraits<ITKApproximateSignedDistanceMapImage>::uuid;
}

std::string ITKApproximateSignedDistanceMapImage::humanName() const
{
  return "ITK::Approximate Signed Distance Map Image Filter";
}

Parameters ITKApproximateSignedDistanceMapImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float64Parameter>(k_InsideValue_Key, "InsideValue", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_OutsideValue_Key, "OutsideValue", "", 2.3456789));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

IFilter::UniquePointer ITKApproximateSignedDistanceMapImage::clone() const
{
  return std::make_unique<ITKApproximateSignedDistanceMapImage>();
}

Result<OutputActions> ITKApproximateSignedDistanceMapImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pInsideValueValue = filterArgs.value<float64>(k_InsideValue_Key);
  auto pOutsideValueValue = filterArgs.value<float64>(k_OutsideValue_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ITKApproximateSignedDistanceMapImageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ITKApproximateSignedDistanceMapImage::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInsideValueValue = filterArgs.value<float64>(k_InsideValue_Key);
  auto pOutsideValueValue = filterArgs.value<float64>(k_OutsideValue_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
