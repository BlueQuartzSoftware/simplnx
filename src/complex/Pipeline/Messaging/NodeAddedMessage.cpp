#include "NodeAddedMessage.hpp"

#include "complex/Pipeline/Pipeline.hpp"

using namespace complex;

NodeAddedMessage::NodeAddedMessage(Pipeline* pipeline, AbstractPipelineNode* newNode, usize index)
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

usize NodeAddedMessage::getIndex() const
{
  return m_Index;
}
