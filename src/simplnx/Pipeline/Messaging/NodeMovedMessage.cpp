#include "simplnx/Pipeline/Messaging/NodeMovedMessage.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"

#include <fmt/format.h>

using namespace nx::core;

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

std::string NodeMovedMessage::toString() const
{
  return fmt::format("Moved {} from {} to {}", getNode()->getName(), getOldIndex(), getNewIndex());
}
