#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  PostSlackMessageInputValues inputValues;

  inputValues.SlackUser = filterArgs.value<StringParameter::ValueType>(k_SlackUser_Key);
  inputValues.SlackUrl = filterArgs.value<StringParameter::ValueType>(k_SlackUrl_Key);
  inputValues.SlackMessage = filterArgs.value<StringParameter::ValueType>(k_SlackMessage_Key);
  inputValues.WarningsAsError = filterArgs.value<bool>(k_WarningsAsError_Key);

  return PostSlackMessage(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT PostSlackMessageInputValues
{
  StringParameter::ValueType SlackUser;
  StringParameter::ValueType SlackUrl;
  StringParameter::ValueType SlackMessage;
  bool WarningsAsError;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT PostSlackMessage
{
public:
  PostSlackMessage(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, PostSlackMessageInputValues* inputValues);
  ~PostSlackMessage() noexcept;

  PostSlackMessage(const PostSlackMessage&) = delete;
  PostSlackMessage(PostSlackMessage&&) noexcept = delete;
  PostSlackMessage& operator=(const PostSlackMessage&) = delete;
  PostSlackMessage& operator=(PostSlackMessage&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const PostSlackMessageInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
