#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "complex/Filter/FilterHandle.hpp"
#include "complex/Filter/IFilter.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
class AbstractPlugin;
class PluginLoader;

/**
 * @class FilterList
 * @brief The FilterList class serves as a lookup point for finding and
 * creating filters. The FilterList stores and loads plugins, adds
 * FilterHandles for each plugin's available filters, and handles the creation
 * of those filters at a later time.
 */
class COMPLEX_EXPORT FilterList
{
public:
  using FilterContainerType = std::unordered_set<FilterHandle>;
  using SearchContainerType = std::vector<FilterHandle>;

  /**
   * Constructs the FilterList and registers core filters.
   */
  FilterList();

  /**
   * @brief Default destructor.
   */
  ~FilterList();

  /**
   * @brief Returns the number of filters available.
   * @return
   */
  size_t size() const;

  /**
   * @brief Returns a set of FilterHandles pointing to each of the known filters.
   * @return FilterContainerType
   */
  const FilterContainerType& getFilterHandles() const;

  /**
   * @brief Searches the FilterHandles for the text within the filter and plugin
   * names. Returns a vector of the FilterHandles that contain the specified text.
   * @param text
   * @return std::vector<FilterHandle>
   */
  SearchContainerType search(const std::string& text) const;

  /**
   * @brief Attempts to create an IFilter specified by the given FilterHandle.
   * Returns a unique pointer to the created filter if one was created.
   * Returns nullptr otherwise.
   * @param handle
   * @return IFilter*
   */
  IFilter::UniquePointer createFilter(const FilterHandle& handle) const;

  /**
   * @brief Attempts to create an IFilter specified by the given uuid.
   * Returns a unique pointer to the created filter if one was created.
   * Returns nullptr otherwise.
   * @param handle
   * @return IFilter::UniquePointer
   */
  IFilter::UniquePointer createFilter(const Uuid& uuid) const;

  /**
   * @brief Returns the AbstractPlugin pointer corresponding to the specified
   * FilterHandle. Returns nullptr if the plugin could not be found.
   *
   * The plugin is owned by the FilterList and will be cleaned up when the
   * complex::Application is destroyed.
   * @param handle
   * @return AbstractPlugin*
   */
  AbstractPlugin* getPlugin(const FilterHandle& handle) const;

  /**
   * @brief Attempts to add a plugin using the specified PluginLoader. Returns
   * true if the plugin was added. Returns false otherwise.
   * @param loader
   * @return bool
   */
  bool addPlugin(const std::shared_ptr<PluginLoader>& loader);

  /**
   * @brief Attempts to add the plugin at the specified filepath. Returns true
   * if the plugin was added. Returns false otherwise.
   * @param path
   * @return bool
   */
  bool addPlugin(const std::string& path);

  /**
   * @brief Returns a set of pointers to loaded plugins.
   * @return std::unordered_set<AbstractPlugin*>
   */
  std::unordered_set<AbstractPlugin*> getLoadedPlugins() const;

  /**
   * @brief Returns a pointer to the plugin with the specified ID. Returns
   * nullptr if no plugin with the given ID is found.
   *
   * This plugin is owned by the FilterList and will be cleaned up when the
   * complex::Application closes.
   * @param id
   * @return AbstractPlugin*
   */
  AbstractPlugin* getPluginById(const FilterHandle::PluginIdType& id) const;

private:
  ////////////
  // Variables
  FilterContainerType m_FilterHandles;
  std::unordered_map<FilterHandle::PluginIdType, std::shared_ptr<PluginLoader>> m_PluginMap;
};
} // namespace complex
