#include "PipelineFilter.hpp"

#include "complex/Core/Application.hpp"
#include "complex/Core/FilterList.hpp"
#include "complex/Pipeline/Messaging/FilterPreflightMessage.hpp"

using namespace complex;

PipelineFilter* PipelineFilter::Create(const FilterHandle& handle)
{
  auto filter = Application::Instance()->getFilterList()->createFilter(handle);
  if(filter == nullptr)
  {
    return nullptr;
  }
  return new PipelineFilter(std::move(filter));
}

PipelineFilter::PipelineFilter(IFilter::UniquePointer&& filter)
: AbstractPipelineNode()
, m_Filter(std::move(filter))
{
}

PipelineFilter::~PipelineFilter() = default;

AbstractPipelineNode::NodeType PipelineFilter::getType() const
{
  return NodeType::Filter;
}

std::string PipelineFilter::getName()
{
  return m_Filter->humanName();
}

IFilter* PipelineFilter::getFilter() const
{
  return m_Filter.get();
}

Parameters PipelineFilter::getParameters() const
{
  return m_Filter->parameters();
}

Arguments PipelineFilter::getArguments() const
{
  return m_Arguments;
}

void PipelineFilter::setArguments(const Arguments& args)
{
  m_Arguments = args;
  markDirty();
}

bool PipelineFilter::preflight(DataStructure& data)
{
  auto result = m_Filter->preflight(data, getArguments());
  if(!result.valid())
  {
    m_Warnings = result.warnings();
    m_Errors = result.errors();

    clearPreflightStructure();
    notify(std::make_shared<FilterPreflightMessage>(this, m_Warnings, m_Errors));
    return false;
  }
  else
  {
    m_Warnings = result.warnings();
    m_Errors.clear();

    setPreflightStructure(data);
    notify(std::make_shared<FilterPreflightMessage>(this, m_Warnings, m_Errors));
    return true;
  }
}

bool PipelineFilter::execute(DataStructure& data)
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

std::vector<complex::Warning> PipelineFilter::getWarnings() const
{
  return m_Warnings;
}

std::vector<complex::Error> PipelineFilter::getErrors() const
{
  return m_Errors;
}
