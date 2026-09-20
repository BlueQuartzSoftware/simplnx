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
   * @brief Sets the process-wide storage-format resolver.
   * @param resolver Has no effect when null.
   * @pre Install the resolver before concurrent DataStructure use.
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
} // namespace nx::core
