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
   * @brief
   * @return std::set<complex::FilterHandle>
   */
  std::set<complex::FilterHandle> getFilterHandles() const;

  /**
   * @brief
   * @param txt
   * @return std::vector<complex::FilterHandle>
   */
  std::vector<complex::FilterHandle> search(const std::string& text) const;

  /**
   * @brief
   * @param handle
   * @return complex::AbstractFilter*
   */
  complex::AbstractFilter* createFilter(const complex::FilterHandle& handle) const;

  /**
   * @brief
   * @param loader
   * @return bool
   */
  bool addPlugin(const std::shared_ptr<PluginLoader>& loader);

  /**
   * @brief
   * @param path
   * @return bool
   */
  bool addPlugin(const std::string& path);

  /**
   * @brief
   * @return std::set<complex::AbstractPlugin*>
   */
  std::set<complex::AbstractPlugin*> getLoadedPlugins() const;

protected:
private:
  std::set<FilterHandle> m_FilterHandles;
  std::map<FilterHandle::PluginIdType, std::shared_ptr<complex::PluginLoader>> m_PluginMap;
};
} // namespace complex
