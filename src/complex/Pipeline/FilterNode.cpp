#include "FilterNode.hpp"

#include "complex/Core/Application.hpp"
#include "complex/Core/FilterList.hpp"
#include "complex/Pipeline/Messaging/FilterArgumentsMessage.hpp"
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
  notify(std::make_shared<FilterArgumentsMessage>(this, args));
}

bool FilterNode::preflight(DataStructure& data) const
{
  auto result = m_Filter->preflight(data, getArguments());
  return result.errors().size() > 0;
}

bool FilterNode::execute(DataStructure& data)
{
  auto result = m_Filter->execute(data, getArguments());
  if(result.errors().size() == 0)
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

void FilterNode::onNotify(IFilter* filter, const std::shared_ptr<FilterMessage>& msg)
{
  notify(std::make_shared<FilterNodeMessage>(this, msg));
}
