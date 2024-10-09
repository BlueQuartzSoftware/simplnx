#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace nx::core
{
class DataStructure;
class DataObject;
class DataPath;

/**
 * @class DataMap
 * @brief The DataMap class is used to handle lookup and storage of DataObjects
 * using the objects' ID values or names. The DataMap class is primarily used
 * within the BaseGroup and DataStructure classes as a consistent.
 */
class SIMPLNX_EXPORT DataMap
{
public:
  using IdType = uint64;
  using MapType = std::map<IdType, std::shared_ptr<DataObject>>;
  using Iterator = typename MapType::iterator;
  using ConstIterator = typename MapType::const_iterator;

  /**
   * @brief Constructs an empty DataMap.
   */
  DataMap();

  /**
   * @brief Creates a copy of the specified DataMap.
   * @param other
   */
  DataMap(const DataMap& other);

  /**
   * @brief Constructs a new DataMap and moves the data from the target DataMap.
   * @param other
   */
  DataMap(DataMap&& other) noexcept;

  /**
   * @brief Destroys the DataMap, deleting the std::shared_ptrs that make up
   * the map. If the map had the last reference to a DataObject, that object
   * is destroyed.
   */
  ~DataMap();

  /**
   * @brief Creates and returns a deep copy of the DataMap.
   * @return
   */
  DataMap deepCopy(const DataPath& parentCopyPath) const;

  /**
   * @brief Returns the number of items in the DataMap.
   * @return usize
   */
  usize getSize() const;

  /**
   * @brief Returns if the DataMap is empty (true) or has elements (false)
   */
  bool empty() const;

  /**
   * @brief Attempts to insert the target DataObject into the map.
   * Returns true if it succeeded. Returns false otherwise.
   * @param obj
   * @return bool
   */
  bool insert(const std::shared_ptr<DataObject>& obj);

  /**
   * @brief Attempts to remove the target DataObject from the map.
   * Returns true if it succeeded. Returns false otherwise.
   * @param obj
   * @return bool
   */
  bool remove(DataObject* obj);

  /**
   * @brief Attempts to remove a DataObject with the target IdType from the map.
   * Returns true if it succeeded. Returns false otherwise.
   * @param identifier
   * @return bool
   */
  bool remove(IdType identifier);

  /**
   * @brief Attempts to remove a DataObject from the map using the target
   * iterator. Returns true if it succeeded. Returns false otherwise.
   * @param iter
   * @return bool
   */
  bool erase(const Iterator& iter);

  /**
   * @brief Removes all DataObjects from the map.
   */
  void clear();

  /**
   * @brief Returns a vector of the IdTypes contained in the map.
   * @return std::vector<IdType>
   */
  std::vector<IdType> getKeys() const;

  /**
   * @brief Returns a vector of all the IdTypes contained in the map and its
   * contained groups.
   * @return std::vector<IdType>
   */
  std::vector<IdType> getAllKeys() const;

  /**
   * @brief Returns a map of Id and DataObjects in the map and its contained
   * groups.
   * @return std::map<IdType, std::weak_ptr<DataObject>>
   */
  std::map<IdType, std::weak_ptr<DataObject>> getAllItems() const;

  /**
   * @brief Returns a vector of the contained DataObject names.
   * @return std::vector<std::string>
   */
  std::vector<std::string> getNames() const;

  /**
   * @brief Returns a pointer to the DataObject with the specified IdType.
   * Returns nullptr if no DataObject is found.
   * @param key
   * @return DataObject*
   */
  DataObject* operator[](IdType key);

  /**
   * @brief Returns a const pointer to the DataObject with the specified IdType.
   * Returns nullptr if no DataObject is found.
   * @param key
   * @return const DataObject*
   */
  const DataObject* operator[](IdType key) const;

  /**
   * @brief Returns a pointer to the DataObject with the specified name.
   * Returns nullptr if no DataObject is found.
   * @param name
   * @return DataObject*
   */
  DataObject* operator[](const std::string& name);

  /**
   * @brief Returns a const pointer to the DataObject with the specified name.
   * Returns nullptr if no DataObject is found.
   * @param name
   * @return const DataObject*
   */
  const DataObject* operator[](const std::string& name) const;

  /**
   * @brief Returns a reference to the DataObject with the specified name.
   * Throws if no DataObject is found.
   * @param name
   * @return DataObject&
   */
  DataObject& at(const std::string& name);

  /**
   * @brief Returns a const reference to the DataObject with the specified name.
   * Throws if no DataObject is found.
   * @param name
   * @return const DataObject&
   */
  const DataObject& at(const std::string& name) const;

  /**
   * @brief Checks if the DataMap contains the specified IdType.
   * @param identifier
   * @return bool
   */
  bool contains(IdType identifier) const;

  /**
   * @brief Checks if the DataMap contains the specified DataObject.
   * @param obj
   * @return bool
   */
  bool contains(const DataObject* obj) const;

  /**
   * @brief Checks if the DataMap contains a DataObject with the specified name.
   * @param name
   * @return bool
   */
  bool contains(const std::string& name) const;

  /**
   * @brief Searches the DataMap for the target IdType.
   * @param identifier
   * @return Iterator
   */
  Iterator find(IdType identifier);

  /**
   * @brief Searches the DataMap for the target IdType.
   *
   * Children are not expanded in this calculation.
   * @param identifier
   * @return ConstIterator
   */
  ConstIterator find(IdType identifier) const;

  /**
   * @brief Searches the DataMap for the target name.
   *
   * Children are not expanded in this calculation.
   * @param identifier
   * @return Iterator
   */
  Iterator find(const std::string& name);

  /**
   * @brief Searches the DataMap for the target name.
   *
   * Children are not expanded in this calculation.
   * @param identifier
   * @return ConstIterator
   */
  ConstIterator find(const std::string& name) const;

  /**
   * @brief Sets a new DataStructure for all items in the DataMap.
   * @param dataStr
   */
  void setDataStructure(DataStructure* dataStr);

  /**
   * @brief Returns an iterator to the beginning of the DataMap.
   *
   * Children are not expanded in this calculation.
   * @return Iterator
   */
  Iterator begin();

  /**
   * @brief Returns an iterator to the end of the DataMap.
   * @return Iterator
   */
  Iterator end();

  /**
   * @brief Returns a const iterator to the beginning of the DataMap.
   *
   * Children are not expanded in this calculation.
   * @return ConstIterator
   */
  ConstIterator begin() const;

  /**
   * @brief Returns a const iterator to the end of the DataMap.
   * @return ConstIterator
   */
  ConstIterator end() const;

  /**
   * @brief Copies values from the specified DataMap.
   * @param rhs
   * @return DataMap&
   */
  DataMap& operator=(const DataMap& rhs);

  /**
   * @brief Moves values from the specified DataMap.
   * @param rhs
   * @return DataMap&
   */
  DataMap& operator=(DataMap&& rhs) noexcept;

  /**
   * @brief Updates the map IDs using an unordered map of IDs and their new values.
   * @param updatedIdsMap
   */
  void updateIds(const std::unordered_map<IdType, IdType>& updatedIdsMap);

private:
  MapType m_Map;
};
} // namespace nx::core
