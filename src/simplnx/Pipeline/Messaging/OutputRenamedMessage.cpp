#include "simplnx/Pipeline/Messaging/OutputRenamedMessage.hpp"

using namespace nx::core;

OutputRenamedMessage::OutputRenamedMessage(AbstractPipelineNode* pipeline, const PipelineFilter::RenamedPath& renamedOutput)
: AbstractPipelineMessage(pipeline)
, m_RenamedPath(renamedOutput)
{
}

OutputRenamedMessage::~OutputRenamedMessage() noexcept = default;

PipelineFilter::RenamedPath OutputRenamedMessage::getRenamedPath() const
{
  return m_RenamedPath;
}

DataPath OutputRenamedMessage::getPreviousPath() const
{
  return m_RenamedPath.second;
}

DataPath OutputRenamedMessage::getNewPath() const
{
  return m_RenamedPath.first;
}

std::string OutputRenamedMessage::toString() const
{
  return fmt::format("Renamed created DataPath from '{}' to '{}'", getPreviousPath().toString(), getNewPath().toString());
}
