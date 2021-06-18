#include "complex/Core/Application.hpp"
#include "complex/Core/FilterHandle.hpp"

using namespace complex;

FilterHandle::FilterHandle(const std::string& filterName, const FilterIdType& filterId, const PluginIdType& pluginId)
: m_FilterName(filterName)
, m_FilterId(filterId)
, m_PluginId(pluginId)
{
}

FilterHandle::FilterHandle(const FilterHandle& rhs)
: m_FilterName(rhs.m_FilterName)
, m_FilterId(rhs.m_FilterId)
, m_PluginId(rhs.m_PluginId)
{
}

FilterHandle::FilterHandle(FilterHandle&& rhs) noexcept
: m_FilterName(std::move(rhs.m_FilterName))
, m_FilterId(std::move(rhs.m_FilterId))
, m_PluginId(std::move(rhs.m_PluginId))
{
}

FilterHandle::~FilterHandle() = default;

std::string FilterHandle::getFilterName() const
{
  return m_FilterName;
}

FilterHandle::FilterIdType FilterHandle::getFilterId() const
{
  return m_FilterId;
}

FilterHandle::PluginIdType FilterHandle::getPluginId() const
{
  return m_PluginId;
}

bool FilterHandle::operator<(const FilterHandle& rhs) const noexcept
{
  if(getPluginId() < rhs.getPluginId())
  {
    return true;
  }
  else if(getPluginId() > rhs.getPluginId())
  {
    return false;
  }
  else
  {
    return getFilterId() < rhs.getFilterId();
  }
}

bool operator==(const complex::FilterHandle& lhs, const complex::FilterHandle& rhs) noexcept
{
  if(lhs.getPluginId() != rhs.getPluginId())
  {
    return false;
  }
  else
  {
    return lhs.getFilterId() == rhs.getFilterId();
  }
}
