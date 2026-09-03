#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
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
 * @brief The DataStructure class is both the control center and origin of the
 * data structure. The DataStructure is where and how DataGroups, montages,
 * geometries, and scalars are added to the structure. The DataStructure allows
 * parents to be added to or removed from DataObjects.
 */
class SIMPLNX_EXPORT DataStructure : public BaseGroup
{
  using WeakCollectionType = std::map<DataObject::IdType, std::weak_ptr<DataObject>>;

protected:
  /**
   * @brief Finalizes adding a DataObject to the DataStructure. This should
   * be called by the create* methods to prevent duplicating code. Returns
   * true if the data was successfully added. Returns false otherwise.
   * @param obj Shared pointer to the DataObject to add
   * @param parent Optional ID of the parent DataObject
   * @return bool True if the data was successfully added, false otherwise
   */
  bool finishAddingObject(const std::shared_ptr<DataObject>& obj, const std::optional<DataObject::IdType>& parent = {});

public:
  static constexpr StringLiteral k_TypeName = "DataStructure";

  using SignalType = nod::signal<void(DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>&)>;
  using Iterator = DataMap::Iterator;
  using ConstIterator = DataMap::ConstIterator;

  friend class DataMap;
  friend class DataObject;

  /**
   * @brief Default constructor
   */
  DataStructure();

  /**
   * @brief Copy constructor
   * @param other The DataStructure to copy from
   */
  DataStructure(const DataStructure& other);

  /**
   * @brief Move constructor
   * @param other The DataStructure to move from
   */
  DataStructure(DataStructure&& other) noexcept;

  /**
   * @brief Destroys the DataStructure and all contained DataObjects.
   */
  ~DataStructure();

  /**
   * @brief Returns the number of unique DataObjects in the DataStructure.
   * @return usize
   */
  usize getSize() const;

  /**
   * @brief Clears the DataStructure by removing all DataObjects. The next
   * DataObject ID remains unchanged after the operation.
   */
  void clear() override;

  /**
   * @brief Returns the IdType for the DataObject found at the specified DataPath. The
   * return type is optional<IdType> for cases where the DataPath does not point to a
   * DataObject. If no DataObject is found at the path, an empty optional object is
   * returned.
   * @param path The DataPath to the DataObject
   * @return std::optional<IdType> The ID of the DataObject, or empty optional if not found
   */
  std::optional<DataObject::IdType> getId(const DataPath& path) const;

  /**
   * @brief Returns true if the DataStructure contains a DataObject with the
   * given key. Returns false otherwise.
   * @param identifier The ID of the DataObject to search for
   * @return bool True if the DataObject exists, false otherwise
   */
  bool containsData(DataObject::IdType identifier) const;

  /**
   * @brief Returns true if the DataStructure contains a DataObject with the
   * given path. Returns false otherwise.
   * @param path The DataPath to the DataObject
   * @return bool
   */
  bool containsData(const DataPath& path) const;

  /**
   * @brief Returns a pointer to the DataObject with the specified IdType.
   * If no such object exists, this method returns nullptr.
   * @param identifier The ID of the DataObject to retrieve
   * @return DataObject* Pointer to the DataObject, or nullptr if not found
   */
  DataObject* getData(DataObject::IdType identifier);

  /**
   * @brief Returns a pointer to the DataObject with the specified IdType.
   * If no such object exists, this method returns nullptr.
   * @param identifier The ID of the DataObject to retrieve
   * @return T* Pointer to the DataObject cast to type T, or nullptr if not found or cast fails
   */
  template <class T>
  inline T* getDataAs(DataObject::IdType identifier)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return dynamic_cast<T*>(getData(identifier));
  }

  /**
   * @brief Returns a pointer to the DataObject with the specified IdType.
   * If no such object exists, this method returns nullptr.
   * PERFORMS NO CHECKS ON THE TYPE OF DATAOBJECT RETURNED
   * ONLY USE WHEN THE TYPE IS ALREADY GUARENTEED TO BE T
   * @param identifier The ID of the DataObject to retrieve
   * @return T* Pointer to the DataObject statically cast to type T, or nullptr if not found
   */
  template <class T>
  T* getDataAsUnsafe(DataObject::IdType identifier)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return static_cast<T*>(getData(identifier));
  }

  /**
   * @brief Returns a pointer to the DataObject with the specified IdType.
   * If no such object exists, or no ID is provided, this method returns nullptr.
   * @param identifier Optional ID of the DataObject to retrieve
   * @return DataObject* Pointer to the DataObject, or nullptr if not found or ID not provided
   */
  DataObject* getData(const std::optional<DataObject::IdType>& identifier);

  /**
   * @brief Returns a pointer to the DataObject with the specified IdType.
   * If no such object exists, or no ID is provided, this method returns nullptr.
   * @param identifier Optional ID of the DataObject to retrieve
   * @return T* Pointer to the DataObject cast to type T, or nullptr if not found, ID not provided, or cast fails
   */
  template <class T>
  inline T* getDataAs(const std::optional<DataObject::IdType>& identifier)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    auto* object = getData(identifier);
    return dynamic_cast<T*>(object);
  }

  /**
   * @brief Returns a pointer to the DataObject with the specified IdType.
   * If no such object exists, or no ID is provided, this method returns nullptr.
   * PERFORMS NO CHECKS ON THE TYPE OF DATAOBJECT RETURNED
   * ONLY USE WHEN THE TYPE IS ALREADY GUARENTEED TO BE T
   * @param identifier Optional ID of the DataObject to retrieve
   * @return T* Pointer to the DataObject statically cast to type T, or nullptr if not found or ID not provided
   */
  template <class T>
  T* getDataAsUnsafe(const std::optional<DataObject::IdType>& identifier)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    auto* object = getData(identifier);
    return static_cast<T*>(object);
  }

  /**
   * @brief Returns a pointer to the DataObject at the given DataPath. If no
   * DataObject is found, this method returns nullptr.
   * @param path The DataPath to the DataObject
   * @return DataObject* Pointer to the DataObject, or nullptr if not found
   */
  DataObject* getData(const DataPath& path);

  /**
   * @brief Returns a reference to the DataObject at the given DataPath. If no
   * DataObject is found, this method throws std::out_of_range.
   * @param path The DataPath to the DataObject
   * @return DataObject& Reference to the DataObject
   */
  DataObject& getDataRef(const DataPath& path);

  /**
   * @brief Returns a reference to the DataObject with the given identifier. If no
   * DataObject is found, this method throws std::out_of_range.
   * @param identifier The ID of the DataObject to retrieve
   * @return DataObject& Reference to the DataObject
   */
  DataObject& getDataRef(DataObject::IdType identifier);

  /**
   * @brief Returns a pointer to the DataObject at the given DataPath. If no
   * DataObject is found, this method returns nullptr.
   * @param path The DataPath to the DataObject
   * @return T* Pointer to the DataObject cast to type T, or nullptr if not found or cast fails
   */
  template <class T>
  inline T* getDataAs(const DataPath& path)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return dynamic_cast<T*>(getData(path));
  }

  /**
   * @brief Returns a pointer to the DataObject at the given DataPath. If no
   * DataObject is found, this method returns nullptr.
   * PERFORMS NO CHECKS ON THE TYPE OF DATAOBJECT RETURNED
   * ONLY USE WHEN THE TYPE IS ALREADY GUARENTEED TO BE T
   * @param path The DataPath to the DataObject
   * @return T* Pointer to the DataObject statically cast to type T, or nullptr if not found
   */
  template <class T>
  T* getDataAsUnsafe(const DataPath& path)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return static_cast<T*>(getData(path));
  }

  /**
   * @brief Returns a reference to the DataObject at the given DataPath. If no
   * DataObject is found, this method throws std::out_of_range.
   * @param path The DataPath to the DataObject
   * @return T& Reference to the DataObject cast to type T
   */
  template <class T>
  inline T& getDataRefAs(const DataPath& path)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return dynamic_cast<T&>(getDataRef(path));
  }

  /**
   * @brief Returns a reference to the DataObject at the given DataPath. If no
   * DataObject is found, this method throws std::out_of_range.
   * PERFORMS NO CHECKS ON THE TYPE OF DATAOBJECT RETURNED
   * ONLY USE WHEN THE TYPE IS ALREADY GUARENTEED TO BE T
   * @param path The DataPath to the DataObject
   * @return T& Reference to the DataObject statically cast to type T
   */
  template <class T>
  T& getDataRefAsUnsafe(const DataPath& path)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return static_cast<T&>(getDataRef(path));
  }

  /**
   * @brief Returns a reference to the DataObject with the given identifier. If no
   * DataObject is found, this method throws std::out_of_range.
   * @param identifier The ID of the DataObject to retrieve
   * @return T& Reference to the DataObject cast to type T
   */
  template <class T>
  T& getDataRefAs(DataObject::IdType identifier)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return dynamic_cast<T&>(getDataRef(identifier));
  }

  /**
   * @brief Returns a reference to the DataObject with the given identifier. If no
   * DataObject is found, this method throws std::out_of_range.
   * PERFORMS NO CHECKS ON THE TYPE OF DATAOBJECT RETURNED
   * ONLY USE WHEN THE TYPE IS ALREADY GUARENTEED TO BE T
   * @param identifier The ID of the DataObject to retrieve
   * @return T& Reference to the DataObject statically cast to type T
   */
  template <class T>
  T& getDataRefAsUnsafe(DataObject::IdType identifier)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return static_cast<T&>(getDataRef(identifier));
  }

  /**
   * @brief Returns a pointer to the DataObject found at the specified
   * LinkedPath. If no such DataObject is found, this method returns nullptr.
   * @param path The LinkedPath to the DataObject
   * @return DataObject* Pointer to the DataObject, or nullptr if not found
   */
  DataObject* getData(const LinkedPath& path);

  /**
   * @brief Returns a pointer to the DataObject found at the specified
   * LinkedPath. If no such DataObject is found, this method returns nullptr.
   * @param path
   * @return T*
   */
  template <class T>
  inline T* getDataAs(const LinkedPath& path)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return dynamic_cast<T*>(getData(path));
  }

  /**
   * @brief Returns a pointer to the DataObject found at the specified
   * LinkedPath. If no such DataObject is found, this method returns nullptr.
   * PERFORMS NO CHECKS ON THE TYPE OF DATAOBJECT RETURNED
   * ONLY USE WHEN THE TYPE IS ALREADY GUARENTEED TO BE T
   * @param path
   * @return T*
   */
  template <class T>
  T* getDataAsUnsafe(const LinkedPath& path)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return static_cast<T*>(getData(path));
  }

  /**
   * @brief Returns a pointer to the DataObject with the specified IdType.
   * If no such object exists, this method returns nullptr.
   * @param identifier
   * @return const DataObject*
   */
  const DataObject* getData(DataObject::IdType identifier) const;

  /**
   * @brief Returns a pointer to the DataObject with the specified IdType.
   * If no such object exists, this method returns nullptr.
   * @param identifier
   * @return const T*
   */
  template <class T>
  inline const T* getDataAs(DataObject::IdType identifier) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return dynamic_cast<const T*>(getData(identifier));
  }

  /**
   * @brief Returns a pointer to the DataObject with the specified IdType.
   * If no such object exists, this method returns nullptr.
   * PERFORMS NO CHECKS ON THE TYPE OF DATAOBJECT RETURNED
   * ONLY USE WHEN THE TYPE IS ALREADY GUARENTEED TO BE T
   * @param identifier
   * @return const T*
   */
  template <class T>
  const T* getDataAsUnsafe(DataObject::IdType identifier) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return static_cast<const T*>(getData(identifier));
  }

  /**
   * @brief Returns a pointer to the DataObject with the specified IdType.
   * If no such object exists, or no ID is provided, this method returns nullptr.
   * @param identifier
   * @return const DataObject*
   */
  const DataObject* getData(const std::optional<DataObject::IdType>& identifier) const;

  /**
   * @brief Returns a pointer to the DataObject with the specified IdType.
   * If no such object exists, or no ID is provided, this method returns nullptr.
   * @param identifier
   * @return const T*
   */
  template <class T>
  inline const T* getDataAs(const std::optional<DataObject::IdType>& identifier) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return dynamic_cast<const T*>(getData(identifier));
  }

  /**
   * @brief Returns a pointer to the DataObject with the specified IdType.
   * If no such object exists, or no ID is provided, this method returns nullptr.
   * PERFORMS NO CHECKS ON THE TYPE OF DATAOBJECT RETURNED
   * ONLY USE WHEN THE TYPE IS ALREADY GUARENTEED TO BE T
   * @param identifier
   * @return const T*
   */
  template <class T>
  const T* getDataAsUnsafe(const std::optional<DataObject::IdType>& identifier) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return static_cast<const T*>(getData(identifier));
  }

  /**
   * @brief Returns a pointer to the DataObject at the given DataPath. If no
   * DataObject is found, this method returns nullptr.
   * @param path
   * @return const DataObject*
   */
  const DataObject* getData(const DataPath& path) const;

  /**
   * @brief Returns a reference to the DataObject at the given DataPath. If no
   * DataObject is found, this method throws std::out_of_range.
   * @param path
   * @return const DataObject&
   */
  const DataObject& getDataRef(const DataPath& path) const;

  /**
   * @brief Returns a reference to the DataObject with the given identifier. If no
   * DataObject is found, this method throws std::out_of_range.
   * @param identifier
   * @return const DataObject&
   */
  const DataObject& getDataRef(DataObject::IdType identifier) const;

  /**
   * @brief Returns a pointer to the DataObject at the given DataPath.
   *
   * @throws std::out_of_range if path does not exist
   * @throws std::bad_cast If the object at path cannnot be dynamic_cast<> to the input type
   *
   * @param path
   * @return const T*
   */
  template <class T>
  inline const T* getDataAs(const DataPath& path) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return dynamic_cast<const T*>(getData(path));
  }

  /**
   * @brief Returns a pointer to the DataObject at the given DataPath.
   * PERFORMS NO CHECKS ON THE TYPE OF DATAOBJECT RETURNED
   * ONLY USE WHEN THE TYPE IS ALREADY GUARENTEED TO BE T
   * @param path
   * @return const T*
   */
  template <class T>
  const T* getDataAsUnsafe(const DataPath& path) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return static_cast<const T*>(getData(path));
  }

  /**
   * @brief Returns a reference to the DataObject at the given DataPath.
   *
   * @throws std::out_of_range if path does not exist
   * @throws std::bad_cast If the object at path cannnot be dynamic_cast<> to the input type
   *
   * @param path
   * @return const T&
   */
  template <class T>
  inline const T& getDataRefAs(const DataPath& path) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return dynamic_cast<const T&>(getDataRef(path));
  }

  /**
   * @brief Returns a reference to the DataObject at the given DataPath.
   * PERFORMS NO CHECKS ON THE TYPE OF DATAOBJECT RETURNED
   * ONLY USE WHEN THE TYPE IS ALREADY GUARENTEED TO BE T
   * @throws std::out_of_range if path does not exist
   * @param path
   * @return const T&
   */
  template <class T>
  const T& getDataRefAsUnsafe(const DataPath& path) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return static_cast<const T&>(getDataRef(path));
  }

  /**
   * @brief Returns a reference to the DataObject with the given identifier. If no
   * DataObject is found, this method throws std::out_of_range.
   * @param identifier
   * @return T&
   */
  template <class T>
  const T& getDataRefAs(DataObject::IdType identifier) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return dynamic_cast<const T&>(getDataRef(identifier));
  }

  /**
   * @brief Returns a reference to the DataObject with the given identifier. If no
   * DataObject is found, this method throws std::out_of_range.
   * PERFORMS NO CHECKS ON THE TYPE OF DATAOBJECT RETURNED
   * ONLY USE WHEN THE TYPE IS ALREADY GUARENTEED TO BE T
   * @param identifier
   * @return T&
   */
  template <class T>
  const T& getDataRefAsUnsafe(DataObject::IdType identifier) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return static_cast<const T&>(getDataRef(identifier));
  }

  /**
   * @brief Returns a pointer to the DataObject found at the specified
   * LinkedPath. If no such DataObject is found, this method returns nullptr.
   * @param path
   * @return const DataObject*
   */
  const DataObject* getData(const LinkedPath& path) const;

  /**
   * @brief Returns a pointer to the DataObject found at the specified
   * LinkedPath. If no such DataObject is found, this method returns nullptr.
   * @param path
   * @return const T*
   */
  template <class T>
  inline const T* getDataAs(const LinkedPath& path) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return dynamic_cast<const T*>(getData(path));
  }

  /**
   * @brief Returns a pointer to the DataObject found at the specified
   * LinkedPath. If no such DataObject is found, this method returns nullptr.
   * PERFORMS NO CHECKS ON THE TYPE OF DATAOBJECT RETURNED
   * ONLY USE WHEN THE TYPE IS ALREADY GUARENTEED TO BE T
   * @param path
   * @return const T*
   */
  template <class T>
  const T* getDataAsUnsafe(const LinkedPath& path) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return static_cast<const T*>(getData(path));
  }

  /**
   * @brief Returns the shared pointer for the specified DataObject.
   * Returns nullptr if no DataObject is found.
   *
   * Use getData(DataObject::IdType) instead. This was only made public for
   * use in visualization where select data might need to be preserved beyond
   * the rest of the DataStructure.
   * @param identifier
   * @return std::shared_ptr<DataObject>
   */
  std::shared_ptr<DataObject> getSharedData(DataObject::IdType id);

  /**
   * @brief Returns the shared pointer for the specified DataObject.
   * Returns nullptr if no DataObject is found.
   *
   * Use getData(DataObject::IdType) instead. This was only made public for
   * use in visualization where select data might need to be preserved beyond
   * the rest of the DataStructure.
   * @param id
   * @return std::shared_ptr<DataObject>
   */
  std::shared_ptr<const DataObject> getSharedData(DataObject::IdType id) const;

  /**
   * @brief Returns the shared pointer for the specified DataObject.
   * Returns nullptr if no DataObject is found.
   *
   * Use getData(DataObject::IdType) instead. This was only made public for
   * use in visualization where select data might need to be preserved beyond
   * the rest of the DataStructure.
   * @param id
   * @return std::shared_ptr<DataObject>
   */
  template <class T>
  std::shared_ptr<T> getSharedDataAs(DataObject::IdType id)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return std::dynamic_pointer_cast<T>(getSharedData(id));
  }

  /**
   * @brief Returns the shared pointer for the specified DataObject.
   * Returns nullptr if no DataObject is found.
   *
   * Use getData(DataObject::IdType) instead. This was only made public for
   * use in visualization where select data might need to be preserved beyond
   * the rest of the DataStructure.
   * @param id
   * @return std::shared_ptr<DataObject>
   */
  template <class T>
  std::shared_ptr<const T> getSharedDataAs(DataObject::IdType id) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return std::dynamic_pointer_cast<const T>(getSharedData(id));
  }

  /**
   * @brief Returns the shared pointer for the DataObject at the target path.
   * Returns nullptr if no DataObject is found.
   *
   * Use getData(const DataPath&) instead. This was only made public for
   * use in importing a DataObject from another DataStructure when select data
   * needs to be preserved beyond the imported DataStructure.
   * @param path
   * @return std::shared_ptr<DataObject>
   */
  std::shared_ptr<DataObject> getSharedData(const DataPath& path);

  /**
   * @brief Returns the shared pointer for the DataObject at the target path.
   * Returns nullptr if no DataObject is found.
   *
   * Use getData(const DataPath&) instead. This was only made public for
   * use in importing a DataObject from another DataStructure when select data
   * needs to be preserved beyond the imported DataStructure.
   * @param path
   * @return std::shared_ptr<DataObject>
   */
  std::shared_ptr<const DataObject> getSharedData(const DataPath& path) const;

  /**
   * @brief Returns the shared pointer for the DataObject at the target path.
   * Returns nullptr if no DataObject is found.
   *
   * Use getData(const DataPath&) instead. This was only made public for
   * use in importing a DataObject from another DataStructure when select data
   * needs to be preserved beyond the imported DataStructure.
   * @param path
   * @return std::shared_ptr<DataObject>
   */
  template <class T>
  std::shared_ptr<T> getSharedDataAs(const DataPath& path)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return std::dynamic_pointer_cast<T>(getSharedData(path));
  }

  /**
   * @brief Returns the shared pointer for the DataObject at the target path.
   * Returns nullptr if no DataObject is found.
   *
   * Use getData(const DataPath&) instead. This was only made public for
   * use in importing a DataObject from another DataStructure when select data
   * needs to be preserved beyond the imported DataStructure.
   * @param path
   * @return std::shared_ptr<DataObject>
   */
  template <class T>
  std::shared_ptr<const T> getSharedDataAs(const DataPath& path) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return std::dynamic_pointer_cast<const T>(getSharedData(path));
  }

  /**
   * @brief Removes the DataObject using the specified IdType. Returns true
   * if an object was found. Otherwise, returns false.
   * @param identifier The ID of the DataObject to remove
   * @return bool True if the object was found and removed, false otherwise
   */
  bool removeData(DataObject::IdType identifier);

  /**
   * @brief Removes the DataObject using the specified IdType. Returns true
   * if an object was found. Otherwise, returns false. If no ID is provided,
   * this returns false.
   * @param identifier Optional ID of the DataObject to remove
   * @return bool True if the object was found and removed, false otherwise or if ID not provided
   */
  bool removeData(const std::optional<DataObject::IdType>& identifier);

  /**
   * @brief Removes the DataObject using the specified DataPath. Returns true
   * if an object was found. Otherwise, returns false.
   * @param path The DataPath to the DataObject to remove
   * @return bool True if the object was found and removed, false otherwise
   */
  bool removeData(const DataPath& path);

  /**
   * @brief Returns a LinkedPath based on the specified DataPath.
   * @param path The DataPath to convert to a LinkedPath
   * @return LinkedPath The corresponding LinkedPath
   */
  LinkedPath getLinkedPath(const DataPath& path) const;

  /**
   * @brief Creates the path in the data structure as a series of DataObjects. This method will
   * create all needed DataObjects until the path is completely created.
   * @param path The DataPath to create
   * @return Result<LinkedPath> Result containing the created LinkedPath, or error if creation failed
   */
  Result<LinkedPath> makePath(const DataPath& path);

  /**
   * @brief Returns a vector of DataPaths for the DataObject with the specified ID.
   * If no DataObject is found with the given ID, an empty vector is returned.
   * @param identifier The ID of the DataObject
   * @return std::vector<DataPath> Vector of all DataPaths pointing to the DataObject
   */
  std::vector<DataPath> getDataPathsForId(DataObject::IdType identifier) const;

  /**
   * @brief Returns a collection of all DataPaths within the structure.
   * @return std::vector<DataPath>
   */
  std::vector<DataPath> getAllDataPaths() const;

  /**
   * @brief Returns a collection of all DataObject ids within the structure.
   * @return std::vector<DataObject::IdType>
   */
  std::vector<DataObject::IdType> getAllDataObjectIds() const;

  /**
   * @brief Returns the top-level of the DataStructure.
   * @return std::vector<DataObject*>
   */
  std::vector<DataObject*> getTopLevelData() const;

  /**
   * @brief Inserts a new DataObject into the DataStructure nested under the given
   * DataPath. If the DataPath is empty, the DataObject is added directly to
   * the DataStructure. The provided DataObject can exist outside of the DataStructure,
   * but calling this method with a DataObject already contained within the DataStructure
   * is undefined behavior.
   *
   * This method is a purely for inserting DataObjects new to the DataStructure.
   *
   * This method is not meant to add additional parents to a DataObject already
   * existing in the DataStructure. Use addAdditionalParent for that purpose.
   *
   * This method is not meant to replace the DataObject at a given DataPath.
   * Using it as such is undefined behavior within the DataStructure.
   *
   * Returns true if the process succeeds. Returns false otherwise. Returns false if dataObject is null.
   * @param dataObject Shared pointer to the DataObject to insert
   * @param dataPath The DataPath where the DataObject should be inserted
   * @return bool True if insertion succeeded, false otherwise
   */
  bool insert(const std::shared_ptr<DataObject>& dataObject, const DataPath& dataPath);

  /**
   * @brief Returns the next ID value to use in the DataStructure
   * @return DataObject::IdType
   */
  DataObject::IdType getNextId() const;

  /**
   * @brief Adds an additional parent to the target DataObject.
   * @param targetId The ID of the DataObject to modify
   * @param newParent The ID of the new parent DataObject
   * @return bool True if the parent was added successfully, false otherwise
   */
  bool setAdditionalParent(DataObject::IdType targetId, DataObject::IdType newParent);

  /**
   * @brief Removes a parent from the target DataObject.
   * @param targetId The ID of the DataObject to modify
   * @param parent The ID of the parent DataObject to remove
   * @return bool True if the parent was removed successfully, false otherwise
   */
  bool removeParent(DataObject::IdType targetId, DataObject::IdType parent);

  /**
   * @brief Returns an iterator for the the beginning of the top-level DataMap.
   * @return Iterator An iterator to the first element in the top-level DataMap
   */
  Iterator begin();

  /**
   * @brief Returns an iterator for the the end of the top-level DataMap.
   * @return Iterator An iterator to the element following the last element
   */
  Iterator end();

  /**
   * @brief Returns an iterator for the the beginning of the top-level DataMap.
   * @return ConstIterator A const iterator to the first element in the top-level DataMap
   */
  ConstIterator begin() const;

  /**
   * @brief Returns an iterator for the the end of the top-level DataMap.
   * @return ConstIterator A const iterator to the element following the last element
   */
  ConstIterator end() const;

  /**
   * @brief Returns a reference the nod signal used to notify observers.
   * @return SignalType&
   */
  SignalType& getSignal();

  /**
   * @brief Checks if all IDataArrays at the target paths have the same tuple count.
   * Returns false if any of the paths are not derived from IDataArray.
   * @param dataPaths Vector of DataPaths to validate
   * @return nonstd::expected<void, std::string> Success if all have the same tuple count, or error message otherwise
   */
  nonstd::expected<void, std::string> validateNumberOfTuples(const std::vector<DataPath>& dataPaths) const;

  /**
   * @brief Resets DataObject IDs starting at the provided value.
   * Because 0 is a reserved value, if the starting value is set to 0, this method will use 1 instead.
   * @param startingId The starting ID value for reassigning DataObject IDs
   */
  void resetIds(DataObject::IdType startingId);

  /**
   * @brief Outputs data graph in .dot file format
   * @param outputStream The output stream to write GraphViz dot syntax to
   */
  void exportHierarchyAsGraphViz(std::ostream& outputStream) const;

  /**
   * @brief Outputs data graph in console readable format
   * @param outputStream The output stream to write the hierarchy text to
   */
  void exportHierarchyAsText(std::ostream& outputStream) const;

  /**
   * @brief Copy assignment operator. The copied DataStructure's observers are not retained.
   * @param rhs The DataStructure to copy from
   * @return DataStructure& Reference to this DataStructure
   */
  DataStructure& operator=(const DataStructure& rhs);

  /**
   * @brief Move assignment operator. The moved DataStructure's observers are retained.
   * @param rhs The DataStructure to move from
   * @return DataStructure& Reference to this DataStructure
   */
  DataStructure& operator=(DataStructure&& rhs) noexcept;

  /**
   * @brief Returns a deep copy of the DataStructure.
   * @return DataObject*
   */
  std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) override;

  /**
   * @brief Returns a shallow copy of the DataObject.
   * @return DataObject*
   */
  DataObject* shallowCopy() override;

  /**
   * @brief Returns typename of the DataObject as a std::string.
   * @return std::string
   */
  std::string getTypeName() const override;

  /**
   * @brief Sets the next ID to use when constructing a DataObject.
   * Because IDs are created to be unique, this should only be called when
   * importing data instead of on an existing DataStructure to avoid
   * overlapping values.
   * @param nextDataId The next ID value to use
   */
  void setNextId(DataObject::IdType nextDataId);

  /**
   * @brief Returns a reference to the root DataMap.
   * @return DataMap&
   */
  DataMap& getRootGroup();

  /**
   * @brief Flushes all DataObjects to their respective target by calling their respective flush() method.
   * In-memory DataObjects are not affected.
   */
  void flush() const;

  uint64 memoryUsage() const;

  /**
   * @brief Transfers array data to OOC if available.
   * @return Result with Warnings and errors
   */
  Result<> transferDataArraysOoc();

  /**
   * @brief This method will validate that each Geometry is valid based on a specific set of criteria
   *
   * The criteria used for validation is that for Node based geometries, for each of the "Shared-List" type of arrays,
   * the matching Attribute Matrix must also have the same number of tuples. For example if there is
   * an Edge Geometry that has 10 edges and the EdgeData AttributeMatrix does **NOT** have 10 tuples total,
   * then a Result<> with errors will be returned.
   *
   * @return Result<> object
   */
  Result<> validateGeometries() const;

  /**
   * @brief This method will validate that each AttributeMatrix is valid.
   *
   * The validation criteria is that for the Attribute Matrix itself: Every contained IArray
   * object must have the same number of Tuples as every other IArray object and the
   * AttributeMatrix itself must also have the same total number of tuples.
   *
   * @return Result<> object
   */
  Result<> validateAttributeMatrices() const;

protected:
  /**
   * @brief Returns a new ID for use constructing a DataObject.
   * IDs created are unique to the DataStructure, not the DataObject. Creating
   * a copy of the DataStructure will result in the same ID being used for the
   * next added DataObject to both structures.
   * @return DataObject::IdType
   */
  DataObject::IdType generateId();

  /**
   * @brief Adds the DataObject to the list of known DataObjects if it is missing.
   * @param dataObject Shared pointer to the DataObject to track
   */
  void trackDataObject(const std::shared_ptr<DataObject>& dataObject);

  /**
   * @brief Inserts the provided DataObject into the root DataMap.
   * @param dataObject Shared pointer to the DataObject to insert
   * @return bool True if insertion succeeded, false otherwise
   */
  bool insertIntoRoot(const std::shared_ptr<DataObject>& dataObject);

  /**
   * @brief Inserts the provided DataObject under the target parent group.
   * @param dataObject Shared pointer to the DataObject to insert
   * @param parentGroup Pointer to the parent BaseGroup
   * @return bool True if insertion succeeded, false otherwise
   */
  bool insertIntoParent(const std::shared_ptr<DataObject>& dataObject, BaseGroup* parentGroup);

  /**
   * @brief Applies a new pointer to the DataObject at a specified ID. Removes
   * the data from the DataMap if no DataObject was provided.
   * @param identifier The ID of the DataObject
   * @param dataObject Shared pointer to the DataObject to set
   */
  void setData(DataObject::IdType identifier, std::shared_ptr<DataObject> dataObject);

private:
  /**
   * @brief Inserts the target DataObject to the top of the DataStructure.
   * @param obj Shared pointer to the DataObject to insert
   * @return bool True if insertion succeeded, false otherwise
   */
  bool insertTopLevel(const std::shared_ptr<DataObject>& obj);

  /**
   * @brief Removes the specified DataObject from the top of the DataStructure.
   * @param data Pointer to the DataObject to remove
   * @return bool True if removal succeeded, false otherwise
   */
  bool removeTopLevel(DataObject* data);

  /**
   * @brief Removes the specified DataObject from the entire DataStructure.
   * @param data Pointer to the DataObject to remove
   * @return bool True if removal succeeded, false otherwise
   */
  bool removeData(DataObject* data);

  /**
   * @brief Called when a DataObject is deleted from the DataStructure. This
   * notifies observers to the change.
   * @param identifier The ID of the deleted DataObject
   * @param name The name of the deleted DataObject
   */
  void dataDeleted(DataObject::IdType identifier, const std::string& name);

  /**
   * @brief Resets the DataStructure for all known DataObjecs in the DataStructure.
   * This method exists for methods that copy or move another DataStructure.
   */
  void applyAllDataStructure();

  /**
   * @brief The recursive function to parse graph and dump names to and output stream in
   * dot file syntax
   * @param outputStream The output stream to write to
   * @param paths Vector of DataPaths to parse recursively
   * @param parent Name of the calling parent to output
   */
  void recurseHierarchyToGraphViz(std::ostream& outputStream, const std::vector<DataPath> paths, const std::string& parent) const;

  /**
   * @brief The recursive function to parse graph and dump names to and output stream in
   * readable syntax
   * @param outputStream The output stream to write to
   * @param paths Vector of DataPaths to parse recursively
   * @param indent The indentation string for the hierarchy
   */
  void recurseHierarchyToText(std::ostream& outputStream, const std::vector<DataPath> paths, std::string indent) const;

  /**
   * @brief Notifies observers to the provided message.
   * @param msg Shared pointer to the message to send to observers
   */
  void notify(const std::shared_ptr<AbstractDataStructureMessage>& msg);

  ////////////
  // Variables
  SignalType m_Signal;
  WeakCollectionType m_DataObjects;
  bool m_IsValid = false;
  DataObject::IdType m_NextId = 1;
};
} // namespace nx::core
