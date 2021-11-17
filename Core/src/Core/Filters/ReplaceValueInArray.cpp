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

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pRemoveValue = filterArgs.value<float64>(k_RemoveValue_Key);
  auto pReplaceValue = filterArgs.value<float64>(k_ReplaceValue_Key);
  auto pSelectedArrayValue = filterArgs.value<DataPath>(k_SelectedArray_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

#if 0
  // Define the OutputActions Object that will hold the actions that would take
  // place if the filter were to execute. This is mainly what would happen to the
  // DataStructure during this filter, i.e., what modificationst to the DataStructure
  // would take place.
  OutputActions actions;
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ReplaceValueInArrayAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ReplaceValueInArray::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pRemoveValue = filterArgs.value<float64>(k_RemoveValue_Key);
  auto pReplaceValue = filterArgs.value<float64>(k_ReplaceValue_Key);
  auto pSelectedArrayValue = filterArgs.value<DataPath>(k_SelectedArray_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
