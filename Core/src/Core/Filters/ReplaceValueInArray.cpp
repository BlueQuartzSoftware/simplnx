#include "ReplaceValueInArray.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ReplaceValueInArray::name() const
{
  return FilterTraits<ReplaceValueInArray>::name.str();
}

//------------------------------------------------------------------------------
std::string ReplaceValueInArray::className() const
{
  return FilterTraits<ReplaceValueInArray>::className;
}

//------------------------------------------------------------------------------
Uuid ReplaceValueInArray::uuid() const
{
  return FilterTraits<ReplaceValueInArray>::uuid;
}

//------------------------------------------------------------------------------
std::string ReplaceValueInArray::humanName() const
{
  return "Replace Value in Array";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReplaceValueInArray::defaultTags() const
{
  return {"#Core", "#Memory Management"};
}

//------------------------------------------------------------------------------
Parameters ReplaceValueInArray::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float64Parameter>(k_RemoveValue_Key, "Value to Replace", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_ReplaceValue_Key, "New Value", "", 2.3456789));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArray_Key, "Attribute Array", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReplaceValueInArray::clone() const
{
  return std::make_unique<ReplaceValueInArray>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReplaceValueInArray::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pRemoveValueValue = filterArgs.value<float64>(k_RemoveValue_Key);
  auto pReplaceValueValue = filterArgs.value<float64>(k_ReplaceValue_Key);
  auto pSelectedArrayValue = filterArgs.value<DataPath>(k_SelectedArray_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ReplaceValueInArrayAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ReplaceValueInArray::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pRemoveValueValue = filterArgs.value<float64>(k_RemoveValue_Key);
  auto pReplaceValueValue = filterArgs.value<float64>(k_ReplaceValue_Key);
  auto pSelectedArrayValue = filterArgs.value<DataPath>(k_SelectedArray_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
