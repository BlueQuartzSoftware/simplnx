#include "CopyDataContainer.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string CopyDataContainer::name() const
{
  return FilterTraits<CopyDataContainer>::name.str();
}

//------------------------------------------------------------------------------
std::string CopyDataContainer::className() const
{
  return FilterTraits<CopyDataContainer>::className;
}

//------------------------------------------------------------------------------
Uuid CopyDataContainer::uuid() const
{
  return FilterTraits<CopyDataContainer>::uuid;
}

//------------------------------------------------------------------------------
std::string CopyDataContainer::humanName() const
{
  return "Copy Data Container";
}

//------------------------------------------------------------------------------
std::vector<std::string> CopyDataContainer::defaultTags() const
{
  return {"#Core", "#Memory Management"};
}

//------------------------------------------------------------------------------
Parameters CopyDataContainer::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SelectedDataContainerName_Key, "Data Container to Copy", "", DataPath{}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_NewDataContainerName_Key, "Copied Data Container", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CopyDataContainer::clone() const
{
  return std::make_unique<CopyDataContainer>();
}

//------------------------------------------------------------------------------
Result<OutputActions> CopyDataContainer::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSelectedDataContainerNameValue = filterArgs.value<DataPath>(k_SelectedDataContainerName_Key);
  auto pNewDataContainerNameValue = filterArgs.value<DataPath>(k_NewDataContainerName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<CopyDataContainerAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> CopyDataContainer::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSelectedDataContainerNameValue = filterArgs.value<DataPath>(k_SelectedDataContainerName_Key);
  auto pNewDataContainerNameValue = filterArgs.value<DataPath>(k_NewDataContainerName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
