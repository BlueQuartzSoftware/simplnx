#include "NodeAddedMessage.hpp"

#include "complex/Pipeline/Pipeline.hpp"

using namespace complex;

NodeAddedMessage::NodeAddedMessage(Pipeline* pipeline, AbstractPipelineNode* newNode, size_t index)
: AbstractPipelineMessage(pipeline)
, m_Node(newNode)
, m_Index(index)
{
}

NodeAddedMessage::~NodeAddedMessage() = default;

AbstractPipelineNode* NodeAddedMessage::getNewNode() const
{
  return m_Node;
}

size_t NodeAddedMessage::getIndex() const
{
  return m_Index;
}
