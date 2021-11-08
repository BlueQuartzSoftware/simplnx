#include "complex/Pipeline/Messaging/NodeMovedMessage.hpp"
#include "complex/Pipeline/Pipeline.hpp"

using namespace complex;

NodeMovedMessage::NodeMovedMessage(Pipeline* pipeline, usize fromIndex, usize toIndex)
: AbstractPipelineMessage(pipeline)
, m_OldIndex(fromIndex)
, m_NewIndex(toIndex)
{
}

NodeMovedMessage::~NodeMovedMessage() = default;

usize NodeMovedMessage::getOldIndex() const
{
  return m_OldIndex;
}

usize NodeMovedMessage::getNewIndex() const
{
  return m_NewIndex;
}
