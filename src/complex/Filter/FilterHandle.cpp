#include "complex/Filter/FilterHandle.hpp"

namespace complex
{
FilterHandle::FilterHandle(const FilterIdType& filterId, const PluginIdType& pluginId)
: m_FilterName("[Unknown]")
, m_FilterId(filterId)
, m_PluginId(pluginId)
{
}

FilterHandle::FilterHandle(const std::string& filterName, const FilterIdType& filterId, const PluginIdType& pluginId)
: m_FilterName(filterName)
, m_FilterId(filterId)
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
} // namespace complex
