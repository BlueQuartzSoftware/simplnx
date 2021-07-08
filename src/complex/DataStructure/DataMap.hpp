#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "complex/complex_export.hpp"

namespace complex
{
class DataStructure;
class DataObject;

/**
 * @class DataMap
 * @brief The DataMap class is used to handle lookup and storage of DataObjects
 * using the objects' ID values or names. The DataMap class is primarily used
 * within the BaseGroup and DataStructure classes as a consistent.
 */
class COMPLEX_EXPORT DataMap
{
public:
  using IdType = size_t;
  using MapType = std::map<IdType, std::shared_ptr<DataObject>>;
  using Iterator = MapType::iterator;
  using ConstIterator = MapType::const_iterator;

  /**
   * @brief Default constructor
   */
  DataMap();

  /**
   * @brief Copy constructor
   * @param other
   */
  DataMap(const DataMap& other);

  /**
   * @brief Move constructor
   * @param other
   */
  DataMap(DataMap&& other) noexcept;

  ~DataMap();

  /**
   * @brief Creates and returns a deep copy of the DataMap.
   * @return
   */
  DataMap deepCopy() const;

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
   * @param id
   * @return bool
   */
  bool remove(IdType id);

  /**
   * @brief Attempts to remove a DataObject from the map using the target
   * iterator. Returns true if it succeeded. Returns false otherwise.
   * @param iter
   * @return bool
   */
  bool erase(const Iterator& iter);

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
   * @brief Returns the DataObject with the specified IdType.
   * @param key
   * @return DataObject*
   */
  DataObject* operator[](IdType key);

  /**
   * @brief Returns the DataObject with the specified IdType.
   * @param key
   * @return const DataObject*
   */
  const DataObject* operator[](IdType key) const;

  /**
   * @brief Returns the DataObject with the specified name.
   * @param name
   * @return DataObject*
   */
  DataObject* operator[](const std::string& name);

  /**
   * @brief Returns the DataObject with the specified name.
   * @param name
   * @return const DataObject*
   */
  const DataObject* operator[](const std::string& name) const;

  /**
   * @brief Checks if the DataMap contains the specified IdType.
   * @param id
   * @return bool
   */
  bool contains(IdType id) const;

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
   * @param id
   * @return Iterator
   */
  Iterator find(IdType id);

  /**
   * @brief Searches the DataMap for the target IdType.
   * @param id
   * @return ConstIterator
   */
  ConstIterator find(IdType id) const;

  /**
   * @brief Searches the DataMap for the target name.
   * @param id
   * @return Iterator
   */
  Iterator find(const std::string& name);

  /**
   * @brief Searches the DataMap for the target name.
   * @param id
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
   * @return Iterator
   */
  Iterator begin();

  /**
   * @brief Returns an iterator to the end of the DataMap.
   * @return Iterator
   */
  Iterator end();

  /**
   * @brief Returns an iterator to the beginning of the DataMap.
   * @return ConstIterator
   */
  ConstIterator begin() const;

  /**
   * @brief Returns an iterator to the end of the DataMap.
   * @return ConstIterator
   */
  ConstIterator end() const;

private:
  MapType m_Map;
};
} // namespace complex
