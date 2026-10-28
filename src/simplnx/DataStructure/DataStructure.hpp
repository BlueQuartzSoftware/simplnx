#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataMap.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/LinkedPath.hpp"
#include "simplnx/simplnx_export.hpp"

#include <nod/nod.hpp>
#include <nonstd/expected.hpp>

#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

namespace nx::core
{
class AbstractDataStructureMessage;
class DataGroup;
class DataPath;
class IDataStoreFormatResolver;

namespace Constants
{

inline const std::string k_ObjectTypeTag = "ObjectType";

inline const std::string k_DataStructureTag = "DataStructure";

inline const std::string k_ObjectIdTag = "ObjectId";

inline const std::string k_NextIdTag = "NextObjectId";

inline const std::string k_ImportableTag = "Importable";
} // namespace Constants

/**
 * @class DataStructure
 * @brief Owns the DataObject hierarchy and object index.
 *
 * Parent groups own objects. The index uses weak references. Callers must
 * serialize access with mutations.
 */
class SIMPLNX_EXPORT DataStructure
{
  using WeakCollectionType = std::map<DataObject::IdType, std::weak_ptr<DataObject>>;

protected:
  /**
   * @brief Registers a factory object and places it under an optional parent.
   * @param obj Object to register.
   * @param parent Parent identifier, when the object is not a root object.
   * @return True when placement and registration succeed.
   */
  bool finishAddingObject(const std::shared_ptr<DataObject>& obj, const std::optional<DataObject::IdType>& parent = {});

public:
  using SignalType = nod::signal<void(DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>&)>;

  using Iterator = DataMap::Iterator;

  using ConstIterator = DataMap::ConstIterator;

  friend class DataMap;
  friend class DataObject;

  DataStructure();

  /**
   * @brief Copies the hierarchy.
   * @param other Source data structure.
   * @post Copies objects with shallowCopy(), shares the resolver, and omits
   * observers.
   */
  DataStructure(const DataStructure& other);

  /**
   * @brief Moves the hierarchy.
   * @param other Source data structure.
   * @post Rebinds object back-pointers.
   */
  DataStructure(DataStructure&& other) noexcept;

  /**
   * @brief Releases hierarchy ownership.
   * @post Detaches surviving objects from this data structure.
   */
  ~DataStructure();

  /**
   * @brief Sets the per-instance storage-format resolver.
   * @param resolver Uses the process default when null.
   * @pre Do not replace the resolver concurrently with formatResolver().
   */
  void setFormatResolver(std::shared_ptr<const IDataStoreFormatResolver> resolver);

  /**
   * @brief Returns the active storage-format resolver.
   * @return Active resolver reference.
   * @post The reference remains valid until the active resolver changes or this
   * data structure is destroyed.
   */
  const IDataStoreFormatResolver& formatResolver() const;

  /**
   * @brief Retains the current process-wide storage-format resolver.
   * @return Shared owner that keeps the resolver alive after the default changes.
   * @pre Do not call while another thread changes the default resolver.
   */
  static std::shared_ptr<const IDataStoreFormatResolver> getDefaultFormatResolver();

  /**
   * @brief Sets the process-wide storage-format resolver.
   * @param resolver Has no effect when null.
   * @pre Do not replace the resolver during concurrent resolver access.
   */
  static void setDefaultFormatResolver(std::shared_ptr<const IDataStoreFormatResolver> resolver);

  usize getSize() const;

  /**
   * @brief Clears the hierarchy and object index.
   * @post Retains the next object identifier.
   */
  void clear();

  /**
   * @brief Finds an object identifier.
   * @param path Object data path.
   * @return Zero for an empty path, or std::nullopt when the path does not
   * resolve.
   */
  std::optional<DataObject::IdType> getId(const DataPath& path) const;

  bool containsData(DataObject::IdType identifier) const;

  bool containsData(const DataPath& path) const;

  DataObject* getData(DataObject::IdType identifier);

  template <class T>
  inline T* getDataAs(DataObject::IdType identifier)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return dynamic_cast<T*>(getData(identifier));
  }

  /**
   * @brief Returns a static typed pointer.
   * @tparam T Specifies the expected DataObject type.
   * @param identifier Identifies the object.
   * @return Static typed pointer, or nullptr when no object resolves.
   * @pre A found object has dynamic type T or a type derived from T.
   */
  template <class T>
  T* getDataAsUnsafe(DataObject::IdType identifier)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return static_cast<T*>(getData(identifier));
  }

  DataObject* getData(const std::optional<DataObject::IdType>& identifier);

  template <class T>
  inline T* getDataAs(const std::optional<DataObject::IdType>& identifier)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    auto* object = getData(identifier);
    return dynamic_cast<T*>(object);
  }

  /**
   * @brief Returns a static typed pointer.
   * @tparam T Specifies the expected DataObject type.
   * @param identifier Optionally identifies the object.
   * @return Static typed pointer, or nullptr when no object resolves.
   * @pre A found object has dynamic type T or a type derived from T.
   */
  template <class T>
  T* getDataAsUnsafe(const std::optional<DataObject::IdType>& identifier)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    auto* object = getData(identifier);
    return static_cast<T*>(object);
  }

  DataObject* getData(const DataPath& path);

  /**
   * @brief Returns an object reference.
   * @param path Identifies the object.
   * @return Object reference.
   * @throws std::out_of_range if the object does not resolve.
   */
  DataObject& getDataRef(const DataPath& path);

  /**
   * @brief Returns an object reference.
   * @param identifier Identifies the object.
   * @return Object reference.
   * @throws std::out_of_range if the object does not resolve.
   */
  DataObject& getDataRef(DataObject::IdType identifier);

  template <class T>
  inline T* getDataAs(const DataPath& path)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return dynamic_cast<T*>(getData(path));
  }

  /**
   * @brief Returns a static typed pointer.
   * @tparam T Specifies the expected DataObject type.
   * @param path Identifies the object.
   * @return Static typed pointer, or nullptr when no object resolves.
   * @pre A found object has dynamic type T or a type derived from T.
   */
  template <class T>
  T* getDataAsUnsafe(const DataPath& path)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return static_cast<T*>(getData(path));
  }

  /**
   * @brief Returns a checked typed reference.
   * @tparam T Specifies the requested DataObject type.
   * @param path Identifies the object.
   * @return Typed object reference.
   * @throws std::out_of_range if the object does not resolve.
   * @throws std::bad_cast if the object is incompatible with T.
   */
  template <class T>
  inline T& getDataRefAs(const DataPath& path)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return dynamic_cast<T&>(getDataRef(path));
  }

  /**
   * @brief Returns a static typed reference.
   * @tparam T Specifies the expected DataObject type.
   * @param path Identifies the object.
   * @return Static typed reference.
   * @throws std::out_of_range if the object does not resolve.
   * @pre The found object has dynamic type T or a type derived from T.
   */
  template <class T>
  T& getDataRefAsUnsafe(const DataPath& path)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return static_cast<T&>(getDataRef(path));
  }

  /**
   * @brief Returns a checked typed reference.
   * @tparam T Specifies the requested DataObject type.
   * @param identifier Identifies the object.
   * @return Typed object reference.
   * @throws std::out_of_range if the object does not resolve.
   * @throws std::bad_cast if the object is incompatible with T.
   */
  template <class T>
  T& getDataRefAs(DataObject::IdType identifier)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return dynamic_cast<T&>(getDataRef(identifier));
  }

  /**
   * @brief Returns a static typed reference.
   * @tparam T Specifies the expected DataObject type.
   * @param identifier Identifies the object.
   * @return Static typed reference.
   * @throws std::out_of_range if the object does not resolve.
   * @pre The found object has dynamic type T or a type derived from T.
   */
  template <class T>
  T& getDataRefAsUnsafe(DataObject::IdType identifier)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return static_cast<T&>(getDataRef(identifier));
  }

  DataObject* getData(const LinkedPath& path);

  template <class T>
  inline T* getDataAs(const LinkedPath& path)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return dynamic_cast<T*>(getData(path));
  }

  /**
   * @brief Returns a static typed pointer.
   * @tparam T Specifies the expected DataObject type.
   * @param path Identifies the object.
   * @return Static typed pointer, or nullptr when no object resolves.
   * @pre A found object has dynamic type T or a type derived from T.
   */
  template <class T>
  T* getDataAsUnsafe(const LinkedPath& path)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return static_cast<T*>(getData(path));
  }

  const DataObject* getData(DataObject::IdType identifier) const;

  template <class T>
  inline const T* getDataAs(DataObject::IdType identifier) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return dynamic_cast<const T*>(getData(identifier));
  }

  /**
   * @brief Returns a static typed pointer.
   * @tparam T Specifies the expected DataObject type.
   * @param identifier Identifies the object.
   * @return Static typed pointer, or nullptr when no object resolves.
   * @pre A found object has dynamic type T or a type derived from T.
   */
  template <class T>
  const T* getDataAsUnsafe(DataObject::IdType identifier) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return static_cast<const T*>(getData(identifier));
  }

  const DataObject* getData(const std::optional<DataObject::IdType>& identifier) const;

  template <class T>
  inline const T* getDataAs(const std::optional<DataObject::IdType>& identifier) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return dynamic_cast<const T*>(getData(identifier));
  }

  /**
   * @brief Returns a static typed pointer.
   * @tparam T Specifies the expected DataObject type.
   * @param identifier Optionally identifies the object.
   * @return Static typed pointer, or nullptr when no object resolves.
   * @pre A found object has dynamic type T or a type derived from T.
   */
  template <class T>
  const T* getDataAsUnsafe(const std::optional<DataObject::IdType>& identifier) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return static_cast<const T*>(getData(identifier));
  }

  const DataObject* getData(const DataPath& path) const;

  /**
   * @brief Returns an object reference.
   * @param path Identifies the object.
   * @return Object reference.
   * @throws std::out_of_range if the object does not resolve.
   */
  const DataObject& getDataRef(const DataPath& path) const;

  /**
   * @brief Returns an object reference.
   * @param identifier Identifies the object.
   * @return Object reference.
   * @throws std::out_of_range if the object does not resolve.
   */
  const DataObject& getDataRef(DataObject::IdType identifier) const;

  template <class T>
  inline const T* getDataAs(const DataPath& path) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return dynamic_cast<const T*>(getData(path));
  }

  /**
   * @brief Returns a static typed pointer.
   * @tparam T Specifies the expected DataObject type.
   * @param path Identifies the object.
   * @return Static typed pointer, or nullptr when no object resolves.
   * @pre A found object has dynamic type T or a type derived from T.
   */
  template <class T>
  const T* getDataAsUnsafe(const DataPath& path) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return static_cast<const T*>(getData(path));
  }

  /**
   * @brief Returns a checked typed reference.
   * @tparam T Specifies the requested DataObject type.
   * @param path Identifies the object.
   * @return Typed object reference.
   * @throws std::out_of_range if the object does not resolve.
   * @throws std::bad_cast if the object is incompatible with T.
   */
  template <class T>
  inline const T& getDataRefAs(const DataPath& path) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return dynamic_cast<const T&>(getDataRef(path));
  }

  /**
   * @brief Returns a static typed reference.
   * @tparam T Specifies the expected DataObject type.
   * @param path Identifies the object.
   * @return Static typed reference.
   * @throws std::out_of_range if the object does not resolve.
   * @pre The found object has dynamic type T or a type derived from T.
   */
  template <class T>
  const T& getDataRefAsUnsafe(const DataPath& path) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return static_cast<const T&>(getDataRef(path));
  }

  /**
   * @brief Returns a checked typed reference.
   * @tparam T Specifies the requested DataObject type.
   * @param identifier Identifies the object.
   * @return Typed object reference.
   * @throws std::out_of_range if the object does not resolve.
   * @throws std::bad_cast if the object is incompatible with T.
   */
  template <class T>
  const T& getDataRefAs(DataObject::IdType identifier) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return dynamic_cast<const T&>(getDataRef(identifier));
  }

  /**
   * @brief Returns a static typed reference.
   * @tparam T Specifies the expected DataObject type.
   * @param identifier Identifies the object.
   * @return Static typed reference.
   * @throws std::out_of_range if the object does not resolve.
   * @pre The found object has dynamic type T or a type derived from T.
   */
  template <class T>
  const T& getDataRefAsUnsafe(DataObject::IdType identifier) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return static_cast<const T&>(getDataRef(identifier));
  }

  const DataObject* getData(const LinkedPath& path) const;

  template <class T>
  inline const T* getDataAs(const LinkedPath& path) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return dynamic_cast<const T*>(getData(path));
  }

  /**
   * @brief Returns a static typed pointer.
   * @tparam T Specifies the expected DataObject type.
   * @param path Identifies the object.
   * @return Static typed pointer, or nullptr when no object resolves.
   * @pre A found object has dynamic type T or a type derived from T.
   */
  template <class T>
  const T* getDataAsUnsafe(const LinkedPath& path) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return static_cast<const T*>(getData(path));
  }

  /**
   * @brief Acquires shared object ownership.
   * @param id Identifies the object.
   * @return Shared object pointer, or nullptr when absent.
   * @post Ownership can retain the object through a hierarchy change.
   */
  std::shared_ptr<DataObject> getSharedData(DataObject::IdType id);

  /**
   * @brief Acquires shared object ownership.
   * @param id Identifies the object.
   * @return Shared object pointer, or nullptr when absent.
   * @post Ownership can retain the object through a hierarchy change.
   */
  std::shared_ptr<const DataObject> getSharedData(DataObject::IdType id) const;

  template <class T>
  std::shared_ptr<T> getSharedDataAs(DataObject::IdType id)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return std::dynamic_pointer_cast<T>(getSharedData(id));
  }

  template <class T>
  std::shared_ptr<const T> getSharedDataAs(DataObject::IdType id) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return std::dynamic_pointer_cast<const T>(getSharedData(id));
  }

  /**
   * @brief Acquires shared object ownership.
   * @param path Identifies the object.
   * @return Shared object pointer, or nullptr when absent.
   * @post Ownership can retain the object through a hierarchy change.
   */
  std::shared_ptr<DataObject> getSharedData(const DataPath& path);

  /**
   * @brief Acquires shared object ownership.
   * @param path Identifies the object.
   * @return Shared object pointer, or nullptr when absent.
   * @post Ownership can retain the object through a hierarchy change.
   */
  std::shared_ptr<const DataObject> getSharedData(const DataPath& path) const;

  template <class T>
  std::shared_ptr<T> getSharedDataAs(const DataPath& path)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return std::dynamic_pointer_cast<T>(getSharedData(path));
  }

  template <class T>
  std::shared_ptr<const T> getSharedDataAs(const DataPath& path) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return std::dynamic_pointer_cast<const T>(getSharedData(path));
  }

  bool removeData(DataObject::IdType identifier);

  bool removeData(const std::optional<DataObject::IdType>& identifier);

  bool removeData(const DataPath& path);

  LinkedPath getLinkedPath(const DataPath& path) const;

  /**
   * @brief Creates missing DataGroup objects along a path.
   * @param path Data path to create.
   * @return A linked path, or an error after removal of groups created by this
   * call.
   * @pre path is not empty.
   */
  Result<LinkedPath> makePath(const DataPath& path);

  std::vector<DataPath> getDataPathsForId(DataObject::IdType identifier) const;

  std::vector<DataPath> getAllDataPaths() const;

  std::vector<DataObject::IdType> getAllDataObjectIds() const;

  std::vector<DataObject*> getTopLevelData() const;

  const DataMap& getDataMap() const;

  /**
   * @brief Inserts an object into the hierarchy.
   * @param dataObject Object to insert.
   * @param dataPath Is empty for the root or resolves to a BaseGroup.
   * @return True when insertion and registration succeed.
   * @post Clears existing parents and assigns a new identifier when needed.
   * @see setAdditionalParent To add a linked placement.
   */
  bool insert(const std::shared_ptr<DataObject>& dataObject, const DataPath& dataPath);

  DataObject::IdType getNextId() const;

  bool setAdditionalParent(DataObject::IdType targetId, DataObject::IdType newParent);

  /**
   * @brief Removes a parent group from an object.
   * @param targetId Identifies the object.
   * @param parent Identifies the parent group.
   * @return True when the parent removes the object.
   * @pre parent is zero or resolves to a BaseGroup.
   */
  bool removeParent(DataObject::IdType targetId, DataObject::IdType parent);

  Iterator begin();

  Iterator end();

  ConstIterator begin() const;

  ConstIterator end() const;

  SignalType& getSignal();

  /**
   * @brief Validates matching tuple counts.
   * @param dataPaths Array data paths to validate.
   * @return An error when resolved arrays have different tuple counts or an
   * unsupported type.
   * @pre Every supplied path resolves.
   */
  nonstd::expected<void, std::string> validateNumberOfTuples(const std::vector<DataPath>& dataPaths) const;

  void resetIds(DataObject::IdType startingId);

  void exportHierarchyAsGraphViz(std::ostream& outputStream) const;

  void exportHierarchyAsText(std::ostream& outputStream) const;

  /**
   * @brief Copies the hierarchy.
   * @param rhs Source data structure.
   * @return This data structure.
   * @post Copies objects with shallowCopy(), shares the resolver, and preserves
   * this object's observers.
   */
  DataStructure& operator=(const DataStructure& rhs);

  /**
   * @brief Moves the hierarchy.
   * @param rhs Source data structure.
   * @return This data structure.
   * @post Rebinds object back-pointers and preserves this object's observers.
   */
  DataStructure& operator=(DataStructure&& rhs) noexcept;

  /**
   * @brief Sets the next generated object identifier.
   * @param nextDataId Identifier to issue next.
   * @pre Use only during import to prevent identifier collisions.
   */
  void setNextId(DataObject::IdType nextDataId);

  DataMap& getRootGroup();

  /**
   * @brief Flushes live objects.
   * @post In-memory objects have no persistent target and do not write data.
   */
  void flush() const;

  /**
   * @brief Flushes live objects once each in identifier order and stops at the first failed object.
   * @return Warnings from the objects flushed so far, plus the failed object's errors, each with its name, identifier, and paths.
   *
   * @pre The caller excludes concurrent mutations.
   *
   * @note Objects after the first failure are not flushed by this call. Their unwritten data stays in memory
   * for a later flush; nothing is discarded. A failure without error diagnostics receives error -6070.
   */
  [[nodiscard]] Result<> flushChecked() const;

  uint64 memoryUsage() const;

  /**
   * @brief Converts eligible in-memory arrays through the storage resolver.
   * @return Warnings for arrays that cannot convert.
   *
   * The method uses the array-creation resolver. Core does not name a concrete
   * out-of-core format.
   */
  Result<> transferDataArraysOoc();

  Result<> validateGeometries() const;

  Result<> validateAttributeMatrices() const;

  /**
   * @class ImportPublicationCheckpoint
   * @brief Captures the destination identity and next ID before import preparation.
   *
   * The checkpoint owns no destination and reserves no IDs. Publication rejects
   * a foreign destination or a changed counter. A complete rollback permits reuse.
   * The destination must remain alive at the same address. Explicit counter
   * rewrites or destination replacement end the checkpoint lifetime.
   */
  class SIMPLNX_EXPORT ImportPublicationCheckpoint
  {
  public:
    /**
     * @brief Captures the current destination counter without changing it.
     * @param destination Remains alive through publication and any explicit cleanup.
     * @pre The caller excludes concurrent destination mutation.
     */
    explicit ImportPublicationCheckpoint(const DataStructure& destination) noexcept;
    ~ImportPublicationCheckpoint() noexcept = default;
    ImportPublicationCheckpoint(const ImportPublicationCheckpoint&) = default;
    ImportPublicationCheckpoint(ImportPublicationCheckpoint&&) noexcept = default;
    ImportPublicationCheckpoint& operator=(const ImportPublicationCheckpoint&) = default;
    ImportPublicationCheckpoint& operator=(ImportPublicationCheckpoint&&) noexcept = default;

    /**
     * @brief Checks the captured destination address without dereferencing it.
     * @param destination Supplies the live publication target.
     * @return True when the target has the captured address.
     */
    [[nodiscard]] bool belongsTo(const DataStructure& destination) const noexcept;
    [[nodiscard]] DataObject::IdType nextId() const noexcept;

  private:
    const DataStructure* m_Destination = nullptr;
    DataObject::IdType m_NextId = 0;
  };

  class ImportPublicationTestControl;

  /**
   * @brief Counts live weak-index entries without counting hierarchy aliases twice.
   * @return Number of nonexpired entries that resetIds renumbers.
   * @pre The caller excludes concurrent mutations and object destruction.
   */
  [[nodiscard]] DataObject::IdType getLiveObjectCount() const;

  /**
   * @brief Collects IDs occupied by the weak index or actual hierarchy placements.
   * @return Independent set containing expired index keys, hierarchy keys, and owner IDs.
   * @pre The caller excludes concurrent mutations and object destruction.
   */
  [[nodiscard]] std::set<DataObject::IdType> getOccupiedObjectIds() const;

  /**
   * @brief Creates a detached shell whose failed construction cannot notify through the source association.
   * @param source Supplies an object whose children remain strongly owned.
   *
   * @return Independent object shell sharing only its value store.
   * @pre The caller excludes concurrent access to the source structure.
   */
  [[nodiscard]] static std::shared_ptr<DataObject> makeImportPublicationCopy(DataObject& source);

  /**
   * @brief Publishes selected prepared shells and rolls back exact placements on failure.
   * @param prepared Owns completed import values; publication shares their stores.
   * @param paths Lists selected paths in parent-before-child order.
   * @param sourceFile Identifies the import in diagnostics; this method does not read it.
   * @param checkpoint Captures this destination before preparation starts.
   * @return Import diagnostics; -6217 rejects a foreign checkpoint and -6218 rejects a stale counter.
   * @throws std::bad_alloc After rollback when publication allocation fails.
   * @pre The caller serializes access to both structures and preserves the checkpoint lifetime.
   * @post Rejected checkpoints leave both structures and test fault state unchanged.
   * @post The returned Result contains no warnings; the caller retains warnings from import preparation.
   *
   * All publication owners and removal messages exist before destination mutation.
   * Structural cleanup precedes removal notifications. Callback-created objects
   * retain their advanced counter. Incomplete cleanup keeps reserved IDs and reports -6215.
   */
  [[nodiscard]] Result<> publishImportedObjects(DataStructure& prepared, const std::vector<DataPath>& paths, const std::filesystem::path& sourceFile, const ImportPublicationCheckpoint& checkpoint);

protected:
  /**
   * @brief Returns and advances the next object identifier.
   * @return Identifier unique within this data structure.
   *
   * Copied data structures begin with the same next identifier.
   */
  DataObject::IdType generateId();

  void trackDataObject(const std::shared_ptr<DataObject>& dataObject);

  bool insertIntoRoot(const std::shared_ptr<DataObject>& dataObject);

  bool insertIntoParent(const std::shared_ptr<DataObject>& dataObject, BaseGroup* parentGroup);

  void setData(DataObject::IdType identifier, std::shared_ptr<DataObject> dataObject);

private:
  /**
   * @enum ImportInsertionStage
   * @brief Identifies the last completed mutation in one import insertion.
   */
  enum class ImportInsertionStage : uint8
  {
    None,      ///< No hierarchy placement exists.
    Hierarchy, ///< The hierarchy owns the object; its parent list can still be empty.
    Parent,    ///< The expected parent list is complete.
    Registered ///< The weak index and destination association are complete.
  };

  /**
   * @enum ImportCleanupStatus
   * @brief Reports structural cleanup without allocating diagnostics.
   */
  enum class ImportCleanupStatus : uint8
  {
    Complete, ///< The exact object is detached and disarmed.
    Conflict, ///< Identity or placement differs from the publication record.
    Children, ///< A group still owns objects that cleanup must preserve.
    Refused   ///< A test fault retains the exact recorded placement.
  };

  /**
   * @struct ImportPublicationRecord
   * @brief Keeps exact owners and preallocated diagnostics alive through import rollback.
   */
  struct ImportPublicationRecord
  {
    std::shared_ptr<DataObject> Owner;
    std::shared_ptr<BaseGroup> Parent;
    DataObject::IdType Id = 0;
    DataObject::IdType ParentId = 0;
    DataPath Path;
    std::shared_ptr<AbstractDataStructureMessage> RemovalMessage;
    ImportInsertionStage Stage = ImportInsertionStage::None;
    ImportCleanupStatus Cleanup = ImportCleanupStatus::Complete;
    bool NotifyRemoval = false;
    bool NotificationAttempted = false;
    bool NotificationFailed = false;
  };

  /**
   * @brief Inserts one prechecked import object without ordinary add notifications.
   * @param record Supplies exact owners and receives the completed mutation stage.
   * @param failAfter
   * Selects a private transaction fault, or None for normal insertion.
   * @param rejectBeforeInsert Selects a returned-failure test boundary before mutation.
   * @return True when hierarchy placement and registration complete.
   */
  bool insertImportedObject(ImportPublicationRecord& record, ImportInsertionStage failAfter, bool rejectBeforeInsert);

  /**
   * @brief Detaches only the recorded import placement and matching weak entry.
   * @param record Keeps all owners alive and receives notification eligibility.
   * @return Structural outcome;
   * no notification or diagnostic allocation occurs here.
   */
  ImportCleanupStatus rollbackImportedObject(ImportPublicationRecord& record) noexcept;

  struct ImportPublicationTestState;

  /**
   * @brief Builds exact publication records without changing the destination.
   * @param prepared Owns completed import shells.
   * @param paths Lists selected paths in parent-before-child order.
   * @param sourceFile Supplies diagnostic context.
   * @param records Receives exact owners and preallocated removal messages.
   * @param reservedNextId Receives the checked high-water counter.
   * @return Failure when an owner, ID, or parent is invalid.
   */
  Result<> prepareImportPublicationRecords(DataStructure& prepared, const std::vector<DataPath>& paths, const std::filesystem::path& sourceFile, std::vector<ImportPublicationRecord>& records,
                                           DataObject::IdType& reservedNextId);

  /**
   * @brief Runs publication with optional action-local test instrumentation.
   * @param prepared Owns completed import shells.
   * @param paths Lists selected paths in parent-before-child order.
   * @param sourceFile Supplies diagnostic context.
   * @param checkpoint Supplies the destination identity and original counter.
   * @param testState Supplies private fault state, or null for ordinary publication.
   * @return Publication or cleanup diagnostics.
   */
  Result<> publishImportedObjectsImpl(DataStructure& prepared, const std::vector<DataPath>& paths, const std::filesystem::path& sourceFile, const ImportPublicationCheckpoint& checkpoint,
                                      ImportPublicationTestState* testState);

  /**
   * @brief Cleans exact placements before callbacks and conditionally restores the counter.
   * @param records Keeps exact shells and removal messages alive.
   * @param savedNextId Supplies the counter before reservation.
   * @param reservedNextId Supplies the transaction high-water counter.
   * @param testState Supplies private fault state, or null for ordinary publication.
   * @param allowRefusal Enables the armed deterministic cleanup refusal.
   * @return True when every transaction placement and index entry is removed.
   */
  bool rollbackImportPublication(std::vector<ImportPublicationRecord>& records, DataObject::IdType savedNextId, DataObject::IdType reservedNextId, ImportPublicationTestState* testState,
                                 bool allowRefusal);

  /**
   * @brief Appends retained-placement and notification failures to an invalid Result.
   * @param result Receives diagnostics after the primary publication error.
   * @param records Supplies completed cleanup and notification states.
   * @param sourceFile Supplies diagnostic context.
   * @pre result is invalid.
   */
  static void appendImportCleanupDiagnostics(Result<>& result, const std::vector<ImportPublicationRecord>& records, const std::filesystem::path& sourceFile);

  /**
   * @brief Transfers failed transaction observations into action-local test storage.
   * @param testState Receives observations, or null to leave records local.
   * @param records Transfers ownership only when test instrumentation is active.
   * @param savedNextId Supplies the original destination counter.
   * @param reservedNextId Supplies the transaction high-water counter.
   */
  void retainImportFailure(ImportPublicationTestState* testState, std::vector<ImportPublicationRecord>& records, DataObject::IdType savedNextId, DataObject::IdType reservedNextId);

  bool insertTopLevel(const std::shared_ptr<DataObject>& obj);

  bool removeTopLevel(DataObject* data);

  bool removeData(DataObject* data);

  void dataDeleted(DataObject::IdType identifier, const std::string& name);

  void applyAllDataStructure();

  void recurseHierarchyToGraphViz(std::ostream& outputStream, const std::vector<DataPath> paths, const std::string& parent) const;

  void recurseHierarchyToText(std::ostream& outputStream, const std::vector<DataPath> paths, std::string indent) const;

  void notify(const std::shared_ptr<AbstractDataStructureMessage>& msg);

  SignalType m_Signal;
  WeakCollectionType m_DataObjects;
  DataMap m_RootGroup;
  bool m_IsValid = false;
  DataObject::IdType m_NextId = 1;
  std::shared_ptr<const IDataStoreFormatResolver> m_FormatResolver; // Null selects the process default resolver.
};

#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
/**
 * @class ImportPublicationTestControl
 * @brief Owns action-local faults and exact failed-publication observations.
 *
 * The control never exposes transaction records or destination mutation fields.
 * A retained destination must remain alive until explicit cleanup completes.
 * Destruction releases observations and does not retry cleanup.
 */
class SIMPLNX_EXPORT DataStructure::ImportPublicationTestControl
{
public:
  /**
   * @enum Fault
   * @brief Selects one deterministic publication failure.
   */
  enum class Fault : uint8
  {
    None,                  ///< Runs without injection.
    BeforePublication,     ///< Fails after ID reservation and before insertion.
    AfterSecondInsert,     ///< Fails after two complete real insertions.
    AfterHierarchy,        ///< Fails after the second hierarchy placement, before its parent list.
    BeforeTracking,        ///< Fails after the second parent list, before its weak entry.
    AllocationAfterSecond, ///< Throws bad_alloc after two complete insertions.
    ReturnFalse            ///< Makes the second insertion fail before mutation.
  };

  /**
   * @brief Creates a control with no armed fault or retained transaction.
   */
  ImportPublicationTestControl();
  /**
   * @brief Releases retained observations without retrying cleanup.
   */
  ~ImportPublicationTestControl() noexcept;
  ImportPublicationTestControl(const ImportPublicationTestControl&) = delete;
  ImportPublicationTestControl(ImportPublicationTestControl&&) = delete;
  ImportPublicationTestControl& operator=(const ImportPublicationTestControl&) = delete;
  ImportPublicationTestControl& operator=(ImportPublicationTestControl&&) = delete;

  /**
   * @brief Replaces prior observations and arms one publication fault.
   * @param fault Selects the failure boundary.
   * @param refuseCleanup Retains the second exact placement during automatic cleanup.
   * @pre The caller excludes publication and explicit cleanup through this control.
   */
  void setFault(Fault fault, bool refuseCleanup);
  bool publicationFaultConsumed() const;
  bool cleanupFaultConsumed() const;
  std::shared_ptr<DataObject> retainedOwner() const;

  /**
   * @brief Runs the ordinary publication operation with this control's fault state.
   * @param destination Receives selected shells.
   * @param prepared Owns completed import values.
   * @param paths Lists selected paths in parent-before-child order.
   * @param sourceFile Supplies diagnostic context.
   * @param checkpoint Captures destination before preparation.
   * @return The same diagnostics as publishImportedObjects, including -6217 and -6218.
   * @pre The caller meets publishImportedObjects preconditions and excludes concurrent control access.
   * @post Checkpoint rejection does not consume faults or replace retained observations.
   */
  Result<> publish(DataStructure& destination, DataStructure& prepared, const std::vector<DataPath>& paths, const std::filesystem::path& sourceFile, const ImportPublicationCheckpoint& checkpoint);

  /**
   * @brief Retries structural cleanup of this control's retained transaction.
   * @param destination Supplies the same live destination used by the failed publication.
   * @return Error -6215 when the destination differs or an exact-object conflict remains.
   * @pre The caller excludes concurrent destination mutation and control access.
   */
  Result<> cleanup(DataStructure& destination);

private:
  std::unique_ptr<ImportPublicationTestState> m_State;
};
#endif
} // namespace nx::core
