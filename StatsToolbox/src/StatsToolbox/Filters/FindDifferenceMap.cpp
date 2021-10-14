#include "FindDifferenceMap.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
std::string FindDifferenceMap::name() const
{
  return FilterTraits<FindDifferenceMap>::name.str();
}

Uuid FindDifferenceMap::uuid() const
{
  return FilterTraits<FindDifferenceMap>::uuid;
}

std::string FindDifferenceMap::humanName() const
{
  return "Find Difference Map";
}

Parameters FindDifferenceMap::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ArraySelectionParameter>(k_FirstInputArrayPath_Key, "First Attribute Array", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SecondInputArrayPath_Key, "Second Attribute Array", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_DifferenceMapArrayPath_Key, "Difference Map", "", DataPath{}));

  return params;
}

IFilter::UniquePointer FindDifferenceMap::clone() const
{
  return std::make_unique<FindDifferenceMap>();
}

Result<OutputActions> FindDifferenceMap::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pFirstInputArrayPathValue = filterArgs.value<DataPath>(k_FirstInputArrayPath_Key);
  auto pSecondInputArrayPathValue = filterArgs.value<DataPath>(k_SecondInputArrayPath_Key);
  auto pDifferenceMapArrayPathValue = filterArgs.value<DataPath>(k_DifferenceMapArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindDifferenceMapAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> FindDifferenceMap::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pFirstInputArrayPathValue = filterArgs.value<DataPath>(k_FirstInputArrayPath_Key);
  auto pSecondInputArrayPathValue = filterArgs.value<DataPath>(k_SecondInputArrayPath_Key);
  auto pDifferenceMapArrayPathValue = filterArgs.value<DataPath>(k_DifferenceMapArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
