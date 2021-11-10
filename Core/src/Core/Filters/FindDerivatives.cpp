#include "FindDerivatives.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindDerivatives::name() const
{
  return FilterTraits<FindDerivatives>::name.str();
}

//------------------------------------------------------------------------------
std::string FindDerivatives::className() const
{
  return FilterTraits<FindDerivatives>::className;
}

//------------------------------------------------------------------------------
Uuid FindDerivatives::uuid() const
{
  return FilterTraits<FindDerivatives>::uuid;
}

//------------------------------------------------------------------------------
std::string FindDerivatives::humanName() const
{
  return "Find Derivatives";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindDerivatives::defaultTags() const
{
  return {"#Core", "#Geometry"};
}

//------------------------------------------------------------------------------
Parameters FindDerivatives::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Data Array to Process", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_DerivativesArrayPath_Key, "Derivatives Array", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindDerivatives::clone() const
{
  return std::make_unique<FindDerivatives>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindDerivatives::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pDerivativesArrayPathValue = filterArgs.value<DataPath>(k_DerivativesArrayPath_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

#if 0
  // Define the OutputActions Object that will hold the actions that would take
  // place if the filter were to execute. This is mainly what would happen to the
  // DataStructure during this filter, i.e., what modificationst to the DataStructure
  // would take place.
  OutputActions actions;
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindDerivativesAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> FindDerivatives::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
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
