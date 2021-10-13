#include "ReplaceValueInArray.hpp"

#include "complex/Core/Parameters/ArraySelectionParameter.hpp"
#include "complex/Core/Parameters/NumberParameter.hpp"
#include "complex/DataStructure/DataPath.hpp"

using namespace complex;

namespace complex
{
std::string ReplaceValueInArray::name() const
{
  return FilterTraits<ReplaceValueInArray>::name.str();
}

Uuid ReplaceValueInArray::uuid() const
{
  return FilterTraits<ReplaceValueInArray>::uuid;
}

std::string ReplaceValueInArray::humanName() const
{
  return "Replace Value in Array";
}

Parameters ReplaceValueInArray::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float64Parameter>(k_RemoveValue_Key, "Value to Replace", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_ReplaceValue_Key, "New Value", "", 2.3456789));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArray_Key, "Attribute Array", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control

  return params;
}

IFilter::UniquePointer ReplaceValueInArray::clone() const
{
  return std::make_unique<ReplaceValueInArray>();
}

Result<OutputActions> ReplaceValueInArray::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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

Result<> ReplaceValueInArray::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
