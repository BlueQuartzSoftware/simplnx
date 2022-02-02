#include "NodeStatusMessage.hpp"

#include "complex/Pipeline/PipelineFilter.hpp"

#include "fmt/format.h"

using namespace complex;

NodeStatusMessage::NodeStatusMessage(AbstractPipelineNode* node, AbstractPipelineNode::FaultState faultState, AbstractPipelineNode::RunState runState)
: AbstractPipelineMessage(node)
, m_FaultState(faultState)
, m_RunState(runState)
{
}

NodeStatusMessage::~NodeStatusMessage() = default;

AbstractPipelineNode::FaultState NodeStatusMessage::getFaultState() const
{
  return m_FaultState;
}

AbstractPipelineNode::RunState NodeStatusMessage::getRunState() const
{
  return m_RunState;
}

std::string NodeStatusMessage::toString() const
{
  auto output = fmt::format("Node {}: ", getNode()->getName());

  if(m_RunState == AbstractPipelineNode::RunState::Idle)
  {
    output += " is Idle;";
  }
  if(m_RunState == AbstractPipelineNode::RunState::Queued)
  {
    output += " is Queued;";
  }
  if(m_RunState == AbstractPipelineNode::RunState::Preflighting)
  {
    output += " is Preflighting;";
  }
  if(m_RunState == AbstractPipelineNode::RunState::Executing)
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
