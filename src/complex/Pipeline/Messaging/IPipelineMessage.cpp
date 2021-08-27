#include "IPipelineMessage.hpp"

using namespace complex;

IPipelineMessage::IPipelineMessage(IPipelineNode* node)
: m_Node(node)
{
}

IPipelineMessage ::~IPipelineMessage() = default;

IPipelineNode* IPipelineMessage::getNode() const
{
  return m_Node;
}
