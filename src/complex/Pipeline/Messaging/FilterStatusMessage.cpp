#include "FilterStatusMessage.hpp"

#include "complex/Pipeline/FilterNode.hpp"

using namespace complex;

FilterStatusMessage::FilterStatusMessage(IPipelineNode* node, IPipelineNode::Status status)
: IPipelineMessage(node)
, m_Status(status)
{
}

FilterStatusMessage::~FilterStatusMessage() = default;

IPipelineNode::Status FilterStatusMessage::getStatus() const
{
  return m_Status;
}
