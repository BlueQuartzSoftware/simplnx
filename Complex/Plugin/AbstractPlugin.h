#pragma once

#include <set>
#include <string>

#include "Complex/Core/FilterHandle.h"

namespace Complex
{
class AbstractFilter;
class AbstractPlugin;

// typedef AbstractPlugin* (*FilterCreationFunc)(void);
using FilterCreationFunc = AbstractPlugin* (*)();

/**
 * class AbstractPlugin
 *
 */

class AbstractPlugin
{
public:
  using IdType = std::string;

  virtual ~AbstractPlugin();

  /**
   * @brief Returns the plugin's name.
   * @return std::string
   */
  std::string getName() const;

  /**
   * @brief Returns the plugin's description.
   * @return std::string
   */
  std::string getDescription() const;

  /**
   * @brief Returns the plugin's ID.
   * @return IdType
   */
  IdType getId() const;

  /**
   * @brief Create's an AbstractFilter with the specified ID. If the plugin
   * does not contain a filter with the specified ID, this function returns
   * nullptr.
   * @param id
   * @return Complex::AbstractFilter*
   */
  Complex::AbstractFilter* createFilter(FilterHandle::FilterIdType id) const;

  /**
   * @brief Returns a set of FilterHandles pointing to each of the filters contained in the plugin.
   * @return std::set<Complex::FilterHandle>
   */
  std::set<FilterHandle> getFilterHandles() const;

  /**
   * @brief Returns the plugin's vendor name.
   * @return std::string
   */
  std::string getVendor() const;

protected:
  /**
   * @brief AbstractPlugin constructor. Takes an ID, name, and description.
   * @param id
   * @param name
   * @param description
   */
  AbstractPlugin(IdType id, const std::string& name, const std::string& description);

  /**
   * @brief
   * @param filterFunc
   */
  void addFilter(FilterCreationFunc filterFunc);

  /**
   * @brief
   * @param vendor
   */
  void setVendor(const std::string& vendor);

  /**
   * @brief Get the list of FilterHandle objects.
   * @return std::set<FilterHandle>
   */
  std::set<FilterHandle> getFilterHandlesList() const;

private:
  std::string m_Name;
  IdType m_Id;
  std::string m_Vendor;
  std::set<FilterHandle> m_FilterHandles;

  /**
   * @brief Add an item to the list of FilterHandles.
   * @param addHandle
   */
  void addFilterHandle(const FilterHandle& addHandle);
};
} // namespace Complex
