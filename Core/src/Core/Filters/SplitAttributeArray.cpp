#include "SplitAttributeArray.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string SplitAttributeArray::name() const
{
  return FilterTraits<SplitAttributeArray>::name.str();
}

//------------------------------------------------------------------------------
std::string SplitAttributeArray::className() const
{
  return FilterTraits<SplitAttributeArray>::className;
}

//------------------------------------------------------------------------------
Uuid SplitAttributeArray::uuid() const
{
  return FilterTraits<SplitAttributeArray>::uuid;
}

//------------------------------------------------------------------------------
std::string SplitAttributeArray::humanName() const
{
  return "Split Multicomponent Attribute Array";
}

//------------------------------------------------------------------------------
std::vector<std::string> SplitAttributeArray::defaultTags() const
{
  return {"#Core", "#Memory Management"};
}

//------------------------------------------------------------------------------
Parameters SplitAttributeArray::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputArrayPath_Key, "Multicomponent Attribute Array", "", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_SplitArraysSuffix_Key, "Postfix", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer SplitAttributeArray::clone() const
{
  return std::make_unique<SplitAttributeArray>();
}

//------------------------------------------------------------------------------
Result<OutputActions> SplitAttributeArray::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pInputArrayPathValue = filterArgs.value<DataPath>(k_InputArrayPath_Key);
  auto pSplitArraysSuffixValue = filterArgs.value<StringParameter::ValueType>(k_SplitArraysSuffix_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<SplitAttributeArrayAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> SplitAttributeArray::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputArrayPathValue = filterArgs.value<DataPath>(k_InputArrayPath_Key);
  auto pSplitArraysSuffixValue = filterArgs.value<StringParameter::ValueType>(k_SplitArraysSuffix_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
