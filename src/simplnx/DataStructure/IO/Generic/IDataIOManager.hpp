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
class AbstractStringStore;
class DataStructure;
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
  /**
   * @brief Factory callback for creating a new in-memory DataStore.
   *
   * Takes the numeric type, tuple shape, component shape, and an optional chunk
   * shape hint. Returns a newly allocated IDataStore. Registered by IO managers
   * that provide writable storage (e.g., CoreDataIOManager for in-memory, or an
   * OOC manager for chunk-backed stores).
   */
  using DataStoreCreateFnc = std::function<std::unique_ptr<IDataStore>(DataType, const ShapeType&, const ShapeType&, const std::optional<ShapeType>&)>;

  /**
   * @brief Factory callback for creating a new in-memory NeighborList store.
   *
   * Takes the numeric type and tuple shape. Returns a newly allocated IListStore.
   * Used by IO managers that can provide writable list-based storage.
   */
  using ListStoreCreateFnc = std::function<std::unique_ptr<IListStore>(DataType, const ShapeType&)>;

  /**
   * @brief Factory callback for creating a new StringStore (e.g., for StringArray).
   *
   * Takes the tuple shape and returns a newly allocated AbstractStringStore.
   * Registered by IO managers that support string storage (in-memory or OOC).
   */
  using StringStoreCreateFnc = std::function<std::unique_ptr<AbstractStringStore>(const ShapeType& tupleShape)>;

  using DataStoreCreationMap = std::map<std::string, DataStoreCreateFnc>;     ///< Maps format name -> writable DataStore factory
  using ListStoreCreationMap = std::map<std::string, ListStoreCreateFnc>;     ///< Maps format name -> writable ListStore factory
  using StringStoreCreationMap = std::map<std::string, StringStoreCreateFnc>; ///< Maps format name -> StringStore factory

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

  /**
   * @brief Checks whether this IO manager has registered a factory for creating
   * StringStores (stores backing StringArray objects).
   *
   * @param type The format name to look up
   * @return true if a StringStoreCreateFnc is registered for @p type
   */
  bool hasStringStoreCreationFnc(const std::string& type) const;

  /**
   * @brief Returns the factory callback for creating a StringStore. The
   * resulting store backs a StringArray and may be in-memory or out-of-core
   * depending on the IO manager that registered it.
   *
   * @param type The format name to look up
   * @return The registered StringStoreCreateFnc, or nullptr if none is registered
   */
  StringStoreCreateFnc stringStoreCreationFnc(const std::string& type) const;

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

  /**
   * @brief Registers a factory callback that creates StringStores for the given
   * format name.
   *
   * Derived IO managers call this during construction to advertise their
   * ability to create StringStores (in-memory or OOC). DataIOCollection::createStringStore()
   * dispatches to the callback registered here.
   *
   * @param type The format name to register under
   * @param creationFnc The factory callback. Replaces any previously registered
   *                     callback for the same @p type.
   */
  void addStringStoreCreationFnc(const std::string& type, StringStoreCreateFnc creationFnc);

private:
  factory_collection m_FactoryCollection;
  DataStoreCreationMap m_DataStoreCreationMap;
  ListStoreCreationMap m_ListStoreCreationMap;
  StringStoreCreationMap m_StringStoreCreationMap; ///< StringStore factories keyed by format name
};
} // namespace nx::core
