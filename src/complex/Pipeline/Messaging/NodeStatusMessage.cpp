#include "NodeStatusMessage.hpp"

#include "complex/Pipeline/PipelineFilter.hpp"

using namespace complex;

NodeStatusMessage::NodeStatusMessage(AbstractPipelineNode* node, AbstractPipelineNode::Status status)
: AbstractPipelineMessage(node)
, m_Status(status)
{
}

NodeStatusMessage::~NodeStatusMessage() = default;

AbstractPipelineNode::Status NodeStatusMessage::getStatus() const
{
  return m_Status;
}
