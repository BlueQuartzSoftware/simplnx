#include "ConditionalSetValue.hpp"

#include "complex/Core/Parameters/ArraySelectionParameter.hpp"
#include "complex/Core/Parameters/NumberParameter.hpp"
#include "complex/DataStructure/DataPath.hpp"

using namespace complex;

namespace complex
{
std::string ConditionalSetValue::name() const
{
  return FilterTraits<ConditionalSetValue>::name.str();
}

Uuid ConditionalSetValue::uuid() const
{
  return FilterTraits<ConditionalSetValue>::uuid;
}

std::string ConditionalSetValue::humanName() const
{
  return "Replace Value in Array (Conditional)";
}

Parameters ConditionalSetValue::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float64Parameter>(k_ReplaceValue_Key, "New Value", "", 2.3456789));
  params.insert(std::make_unique<ArraySelectionParameter>(k_ConditionalArrayPath_Key, "Conditional Array", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Attribute Array", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control

  return params;
}

IFilter::UniquePointer ConditionalSetValue::clone() const
{
  return std::make_unique<ConditionalSetValue>();
}

Result<OutputActions> ConditionalSetValue::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pReplaceValueValue = filterArgs.value<float64>(k_ReplaceValue_Key);
  auto pConditionalArrayPathValue = filterArgs.value<DataPath>(k_ConditionalArrayPath_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ConditionalSetValueAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ConditionalSetValue::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pReplaceValueValue = filterArgs.value<float64>(k_ReplaceValue_Key);
  auto pConditionalArrayPathValue = filterArgs.value<DataPath>(k_ConditionalArrayPath_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
