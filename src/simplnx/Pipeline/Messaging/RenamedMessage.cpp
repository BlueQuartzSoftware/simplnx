#include "RenamedMessage.hpp"

#include "simplnx/Pipeline/Pipeline.hpp"

#include <fmt/format.h>

#include <utility>

using namespace nx::core;

RenamedMessage::RenamedMessage(Pipeline* pipeline, std::string newName, std::string oldName)
: AbstractPipelineMessage(pipeline)
, m_NewName(std::move(newName))
, m_OldName(std::move(oldName))
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
