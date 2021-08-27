#include "NodeRemovedMessage.hpp"

#include "complex/Pipeline/Pipeline.hpp"

using namespace complex;

NodeRemovedMessage::NodeRemovedMessage(Pipeline* pipeline, size_t index)
: IPipelineMessage(pipeline)
, m_Index(index)
{
}

NodeRemovedMessage::~NodeRemovedMessage() = default;

size_t NodeRemovedMessage::getIndex() const
{
  return m_Index;
}
