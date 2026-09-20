#pragma once

#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/AbstractListStore.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/IO/Generic/IExternalSort.hpp"
#include "simplnx/DataStructure/IO/Generic/ITemporaryRecordStore.hpp"
#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace nx::core::HDF5
{
class DataStructureWriter;
class FileIO;
class GroupIO;
} // namespace nx::core::HDF5

namespace nx::core
{
template <typename T>
class AbstractDataStore;
class AbstractStringStore;
class DataObject;
class DataStructure;
class IDataIOManager;

/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */

/**
 * @class DataIOCollection
 * @brief Owns registered storage-format managers for one Application.
 *
 * The collection always owns the built-in in-memory and HDF5 managers. Plugins
 * add optional managers during application setup. Map iteration defines the
 * fallback order for capabilities and recovery overrides.
 *
 * ArrayCreationUtilities resolves storage policy before factory creation. The
 * collection selects a manager for that identifier. Unknown numeric and
 * NeighborList formats fall back to the built-in in-memory manager.
 *
 * Import code installs resolver policy on the DataStructure before finalization.
 * The collection does not retain that resolver or the borrowed HDF5 reader.
 *
 * Registration and lifecycle changes are not synchronized with lookups. Finish
 * registration before concurrent creation, import, or manager iteration.
 */
class SIMPLNX_EXPORT DataIOCollection
{
public:
  /**
   * @brief Maps format names to shared I/O managers.
   */
  using map_type = std::map<std::string, std::shared_ptr<IDataIOManager>>;

  /**
   * @brief Names a mutable manager-map iterator.
   */
  using iterator = typename map_type::iterator;

  /**
   * @brief Names a read-only manager-map iterator.
   */
  using const_iterator = typename map_type::const_iterator;

  /**
   * @brief Names a callback that eagerly loads one imported DataObject.
   *
   * The import path supplies a valid DataStructure and object path. The callback
   * returns its HDF5 load result to the finalizing manager.
   */
  using EagerLoadFnc = std::function<Result<>(DataStructure& dataStructure, const DataPath& path)>;

  /**
   * @brief Labels automatic format resolution.
   */
  static inline constexpr const char* k_AutomaticDisplayName = "Automatic";

  /**
   * @brief Labels explicit in-memory storage.
   */
  static inline constexpr const char* k_InMemoryDisplayName = "In Memory";

  /**
   * @brief Creates the built-in in-memory and HDF5 managers.
   */
  DataIOCollection();

  /**
   * @brief Destroys the collection and its shared managers.
   */
  ~DataIOCollection() noexcept;

  /**
   * @brief Registers or replaces a non-core I/O manager.
   * @param manager Shared manager to retain.
   * @return Valid result or an error for a null manager or reserved in-memory name.
   *
   * The manager format name is the map key. Debug builds assert that no more
   * than one manager finalizes deferred imports.
   */
  Result<> addIOManager(std::shared_ptr<IDataIOManager> manager);

  /**
   * @brief Returns a manager for one format name.
   * @param formatName Registered format identifier.
   * @return Shared manager, or null when formatName is not registered.
   */
  std::shared_ptr<IDataIOManager> getManager(const std::string& formatName) const;

  /**
   * @brief Reports external-sort capability across registered managers.
   * @return True when at least one manager advertises external sorting.
   */
  bool hasExternalSortCapability() const;

  /**
   * @brief Creates an external sorter through the first capable manager.
   * @param config Sort configuration.
   * @return Owned sorter or a capability or manager-creation error.
   *
   * Map order chooses the manager. A creation error does not try a later manager.
   */
  Result<std::unique_ptr<IExternalSort>> createExternalSort(const ExternalSortConfig& config) const;

  /**
   * @brief Reports temporary-record-store capability across registered managers.
   * @return True when at least one manager advertises temporary record storage.
   */
  bool hasTemporaryRecordStoreCapability() const;

  /**
   * @brief Creates temporary records through the first capable manager.
   * @param config Record-store configuration.
   * @return Owned record store or a capability or manager-creation error.
   *
   * Map order chooses the manager. A creation error does not try a later manager.
   */
  Result<std::unique_ptr<ITemporaryRecordStore>> createTemporaryRecordStore(const TemporaryRecordStoreConfig& config) const;

  /**
   * @brief Returns format names that create numeric stores.
   * @return Registered numeric-store format identifiers in map order.
   */
  std::vector<std::string> getFormatNames() const;

  bool hasDataStoreCreationFunction(const std::string& type) const;

  /**
   * @brief Creates a numeric store for a requested format.
   * @param type Requested format identifier.
   * @param numericType Numeric value type.
   * @param tupleShape Tuple dimensions.
   * @param componentShape Component dimensions.
   * @return Owned store from the first manager that handles type.
   *
   * If no manager handles type, the built-in in-memory manager provides the
   * fallback store.
   */
  std::unique_ptr<IDataStore> createDataStore(const std::string& type, DataType numericType, const ShapeType& tupleShape, const ShapeType& componentShape);

  /**
   * @brief Creates a typed numeric store for a requested format.
   * @tparam T Numeric value type.
   * @param type Requested format identifier.
   * @param tupleShape Tuple dimensions.
   * @param componentShape Component dimensions.
   * @return Shared typed store, or null when the selected factory returns an incompatible store.
   */
  template <typename T>
  std::shared_ptr<AbstractDataStore<T>> createDataStoreWithType(const std::string& type, const ShapeType& tupleShape, const ShapeType& componentShape)
  {
    DataType numericType = GetDataType<T>();
    std::shared_ptr<IDataStore> dataStore = createDataStore(type, numericType, tupleShape, componentShape);
    return std::dynamic_pointer_cast<AbstractDataStore<T>>(dataStore);
  }

  /**
   * @brief Creates a NeighborList store for a requested format.
   * @param type Requested format identifier.
   * @param numericType Numeric value type.
   * @param tupleShape Tuple dimensions.
   * @return Owned list store from the first manager that handles type.
   *
   * If no manager handles type, the built-in in-memory manager provides the
   * fallback store.
   */
  std::unique_ptr<IListStore> createListStore(const std::string& type, DataType numericType, const ShapeType& tupleShape) const;

  /**
   * @brief Creates a typed NeighborList store for a requested format.
   * @tparam T List element type.
   * @param dataFormat Requested format identifier.
   * @param tupleShape Tuple dimensions.
   * @return Shared typed list store, or null when the selected factory returns an incompatible store.
   */
  template <typename T>
  std::shared_ptr<AbstractListStore<T>> createListStoreWithType(const std::string& dataFormat, const ShapeType& tupleShape) const
  {
    DataType numericType = GetDataType<T>();
    std::shared_ptr<IListStore> listStore = createListStore(dataFormat, numericType, tupleShape);
    return std::dynamic_pointer_cast<AbstractListStore<T>>(listStore);
  }

  bool hasStringStoreCreationFnc(const std::string& type) const;

  /**
   * @brief Creates a StringArray store for a requested format.
   * @param type Requested format identifier.
   * @param tupleShape String-array tuple dimensions.
   * @return Owned string store, or null when no manager handles type.
   *
   * String stores have no in-memory fallback. A requested format must register
   * a StringStore factory.
   */
  std::unique_ptr<AbstractStringStore> createStringStore(const std::string& type, const ShapeType& tupleShape);

  /**
   * @brief Finalizes stores through every registered manager.
   * @param dataStructure DataStructure that owns pipeline output stores.
   *
   * Managers run in map order. An out-of-core manager can close write handles
   * and reopen read handles. The core in-memory manager is a no-op.
   */
  void finalizeStores(DataStructure& dataStructure);

  /**
   * @brief Reports whether deferred DREAM3D import finalization is available.
   * @return True when a manager reports finalizesImport().
   *
   * The importer uses this result to defer array loading. Configuration must
   * provide no more than one finalizing manager.
   */
  bool anyManagerFinalizesImport() const;

  /**
   * @brief Dispatches deferred import finalization to the unique capable manager.
   * @param dataStructure DataStructure containing imported placeholders and resolver policy.
   * @param paths Selected top-level imported paths.
   * @param fileReader Source DREAM3D reader borrowed for this call.
   * @return Finalizer result, or std::nullopt when no manager finalizes imports.
   *
   * The selected manager leaves placeholders it does not own. The importer later
   * eager-loads those remaining placeholders in memory. If invalid configuration
   * provides more than one finalizer, map order selects the first manager.
   */
  std::optional<Result<>> onImportFinalize(DataStructure& dataStructure, const std::vector<DataPath>& paths, const nx::core::HDF5::FileIO& fileReader);

  /**
   * @brief Offers a recovery-write override to registered managers.
   * @param writer Active DREAM3D writer borrowed for this call.
   * @param dataObject DataObject to write.
   * @param parentGroup Destination group borrowed for this call.
   * @return First manager result in map order, or std::nullopt for normal serialization.
   */
  std::optional<Result<>> onRecoveryWrite(nx::core::HDF5::DataStructureWriter& writer, const DataObject* dataObject, nx::core::HDF5::GroupIO& parentGroup);

  /**
   * @brief Propagates a working-file base directory to all managers.
   * @param path Directory for manager-specific session files.
   */
  void setBaseDirectory(const std::filesystem::path& path);

  /**
   * @brief Shuts down all managers before temporary-file cleanup.
   *
   * Managers can flush dirty data and clear caches during this fan-out.
   */
  void shutdownManagers();

  iterator begin();

  iterator end();

  const_iterator begin() const;

  const_iterator end() const;

  /**
   * @brief Registers a display label for a storage format.
   * @param formatName Internal format identifier.
   * @param displayName User-visible format label.
   *
   * A later registration replaces the label for formatName.
   */
  void registerFormatDisplayName(const std::string& formatName, const std::string& displayName);

  /**
   * @brief Returns storage-format identifiers and display labels.
   * @return Format-label pairs with Automatic and In Memory first.
   *
   * Registered labels follow in map order.
   */
  std::vector<std::pair<std::string, std::string>> getFormatDisplayNames() const;

  /**
   * @brief Builds a multi-line summary of manager factory capabilities.
   * @return Error-message-ready table of manager labels and store factories.
   *
   * Each row uses a registered display label or the format identifier. The row
   * lists numeric, NeighborList, and StringArray factory support.
   */
  std::string generateManagerListString() const;

private:
  map_type m_ManagerMap;
  std::map<std::string, std::string> m_FormatDisplayNames;
};
} // namespace nx::core
