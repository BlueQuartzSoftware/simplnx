#include "CreateDataContainer.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"

using namespace complex;

namespace complex
{
std::string CreateDataContainer::name() const
{
  return FilterTraits<CreateDataContainer>::name.str();
}

std::string CreateDataContainer::className() const
{
  return FilterTraits<CreateDataContainer>::className;
}

Uuid CreateDataContainer::uuid() const
{
  return FilterTraits<CreateDataContainer>::uuid;
}

std::string CreateDataContainer::humanName() const
{
  return "Create Data Container";
}

Parameters CreateDataContainer::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<DataGroupCreationParameter>(k_DataContainerName_Key, "Data Container Name", "", DataPath{}));

  return params;
}

IFilter::UniquePointer CreateDataContainer::clone() const
{
  return std::make_unique<CreateDataContainer>();
}

Result<OutputActions> CreateDataContainer::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<CreateDataContainerAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> CreateDataContainer::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
