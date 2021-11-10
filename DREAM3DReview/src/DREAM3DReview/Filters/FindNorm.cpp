#include "FindNorm.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindNorm::name() const
{
  return FilterTraits<FindNorm>::name.str();
}

//------------------------------------------------------------------------------
std::string FindNorm::className() const
{
  return FilterTraits<FindNorm>::className;
}

//------------------------------------------------------------------------------
Uuid FindNorm::uuid() const
{
  return FilterTraits<FindNorm>::uuid;
}

//------------------------------------------------------------------------------
std::string FindNorm::humanName() const
{
  return "Find Norm";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindNorm::defaultTags() const
{
  return {"#DREAM3D Review", "#Statistics"};
}

//------------------------------------------------------------------------------
Parameters FindNorm::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float32Parameter>(k_PSpace_Key, "p-Space Value", "", 1.23345f));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedArrayPath_Key, "Input Attribute Array", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NormArrayPath_Key, "Norm", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindNorm::clone() const
{
  return std::make_unique<FindNorm>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindNorm::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pPSpaceValue = filterArgs.value<float32>(k_PSpace_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pNormArrayPathValue = filterArgs.value<DataPath>(k_NormArrayPath_Key);

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
  auto action = std::make_unique<FindNormAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> FindNorm::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pPSpaceValue = filterArgs.value<float32>(k_PSpace_Key);
  auto pSelectedArrayPathValue = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  auto pNormArrayPathValue = filterArgs.value<DataPath>(k_NormArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
