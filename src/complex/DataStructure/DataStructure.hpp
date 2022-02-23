#pragma once

#include "complex/Common/Result.hpp"
#include "complex/DataStructure/DataMap.hpp"
#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/LinkedPath.hpp"
#include "complex/complex_export.hpp"

#include "nod/nod.hpp"

#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

namespace complex
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

namespace H5
{
class DataStructureReader;
class FileReader;
class FileWriter;
} // namespace H5

/**
 * @class DataStructure
 * @brief The DataStructure class is both the control center and origin of the
 * data structure. The DataStructure is where and how DataGroups, montages,
 * geometries, and scalars are added to the structure. The DataStructure allows
 * parents to be added to or removed from DataObjects.
 */
class COMPLEX_EXPORT DataStructure
{
protected:
  /**
   * @brief Finalizes adding a DataObject to the DataStructure. This should
   * be called by the create* methods to prevent duplicating code. Returns
   * true if the data was successfully added. Returns false otherwise.
   * @param obj
   * @param parent = {}
   * @return bool
   */
  bool finishAddingObject(const std::shared_ptr<DataObject>& obj, const std::optional<DataObject::IdType>& parent = {});

public:
  using SignalType = nod::signal<void(DataStructure*, const std::shared_ptr<AbstractDataStructureMessage>&)>;
  using Iterator = DataMap::Iterator;
  using ConstIterator = DataMap::ConstIterator;

  friend class DataMap;
  friend class DataObject;
  friend class H5::DataStructureReader;

  /**
   * @brief Default constructor
   */
  DataStructure();

  /**
   * @brief Copy constructor
   * @param other
   */
  DataStructure(const DataStructure& other);

  /**
   * @brief Move constructor
   * @param other
   */
  DataStructure(DataStructure&& other) noexcept;

  /**
   * @brief
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
  void clear();

  /**
   * @brief Returns the IdType for the DataObject found at the specified DataPath. The
   * return type is optional<IdType> for cases where the DataPath does not point to a
   * DataObject. If no DataObject is found at the path, an empty optional object is
   * returned.
   * @param path
   * @return std::optional<IdType>
   */
  std::optional<DataObject::IdType> getId(const DataPath& path) const;

  /**
   * @brief Returns true if the DataStructure contains a DataObject with the
   * given key. Returns false otherwise.
   * @param id
   * @return bool
   */
  bool containsData(DataObject::IdType id) const;

  /**
   * @brief Returns a pointer to the DataObject with the specified IdType.
   * If no such object exists, this method returns nullptr.
   * @param id
   * @return DataObject*
   */
  DataObject* getData(DataObject::IdType id);

  /**
   * @brief Returns a pointer to the DataObject with the specified IdType.
   * If no such object exists, this method returns nullptr.
   * @param id
   * @return T*
   */
  template <class T>
  inline T* getDataAs(DataObject::IdType id)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return dynamic_cast<T*>(getData(id));
  }

  /**
   * @brief Returns a pointer to the DataObject with the specified IdType.
   * If no such object exists, or no ID is provided, this method returns nullptr.
   * @param id
   * @return DataObject*
   */
  DataObject* getData(const std::optional<DataObject::IdType>& id);

  /**
   * @brief Returns a pointer to the DataObject with the specified IdType.
   * If no such object exists, or no ID is provided, this method returns nullptr.
   * @param id
   * @return T*
   */
  template <class T>
  inline T* getDataAs(const std::optional<DataObject::IdType>& id)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    auto* object = getData(id);
    return dynamic_cast<T*>(object);
  }

  /**
   * @brief Returns a pointer to the DataObject at the given DataPath. If no
   * DataObject is found, this method returns nullptr.
   * @param path
   * @return DataObject*
   */
  DataObject* getData(const DataPath& path);

  /**
   * @brief Returns a reference to the DataObject at the given DataPath. If no
   * DataObject is found, this method throws std::out_of_range.
   * @param path
   * @return DataObject&
   */
  DataObject& getDataRef(const DataPath& path);

  /**
   * @brief Returns a pointer to the DataObject at the given DataPath. If no
   * DataObject is found, this method returns nullptr.
   * @param path
   * @return T*
   */
  template <class T>
  inline T* getDataAs(const DataPath& path)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return dynamic_cast<T*>(getData(path));
  }

  /**
   * @brief Returns a reference to the DataObject at the given DataPath. If no
   * DataObject is found, this method throws std::out_of_range.
   * @param path
   * @return T&
   */
  template <class T>
  inline T& getDataRefAs(const DataPath& path)
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return dynamic_cast<T&>(getDataRef(path));
  }

  /**
   * @brief Returns a pointer to the DataObject found at the specified
   * LinkedPath. If no such DataObject is found, this method returns nullptr.
   * @param path
   * @return DataObject*
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
   * @brief Returns a pointer to the DataObject with the specified IdType.
   * If no such object exists, this method returns nullptr.
   * @param id
   * @return const DataObject*
   */
  const DataObject* getData(DataObject::IdType id) const;

  /**
   * @brief Returns a pointer to the DataObject with the specified IdType.
   * If no such object exists, this method returns nullptr.
   * @param id
   * @return const T*
   */
  template <class T>
  inline const T* getDataAs(DataObject::IdType id) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return dynamic_cast<const T*>(getData(id));
  }

  /**
   * @brief Returns a pointer to the DataObject with the specified IdType.
   * If no such object exists, or no ID is provided, this method returns nullptr.
   * @param id
   * @return const DataObject*
   */
  const DataObject* getData(const std::optional<DataObject::IdType>& id) const;

  /**
   * @brief Returns a pointer to the DataObject with the specified IdType.
   * If no such object exists, or no ID is provided, this method returns nullptr.
   * @param id
   * @return const T*
   */
  template <class T>
  inline const T* getDataAs(const std::optional<DataObject::IdType>& id) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return dynamic_cast<const T*>(getData(id));
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
   * @brief Returns the shared pointer for the specified DataObject.
   * Returns nullptr if no DataObject is found.
   *
   * Use getData(DataObject::IdType) instead. This was only made public for
   * use in visualization where select data might need to be preserved beyond
   * the rest of the DataStructure.
   * @param id
   * @return std::shared_ptr<DataObject>
   */
  std::shared_ptr<DataObject> getSharedData(DataObject::IdType id) const;

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
  inline std::shared_ptr<T> getSharedDataAs(DataObject::IdType id) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return std::dynamic_pointer_cast<T>(getSharedData(id));
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
  std::shared_ptr<DataObject> getSharedData(const DataPath& path) const;

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
  inline std::shared_ptr<T> getSharedDataAs(const DataPath& path) const
  {
    static_assert(std::is_base_of_v<DataObject, T>);
    return std::dynamic_pointer_cast<T>(getSharedData(path));
  }

  /**
   * @brief Removes the DataObject using the specified IdType. Returns true
   * if an object was found. Otherwise, returns false.
   * @param id
   * @return bool
   */
  bool removeData(DataObject::IdType id);

  /**
   * @brief Removes the DataObject using the specified IdType. Returns true
   * if an object was found. Otherwise, returns false. If no ID is provided,
   * this returns false.
   * @param id
   * @return bool
   */
  bool removeData(const std::optional<DataObject::IdType>& id);

  /**
   * @brief Removes the DataObject using the specified DataPath. Returns true
   * if an object
   * was found. Otherwise, returns false.
   * @param path
   * @return bool
   */
  bool removeData(const DataPath& path);

  /**
   * @brief Returns a LinkedPath based on the specified DataPath.
   * @param path
   * @return LinkedPath
   */
  LinkedPath getLinkedPath(const DataPath& path) const;

  /**
   * @brief Creates the path in the data structure as a series of DataObjects. This method will
   * create all needed DataObjects until the path is completely created.
   * @param path The path to create.
   * @return Result<LinkedPath> object.
   */
  Result<LinkedPath> makePath(const DataPath& path);

  /**
   * @brief Returns a collection of all DataPaths within the structure.
   * @return std::vector<DataPath>
   */
  std::vector<DataPath> getAllDataPaths() const;

  /**
   * @brief Returns the top-level of the DataStructure.
   * @return std::vector<DataObject*>
   */
  std::vector<DataObject*> getTopLevelData() const;

  /**
   * @brief Returns a reference to the DataMap backing the top level of the DataStructure.
   * @return const DataMap&
   */
  const DataMap& getDataMap() const;

  /**
   * @brief Inserts a DataObject into the DataStructure nested under the given
   * DataPath. If the DataPath is empty, the DataObject is added directly to
   * the DataStructure.
   *
   * Returns true if the process succeeds. Returns false otherwise.
   * @param dataObject
   * @param dataPath
   * @return bool
   */
  bool insert(const std::shared_ptr<DataObject>& dataObject, const DataPath& dataPath);

  /**
   * @brief Adds an additional parent to the target DataObject.
   * @param targetId
   * @param newParent
   * @return bool
   */
  bool setAdditionalParent(DataObject::IdType targetId, DataObject::IdType newParent);

  /**
   * @brief Removes a parent from the target DataObject.
   * @param targetId
   * @param parent
   * @return bool
   */
  bool removeParent(DataObject::IdType targetId, DataObject::IdType parent);

  /**
   * @brief Returns an iterator for the the beginning of the top-level DataMap.
   * @return iterator
   */
  Iterator begin();

  /**
   * @brief Returns an iterator for the the end of the top-level DataMap.
   * @return iterator
   */
  Iterator end();

  /**
   * @brief Returns an iterator for the the beginning of the top-level DataMap.
   * @return
   */
  ConstIterator begin() const;

  /**
   * @brief Returns an iterator for the the end of the top-level DataMap.
   * @return
   */
  ConstIterator end() const;

  /**
   * @brief Returns a reference the nod signal used to notify observers.
   * @return SignalType&
   */
  SignalType& getSignal();

  /**
   * @brief Writes the DataStructure to the target HDF5 file or group.
   * @param parentGroupWriter HDF5 group writer
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5(H5::GroupWriter& parentGroupWriter) const;

  /**
   * @brief Creates a DataStructure by reading the specified H5::GroupReader or
   * H5::FileReader. Passes any potential errors back to the caller by reference.
   * @param groupReader
   * @return H5::ErrorType
   */
  static DataStructure readFromHdf5(const H5::GroupReader& groupReader, H5::ErrorType& err);

  /**
   * @brief Checks if all IDataArrays at the target paths have the same tuple count.
   * Returns false if any of the paths are not derived from IDataArray.
   * @param dataPaths
   * @return bool
   */
  bool validateNumberOfTuples(const std::vector<DataPath>& dataPaths) const;

  /**
   * @brief Copy assignment operator. The copied DataStructure's observers are not retained.
   * @param rhs
   * @return DataStructure&
   */
  DataStructure& operator=(const DataStructure& rhs);

  /**
   * @brief Move assignment operator. The moved DataStructure's observers are retained.
   * @param rhs
   * @return DataStructure&
   */
  DataStructure& operator=(DataStructure&& rhs) noexcept;

protected:
  /**
   * @brief Returns a reference to the root DataMap.
   * @return DataMap&
   */
  DataMap& getRootGroup();

  /**
   * @brief Returns a new ID for use constructing a DataObject.
   * IDs created are unique to the DataStructure, not the DataObject. Creating
   * a copy of the DataStructure will result in the same ID being used for the
   * next added DataObject to both structures.
   * @return DataObject::IdType
   */
  DataObject::IdType generateId();

  /**
   * @brief Sets the next ID to use when constructing a DataObject.
   * Because IDs are created to be unique, this should only be called when
   * importing data instead of on an existing DataStructure to avoid
   * overlapping values.
   * @param nextDataId
   */
  void setNextId(DataObject::IdType nextDataId);

  /**
   * @brief Adds the DataObject to the list of known DataObjects if it is missing.
   * @param dataObject
   */
  void trackDataObject(const std::shared_ptr<DataObject>& dataObject);

  /**
   * @brief Inserts the provided DataObject into the root DataMap.
   * @param dataObject
   * @return bool
   */
  bool insertIntoRoot(const std::shared_ptr<DataObject>& dataObject);

  /**
   * @brief Inserts the provided DataObject under the target parent group.
   * @param dataObject
   * @param parentGroup
   * @return bool
   */
  bool insertIntoParent(const std::shared_ptr<DataObject>& dataObject, BaseGroup* parentGroup);

  /**
   * @brief Applies a new pointer to the DataObject at a specified ID. Removes
   * the data from the DataMap if no DataObject was provided.
   * @param id
   * @param dataObject
   */
  void setData(DataObject::IdType id, std::shared_ptr<DataObject> dataObject);

private:
  /**
   * @brief Inserts the target DataObject to the top of the DataStructure.
   * @param obj
   * @return
   */
  bool insertTopLevel(const std::shared_ptr<DataObject>& obj);

  /**
   * @brief Removes the specified DataObject from the top of the DataStructure.
   * @param data
   * @return
   */
  bool removeTopLevel(DataObject* data);

  /**
   * @brief Removes the specified DataObject from the entire DataStructure.
   * @param data
   * @return bool
   */
  bool removeData(DataObject* data);

  /**
   * @brief Called when a DataObject is deleted from the DataStructure. This
   * notifies observers to the change.
   * @param id
   * @param name
   */
  void dataDeleted(DataObject::IdType id, const std::string& name);

  /**
   * @brief Resets the DataStructure for all known DataObjecs in the DataStructure.
   * This method exists for methods that copy or move another DataStructure.
   */
  void applyAllDataStructure();

  /**
   * @brief Notifies observers to the provided message.
   * @param msg
   */
  void notify(const std::shared_ptr<AbstractDataStructureMessage>& msg);

  ////////////
  // Variables
  SignalType m_Signal;
  std::map<DataObject::IdType, std::weak_ptr<DataObject>> m_DataObjects;
  DataMap m_RootGroup;
  bool m_IsValid = false;
  DataObject::IdType m_NextId = 1;
};
} // namespace complex
