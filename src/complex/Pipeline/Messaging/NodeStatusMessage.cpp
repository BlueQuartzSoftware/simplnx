#include "NodeStatusMessage.hpp"

#include "complex/Pipeline/FilterNode.hpp"

using namespace complex;

NodeStatusMessage::NodeStatusMessage(IPipelineNode* node, IPipelineNode::Status status)
: IPipelineMessage(node)
, m_Status(status)
{
}

NodeStatusMessage::~NodeStatusMessage() = default;

IPipelineNode::Status NodeStatusMessage::getStatus() const
{
  return m_Status;
}
