#include "FilterNode.hpp"

#include "complex/Core/Application.hpp"
#include "complex/Core/FilterList.hpp"

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
  auto result = m_Filter->preflight(data, getArguments());
  if(!result.valid())
  {
    m_Warnings = result.warnings();
    m_Errors = result.errors();

    clearPreflightStructure();
    return false;
  }
  else
  {
    m_Warnings = result.warnings();
    m_Errors.clear();

    setPreflightStructure(data);
    return true;
  }
}

bool FilterNode::execute(DataStructure& data)
{
  auto results = m_Filter->execute(data, getArguments());
  if(!results.valid())
  {
    m_Warnings = results.warnings();
    m_Errors = results.errors();

    clearPreflightStructure();
    markDirty();
    return false;
  }
  else
  {
    m_Warnings = results.warnings();
    m_Errors.clear();

    setDataStructure(data);
    setStatus(Status::Completed);
    return true;
  }
}

std::vector<complex::Warning> FilterNode::getWarnings() const
{
  return m_Warnings;
}

std::vector<complex::Error> FilterNode::getErrors() const
{
  return m_Errors;
}
