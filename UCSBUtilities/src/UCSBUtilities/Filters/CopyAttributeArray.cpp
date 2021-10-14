#include "CopyAttributeArray.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
std::string CopyAttributeArray::name() const
{
  return FilterTraits<CopyAttributeArray>::name.str();
}

std::string CopyAttributeArray::className() const
{
  return FilterTraits<CopyAttributeArray>::className;
}

Uuid CopyAttributeArray::uuid() const
{
  return FilterTraits<CopyAttributeArray>::uuid;
}

std::string CopyAttributeArray::humanName() const
{
  return "Copy Attribute Array";
}

Parameters CopyAttributeArray::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Attribute Array to Copy", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewArrayName_Key, "Copied Attribute Array", "", DataPath{}));

  return params;
}

IFilter::UniquePointer CopyAttributeArray::clone() const
{
  return std::make_unique<CopyAttributeArray>();
}

Result<OutputActions> CopyAttributeArray::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pNewArrayNameValue = filterArgs.value<DataPath>(k_NewArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<CopyAttributeArrayAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> CopyAttributeArray::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pNewArrayNameValue = filterArgs.value<DataPath>(k_NewArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
