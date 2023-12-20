#include "NodeStatusMessage.hpp"

#include <fmt/format.h>

using namespace nx::core;

NodeStatusMessage::NodeStatusMessage(AbstractPipelineNode* node, nx::core::FaultState faultState, nx::core::RunState runState)
: AbstractPipelineMessage(node)
, m_FaultState(faultState)
, m_RunState(runState)
{
}

NodeStatusMessage::~NodeStatusMessage() = default;

nx::core::FaultState NodeStatusMessage::getFaultState() const
{
  return m_FaultState;
}

nx::core::RunState NodeStatusMessage::getRunState() const
{
  return m_RunState;
}

std::string NodeStatusMessage::toString() const
{
  auto output = fmt::format("Node {}: ", getNode()->getName());

  if(m_RunState == nx::core::RunState::Idle)
  {
    output += " is Idle;";
  }
  if(m_RunState == nx::core::RunState::Queued)
  {
    output += " is Queued;";
  }
  if(m_RunState == nx::core::RunState::Preflighting)
  {
    output += " is Preflighting;";
  }
  if(m_RunState == nx::core::RunState::Executing)
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
