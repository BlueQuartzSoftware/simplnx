#include "PipelineFilterMessage.hpp"

#include "simplnx/Pipeline/PipelineFilter.hpp"

#include <fmt/format.h>

using namespace nx::core;

PipelineFilterMessage::PipelineFilterMessage(PipelineFilter* filter, const IFilter::Message& filterMessage)
: AbstractPipelineMessage(filter)
, m_FilterMessage(filterMessage)
{
}

PipelineFilterMessage::~PipelineFilterMessage() = default;

IFilter::Message PipelineFilterMessage::getFilterMessage() const
{
  return m_FilterMessage;
}

std::string PipelineFilterMessage::toString() const
{
  auto msg = getFilterMessage();
  auto name = getNode()->getName();
  std::string typeName;
  switch(msg.type)
  {
  case IFilter::Message::Type::Debug:
    typeName = "Debug";
    break;
  case IFilter::Message::Type::Error:
    typeName = "Error";
    break;
  case IFilter::Message::Type::Info:
    typeName = "Info";
    break;
  case IFilter::Message::Type::Warning:
    typeName = "Warning";
    break;
  case IFilter::Message::Type::Progress:
    typeName = "Progress";
    break;
  }

  return fmt::format("{}: {} Message '{}'", name, typeName, msg.message);
}
