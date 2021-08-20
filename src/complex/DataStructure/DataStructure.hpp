#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "complex/DataStructure/DataMap.hpp"
#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DynamicListArray.hpp"
#include "complex/DataStructure/Geometry/AbstractGeometry.hpp"
#include "complex/DataStructure/LinkedPath.hpp"
#include "complex/DataStructure/Montage/AbstractMontage.hpp"
#include "complex/DataStructure/ScalarData.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
class AbstractDataStructureMessage;
class AbstractDataStructureObserver;
class DataGroup;
class DataPath;

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
   * @brief Returns the shared pointer for the specified DataObject.
   * Returns nullptr if no DataObject is found.
   * @param id
   * @return std::shared_ptr<DataObject>
   */
  std::shared_ptr<DataObject> getSharedData(DataObject::IdType id) const;

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
  virtual ~DataStructure();

  /**
   * @brief Returns the number of unique DataObjects in the DataStructure.
   * @return size_t
   */
  size_t size() const;

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
   * @brief Returns a pointer to the DataObject with the specified IdType.
   * If no such object exists, this method returns nullptr.
   * @param id
   * @return DataObject*
   */
  DataObject* getData(DataObject::IdType id);

  /**
   * @brief Returns a pointer to the DataObject with the specified IdType.
   * If no such object exists, or no ID is provided, this method returns nullptr.
   * @param id
   * @return DataObject*
   */
  DataObject* getData(const std::optional<DataObject::IdType>& id);

  /**
   * @brief Returns a pointer to the DataObject at the given DataPath. If no
   * DataObject is found, this method returns nullptr.
   * @param path
   * @return DataObject*
   */
  DataObject* getData(const DataPath& path);

  /**
   * @brief Returns a pointer to the DataObject found at the specified
   * LinkedPath. If no such DataObject is found, this method returns nullptr.
   * @param path
   * @return DataObject*
   */
  DataObject* getData(const LinkedPath& path);

  /**
   * @brief Returns a pointer to the DataObject with the specified IdType.
   * If no such object exists, this method returns nullptr.
   * @param id
   * @return DataObject*
   */
  const DataObject* getData(DataObject::IdType id) const;

  /**
   * @brief Returns a pointer to the DataObject with the specified IdType.
   * If no such object exists, or no ID is provided, this method returns nullptr.
   * @param id
   * @return DataObject*
   */
  const DataObject* getData(const std::optional<DataObject::IdType>& id) const;

  /**
   * @brief Returns a pointer to the DataObject at the given DataPath. If no
   * DataObject is found, this method returns nullptr.
   * @param path
   * @return DataObject*
   */
  const DataObject* getData(const DataPath& path) const;

  /**
   * @brief Returns a pointer to the DataObject found at the specified
   * LinkedPath. If no such DataObject is found, this method returns nullptr.
   * @param path
   * @return DataObject*
   */
  const DataObject* getData(const LinkedPath& path) const;

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
   * @brief Returns the top-level of the DataStructure.
   * @return std::vector<DataObject*>
   */
  std::vector<DataObject*> getTopLevelData() const;

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
   * @brief Returns a collection of the DataStructure observers.
   * @return std::set<AbstractDataStructureObserver*>
   */
  std::set<AbstractDataStructureObserver*> getObservers() const;

  /**
   * @brief Adds an observer for broadcasting messages.
   * @param obs
   */
  void addObserver(AbstractDataStructureObserver* obs);

  /**
   * @brief Removes an observer.
   * @param obs
   */
  void removeObserver(AbstractDataStructureObserver* obs);

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
   * @brief Writes the DataStructure to the target HDF5 file or group.
   * @param fileId parent ID
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5(H5::IdType fileId) const;

  /**
   * @brief Creates a DataStructure by reading the specified HDF5 file ID.
   * @param fileId
   * @return H5::ErrorType
   */
  static DataStructure ReadFromHdf5(H5::IdType fileId, H5::ErrorType& err);

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
   * @return
   */
  bool removeData(DataObject* data);

  /**
   * @brief Called when a DataObject is deleted from the DataStructure. This notifies observers to the change.
   * @param id
   * @param name
   */
  void dataDeleted(DataObject::IdType id, const std::string& name);

  /**
   * @brief Notifies observers to the provided message.
   * @param msg
   */
  void notify(const std::shared_ptr<AbstractDataStructureMessage>& msg);

  ////////////
  // Variables
  std::map<DataObject::IdType, std::weak_ptr<DataObject>> m_DataObjects;
  DataMap m_RootGroup;
  std::set<AbstractDataStructureObserver*> m_Observers;
  bool m_IsValid = false;
};
} // namespace complex
