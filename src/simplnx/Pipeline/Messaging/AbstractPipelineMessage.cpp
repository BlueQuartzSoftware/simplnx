#include "AbstractPipelineMessage.hpp"

using namespace nx::core;

AbstractPipelineMessage::AbstractPipelineMessage(AbstractPipelineNode* node)
: m_Node(node)
{
}

AbstractPipelineMessage::~AbstractPipelineMessage() = default;

AbstractPipelineNode* AbstractPipelineMessage::getNode() const
{
  return m_Node;
}
