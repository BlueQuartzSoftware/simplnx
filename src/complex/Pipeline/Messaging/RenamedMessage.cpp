#include "RenamedMessage.hpp"

#include "complex/Pipeline/Pipeline.hpp"

using namespace complex;

RenamedMessage::RenamedMessage(Pipeline* pipeline, const std::string& newName, const std::string& oldName)
: IPipelineMessage(pipeline)
, m_NewName(newName)
, m_OldName(oldName)
{
}

RenamedMessage::~RenamedMessage() = default;

std::string RenamedMessage::getNewName() const
{
  return m_NewName;
}

std::string RenamedMessage::getOldName() const
{
  return m_OldName;
}
