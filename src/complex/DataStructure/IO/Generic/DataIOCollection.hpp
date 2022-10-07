#pragma once

#include "complex/complex_export.hpp"

#include <map>
#include <memory>
#include <string>

namespace complex
{
class IDataIOManager;

/**
 * @brief The DataIOCollection class contains all known IDataIOManagers for the current complex::Application instance.
 */
class COMPLEX_EXPORT DataIOCollection
{
public:
  using map_type = std::map<std::string, std::shared_ptr<IDataIOManager>>;
  using iterator = typename map_type::iterator;
  using const_iterator = typename map_type::const_iterator;

  DataIOCollection();
  ~DataIOCollection() noexcept;

  /**
   * Adds a specified data IO manager for reading and writing to the target format.
   * @param manager
   */
  void addIOManager(std::shared_ptr<IDataIOManager> manager);

  /**
   * @brief Returns the IDataIOManager for the specified format name.
   * Complex comes with HDF5 IO Manager.
   * Additional IDataIOManagers are added through plugins.
   * @param formatName
   * @return std::shared_ptr<IDataIOManager>
   */
  std::shared_ptr<IDataIOManager> getManager(const std::string& formatName) const;

  iterator begin();
  iterator end();

  const_iterator begin() const;
  const_iterator end() const;

private:
  map_type m_ManagerMap;
};
} // namespace complex
