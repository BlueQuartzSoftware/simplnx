#include "PostSlackMessage.hpp"

#include "complex/Core/Parameters/BoolParameter.hpp"
#include "complex/Core/Parameters/StringParameter.hpp"
#include "complex/DataStructure/DataPath.hpp"

using namespace complex;

namespace complex
{
std::string PostSlackMessage::name() const
{
  return FilterTraits<PostSlackMessage>::name.str();
}

Uuid PostSlackMessage::uuid() const
{
  return FilterTraits<PostSlackMessage>::uuid;
}

std::string PostSlackMessage::humanName() const
{
  return "Post Slack Message";
}

Parameters PostSlackMessage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<StringParameter>(k_SlackUser_Key, "Slack User", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_SlackUrl_Key, "Slack Url", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_SlackMessage_Key, "Slack Message", "", "SomeString"));
  params.insert(std::make_unique<BoolParameter>(k_WarningsAsError_Key, "Treat a Warning as an Error", "", false));
  // Associate the Linkable Parameter(s) to the children parameters that they control

  return params;
}

IFilter::UniquePointer PostSlackMessage::clone() const
{
  return std::make_unique<PostSlackMessage>();
}

Result<OutputActions> PostSlackMessage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pSlackUserValue = filterArgs.value<StringParameter::ValueType>(k_SlackUser_Key);
  auto pSlackUrlValue = filterArgs.value<StringParameter::ValueType>(k_SlackUrl_Key);
  auto pSlackMessageValue = filterArgs.value<StringParameter::ValueType>(k_SlackMessage_Key);
  auto pWarningsAsErrorValue = filterArgs.value<bool>(k_WarningsAsError_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<PostSlackMessageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> PostSlackMessage::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
