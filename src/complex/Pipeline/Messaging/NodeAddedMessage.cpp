#include "NodeAddedMessage.hpp"

#include "complex/Pipeline/Pipeline.hpp"

using namespace complex;

NodeAddedMessage::NodeAddedMessage(Pipeline* pipeline, IPipelineNode* newNode, size_t index)
: IPipelineMessage(pipeline)
, m_Node(newNode)
, m_Index(index)
{
}

NodeAddedMessage::~NodeAddedMessage() = default;

IPipelineNode* NodeAddedMessage::getNewNode() const
{
  return m_Node;
}

size_t NodeAddedMessage::getIndex() const
{
  return m_Index;
}
