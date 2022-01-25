#include "NodeStatusMessage.hpp"

#include "complex/Pipeline/PipelineFilter.hpp"
#include "complex/Pipeline/AbstractPipelineNode.hpp"



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

  switch (getNode()->getStatus())
  {
    case complex::AbstractPipelineNode::Executing:
      output += " is Executing";
      break;
    case complex::AbstractPipelineNode::Error:
      output += " has Errors";
      break;
    case complex::AbstractPipelineNode::Warning:
      output += " has Warnings";
      break;
    case complex::AbstractPipelineNode::Executed:
      output += " Executed Successfully";
      break;
    case complex::AbstractPipelineNode::Disabled:
      output += " is Disabled";
      break;
    default:
      output.clear();
      break;
  }

  return output;
}
