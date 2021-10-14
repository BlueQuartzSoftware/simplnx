#include "FindDerivatives.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
std::string FindDerivatives::name() const
{
  return FilterTraits<FindDerivatives>::name.str();
}

Uuid FindDerivatives::uuid() const
{
  return FilterTraits<FindDerivatives>::uuid;
}

std::string FindDerivatives::humanName() const
{
  return "Find Derivatives";
}

Parameters FindDerivatives::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Data Array to Process", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_DerivativesArrayPath_Key, "Derivatives Array", "", DataPath{}));

  return params;
}

IFilter::UniquePointer FindDerivatives::clone() const
{
  return std::make_unique<FindDerivatives>();
}

Result<OutputActions> FindDerivatives::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pDerivativesArrayPathValue = filterArgs.value<DataPath>(k_DerivativesArrayPath_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindDerivativesAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> FindDerivatives::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pDerivativesArrayPathValue = filterArgs.value<DataPath>(k_DerivativesArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
