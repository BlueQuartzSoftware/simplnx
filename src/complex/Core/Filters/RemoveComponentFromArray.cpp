#include "RemoveComponentFromArray.hpp"

#include "complex/Core/Parameters/ArrayCreationParameter.hpp"
#include "complex/Core/Parameters/ArraySelectionParameter.hpp"
#include "complex/Core/Parameters/BoolParameter.hpp"
#include "complex/Core/Parameters/NumberParameter.hpp"
#include "complex/DataStructure/DataPath.hpp"

using namespace complex;

namespace complex
{
std::string RemoveComponentFromArray::name() const
{
  return FilterTraits<RemoveComponentFromArray>::name.str();
}

Uuid RemoveComponentFromArray::uuid() const
{
  return FilterTraits<RemoveComponentFromArray>::uuid;
}

std::string RemoveComponentFromArray::humanName() const
{
  return "Remove Component From Array";
}

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

IFilter::UniquePointer RemoveComponentFromArray::clone() const
{
  return std::make_unique<RemoveComponentFromArray>();
}

Result<OutputActions> RemoveComponentFromArray::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pCompNumberValue = filterArgs.value<int32>(k_CompNumber_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pNewArrayArrayNameValue = filterArgs.value<DataPath>(k_NewArrayArrayName_Key);
  auto pReducedArrayArrayNameValue = filterArgs.value<DataPath>(k_ReducedArrayArrayName_Key);
  auto pSaveRemovedComponentValue = filterArgs.value<bool>(k_SaveRemovedComponent_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<RemoveComponentFromArrayAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> RemoveComponentFromArray::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
