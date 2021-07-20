#pragma once

#include <functional>
#include <string>

#include "complex/Common/Uuid.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class FilterHandle
 * @brief The FilterHandle class serves as an ID for looking up information on
 * a specific filter in the FilterList. FilterHandles store the name of the
 * filter, the filter's ID, and the ID of the plugin to which the filter
 * belongs. This class serves no other purpose other than to identify a
 * specific filter in the FilterList.
 */
class COMPLEX_EXPORT FilterHandle
{
public:
  /**
   * @brief The FilterIdType alias points to the AbstractFilter::ID type while
   * avoiding circular references while compiling.
   */
  using FilterIdType = Uuid;

  /**
   * @brief The PluginIdType alias points to the AbstractPlugin::ID type while
   * avoiding circular references while compiling.
   */
  using PluginIdType = Uuid;

  FilterHandle() = delete;

  /**
   * @brief Constructs a FilterHandle for the target filter and plugin ID.
   * The filter name is assigned for human readability.
   * @param filterName
   * @param filterId
   * @param pluginId
   */
  FilterHandle(const std::string& filterName, const FilterIdType& filterId, const PluginIdType& pluginId);

  /**
   * @brief Copy constructor
   * @param rhs
   */
  FilterHandle(const FilterHandle& rhs);

  /**
   * @brief Move constructor
   * @param rhs
   */
  FilterHandle(FilterHandle&& rhs) noexcept;

  FilterHandle& operator=(const FilterHandle& rhs);

  FilterHandle& operator=(FilterHandle&& rhs) noexcept;

  ~FilterHandle() noexcept;

  /**
   * @brief Returns the name of the target filter as provided to the constructor.
   * @return std::string
   */
  std::string getFilterName() const;

  /**
   * @brief Returns the filter ID within the target plugin.
   * @return PluginIdType
   */
  FilterIdType getFilterId() const;

  /**
   * @brief Returns the target plugin ID.
   * @return PluginIdType
   */
  PluginIdType getPluginId() const;

private:
  std::string m_FilterName;
  FilterIdType m_FilterId;
  PluginIdType m_PluginId;
};

/**
 * @brief The FilterHandle equality operator checks that the filter name,
 * filter ID, and plugin ID are identical between the two FilterHandles.
 * @param lhs
 * @param rhs
 * @return bool
 */
bool operator==(const FilterHandle& lhs, const FilterHandle& rhs) noexcept;

bool operator!=(const FilterHandle& lhs, const FilterHandle& rhs) noexcept;
} // namespace complex

namespace std
{
template <>
struct hash<complex::FilterHandle>
{
  std::size_t operator()(const complex::FilterHandle& value) const noexcept
  {
    std::hash<complex::FilterHandle::FilterIdType> hasher;
    std::size_t h1 = hasher(value.getPluginId());
    std::size_t h2 = hasher(value.getFilterId());
    return h1 ^ (h2 << 1);
  }
};
} // namespace std
