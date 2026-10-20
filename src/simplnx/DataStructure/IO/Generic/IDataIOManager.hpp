#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/DataStructure/IListStore.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataFactory.hpp"
#include "simplnx/DataStructure/IO/Generic/IExternalSort.hpp"
#include "simplnx/DataStructure/IO/Generic/ITemporaryRecordStore.hpp"
#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Types.hpp"

#include <filesystem>
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
class AbstractStringStore;
class DataStructure;
class IDataFactory;

/**
 * @brief Selects the initial physical-storage policy for a new DataStore.
 *
 * DeferredZeroFill preserves logical zero reads. A storage provider can defer
 * physical zero writes until a caller overwrites a chunk.
 */
enum class DataStoreInitializationMode : uint8
{
  Default,
  DeferredZeroFill
};

/**
 * @enum FileWriteHookMode
 * @brief Selects what IDataIOManager::onBeforeFileWrite() does with a store that reads the target file.
 *
 * A writer replaces the file at its target path. A store that reads that file keeps the path, so after the save it
 * reads other data or holds a handle that blocks the write. The caller selects how a manager prevents this.
 */
enum class FileWriteHookMode : uint8
{
  ReleaseReaders, ///< Copies each store that reads the target to a session file before the write. Pipeline writers and APIs use this mode.
  RefuseIfRead    ///< Copies nothing and returns an error if a store reads the target. The GUI Save File action uses this mode, because its recovery checkpoint still refers to those stores.
};

/**
 * @class IDataIOManager
 * @brief Defines factories and lifecycle hooks for one storage format.
 *
 * DataIOCollection holds managers by format name. Each manager retains shared DataObject factories and owning store-creation callbacks.
 * Derived managers register factories during application setup.
 *
 * Registration and lifecycle changes are not synchronized with factory lookup. Complete manager configuration before concurrent store creation or import.
 */
class SIMPLNX_EXPORT IDataIOManager
{
public:
  /**
   * @brief Names a DataObject factory identifier.
   */
  using factory_id_type = std::string;

  /**
   * @brief Shares ownership of a registered DataObject factory.
   */
  using factory_ptr = std::shared_ptr<IDataFactory>;

  /**
   * @brief Maps factory identifiers to shared factories.
   */
  using factory_collection = std::map<factory_id_type, factory_ptr>;

  /**
   * @brief Names a callback that creates a writable numeric store.
   *
   * The callback takes the numeric type, shapes, optional chunk-shape hint, and initialization policy.
   * The callback transfers store ownership to its caller.
   */
  using DataStoreCreateFnc = std::function<std::unique_ptr<IDataStore>(DataType, const ShapeType&, const ShapeType&, const std::optional<ShapeType>&, DataStoreInitializationMode)>;

  /**
   * @brief Names a callback that creates a writable NeighborList store.
   *
   * The callback transfers ownership to its caller.
   */
  using ListStoreCreateFnc = std::function<std::unique_ptr<IListStore>(DataType, const ShapeType&)>;

  /**
   * @brief Names a callback that creates a StringArray backing store.
   *
   * The callback transfers ownership to its caller.
   */
  using StringStoreCreateFnc = std::function<std::unique_ptr<AbstractStringStore>(const ShapeType& tupleShape)>;

  /**
   * @brief Maps format names to numeric-store factories.
   */
  using DataStoreCreationMap = std::map<std::string, DataStoreCreateFnc>;

  /**
   * @brief Maps format names to NeighborList-store factories.
   */
  using ListStoreCreationMap = std::map<std::string, ListStoreCreateFnc>;

  /**
   * @brief Maps format names to StringArray-store factories.
   */
  using StringStoreCreationMap = std::map<std::string, StringStoreCreateFnc>;

  /**
   * @brief Destroys the I/O manager.
   */
  virtual ~IDataIOManager() noexcept;

  /**
   * @brief Returns the manager's format identifier.
   * @return Identifier used by DataIOCollection registration and factory lookup.
   *
   * The built-in CoreDataIOManager owns the reserved in-memory identifier.
   */
  virtual std::string formatName() const = 0;

  /**
   * @brief Returns a snapshot of registered DataObject factories.
   * @return Factory map that shares ownership with this manager.
   */
  factory_collection getFactories() const;

  /**
   * @brief Returns a factory for one DataObject type.
   * @param typeName DataObject factory identifier.
   * @return Shared factory, or null when typeName is not registered.
   */
  factory_ptr getFactory(factory_id_type typeName) const;

  /**
   * @brief Returns a registered factory cast to a selected type.
   * @tparam T Expected IDataFactory subclass.
   * @param typeName DataObject factory identifier.
   * @return Shared factory cast to T, or null when the factory is absent or incompatible.
   */
  template <typename T>
  std::shared_ptr<T> getFactoryAs(factory_id_type typeName) const
  {
    return std::dynamic_pointer_cast<T>(getFactory(typeName));
  }

  /**
   * @brief Default-constructs and registers a DataObject factory.
   * @tparam T Default-constructible IDataFactory subclass.
   *
   * A factory with the same type name is replaced.
   */
  template <typename T>
  void addFactory()
  {
    auto sharedIO = std::make_shared<T>();
    const auto key = sharedIO->getTypeName();
    m_FactoryCollection[key] = sharedIO;
  }

  /**
   * @brief Returns a snapshot of numeric-store factories.
   * @return Factory map that copies callback ownership.
   */
  DataStoreCreationMap getDataStoreCreationFunctions();

  bool hasDataStoreCreationFnc(const std::string& type) const;

  /**
   * @brief Returns a numeric-store creation callback.
   * @param type Format identifier to look up.
   * @return Registered callback for type.
   * @throws std::out_of_range if type is not registered.
   */
  DataStoreCreateFnc dataStoreCreationFnc(const std::string& type) const;

  bool hasListStoreCreationFnc(const std::string& type) const;

  /**
   * @brief Returns a NeighborList-store creation callback.
   * @param type Format identifier to look up.
   * @return Registered callback for type.
   * @throws std::out_of_range if type is not registered.
   */
  ListStoreCreateFnc listStoreCreationFnc(const std::string& type) const;

  bool hasStringStoreCreationFnc(const std::string& type) const;

  /**
   * @brief Returns a StringArray-store creation callback.
   * @param type Format identifier to look up.
   * @return Registered callback, or an empty callback when type is not registered.
   */
  StringStoreCreateFnc stringStoreCreationFnc(const std::string& type) const;

  /**
   * @brief Reports whether this manager finalizes deferred DREAM3D imports.
   * @return True when onImportFinalize() replaces placeholder stores.
   *
   * The base returns false. Import configuration must provide at most one
   * finalizing manager.
   */
  virtual bool finalizesImport() const
  {
    return false;
  }

  /**
   * @brief Replaces this manager's placeholder stores after a deferred import.
   * @param dataStructure DataStructure that owns imported placeholders and resolver policy.
   * @param paths Selected top-level imported paths. The manager can walk their descendants.
   * @param fileReader Source DREAM3D reader borrowed for this call.
   * @return Valid result, warnings, or an import-finalization error.
   *
   * The manager must leave placeholders that it does not own. The core import
   * sweep eagerly loads those remaining placeholders in memory. The manager must
   * not retain fileReader after this call.
   */
  virtual Result<> onImportFinalize([[maybe_unused]] DataStructure& dataStructure, [[maybe_unused]] const std::vector<DataPath>& paths, [[maybe_unused]] const HDF5::FileIO& fileReader)
  {
    return {};
  }

  /**
   * @brief Optionally writes a recovery representation for one DataObject.
   * @param writer Active DREAM3D writer borrowed for this call.
   * @param dataObject DataObject to write.
   * @param parentGroup Destination group borrowed for this call.
   * @return Write result when handled, or std::nullopt to use normal serialization.
   */
  virtual std::optional<Result<>> onRecoveryWrite([[maybe_unused]] HDF5::DataStructureWriter& writer, [[maybe_unused]] const DataObject* dataObject, [[maybe_unused]] HDF5::GroupIO& parentGroup)
  {
    return std::nullopt;
  }

  /**
   * @brief Finalizes this manager's stores after pipeline execution.
   * @param dataStructure DataStructure that owns the stores.
   *
   * An out-of-core manager can close write handles and reopen read handles. The
   * base implementation is a no-op.
   */
  virtual void onFinalizeStores([[maybe_unused]] DataStructure& dataStructure)
  {
  }

  /**
   * @brief Prepares this manager's stores before a writer changes a file.
   * @param dataStructure Structure that the writer writes. The manager examines only the stores in this structure.
   * @param target Final path that the writer creates, truncates, opens for writing or renames over.
   * @param mode Selects whether a store that reads target is copied to a session file or causes an error.
   * @return Valid result, possibly with warnings, when no store in dataStructure reads target after the call.
   * Otherwise an error. Its message contains target.
   * - -6039: A store that reads target cannot be released, or the manager cannot prove that a store does not read target.
   * - -6053: Mode is RefuseIfRead and a store reads target.
   *
   * A store that reads a file keeps the path of that file. If a writer replaces the file, the store reads other data
   * or holds a handle that blocks the write. Each path-based writer calls DataIOCollection::beforeFileWrite() on every
   * final path before it changes one of them. On error, the writer returns the error and changes none of them. The
   * base implementation does nothing, because its stores do not read a file after import.
   *
   * A release changes the backing file of a store, not a value that dataStructure exposes. Thus the parameter is const.
   * Stores are created as mutable objects and are owned through std::shared_ptr. Thus an override can cast const away
   * from a store that it releases. Each store type must keep this mutable-pointee guarantee. The const parameter does
   * not make the release safe to run concurrently.
   *
   * Concurrency contract:
   * - Reads that can run during the call: numeric tuple, value and extent reads, and NeighborList list reads. The 3D
   *   view, charts and spreadsheets use these reads.
   * - Operations that the caller must not run during the call:
   *   - Another hook call or save on the same structure.
   *   - A write or other change to a store in the structure, including resize.
   *   - Deep copies, recovery-metadata queries, file-path queries and handle closes on a store in the structure.
   *   - Replacement of stores in the structure.
   *
   * Not covered:
   * - Stores outside dataStructure that read target, for example earlier results, another window or a Python session.
   *   On POSIX they keep the old file or read the new file by path. On Windows their open handle can make the final
   *   rename fail. Then the file at target stays unchanged.
   * - A rename, unlink or retarget of the source file of a store while the store exists.
   * - A change of the working directory while a store reads a relative path.
   * - Cancellation of a release. Stores released before an error or a cancellation stay released.
   * - A recovery checkpoint that refers to a released store. The checkpoint refers to a session file, and the store
   *   deletes that file when the store is destroyed. Recovery does not take ownership of that file.
   */
  virtual Result<> onBeforeFileWrite([[maybe_unused]] const DataStructure& dataStructure, [[maybe_unused]] const std::filesystem::path& target, [[maybe_unused]] FileWriteHookMode mode)
  {
    return {};
  }

  /**
   * @brief Sets the working-file base directory.
   * @param path Directory for manager-specific session files.
   *
   * The base implementation is a no-op.
   */
  virtual void setBaseDirectory([[maybe_unused]] const std::filesystem::path& path)
  {
  }

  /**
   * @brief Flushes manager state before temporary-file cleanup.
   *
   * The base implementation is a no-op.
   */
  virtual void shutdownManager()
  {
  }

  /**
   * @brief Reports whether this manager provides bounded external sorting.
   * @return True when createExternalSort() can create a sorter.
   */
  virtual bool supportsExternalSort() const
  {
    return false;
  }

  /**
   * @brief Creates an owned bounded external sorter.
   * @param config Sort configuration.
   * @return Owned sorter or an unsupported-capability error.
   */
  virtual Result<std::unique_ptr<IExternalSort>> createExternalSort([[maybe_unused]] const ExternalSortConfig& config) const
  {
    return MakeErrorResult<std::unique_ptr<IExternalSort>>(-6012, "This I/O manager does not provide external sorting");
  }

  /**
   * @brief Reports whether this manager provides temporary fixed-record storage.
   * @return True when createTemporaryRecordStore() can create a store.
   */
  virtual bool supportsTemporaryRecordStore() const
  {
    return false;
  }

  /**
   * @brief Creates owned temporary fixed-record storage.
   * @param config Record-store configuration.
   * @return Owned record store or an unsupported-capability error.
   */
  virtual Result<std::unique_ptr<ITemporaryRecordStore>> createTemporaryRecordStore([[maybe_unused]] const TemporaryRecordStoreConfig& config) const
  {
    return MakeErrorResult<std::unique_ptr<ITemporaryRecordStore>>(-6014, "This I/O manager does not provide temporary record storage");
  }

protected:
  /**
   * @brief Creates an empty I/O manager.
   */
  IDataIOManager();

  /**
   * @brief Registers a numeric-store creation callback.
   * @param type Format identifier to register.
   * @param creationFnc Callback that replaces any existing callback for type.
   */
  void addDataStoreCreationFnc(const std::string& type, DataStoreCreateFnc creationFnc);

  /**
   * @brief Registers a NeighborList-store creation callback.
   * @param type Format identifier to register.
   * @param creationFnc Callback that replaces any existing callback for type.
   */
  void addListStoreCreationFnc(const std::string& type, ListStoreCreateFnc creationFnc);

  /**
   * @brief Registers a StringArray-store creation callback.
   * @param type Format identifier to register.
   * @param creationFnc Callback that replaces any existing callback for type.
   */
  void addStringStoreCreationFnc(const std::string& type, StringStoreCreateFnc creationFnc);

private:
  factory_collection m_FactoryCollection;
  DataStoreCreationMap m_DataStoreCreationMap;
  ListStoreCreationMap m_ListStoreCreationMap;
  StringStoreCreationMap m_StringStoreCreationMap;
};
} // namespace nx::core
