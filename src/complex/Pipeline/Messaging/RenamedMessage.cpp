#include "RenamedMessage.hpp"

#include "complex/Pipeline/Pipeline.hpp"

#include <fmt/format.h>

using namespace complex;

RenamedMessage::RenamedMessage(Pipeline* pipeline, const std::string& newName, const std::string& oldName)
: AbstractPipelineMessage(pipeline)
, m_NewName(newName)
, m_OldName(oldName)
{
}

RenamedMessage::~RenamedMessage() = default;

std::string RenamedMessage::getNewName() const
{
  return m_NewName;
}

std::string RenamedMessage::getPreviousName() const
{
  return m_OldName;
}

std::string RenamedMessage::toString() const
{
  return fmt::format("Renamed node from '{}' to '{}'", getPreviousName(), getNewName());
}
