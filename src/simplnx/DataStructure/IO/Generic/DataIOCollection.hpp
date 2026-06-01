#pragma once

#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/AbstractListStore.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace nx::core
{
template <typename T>
class AbstractDataStore;
class AbstractStringStore;
class DataObject;
class DataStructure;
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

  /**
   * @brief Callback that eagerly loads a single DataObject's data from HDF5 into memory.
   * Constructed by the loading infrastructure and used by Dream3dIO import and the
   * OOC import strategy to bring individual arrays fully into memory.
   */
  using EagerLoadFnc = std::function<Result<>(DataStructure& dataStructure, const DataPath& path)>;

  DataIOCollection();
  ~DataIOCollection() noexcept;

  /**
   * Adds a specified data IO manager for reading and writing to the target format.
   * @param manager
   * @return Result<> with an error if the manager is null or attempts to register
   * under the reserved k_InMemoryFormat name.
   */
  Result<> addIOManager(std::shared_ptr<IDataIOManager> manager);

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
   * @brief Checks whether any registered IO manager provides a factory for
   * creating StringStores of the specified format.
   *
   * @param type The format name to query
   * @return true if at least one registered IO manager has a StringStore factory for @p type
   */
  bool hasStringStoreCreationFnc(const std::string& type) const;

  /**
   * @brief Creates a StringStore (backing store for StringArray) of the
   * specified format and dimensions.
   *
   * Searches all registered IO managers for one that handles the requested
   * format, then delegates to its StringStoreCreateFnc factory. The resulting
   * store may be in-memory or disk-backed depending on the IO manager.
   *
   * @param type       The format name to use
   * @param tupleShape Tuple dimensions for the string array
   * @return A new AbstractStringStore, or nullptr if no factory handles @p type
   */
  std::unique_ptr<AbstractStringStore> createStringStore(const std::string& type, const ShapeType& tupleShape);

  /**
   * @brief Finalizes all stores after pipeline execution.
   *
   * Called after a pipeline finishes executing. When built with
   * SIMPLNX_USE_OOC, this forwards to SimplnxOoc::finalizeStores, which walks
   * the DataStructure and transitions OOC stores from write mode to read-only
   * mode (e.g., closing HDF5 write handles and re-opening as read handles). It
   * is a no-op for in-core builds and for in-memory stores.
   *
   * @param dataStructure The DataStructure whose stores should be finalized
   */
  void finalizeStores(DataStructure& dataStructure);

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

  /**
   * @brief Registers a human-readable display name for a data store format.
   *
   * Associates an internal format name (e.g., "HDF5-OOC") with a user-friendly
   * label (e.g., "HDF5 Out-of-Core") for display in the DataStoreFormatParameter
   * dropdown.
   *
   * @param formatName   The internal format identifier
   * @param displayName  The human-readable label shown in the UI
   */
  void registerFormatDisplayName(const std::string& formatName, const std::string& displayName);

  /**
   * @brief Returns all known format display names as (formatName, displayName) pairs.
   *
   * The returned list always starts with:
   *   - ("", "Automatic") -- lets the resolver decide
   *   - (Preferences::k_InMemoryFormat, "In Memory") -- explicit in-memory
   *
   * Followed by any additionally registered entries (e.g., ("HDF5-OOC", "HDF5 Out-of-Core")).
   *
   * @return Vector of (formatName, displayName) pairs
   */
  std::vector<std::pair<std::string, std::string>> getFormatDisplayNames() const;

  /**
   * @brief Produces a human-readable, multi-line description of every registered
   * IO manager and the store types it can create.
   *
   * Intended for error messages when a createXxxStore() call returns nullptr so
   * the user can immediately see which format names are available and what each
   * one supports. Each row lists the manager's display name (falling back to its
   * format-name identifier) followed by the set of factories it registers:
   * DataStore, ListStore, StringStore.
   *
   * Example output:
   * @code
   * Registered IO managers and their capabilities:
   *   In Memory : DataStore, ListStore
   *   HDF5      : DataStore, ListStore, StringStore
   * @endcode
   *
   * @return A multi-line string ready to drop into a fmt::format error message.
   */
  std::string generateManagerListString() const;

private:
  map_type m_ManagerMap;
  std::map<std::string, std::string> m_FormatDisplayNames; ///< Registered human-readable format names
};
} // namespace nx::core
