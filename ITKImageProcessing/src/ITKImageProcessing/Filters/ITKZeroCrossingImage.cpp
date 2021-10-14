#include "ITKZeroCrossingImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
std::string ITKZeroCrossingImage::name() const
{
  return FilterTraits<ITKZeroCrossingImage>::name.str();
}

std::string ITKZeroCrossingImage::className() const
{
  return FilterTraits<ITKZeroCrossingImage>::className;
}

Uuid ITKZeroCrossingImage::uuid() const
{
  return FilterTraits<ITKZeroCrossingImage>::uuid;
}

std::string ITKZeroCrossingImage::humanName() const
{
  return "ITK::Zero Crossing Image Filter";
}

Parameters ITKZeroCrossingImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Int32Parameter>(k_ForegroundValue_Key, "ForegroundValue", "", 1234356));
  params.insert(std::make_unique<Int32Parameter>(k_BackgroundValue_Key, "BackgroundValue", "", 1234356));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

IFilter::UniquePointer ITKZeroCrossingImage::clone() const
{
  return std::make_unique<ITKZeroCrossingImage>();
}

Result<OutputActions> ITKZeroCrossingImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pForegroundValueValue = filterArgs.value<int32>(k_ForegroundValue_Key);
  auto pBackgroundValueValue = filterArgs.value<int32>(k_BackgroundValue_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ITKZeroCrossingImageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ITKZeroCrossingImage::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pForegroundValueValue = filterArgs.value<int32>(k_ForegroundValue_Key);
  auto pBackgroundValueValue = filterArgs.value<int32>(k_BackgroundValue_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
