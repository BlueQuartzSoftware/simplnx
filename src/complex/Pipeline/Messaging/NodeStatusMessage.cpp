#include "NodeStatusMessage.hpp"

#include "complex/Pipeline/PipelineFilter.hpp"

#include "fmt/format.h"

using namespace complex;

NodeStatusMessage::NodeStatusMessage(AbstractPipelineNode* node, AbstractPipelineNode::Status status)
: AbstractPipelineMessage(node)
, m_Status(status)
{
}

NodeStatusMessage::~NodeStatusMessage() = default;

AbstractPipelineNode::Status NodeStatusMessage::getStatus() const
{
  return m_Status;
}

std::string NodeStatusMessage::toString() const
{
  auto output = fmt::format("Node {}: ", getNode()->getName());
  switch(getStatus())
  {
  case AbstractPipelineNode::Status::Completed:
    output += "Completed";
    break;
  case AbstractPipelineNode::Status::Dirty:
    output += "Dirty";
    break;
  case AbstractPipelineNode::Status::Executing:
    output += "Executing";
    break;
  }
  return output;
}
