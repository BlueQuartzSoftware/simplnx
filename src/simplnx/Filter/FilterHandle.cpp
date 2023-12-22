#include "simplnx/Filter/FilterHandle.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
FilterHandle::FilterHandle(const FilterIdType& filterId, const PluginIdType& pluginId)
: m_FilterName("[Unknown Filter]")
, m_ClassName("[Unknown Class Name]")
, m_FilterId(filterId)
, m_PluginId(pluginId)
{
}

FilterHandle::FilterHandle(const IFilter& filter, const PluginIdType& pluginId)
: m_FilterName(filter.humanName())
, m_ClassName(filter.className())
, m_DefaultTags(filter.defaultTags())
, m_FilterId(filter.uuid())
, m_PluginId(pluginId)
{
}

FilterHandle::FilterHandle(const FilterHandle& rhs) = default;

FilterHandle::FilterHandle(FilterHandle&& rhs) noexcept = default;

FilterHandle& FilterHandle::operator=(const FilterHandle& rhs) = default;

FilterHandle& FilterHandle::operator=(FilterHandle&& rhs) noexcept = default;

FilterHandle::~FilterHandle() noexcept = default;

std::string FilterHandle::getFilterName() const
{
  return m_FilterName;
}

std::string FilterHandle::getClassName() const
{
  return m_ClassName;
}

std::vector<std::string> FilterHandle::getDefaultTags() const
{
  return m_DefaultTags;
}

FilterHandle::FilterIdType FilterHandle::getFilterId() const
{
  return m_FilterId;
}

FilterHandle::PluginIdType FilterHandle::getPluginId() const
{
  return m_PluginId;
}

bool operator==(const FilterHandle& lhs, const FilterHandle& rhs) noexcept
{
  return (lhs.getPluginId() == rhs.getPluginId()) && (lhs.getFilterId() == rhs.getFilterId());
}

bool operator!=(const FilterHandle& lhs, const FilterHandle& rhs) noexcept
{
  return !(lhs == rhs);
}
} // namespace nx::core
