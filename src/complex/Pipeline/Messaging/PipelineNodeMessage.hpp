#pragma once

#include <memory>
#include <string>

#include "complex/Pipeline/Messaging/AbstractPipelineMessage.hpp"

namespace complex
{
class Pipeline;

/**
 * @class PipelineNodeMessage
 * @brief The PipelineNodeMessage class exists to pass node message from a
 * pipeline node to observers of the parent pipeline.
 */
class COMPLEX_EXPORT PipelineNodeMessage : public AbstractPipelineMessage
{
public:
  /**
   * @brief Constructs a new pipeline message to pass a message up to observers
   * of the parent pipeline.
   * @param node
   * @param message
   */
  PipelineNodeMessage(AbstractPipelineNode* node, const std::shared_ptr<AbstractPipelineMessage>& msg);

  ~PipelineNodeMessage() override;

  /**
   * @brief Returns the Pipeline's new name.
   * @return std::shared_ptr<AbstractPipelineMessage>
   */
  std::shared_ptr<AbstractPipelineMessage> getMessage() const;

  /**
   * @brief Returns a string representation of the message.
   * @return std::string
   */
  std::string toString() const override;

private:
  std::shared_ptr<AbstractPipelineMessage> m_Message;
};
} // namespace complex
