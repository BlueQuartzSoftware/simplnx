#include "PipelineNodeMessage.hpp"

using namespace nx::core;

PipelineNodeMessage::PipelineNodeMessage(AbstractPipelineNode* node, const std::shared_ptr<AbstractPipelineMessage>& message)
: AbstractPipelineMessage(node)
, m_Message(message)
{
}

PipelineNodeMessage::~PipelineNodeMessage() = default;

std::shared_ptr<AbstractPipelineMessage> PipelineNodeMessage::getMessage() const
{
  return m_Message;
}

std::string PipelineNodeMessage::toString() const
{
  return getMessage()->toString();
}
