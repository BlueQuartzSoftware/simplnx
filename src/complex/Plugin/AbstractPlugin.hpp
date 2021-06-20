#pragma once

#include <map>
#include <set>
#include <string>

#include "complex/Core/FilterHandle.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
class AbstractFilter;
class AbstractPlugin;

using FilterCreationFunc = AbstractFilter* (*)();

/**
 * class AbstractPlugin
 *
 */

class COMPLEX_EXPORT AbstractPlugin
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
   * @brief Checks if the plugin contains a filter with the given ID. Returns
   * true if the plugin contains the filter. Returns false otherwise.
   * @param id
   * @return bool
  */
  bool containsFilterId(FilterHandle::FilterIdType id) const;

  /**
   * @brief Create's an AbstractFilter with the specified ID. If the plugin
   * does not contain a filter with the specified ID, this function returns
   * nullptr.
   * @param id
   * @return complex::AbstractFilter*
   */
  complex::AbstractFilter* createFilter(FilterHandle::FilterIdType id) const;

  /**
   * @brief Returns a set of FilterHandles pointing to each of the filters contained in the plugin.
   * @return std::set<complex::FilterHandle>
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
   * @brief Records information for creating a filter using the provided
   * FilterCreationFunc. The filter ID is provided by the filter created by the
   * FilterCreationFunc. If the FilterCreationFunc fails to create a filter,
   * this method does nothing.
   * @param filterFunc
   */
  void addFilter(FilterCreationFunc filterFunc);

  /**
   * @brief Sets the plugin vendor name.
   * @param vendor
   */
  void setVendor(const std::string& vendor);

private:
  IdType m_Id;
  std::string m_Name;
  std::string m_Description;
  std::string m_Vendor;
  std::set<FilterHandle> m_FilterHandles;
  std::map<FilterHandle::FilterIdType, FilterCreationFunc> m_InitializerMap;

  /**
   * @brief Add an item to the list of FilterHandles.
   * @param addHandle
   */
  void addFilterHandle(const FilterHandle& addHandle);
};
} // namespace complex
