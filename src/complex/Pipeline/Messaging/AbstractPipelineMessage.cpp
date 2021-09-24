#include "AbstractPipelineMessage.hpp"

using namespace complex;

AbstractPipelineMessage::AbstractPipelineMessage(AbstractPipelineNode* node)
: m_Node(node)
{
}

AbstractPipelineMessage ::~AbstractPipelineMessage() = default;

AbstractPipelineNode* AbstractPipelineMessage::getNode() const
{
  return m_Node;
}
