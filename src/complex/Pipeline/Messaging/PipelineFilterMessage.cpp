#include "PipelineFilterMessage.hpp"

#include "complex/Pipeline/PipelineFilter.hpp"

using namespace complex;

PipelineFilterMessage::PipelineFilterMessage(PipelineFilter* filter, const IFilter::Message& filterMessage)
: AbstractPipelineMessage(filter)
, m_FilterMessage(filterMessage)
{
}

PipelineFilterMessage::~PipelineFilterMessage() = default;

IFilter::Message PipelineFilterMessage::getFilterMessage()
{
  return m_FilterMessage;
}
