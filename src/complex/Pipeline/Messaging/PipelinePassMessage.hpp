#pragma once

#include <memory>

#include "complex/Pipeline/Messaging/IPipelineMessage.hpp"

namespace complex
{
class Pipeline;

/**
 * @class PipelinePassMessage
 * @brief
 */
class COMPLEX_EXPORT PipelinePassMessage : public IPipelineMessage
{
public:
  /**
   * @brief
   * @param msg
   */
  PipelinePassMessage(Pipeline* pipeline, const std::shared_ptr<IPipelineMessage>& msg);

  virtual ~PipelinePassMessage();

  /**
   * @brief Returns a pointer to the message passed through the pipeline.
   * @return IPipelineMessage*
   */
  IPipelineMessage* getMessage() const;

private:
  std::shared_ptr<IPipelineMessage> m_Message;
};
} // namespace complex
