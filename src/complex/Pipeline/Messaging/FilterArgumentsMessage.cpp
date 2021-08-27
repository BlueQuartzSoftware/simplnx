#include "FilterArgumentsMessage.hpp"

#include "complex/Filter/IFilter.hpp"
#include "complex/Pipeline/FilterNode.hpp"

using namespace complex;

FilterArgumentsMessage::FilterArgumentsMessage(FilterNode* node, const Arguments& args)
: IPipelineMessage(node)
, m_Filter(node->getFilter())
, m_Args(args)
{
}

FilterArgumentsMessage::~FilterArgumentsMessage() = default;

IFilter* FilterArgumentsMessage::getFilter() const
{
  return m_Filter;
}

Arguments FilterArgumentsMessage::getArguments() const
{
  return m_Args;
}
