#include "NodeAddedMessage.hpp"

#include "simplnx/Pipeline/Pipeline.hpp"

using namespace nx::core;

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

std::string NodeAddedMessage::toString() const
{
  return fmt::format("Added {} at {}", getNewNode()->getName(), getIndex());
}
