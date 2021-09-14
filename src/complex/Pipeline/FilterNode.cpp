#include "FilterNode.hpp"

#include "complex/Core/Application.hpp"
#include "complex/Core/FilterList.hpp"
#include "complex/Pipeline/Messaging/FilterNodeMessage.hpp"

using namespace complex;

FilterNode* FilterNode::Create(const FilterHandle& handle)
{
  auto filter = Application::Instance()->getFilterList()->createFilter(handle);
  if(filter == nullptr)
  {
    return nullptr;
  }
  return new FilterNode(std::move(filter));
}

FilterNode::FilterNode(IFilter::UniquePointer&& filter)
: IPipelineNode()
, m_Filter(std::move(filter))
{
  startObservingFilter(filter.get());
}

FilterNode::~FilterNode() = default;

IPipelineNode::NodeType FilterNode::getType() const
{
  return NodeType::Filter;
}

std::string FilterNode::getName()
{
  return m_Filter->humanName();
}

IFilter* FilterNode::getFilter() const
{
  return m_Filter.get();
}

Parameters FilterNode::getParameters() const
{
  return m_Filter->parameters();
}

Arguments FilterNode::getArguments() const
{
  return m_Arguments;
}

void FilterNode::setArguments(const Arguments& args)
{
  m_Arguments = args;
  markDirty();
}

bool FilterNode::preflight(DataStructure& data)
{
  clearMsgs();
  m_Filter->preflight(data, getArguments());
  return getErrors().size() > 0;
}

bool FilterNode::execute(DataStructure& data)
{
  clearMsgs();
  m_Filter->execute(data, getArguments());

  auto errors = getErrors();
  if(errors.size() == 0)
  {
    setDataStructure(data);
    setStatus(Status::Completed);
    return true;
  }
  else
  {
    clearDataStructure();
    markDirty();
    return false;
  }
}

void FilterNode::clearMsgs()
{
  m_ErrorMsgs.clear();
  m_WarningMsgs.clear();
}

FilterNode::Messages FilterNode::getWarnings() const
{
  return m_WarningMsgs;
}

FilterNode::Messages FilterNode::getErrors() const
{
  return m_ErrorMsgs;
}

void FilterNode::onNotify(IFilter* filter, const std::shared_ptr<FilterMessage>& msg)
{
  switch(msg->getType())
  {
  case FilterMessage::Type::Error:
    m_ErrorMsgs.push_back(msg);
    break;
  case FilterMessage::Type::Warning:
    m_WarningMsgs.push_back(msg);
    break;
  default:
    break;
  }

  notify(std::make_shared<FilterNodeMessage>(this, msg));
}
