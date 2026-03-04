#pragma once

#include "simplnx/Filter/FilterHandle.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/simplnx_export.hpp"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace nx::core
{
class AbstractDataIOManager;

/**
 * @class IPlugin
 * @brief Pure interface defining the public contract for all plugins.
 * Provides identity, filter creation, IO manager, and SIMPL compatibility
 * methods that every plugin must implement. The AbstractPlugin base class
 * provides shared implementation details.
 */
class SIMPLNX_EXPORT IPlugin
{
public:
  using IdType = Uuid;
  using FilterContainerType = std::unordered_set<FilterHandle>;
  using IOManagerPointer = std::shared_ptr<AbstractDataIOManager>;
  using IOManagersContainerType = std::vector<IOManagerPointer>;

  struct SIMPLNX_EXPORT SIMPLData
  {
    using ConversionFunction = std::function<Result<Arguments>(const nlohmann::json&)>;

    Uuid simplnxUuid;
    ConversionFunction convertJson;
  };

  using SIMPLMapType = std::map<Uuid, SIMPLData>;

  virtual ~IPlugin() noexcept;

  IPlugin(const IPlugin&) = delete;
  IPlugin(IPlugin&&) noexcept = delete;
  IPlugin& operator=(const IPlugin&) = delete;
  IPlugin& operator=(IPlugin&&) noexcept = delete;

  /**
   * @brief Returns the plugin's name.
   * @return std::string
   */
  virtual std::string getName() const = 0;

  /**
   * @brief Returns the plugin's description.
   * @return std::string
   */
  virtual std::string getDescription() const = 0;

  /**
   * @brief Returns the plugin's ID.
   * @return IdType
   */
  virtual IdType getId() const = 0;

  /**
   * @brief Returns the plugin's vendor name.
   * @return std::string
   */
  virtual std::string getVendor() const = 0;

  /**
   * @brief Checks if the plugin contains a filter with the given ID.
   * @param identifier
   * @return bool
   */
  virtual bool containsFilterId(FilterHandle::FilterIdType identifier) const = 0;

  /**
   * @brief Creates an IFilter with the specified ID. Returns nullptr if the
   * plugin does not contain a filter with that ID.
   * @param filterId
   * @return IFilter::UniquePointer
   */
  virtual IFilter::UniquePointer createFilter(FilterHandle::FilterIdType filterId) const = 0;

  /**
   * @brief Returns a set of FilterHandles pointing to each of the filters
   * contained in the plugin.
   * @return FilterContainerType
   */
  virtual FilterContainerType getFilterHandles() const = 0;

  /**
   * @brief Returns the number of filters in the plugin.
   * @return FilterContainerType::size_type
   */
  virtual FilterContainerType::size_type getFilterCount() const = 0;

  /**
   * @brief Returns a collection of DataStructure IO managers available
   * through the plugin.
   * @return IOManagersContainerType
   */
  virtual IOManagersContainerType getDataIOManagers() const = 0;

  /**
   * @brief Returns a map of UUIDs where SIMPL UUIDs are keys to their
   * simplnx counterpart.
   * @return SIMPLMapType
   */
  virtual SIMPLMapType getSimplToSimplnxMap() const = 0;

  /**
   * @brief Sets the out-of-core temporary directory path.
   * @param path
   */
  virtual void setOocTempDirectory(const std::string& path) = 0;

protected:
  IPlugin() = default;
};
} // namespace nx::core
