#include "PostSlackMessage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string PostSlackMessage::name() const
{
  return FilterTraits<PostSlackMessage>::name.str();
}

//------------------------------------------------------------------------------
std::string PostSlackMessage::className() const
{
  return FilterTraits<PostSlackMessage>::className;
}

//------------------------------------------------------------------------------
Uuid PostSlackMessage::uuid() const
{
  return FilterTraits<PostSlackMessage>::uuid;
}

//------------------------------------------------------------------------------
std::string PostSlackMessage::humanName() const
{
  return "Post Slack Message";
}

//------------------------------------------------------------------------------
std::vector<std::string> PostSlackMessage::defaultTags() const
{
  return {"#Core", "#Misc"};
}

//------------------------------------------------------------------------------
Parameters PostSlackMessage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<StringParameter>(k_SlackUser_Key, "Slack User", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_SlackUrl_Key, "Slack Url", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_SlackMessage_Key, "Slack Message", "", "SomeString"));
  params.insert(std::make_unique<BoolParameter>(k_WarningsAsError_Key, "Treat a Warning as an Error", "", false));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer PostSlackMessage::clone() const
{
  return std::make_unique<PostSlackMessage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult PostSlackMessage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pSlackUserValue = filterArgs.value<StringParameter::ValueType>(k_SlackUser_Key);
  auto pSlackUrlValue = filterArgs.value<StringParameter::ValueType>(k_SlackUrl_Key);
  auto pSlackMessageValue = filterArgs.value<StringParameter::ValueType>(k_SlackMessage_Key);
  auto pWarningsAsErrorValue = filterArgs.value<bool>(k_WarningsAsError_Key);

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
  auto action = std::make_unique<PostSlackMessageAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> PostSlackMessage::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSlackUserValue = filterArgs.value<StringParameter::ValueType>(k_SlackUser_Key);
  auto pSlackUrlValue = filterArgs.value<StringParameter::ValueType>(k_SlackUrl_Key);
  auto pSlackMessageValue = filterArgs.value<StringParameter::ValueType>(k_SlackMessage_Key);
  auto pWarningsAsErrorValue = filterArgs.value<bool>(k_WarningsAsError_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
