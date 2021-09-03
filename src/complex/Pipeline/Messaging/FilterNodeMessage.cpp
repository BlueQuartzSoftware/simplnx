#include "FilterNodeMessage.hpp"

#include "complex/Pipeline/FilterNode.hpp"

using namespace complex;

FilterNodeMessage::FilterNodeMessage(FilterNode* node, const std::shared_ptr<FilterMessage>& msg)
: IPipelineMessage(node)
, m_Filter(node->getFilter())
, m_Message(msg)
{
}

FilterNodeMessage::~FilterNodeMessage() = default;

IFilter* FilterNodeMessage::getFilter() const
{
  return m_Filter;
}

std::shared_ptr<FilterMessage> FilterNodeMessage::getMessage() const
{
  return m_Message;
}
