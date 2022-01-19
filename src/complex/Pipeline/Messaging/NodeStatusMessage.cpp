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

  if(getNode()->isExecuting())
  {
    output += " is Executing;";
  }
  if(getNode()->hasErrors())
  {
    output += " has Errors;";
  }
  if(getNode()->hasWarnings())
  {
    output += " has Warnings;";
  }
  if(getNode()->isDisabled())
  {
    output += " is Disabled;";
  }

  return output;
}
