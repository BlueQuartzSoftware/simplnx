#include "FilterObserver.hpp"

#include "complex/Filter/IFilter.hpp"

using namespace complex;

FilterObserver::FilterObserver()
{
}

FilterObserver::~FilterObserver()
{
  for(auto filter : m_ObservedFilters)
  {
    filter->removeObserver(this);
  }
}

std::vector<IFilter*> FilterObserver::getObservedFilters() const
{
  return m_ObservedFilters;
}

bool FilterObserver::isObservingFilter(IFilter* filter) const
{
  return std::find(m_ObservedFilters.begin(), m_ObservedFilters.end(), filter) != m_ObservedFilters.end();
}

void FilterObserver::startObservingFilter(IFilter* filter)
{
  if(filter == nullptr)
  {
    return;
  }

  if(isObservingFilter(filter))
  {
    return;
  }
  m_ObservedFilters.push_back(filter);
  filter->addObserver(this);
}

void FilterObserver::stopObservingFilter(IFilter* filter)
{
  if(!isObservingFilter(filter))
  {
    return;
  }

  m_ObservedFilters.erase(std::remove(m_ObservedFilters.begin(), m_ObservedFilters.end(), filter));
  filter->removeObserver(this);
}
