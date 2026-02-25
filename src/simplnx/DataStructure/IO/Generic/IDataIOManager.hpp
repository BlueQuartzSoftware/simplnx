#pragma once

#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/DataStructure/IListStore.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataFactory.hpp"
#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Types.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace nx::core
{
class IDataFactory;

/**
 * @brief The IDataIOManager class serves as a base point for reading and writing DataStructures to specific file formats.
 * To support a new file format, create a derived class and provide subclasses of IDataFactory for all concrete DataObject types.
 */
class SIMPLNX_EXPORT IDataIOManager
{
public:
  using factory_id_type = std::string;
  using factory_ptr = std::shared_ptr<IDataFactory>;
  using factory_collection = std::map<factory_id_type, factory_ptr>;
  using DataStoreCreateFnc = std::function<std::unique_ptr<IDataStore>(DataType, const ShapeType&, const ShapeType&, const std::optional<ShapeType>&)>;
  using ListStoreCreateFnc = std::function<std::unique_ptr<IListStore>(DataType, const ShapeType&)>;
  using DataStoreCreationMap = std::map<std::string, DataStoreCreateFnc>;
  using ListStoreCreationMap = std::map<std::string, ListStoreCreateFnc>;

  virtual ~IDataIOManager() noexcept;

  /**
   * @brief Returns the format name for this IO manager as a string.
   * @return std::string The format name
   */
  virtual std::string formatName() const = 0;

  /**
   * @brief Returns a collection of available data factories.
   * @return factory_collection
   */
  factory_collection getFactories() const;

  /**
   * @brief Returns a pointer to the factory used for creating a specific
   * DataObject subclass.
   * @param typeName
   * @return factory_ptr
   */
  factory_ptr getFactory(factory_id_type typeName) const;

  /**
   * @brief Returns a pointer to the factory used for creating a specific DataObject subclass.
   * @param typeName
   * @return std::shared_ptr<T>
   */
  template <typename T>
  std::shared_ptr<T> getFactoryAs(factory_id_type typeName) const
  {
    return std::dynamic_pointer_cast<T>(getFactory(typeName));
  }

  /**
   * Adds a factory of the specified type. T should be the IDataFactory subclass type.
   */
  template <typename T>
  void addFactory()
  {
    auto sharedIO = std::make_shared<T>();
    const auto key = sharedIO->getTypeName();
    m_FactoryCollection[key] = sharedIO;
  }

  /**
   * @brief Returns a map of all data store creation functions.
   * @return DataStoreCreationMap Map of type names to creation functions
   */
  DataStoreCreationMap getDataStoreCreationFunctions();

  /**
   * @brief Checks if a data store creation function exists for the specified type.
   * @param type The data store type name
   * @return bool True if the creation function exists, false otherwise
   */
  bool hasDataStoreCreationFnc(const std::string& type) const;

  /**
   * @brief Returns the data store creation function for the specified type.
   * @param type The data store type name
   * @return DataStoreCreateFnc The creation function
   */
  DataStoreCreateFnc dataStoreCreationFnc(const std::string& type) const;

  /**
   * @brief Checks if a list store creation function exists for the specified type.
   * @param type The list store type name
   * @return bool True if the creation function exists, false otherwise
   */
  bool hasListStoreCreationFnc(const std::string& type) const;

  /**
   * @brief Returns the list store creation function for the specified type.
   * @param type The list store type name
   * @return ListStoreCreateFnc The creation function
   */
  ListStoreCreateFnc listStoreCreationFnc(const std::string& type) const;

protected:
  /**
   * @brief Default constructor.
   */
  IDataIOManager();

  /**
   * @brief Adds a data store creation function for the specified type.
   * @param type The data store type name
   * @param creationFnc The creation function to add
   */
  void addDataStoreCreationFnc(const std::string& type, DataStoreCreateFnc creationFnc);

  /**
   * @brief Adds a list store creation function for the specified type.
   * @param type The list store type name
   * @param creationFnc The creation function to add
   */
  void addListStoreCreationFnc(const std::string& type, ListStoreCreateFnc creationFnc);

private:
  factory_collection m_FactoryCollection;
  DataStoreCreationMap m_DataStoreCreationMap;
  ListStoreCreationMap m_ListStoreCreationMap;
};
} // namespace nx::core
