#include "RemoveComponentFromArray.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string RemoveComponentFromArray::name() const
{
  return FilterTraits<RemoveComponentFromArray>::name.str();
}

//------------------------------------------------------------------------------
std::string RemoveComponentFromArray::className() const
{
  return FilterTraits<RemoveComponentFromArray>::className;
}

//------------------------------------------------------------------------------
Uuid RemoveComponentFromArray::uuid() const
{
  return FilterTraits<RemoveComponentFromArray>::uuid;
}

//------------------------------------------------------------------------------
std::string RemoveComponentFromArray::humanName() const
{
  return "Remove Component From Array";
}

//------------------------------------------------------------------------------
std::vector<std::string> RemoveComponentFromArray::defaultTags() const
{
  return {"#Core", "#Memory Management"};
}

//------------------------------------------------------------------------------
Parameters RemoveComponentFromArray::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Int32Parameter>(k_CompNumber_Key, "Component Number to Remove", "", 1234356));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Multicomponent Attribute Array", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewArrayArrayName_Key, "Removed Component Attribute Array", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_ReducedArrayArrayName_Key, "Reduced Attribute Array", "", DataPath{}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_SaveRemovedComponent_Key, "Save Removed Component in New Array", "", false));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_SaveRemovedComponent_Key, k_NewArrayArrayName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RemoveComponentFromArray::clone() const
{
  return std::make_unique<RemoveComponentFromArray>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult RemoveComponentFromArray::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pCompNumberValue = filterArgs.value<int32>(k_CompNumber_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pNewArrayArrayNameValue = filterArgs.value<DataPath>(k_NewArrayArrayName_Key);
  auto pReducedArrayArrayNameValue = filterArgs.value<DataPath>(k_ReducedArrayArrayName_Key);
  auto pSaveRemovedComponentValue = filterArgs.value<bool>(k_SaveRemovedComponent_Key);

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
  auto action = std::make_unique<RemoveComponentFromArrayAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> RemoveComponentFromArray::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pCompNumberValue = filterArgs.value<int32>(k_CompNumber_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pNewArrayArrayNameValue = filterArgs.value<DataPath>(k_NewArrayArrayName_Key);
  auto pReducedArrayArrayNameValue = filterArgs.value<DataPath>(k_ReducedArrayArrayName_Key);
  auto pSaveRemovedComponentValue = filterArgs.value<bool>(k_SaveRemovedComponent_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
