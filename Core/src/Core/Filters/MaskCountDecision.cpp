#include "MaskCountDecision.hpp"

#include "complex/DataStructure/DataPath.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string MaskCountDecision::name() const
{
  return FilterTraits<MaskCountDecision>::name.str();
}

//------------------------------------------------------------------------------
std::string MaskCountDecision::className() const
{
  return FilterTraits<MaskCountDecision>::className;
}

//------------------------------------------------------------------------------
Uuid MaskCountDecision::uuid() const
{
  return FilterTraits<MaskCountDecision>::uuid;
}

//------------------------------------------------------------------------------
std::string MaskCountDecision::humanName() const
{
  return "Mask Count Decision";
}

//------------------------------------------------------------------------------
std::vector<std::string> MaskCountDecision::defaultTags() const
{
  return {"#Core", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters MaskCountDecision::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer MaskCountDecision::clone() const
{
  return std::make_unique<MaskCountDecision>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult MaskCountDecision::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */

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
  auto action = std::make_unique<MaskCountDecisionAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> MaskCountDecision::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
