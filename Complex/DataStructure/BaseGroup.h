#pragma once

#include <memory>
#include <string>
#include <vector>

#include "SIMPL/DataStructure/DataMap.h"
#include "SIMPL/DataStructure/DataObject.h"

namespace SIMPL
{
class BaseGroup : public DataObject
{
public:
  using Iterator = DataMap::Iterator;
  using ConstIterator = DataMap::ConstIterator;

  BaseGroup(const BaseGroup& other);
  BaseGroup(BaseGroup&& other) noexcept;

  /**
   * Empty Destructor
   */
  virtual ~BaseGroup();

  /**
   * @brief Returns the underlying DataMap by value.
   * @return DataMap
   */
  DataMap getDataMap() const;

  /**
   * @brief Returns true if a child with the specified name exists in the container. Returns
   * false otherwise. This operation does not expand any BaseGroups found among
   * its children.
   * @param name
   * @return bool
   */
  bool contains(const std::string& name) const;

  /**
   * @brief Returns true if the specified DataObject is found among the container's
   * children. Returns false otherwise. BaseGroups found among the container's
   * children are not expanded during the operation.
   * @param obj
   * @return bool
   */
  bool contains(const DataObject* obj) const;

  /**
   * Returns a pointer to the DataObject child with the specified name. Returns
   * nullptr if no child exists with the specified name exists.
   * @param name
   * @return DataObject*
   */
  DataObject* operator[](const std::string& name);

  /**
   * Returns a pointer to the DataObject child with the specified name. Returns
   * nullptr if no child exists with the specified name exists.
   * @param name
   * @return DataObject*
   */
  const DataObject* operator[](const std::string& name) const;

  /**
   * @brief Returns an iterator to the child with the specified name. If no such child is
   * found, this function returns end().
   * @param name
   * @return iterator<DataObject *>
   */
  Iterator find(const std::string& name);

  /**
   * @brief Returns an iterator to the child with the specified name. If no such child is
   * found, this function returns end().
   * @param name
   * @return iterator<DataObject *>
   */
  ConstIterator find(const std::string& name) const;

  /**
   * @brief Attempts to insert the specified DataObject into the container. If the
   * DataObject passes *canInsert(obj: weak_ptr<DataObject>): bool*, the DataObject
   * will be inserted into the container and the method returns true. Otherwise,
   * returns false.
   * @param obj
   * @return bool
   */
  bool insert(const std::weak_ptr<DataObject>& obj);

  /**
   * Attempts to remove the specified DataObject from the container. Returns true if
   * it succeeded. Returns false otherwise.
   * @param obj
   * @return bool
   */
  bool remove(DataObject* obj);

  /**
   * Attempts to remove a DataObject with the specified name from the container.
   * Returns true if it succeeded. Returns false otherwise.
   * @param name
   * @return bool
   */
  bool remove(const std::string& name);

  /**
   * @brief Returns an iterator to the beginning of the container.
   * @return Iterator
   */
  Iterator begin();

  /**
   * @brief Returns an iterator to the end of the container.
   * @return Iterator
   */
  Iterator end();

  /**
   * @brief Returns an iterator to the beginning of the container.
   * @return Iterator
   */
  ConstIterator begin() const;

  /**
   * @brief Returns an iterator to the end of the container.
   * @return Iterator
   */
  ConstIterator end() const;

protected:
  /**
   * @brief
   * @param ds
   * @param name
   */
  BaseGroup(DataStructure* ds, const std::string& name);

  /**
   * @brief Checks if the provided DataObject can be added to the container. This is a
   * virtual method that is overwritten by derived classes to specify what can or
   * cannot be added to the container. Returns true if the DataObject can be added to
   * the container. Otherwise, returns false.
   * @param obj
   * @return bool
   */
  virtual bool canInsert(const DataObject* obj) const;

  /**
   * @brief
   * @param ds
   */
  void setDataStructure(DataStructure* ds) override;

private:
  DataMap m_DataMap;
};
} // namespace SIMPL
