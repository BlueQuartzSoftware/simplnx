#pragma once

#include <string>

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
  using FilterIdType = std::string;

  /**
   * @brief The PluginIdType alias points to the AbstractPlugin::ID type while
   * avoiding circular references while compiling.
   */
  using PluginIdType = std::string;

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

  ~FilterHandle();

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

  /**
   * @brief Less than operator is required for including within std::set.
   * Returns true if the plugin ID is less than the other handle's plugin ID.
   * Returns true if the filter ID is less than the other handle's filter ID
   * and the plugin IDs are identical. Returns false otherwise.
   * @param rhs
   * @return bool
   */
  bool operator<(const FilterHandle& rhs) const noexcept;

private:
  std::string m_FilterName;
  FilterIdType m_FilterId;
  PluginIdType m_PluginId;
};
} // namespace complex

/**
 * @brief The FilterHandle equality operator checks that the filter name,
 * filter ID, and plugin ID are identical between the two FilterHandles.
 * @param lhs
 * @param rhs
 * @return bool
 */
bool operator==(const complex::FilterHandle& lhs, const complex::FilterHandle& rhs) noexcept;
