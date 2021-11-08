#include "RenameAttributeArray.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string RenameAttributeArray::name() const
{
  return FilterTraits<RenameAttributeArray>::name.str();
}

//------------------------------------------------------------------------------
std::string RenameAttributeArray::className() const
{
  return FilterTraits<RenameAttributeArray>::className;
}

//------------------------------------------------------------------------------
Uuid RenameAttributeArray::uuid() const
{
  return FilterTraits<RenameAttributeArray>::uuid;
}

//------------------------------------------------------------------------------
std::string RenameAttributeArray::humanName() const
{
  return "Rename Attribute Array";
}

//------------------------------------------------------------------------------
std::vector<std::string> RenameAttributeArray::defaultTags() const
{
  return {"#Core", "#Memory Management"};
}

//------------------------------------------------------------------------------
Parameters RenameAttributeArray::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Attribute Array to Rename", "", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_NewArrayName_Key, "New Attribute Array Name", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RenameAttributeArray::clone() const
{
  return std::make_unique<RenameAttributeArray>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult RenameAttributeArray::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pNewArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<RenameAttributeArrayAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> RenameAttributeArray::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pNewArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
