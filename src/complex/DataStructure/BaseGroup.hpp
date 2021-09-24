#pragma once

#include <memory>
#include <string>
#include <vector>

#include "complex/DataStructure/DataMap.hpp"
#include "complex/DataStructure/DataObject.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class BaseGroup
 * @brief The BaseGroup class is the base class for all DataObject containers
 * in the DataStructure. This is not an abstract class as all core functionality
 * is provided by this class, but for type-checking purposes, this cannot not be
 * the specified type when creating objects directly. Use DataGroup when a normal
 * group of DataObjects is desired.
 *
 * Child classes should override 'bool canInsert(const DataObject*) const'
 * to determine which DataObjects can be added to the group and which cannot.
 * By default, an object cannot be added if another object with the same name
 * already exists in the group. Additional rules can be added in derived
 * classes.
 */
class COMPLEX_EXPORT BaseGroup : public DataObject
{
public:
  using Iterator = typename DataMap::Iterator;
  using ConstIterator = typename DataMap::ConstIterator;

  /**
   * @brief Copy constructor creates a BaseGroup as a shallow copy of the
   * provided group.
   * @param other
   */
  BaseGroup(const BaseGroup& other);

  /**
   * @brief Move constructor creates a BaseGroup using the existing values of
   * the provided BaseGroup.
   * @param other
   */
  BaseGroup(BaseGroup&& other) noexcept;

  /**
   * @brief Destroys the BaseGroup and removes it from the list of it's
   * children's known parents. If a child no longer has any parents, the
   * DataObject is destroyed.
   */
  virtual ~BaseGroup();

  /**
   * @brief Returns the number of DataObjects in the group.
   * 
   * BaseGroups found among the container's children are not expanded during
   * the operation.
   * @return size_t
   */
  size_t getSize() const;

  /**
   * @brief Returns the underlying DataMap by value.
   * @return const DataMap&
   */
  const DataMap& getDataMap() const;

  /**
   * @brief Returns true if a child with the specified name exists in the
   * container. Returns false otherwise. This operation does not expand any
   * BaseGroups found among its children.
   * @param name
   * @return bool
   */
  bool contains(const std::string& name) const;

  /**
   * @brief Returns true if the specified DataObject is found among the
   * container's children. Returns false otherwise.
   *
   * BaseGroups found among the container's children are not expanded during
   * the operation.
   * @param obj
   * @return bool
   */
  bool contains(const DataObject* obj) const;

  /**
   * Returns a pointer to the DataObject child with the specified name. Returns
   * nullptr if no child exists with the specified name exists.
   *
   * BaseGroups found among the container's children are not expanded during
   * the operation.
   * @param name
   * @return DataObject*
   */
  DataObject* operator[](const std::string& name);

  /**
   * Returns a pointer to the DataObject child with the specified name. Returns
   * nullptr if no child exists with the specified name exists.
   *
   * BaseGroups found among the container's children are not expanded during
   * the operation.
   * @param name
   * @return DataObject*
   */
  const DataObject* operator[](const std::string& name) const;

  /**
   * @brief Returns an iterator to the child with the specified name. If no
   * such child is found, this function returns end().
   *
   * BaseGroups found among the container's children are not expanded during
   * the operation.
   * @param name
   * @return Iterator
   */
  Iterator find(const std::string& name);

  /**
   * @brief Returns a ConstIterator to the child with the specified name. If no
   * such child is found, this function returns end().
   *
   * BaseGroups found among the container's children are not expanded during
   * the operation.
   * @param name
   * @return ConstIterator
   */
  ConstIterator find(const std::string& name) const;

  /**
   * @brief Attempts to insert the specified DataObject into the container. If the
   * DataObject passes 'canInsert(obj: weak_ptr<DataObject>): bool', the DataObject
   * will be inserted into the container and the method returns true. Otherwise,
   * returns false.
   * @param obj
   * @return bool
   */
  bool insert(const std::weak_ptr<DataObject>& obj);

  /**
   * Attempts to remove the specified DataObject from the container. Returns
   * true if it succeeded. Returns false otherwise.
   *
   * BaseGroups found among the container's children are not expanded during
   * the operation.
   * @param obj
   * @return bool
   */
  bool remove(DataObject* obj);

  /**
   * Attempts to remove a DataObject with the specified name from the container.
   * Returns true if it succeeded. Returns false otherwise.
   *
   * BaseGroups found among the container's children are not expanded during
   * the operation.
   * @param name
   * @return bool
   */
  bool remove(const std::string& name);

  /**
   * @brief Returns an iterator to the beginning of the container.
   *
   * BaseGroups found among the container's children are not expanded during
   * the use of the returned iterator.
   * @return Iterator
   */
  Iterator begin();

  /**
   * @brief Returns an iterator to the end of the container.
   * @return Iterator
   */
  Iterator end();

  /**
   * @brief Returns a const iterator to the beginning of the container.
   *
   * BaseGroups found among the container's children are not expanded during
   * the use of the returned iterator.
   * @return ConstIterator
   */
  ConstIterator begin() const;

  /**
   * @brief Returns a const iterator to the end of the container.
   * @return ConstIterator
   */
  ConstIterator end() const;

protected:
  /**
   * @brief Creates a BaseGroup with the target DataStructure and name.
   * @param ds
   * @param name
   */
  BaseGroup(DataStructure& ds, const std::string& name);

  /**
   * @brief Checks if the provided DataObject can be added to the container.
   * This is a virtual method so that derived classes can modify what can or
   * cannot be added to the container. Returns true if the DataObject can be
   * added to the container. Otherwise, returns false.
   *
   * By default, a DataObject cannot be added to the BaseContainer if an object
   * with that name is already in the container. No BaseGroup children are
   * expanded during this operation.
   * @param obj
   * @return bool
   */
  virtual bool canInsert(const DataObject* obj) const;

  /**
   * @brief Sets a new DataStructure for the BaseGroup. Updates the DataMap
   * and its contained DataObjects as well.
   * @param ds
   */
  void setDataStructure(DataStructure* ds) override;

  /**
   * @brief Returns the underlying DataMap by reference.
   * @return DataMap&
   */
  DataMap& getDataMap();

  /**
   * @brief Reads the DataStructure group from a target HDF5 group.
   * @param targetId
   * @param parentId
   * @return H5::Error
   */
  virtual H5::ErrorType readHdf5(H5::IdType targetId, H5::IdType parentId);

  /**
   * @brief Writes the contained DataObjects to the target HDF5 group.
   * @param parentId
   * @param groupId
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5_impl(H5::IdType parentId, H5::IdType groupId) const override;

private:
  DataMap m_DataMap;
};
} // namespace complex
