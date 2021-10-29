#include "ConditionalSetValue.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ConditionalSetValue::name() const
{
  return FilterTraits<ConditionalSetValue>::name.str();
}

//------------------------------------------------------------------------------
std::string ConditionalSetValue::className() const
{
  return FilterTraits<ConditionalSetValue>::className;
}

//------------------------------------------------------------------------------
Uuid ConditionalSetValue::uuid() const
{
  return FilterTraits<ConditionalSetValue>::uuid;
}

//------------------------------------------------------------------------------
std::string ConditionalSetValue::humanName() const
{
  return "Replace Value in Array (Conditional)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ConditionalSetValue::defaultTags() const
{
  return {"#Core", "#Memory Management"};
}

//------------------------------------------------------------------------------
Parameters ConditionalSetValue::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float64Parameter>(k_ReplaceValue_Key, "New Value", "", 2.3456789));
  params.insert(std::make_unique<ArraySelectionParameter>(k_ConditionalArrayPath_Key, "Conditional Array", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Attribute Array", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ConditionalSetValue::clone() const
{
  return std::make_unique<ConditionalSetValue>();
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
Result<> ConditionalSetValue::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
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
