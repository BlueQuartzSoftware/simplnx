#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "Complex/Core/FilterHandle.h"

namespace Complex
{
class AbstractFilter;
class AbstractPlugin;
class PluginLoader;

/**
 * class FilterList
 *
 */

class FilterList
{
public:
  // Constructors/Destructors
  //

  /**
   * Empty Constructor
   */
  FilterList();

  /**
   * Empty Destructor
   */
  virtual ~FilterList();

  // Static Public attributes
  //

  // Public attributes
  //

  /**
   * @brief
   * @return std::set<Complex::FilterHandle>
   */
  std::set<Complex::FilterHandle> getFilterHandles() const;

  /**
   * @brief
   * @param txt
   * @return std::vector<Complex::FilterHandle>
   */
  std::vector<Complex::FilterHandle> search(const std::string& text) const;

  /**
   * @brief
   * @param handle
   * @return Complex::AbstractFilter*
   */
  Complex::AbstractFilter* createFilter(const Complex::FilterHandle& handle) const;

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
   * @return std::set<Complex::AbstractPlugin*>
   */
  std::set<Complex::AbstractPlugin*> getLoadedPlugins() const;

protected:
private:
  std::set<FilterHandle> m_FilterHandles;
  std::map<FilterHandle::PluginIdType, std::shared_ptr<Complex::PluginLoader>> m_PluginMap;
};
} // namespace Complex
