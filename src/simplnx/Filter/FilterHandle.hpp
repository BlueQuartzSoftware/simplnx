#pragma once

#include "simplnx/Common/Uuid.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/simplnx_export.hpp"

#include <functional>
#include <string>

namespace nx::core
{
class AbstractPlugin;
class FilterList;

/**
 * @class FilterHandle
 * @brief The FilterHandle class serves as an ID for looking up information on
 * a specific filter in the FilterList. FilterHandles store the name of the
 * filter, the filter's ID, and the ID of the plugin to which the filter
 * belongs. This class serves no other purpose other than to identify a
 * specific filter in the FilterList.
 */
class SIMPLNX_EXPORT FilterHandle
{
public:
  friend class AbstractPlugin;
  friend class FilterList;

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
   * @param filterId
   * @param pluginId
   */
  FilterHandle(const FilterIdType& filterId, const PluginIdType& pluginId);

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

  /**
   * @brief Copy assignment operator assigns the name and IDs from the
   * provided FilterHandle.
   * @param rhs
   * @return FilterHandle&
   */
  FilterHandle& operator=(const FilterHandle& rhs);

  /**
   * @brief Move assignment operator moves the name and IDs from the provided
   * FilterHandle.
   * @param rhs
   * @return FilterHandle&
   */
  FilterHandle& operator=(FilterHandle&& rhs) noexcept;

  /**
   * @brief Default destructor.
   */
  ~FilterHandle() noexcept;

  /**
   * @brief Returns the name of the target filter as provided to the constructor.
   * @return std::string
   */
  std::string getFilterName() const;

  /**
   * @brief Returns the C++ classname for the filter
   * @return
   */
  std::string getClassName() const;

  /**
   * @brief Returns the Default Tags for the filter
   * @return
   */
  std::vector<std::string> getDefaultTags() const;

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

protected:
  /**
   * @brief Constructs a FilterHandle for the target filter and plugin ID.
   * The filter name is assigned for human readability.
   * @param filterName
   * @param filter The IFilter object that the handle wraps
   * @param pluginId
   */
  FilterHandle(const IFilter& filter, const PluginIdType& pluginId);

private:
  std::string m_FilterName;
  std::string m_ClassName;
  std::vector<std::string> m_DefaultTags;
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
bool SIMPLNX_EXPORT operator==(const FilterHandle& lhs, const FilterHandle& rhs) noexcept;

/**
 * @brief The FilterHandle inequality operator checks that the filter name,
 * filter ID, and plugin ID are not identical between the two FilterHandles.
 * @param lhs
 * @param rhs
 * @return bool
 */
bool SIMPLNX_EXPORT operator!=(const FilterHandle& lhs, const FilterHandle& rhs) noexcept;
} // namespace nx::core

namespace std
{
template <>
struct hash<::nx::core::FilterHandle>
{
  /**
   * @brief Hash operator for placing in a collection that requires hashing values.
   * @param value
   * @return std::size_t
   */
  std::size_t operator()(const ::nx::core::FilterHandle& value) const noexcept
  {
    std::hash<::nx::core::FilterHandle::FilterIdType> hasher;
    std::size_t h1 = hasher(value.getPluginId());
    std::size_t h2 = hasher(value.getFilterId());
    return h1 ^ (h2 << 1);
  }
};
} // namespace std
