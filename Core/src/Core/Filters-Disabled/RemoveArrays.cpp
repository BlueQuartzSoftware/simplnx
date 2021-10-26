#include "RemoveArrays.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/DataContainerArrayProxyFilterParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string RemoveArrays::name() const
{
  return FilterTraits<RemoveArrays>::name.str();
}

//------------------------------------------------------------------------------
std::string RemoveArrays::className() const
{
  return FilterTraits<RemoveArrays>::className;
}

//------------------------------------------------------------------------------
Uuid RemoveArrays::uuid() const
{
  return FilterTraits<RemoveArrays>::uuid;
}

//------------------------------------------------------------------------------
std::string RemoveArrays::humanName() const
{
  return "Delete Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> RemoveArrays::defaultTags() const
{
  return {"#Core", "#Memory Management"};
}

//------------------------------------------------------------------------------
Parameters RemoveArrays::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  /*[x]*/ params.insert(std::make_unique<DataContainerArrayProxyFilterParameter>(k_DataArraysToRemove_Key, "Objects to Delete", "", {}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RemoveArrays::clone() const
{
  return std::make_unique<RemoveArrays>();
}

//------------------------------------------------------------------------------
Result<OutputActions> RemoveArrays::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pDataArraysToRemoveValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_DataArraysToRemove_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<RemoveArraysAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> RemoveArrays::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pDataArraysToRemoveValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_DataArraysToRemove_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
