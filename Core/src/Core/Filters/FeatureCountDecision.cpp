#include "FeatureCountDecision.hpp"

#include "complex/DataStructure/DataPath.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FeatureCountDecision::name() const
{
  return FilterTraits<FeatureCountDecision>::name.str();
}

//------------------------------------------------------------------------------
std::string FeatureCountDecision::className() const
{
  return FilterTraits<FeatureCountDecision>::className;
}

//------------------------------------------------------------------------------
Uuid FeatureCountDecision::uuid() const
{
  return FilterTraits<FeatureCountDecision>::uuid;
}

//------------------------------------------------------------------------------
std::string FeatureCountDecision::humanName() const
{
  return "Feature Count Decision";
}

//------------------------------------------------------------------------------
std::vector<std::string> FeatureCountDecision::defaultTags() const
{
  return {"#Core", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters FeatureCountDecision::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FeatureCountDecision::clone() const
{
  return std::make_unique<FeatureCountDecision>();
}

//------------------------------------------------------------------------------
Result<OutputActions> FeatureCountDecision::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FeatureCountDecisionAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FeatureCountDecision::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
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
