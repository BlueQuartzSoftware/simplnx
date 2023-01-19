#include "NodeStatusMessage.hpp"

#include <fmt/format.h>

using namespace complex;

NodeStatusMessage::NodeStatusMessage(AbstractPipelineNode* node, complex::FaultState faultState, complex::RunState runState)
: AbstractPipelineMessage(node)
, m_FaultState(faultState)
, m_RunState(runState)
{
}

NodeStatusMessage::~NodeStatusMessage() = default;

complex::FaultState NodeStatusMessage::getFaultState() const
{
  return m_FaultState;
}

complex::RunState NodeStatusMessage::getRunState() const
{
  return m_RunState;
}

std::string NodeStatusMessage::toString() const
{
  auto output = fmt::format("Node {}: ", getNode()->getName());

  if(m_RunState == complex::RunState::Idle)
  {
    output += " is Idle;";
  }
  if(m_RunState == complex::RunState::Queued)
  {
    output += " is Queued;";
  }
  if(m_RunState == complex::RunState::Preflighting)
  {
    output += " is Preflighting;";
  }
  if(m_RunState == complex::RunState::Executing)
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
