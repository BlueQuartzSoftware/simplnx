#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "complex/Core/FilterHandle.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
class AbstractFilter;
class AbstractPlugin;
class PluginLoader;

/**
 * class FilterList
 *
 */

class COMPLEX_EXPORT FilterList
{
public:
  /**
   * Empty Constructor
   */
  FilterList();

  virtual ~FilterList();

  /**
   * @brief Returns a set of FilterHandles for each of the filters.
   * @return std::set<complex::FilterHandle>
   */
  std::set<complex::FilterHandle> getFilterHandles() const;

  /**
   * @brief Searches the FilterHandles for the text in the filter and plugin
   * names. Returns a vector of the FilterHandles that contain the specified text.
   * @param text
   * @return std::vector<complex::FilterHandle>
   */
  std::vector<complex::FilterHandle> search(const std::string& text) const;

  /**
   * @brief Attempts to create an AbstractFilter specified by the given
   * FilterHandle. Returns a pointer to the created filter if one was created.
   * Returns nullptr otherwise.
   * 
   * It is the caller's responsibility to delete the pointer when they are
   * finished with it.
   * @param handle
   * @return complex::AbstractFilter*
   */
  complex::AbstractFilter* createFilter(const complex::FilterHandle& handle) const;

  /**
   * @brief Returns the AbstractPlugin pointer corresponding to the specified
   * FilterHandle. Returns nullptr if the plugin could not be found.
   * @param handle
   * @return complex::AbstractPlugin*
   */
  complex::AbstractPlugin* getPlugin(const complex::FilterHandle& handle) const;

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
   * @return std::set<complex::AbstractPlugin*>
   */
  std::set<complex::AbstractPlugin*> getLoadedPlugins() const;

  /**
   * @brief Returns a pointer to the plugin with the specified ID. Returns
   * nullptr if no plugin with the given ID is found.
   * @param id
   * @return AbstractPlugin*
   */
  AbstractPlugin* getPluginById(const FilterHandle::PluginIdType& id) const;

protected:
private:
  std::set<FilterHandle> m_FilterHandles;
  std::map<FilterHandle::PluginIdType, std::shared_ptr<complex::PluginLoader>> m_PluginMap;
};
} // namespace complex
