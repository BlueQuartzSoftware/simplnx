#include "complex/Filtering/AbstractFilter.hpp"

using namespace complex;

AbstractFilter::AbstractFilter(const std::string& name, IdType id)
: m_Name(name)
, m_Id(id)
{
}

AbstractFilter::~AbstractFilter() = default;

std::string AbstractFilter::getName() const
{
  return m_Name;
}

AbstractFilter::IdType AbstractFilter::getId() const
{
  return m_Id;
}

bool AbstractFilter::preflight(const DataStructure& ds, const Arguments& args) const
{
  return preflightImpl(ds, args);
}

void AbstractFilter::execute(DataStructure& ds, const Arguments& args) const
{
  if(!preflight(ds, args))
  {
    return;
  }
  executeImpl(ds, args);
}
