#pragma once

#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"

#include <map>
#include <memory>
#include <string>

namespace nx::core
{
template <typename T>
class AbstractDataStore;
class IDataIOManager;

/**
 * @brief The DataIOCollection class contains all known IDataIOManagers for the current Application instance.
 */
class SIMPLNX_EXPORT DataIOCollection
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
   * Simplnx comes with HDF5 IO Manager.
   * Additional IDataIOManagers are added through plugins.
   * @param formatName
   * @return std::shared_ptr<IDataIOManager>
   */
  std::shared_ptr<IDataIOManager> getManager(const std::string& formatName) const;

  /**
   * @brief Returns a vector of names used to reference available DataStructure IO formats.
   * @return std::vector<std::string>
   */
  std::vector<std::string> getFormatNames() const;

  bool hasDataStoreCreationFunction(const std::string& type) const;
  std::unique_ptr<IDataStore> createDataStore(const std::string& type, DataType numericType, const typename IDataStore::ShapeType& tupleShape, const typename IDataStore::ShapeType& componentShape);
  template <typename T>
  std::shared_ptr<AbstractDataStore<T>> createDataStoreWithType(const std::string& type, const typename IDataStore::ShapeType& tupleShape, const typename IDataStore::ShapeType& componentShape)
  {
    DataType numericType = GetDataType<T>();
    std::shared_ptr<IDataStore> dataStore = createDataStore(type, numericType, tupleShape, componentShape);
    return std::dynamic_pointer_cast<AbstractDataStore<T>>(dataStore);
  }

  /**
   * @brief Checks the
   */
  void checkStoreDataFormat(uint64 dataSize, std::string& dataFormat) const;

  iterator begin();
  iterator end();

  const_iterator begin() const;
  const_iterator end() const;

private:
  map_type m_ManagerMap;
};
} // namespace nx::core
