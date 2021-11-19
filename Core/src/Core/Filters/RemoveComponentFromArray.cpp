#include "RemoveComponentFromArray.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
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

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.

  // Assuming this filter did make some structural changes to the DataStructure then store
  // the outputAction into the resultOutputActions object via a std::move().
  // NOTE: That using std::move() means that you can *NOT* use the outputAction variable
  // past this point so let us scope this part which will stop stupid subtle bugs
  // from being introduced. If you have multiple `Actions` classes that you are
  // using such as a CreateDataGroupAction followed by a CreateArrayAction you might
  // want to consider scoping each of those bits of code into their own section of code
  {
    // Replace the "EmptyAction" with one of the prebuilt actions that apply changes
    // to the DataStructure. If none are available then create a new custom Action subclass.
    // If your filter does not make any structural modifications to the DataStructure then
    // you can skip this code.

    auto outputAction = std::make_unique<EmptyAction>();
    resultOutputActions.value().actions.push_back(std::move(outputAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
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
