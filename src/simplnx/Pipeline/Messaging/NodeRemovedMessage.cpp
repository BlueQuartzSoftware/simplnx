#include "NodeRemovedMessage.hpp"

#include "simplnx/Pipeline/Pipeline.hpp"

#include <fmt/format.h>

using namespace nx::core;

NodeRemovedMessage::NodeRemovedMessage(Pipeline* pipeline, usize index)
: AbstractPipelineMessage(pipeline)
, m_Index(index)
{
}

NodeRemovedMessage::~NodeRemovedMessage() = default;

usize NodeRemovedMessage::getIndex() const
{
  return m_Index;
}

std::string NodeRemovedMessage::toString() const
{
  return fmt::format("Removed node at {}", getIndex());
}
