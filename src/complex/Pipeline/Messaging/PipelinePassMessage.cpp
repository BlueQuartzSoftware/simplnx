#include "PipelinePassMessage.hpp"

#include "complex/Pipeline/Pipeline.hpp"

using namespace complex;

PipelinePassMessage::PipelinePassMessage(Pipeline* pipeline, const std::shared_ptr<IPipelineMessage>& msg)
: IPipelineMessage(pipeline)
, m_Message(msg)
{
}

PipelinePassMessage::~PipelinePassMessage() = default;

IPipelineMessage* PipelinePassMessage::getMessage() const
{
  return m_Message.get();
}
