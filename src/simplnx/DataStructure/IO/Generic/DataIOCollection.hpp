#pragma once

#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/AbstractListStore.hpp"
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

  /**
   * @brief Checks if a data store creation function exists for the specified type.
   * @param type The data store type name
   * @return bool True if the creation function exists, false otherwise
   */
  bool hasDataStoreCreationFunction(const std::string& type) const;

  /**
   * @brief Creates a data store of the specified type and dimensions.
   * @param type The data store type name
   * @param numericType The numeric data type
   * @param tupleShape The shape of the tuple dimensions
   * @param componentShape The shape of the component dimensions
   * @return std::unique_ptr<IDataStore> Unique pointer to the created data store
   */
  std::unique_ptr<IDataStore> createDataStore(const std::string& type, DataType numericType, const ShapeType& tupleShape, const ShapeType& componentShape);

  /**
   * @brief Creates a typed data store with the specified dimensions.
   * @tparam T The data type for the store
   * @param type The data store type name
   * @param tupleShape The shape of the tuple dimensions
   * @param componentShape The shape of the component dimensions
   * @return std::shared_ptr<AbstractDataStore<T>> Shared pointer to the created typed data store
   */
  template <typename T>
  std::shared_ptr<AbstractDataStore<T>> createDataStoreWithType(const std::string& type, const ShapeType& tupleShape, const ShapeType& componentShape)
  {
    DataType numericType = GetDataType<T>();
    std::shared_ptr<IDataStore> dataStore = createDataStore(type, numericType, tupleShape, componentShape);
    return std::dynamic_pointer_cast<AbstractDataStore<T>>(dataStore);
  }

  /**
   * @brief Creates a list store of the specified type and dimensions.
   * @param type The list store type name
   * @param numericType The numeric data type
   * @param tupleShape The shape of the tuple dimensions
   * @return std::unique_ptr<IListStore> Unique pointer to the created list store
   */
  std::unique_ptr<IListStore> createListStore(const std::string& type, DataType numericType, const ShapeType& tupleShape) const;

  /**
   * @brief Creates a typed list store with the specified dimensions.
   * @tparam T The data type for the store
   * @param dataFormat The list store format name
   * @param tupleShape The shape of the tuple dimensions
   * @return std::shared_ptr<AbstractListStore<T>> Shared pointer to the created typed list store
   */
  template <typename T>
  std::shared_ptr<AbstractListStore<T>> createListStoreWithType(const std::string& dataFormat, const ShapeType& tupleShape) const
  {
    DataType numericType = GetDataType<T>();
    std::shared_ptr<IListStore> listStore = createListStore(dataFormat, numericType, tupleShape);
    return std::dynamic_pointer_cast<AbstractListStore<T>>(listStore);
  }

  /**
   * @brief Checks and validates the data format for the given data size.
   * @param dataSize The size of the data in bytes
   * @param dataFormat Reference to the data format string to validate/update
   */
  void checkStoreDataFormat(uint64 dataSize, std::string& dataFormat) const;

  /**
   * @brief Returns an iterator to the beginning of the manager collection.
   * @return iterator Iterator to the beginning
   */
  iterator begin();

  /**
   * @brief Returns an iterator to the end of the manager collection.
   * @return iterator Iterator to the end
   */
  iterator end();

  /**
   * @brief Returns a const iterator to the beginning of the manager collection.
   * @return const_iterator Const iterator to the beginning
   */
  const_iterator begin() const;

  /**
   * @brief Returns a const iterator to the end of the manager collection.
   * @return const_iterator Const iterator to the end
   */
  const_iterator end() const;

private:
  map_type m_ManagerMap;
};
} // namespace nx::core
