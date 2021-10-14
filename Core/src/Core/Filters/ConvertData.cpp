#include "ConvertData.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"

using namespace complex;

namespace complex
{
std::string ConvertData::name() const
{
  return FilterTraits<ConvertData>::name.str();
}

std::string ConvertData::className() const
{
  return FilterTraits<ConvertData>::className;
}

Uuid ConvertData::uuid() const
{
  return FilterTraits<ConvertData>::uuid;
}

std::string ConvertData::humanName() const
{
  return "Convert AttributeArray Data Type";
}

Parameters ConvertData::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<NumericTypeParameter>(k_ScalarType_Key, "Scalar Type", "", NumericType::int8));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to Convert", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputArrayName_Key, "Converted Attribute Array", "", DataPath{}));

  return params;
}

IFilter::UniquePointer ConvertData::clone() const
{
  return std::make_unique<ConvertData>();
}

Result<OutputActions> ConvertData::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pScalarTypeValue = filterArgs.value<NumericType>(k_ScalarType_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pOutputArrayNameValue = filterArgs.value<DataPath>(k_OutputArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ConvertDataAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ConvertData::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pScalarTypeValue = filterArgs.value<NumericType>(k_ScalarType_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pOutputArrayNameValue = filterArgs.value<DataPath>(k_OutputArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
