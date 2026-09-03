#pragma once

#include "simplnx/DataStructure/DataMap.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/simplnx_export.hpp"

#include <memory>
#include <set>
#include <string>
#include <vector>

namespace nx::core
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
class SIMPLNX_EXPORT BaseGroup : public DataObject
{
public:
  using Iterator = typename DataMap::Iterator;
  using ConstIterator = typename DataMap::ConstIterator;

  static inline constexpr StringLiteral k_TypeName = "BaseGroup";

  enum class GroupType : uint32
  {
    BaseGroup,
    DataGroup,
    AttributeMatrix,
    IGeometry,
    IGridGeometry,
    RectGridGeom,
    ImageGeom,
    INodeGeometry0D,
    VertexGeom,
    INodeGeometry1D,
    EdgeGeom,
    INodeGeometry2D,
    QuadGeom,
    TriangleGeom,
    INodeGeometry3D,
    HexahedralGeom,
    TetrahedralGeom,
    Unknown
  };

  /**
   * @brief Copy constructor creates a BaseGroup as a shallow copy of the
   * provided group.
   * @param other The BaseGroup to copy from
   */
  BaseGroup(const BaseGroup& other);

  /**
   * @brief Move constructor creates a BaseGroup using the existing values of
   * the provided BaseGroup.
   * @param other The BaseGroup to move from
   */
  BaseGroup(BaseGroup&& other) noexcept;

  /**
   * @brief Destroys the BaseGroup and removes it from the list of it's
   * children's known parents. If a child no longer has any parents, the
   * DataObject is destroyed.
   */
  ~BaseGroup() override;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return The DataObject type enumeration value
   */
  DataObject::Type getDataObjectType() const override;

  /**
   * @brief Returns true if this object is derived from BaseGroup.
   * @return True (BaseGroup is always a group)
   */
  bool isGroup() const override;

  /**
   * @brief Returns an enumeration of the class or subclass GroupType. Used for quick comparison or type deduction
   * @return The GroupType enumeration value
   */
  virtual GroupType getGroupType() const;

  /**
   * @brief Returns the number of DataObjects in the group.
   *
   * BaseGroups found among the container's children are not expanded during
   * the operation.
   * @return Number of DataObjects in this group
   */
  usize getSize() const;

  /**
   * @brief Returns if there are any DataObjects in the group
   * @return True if the group is empty, false otherwise
   */
  bool empty() const;

  /**
   * @brief Returns the underlying DataMap by value.
   * @return Const reference to the underlying DataMap
   */
  const DataMap& getDataMap() const;

  /**
   * @brief Returns the underlying DataMap by reference.
   * @return Reference to the underlying DataMap
   */
  DataMap& getDataMap();

  /**
   * @brief Returns true if a child with the specified name exists in the
   * container. Returns false otherwise. This operation does not expand any
   * BaseGroups found among its children.
   * @param name The name of the child to search for
   * @return True if a child with the given name exists, false otherwise
   */
  bool contains(const std::string& name) const;

  /**
   * @brief Returns true if the specified DataObject is found among the
   * container's children. Returns false otherwise.
   *
   * BaseGroups found among the container's children are not expanded during
   * the operation.
   * @param obj Pointer to the DataObject to search for
   * @return True if the object is found among children, false otherwise
   */
  bool contains(const DataObject* obj) const;

  /**
   * @brief Returns a pointer to the DataObject child with the specified name. Returns
   * nullptr if no child exists with the specified name exists.
   *
   * BaseGroups found among the container's children are not expanded during
   * the operation.
   * @param name The name of the child to retrieve
   * @return Pointer to the DataObject if found, nullptr otherwise
   */
  DataObject* operator[](const std::string& name);

  /**
   * @brief Returns a const pointer to the DataObject child with the specified name. Returns
   * nullptr if no child exists with the specified name exists.
   *
   * BaseGroups found among the container's children are not expanded during
   * the operation.
   * @param name The name of the child to retrieve
   * @return Const pointer to the DataObject if found, nullptr otherwise
   */
  const DataObject* operator[](const std::string& name) const;

  /**
   * @brief Returns a reference to the DataObject child with the specified name.
   * Throws if no child exists with the specified name exists.
   *
   * BaseGroups found among the container's children are not expanded during
   * the operation.
   * @param name The name of the child to retrieve
   * @return Reference to the DataObject
   */
  DataObject& at(const std::string& name);

  /**
   * @brief Returns a const reference to the DataObject child with the specified name.
   * Throws if no child exists with the specified name exists.
   *
   * BaseGroups found among the container's children are not expanded during
   * the operation.
   * @param name The name of the child to retrieve
   * @return Const reference to the DataObject
   */
  const DataObject& at(const std::string& name) const;

  /**
   * @brief Returns an iterator to the child with the specified name. If no
   * such child is found, this function returns end().
   *
   * BaseGroups found among the container's children are not expanded during
   * the operation.
   * @param name The name of the child to find
   * @return Iterator to the child if found, end() otherwise
   */
  Iterator find(const std::string& name);

  /**
   * @brief Returns a ConstIterator to the child with the specified name. If no
   * such child is found, this function returns end().
   *
   * BaseGroups found among the container's children are not expanded during
   * the operation.
   * @param name The name of the child to find
   * @return ConstIterator to the child if found, end() otherwise
   */
  ConstIterator find(const std::string& name) const;

  /**
   * @brief Searches the group for all DataObjects of the templated type.
   * If the optional parameter recursive is set to true, this will recursively
   * search any child groups as well.
   * @tparam T The type of DataObject to search for
   * @param recursive If true, recursively searches child groups (default is false)
   * @return Set of shared pointers to all found objects of type T
   */
  template <class T>
  std::set<std::shared_ptr<T>> findAllChildrenOfType(bool recursive = false) const
  {
    std::set<std::shared_ptr<T>> foundItems;
    for(const auto& [key, ptr] : *this)
    {
      if(auto typePtr = std::dynamic_pointer_cast<T>(ptr); typePtr != nullptr)
      {
        foundItems.insert(typePtr);
      }
      if(recursive)
      {
        if(auto groupPtr = std::dynamic_pointer_cast<BaseGroup>(ptr); groupPtr != nullptr)
        {
          auto recursiveItems = groupPtr->template findAllChildrenOfType<T>(recursive);
          foundItems.merge(recursiveItems);
        }
      }
    }
    return foundItems;
  }

  /**
   * @brief Returns true if this group is a parent of the given DataObject.
   * @param dataObj Pointer to the DataObject to check
   * @return True if this group is a parent of the given object, false otherwise
   */
  bool isParentOf(const DataObject* dataObj) const;

  /**
   * @brief Attempts to insert the specified DataObject into the container. If the
   * DataObject passes 'canInsert(obj: weak_ptr<DataObject>): bool', the DataObject
   * will be inserted into the container and the method returns true. Otherwise,
   * returns false.
   * @param obj Weak pointer to the DataObject to insert
   * @return True if the object was inserted successfully, false otherwise
   */
  bool insert(const std::weak_ptr<DataObject>& obj);

  /**
   * @brief Attempts to remove the specified DataObject from the container. Returns
   * true if it succeeded. Returns false otherwise.
   *
   * BaseGroups found among the container's children are not expanded during
   * the operation.
   * @param obj Pointer to the DataObject to remove
   * @return True if the object was removed successfully, false otherwise
   */
  bool remove(DataObject* obj);

  /**
   * @brief Attempts to remove a DataObject with the specified name from the container.
   * Returns true if it succeeded. Returns false otherwise.
   *
   * BaseGroups found among the container's children are not expanded during
   * the operation.
   * @param name The name of the child to remove
   * @return True if the object was removed successfully, false otherwise
   */
  bool remove(const std::string& name);

  /**
   * @brief Clears the group of all children.
   */
  virtual void clear();

  /**
   * @brief Returns an iterator to the beginning of the container.
   *
   * BaseGroups found among the container's children are not expanded during
   * the use of the returned iterator.
   * @return Iterator to the beginning of the container
   */
  Iterator begin();

  /**
   * @brief Returns an iterator to the end of the container.
   * @return Iterator to the end of the container
   */
  Iterator end();

  /**
   * @brief Returns a const iterator to the beginning of the container.
   *
   * BaseGroups found among the container's children are not expanded during
   * the use of the returned iterator.
   * @return ConstIterator to the beginning of the container
   */
  ConstIterator begin() const;

  /**
   * @brief Returns a const iterator to the end of the container.
   * @return ConstIterator to the end of the container
   */
  ConstIterator end() const;

  /**
   * @brief Returns a reference to the set of all BaseGroup GroupTypes.
   * @return Const reference to the set containing all GroupTypes
   */
  static const std::set<GroupType>& GetAllGroupTypes();

  /**
   * @brief Returns a reference to the set of all geometry-related BaseGroup GroupTypes.
   * @return Const reference to the set containing all geometry GroupTypes
   */
  static const std::set<GroupType>& GetAllGeometryGroupTypes();

  /**
   * @brief Converts the set of BaseGroup GroupTypes to their string representations.
   * @param groupTypes Set of GroupType enumerations to convert
   * @return Set of strings representing the GroupTypes
   */
  static std::set<std::string> StringListFromGroupType(const std::set<GroupType>& groupTypes);

  /**
   * @brief Querys the DataMap for the object names in m_DataMap
   */
  std::vector<std::string> GetChildrenNames();

  /**
   * @brief Querys the DataMap for the object ids in m_DataMap
   */
  std::vector<DataObject::IdType> GetChildrenIds();

protected:
  /**
   * @brief Protected constructor creates a BaseGroup with the target DataStructure and name.
   * @param dataStructure The DataStructure that will own this BaseGroup
   * @param name The name for the BaseGroup
   */
  BaseGroup(DataStructure& dataStructure, std::string name);

  /**
   * @brief Protected constructor creates a BaseGroup with the target DataStructure, name, and import ID.
   * @param dataStructure The DataStructure that will own this BaseGroup
   * @param name The name for the BaseGroup
   * @param importId The ID to assign to this BaseGroup
   */
  BaseGroup(DataStructure& dataStructure, std::string name, IdType importId);

  /**
   * @brief Updates the DataMap IDs. Should only be called by DataObject::checkUpdatedIds.
   * @param updatedIdsMap Map of old IDs to new IDs for updating references
   */
  void checkUpdatedIdsImpl(const std::unordered_map<DataObject::IdType, DataObject::IdType>& updatedIdsMap) override;

  /**
   * @brief Checks if the provided DataObject can be added to the container.
   * This is a virtual method so that derived classes can modify what can or
   * cannot be added to the container. Returns true if the DataObject can be
   * added to the container. Otherwise, returns false.
   *
   * By default, a DataObject cannot be added to the BaseContainer if an object
   * with that name is already in the container. No BaseGroup children are
   * expanded during this operation.
   * @param obj Pointer to the DataObject to check for insertion
   * @return True if the object can be inserted, false otherwise
   */
  virtual bool canInsert(const DataObject* obj) const;

  /**
   * @brief Sets a new DataStructure for the BaseGroup. Updates the DataMap
   * and its contained DataObjects as well.
   * @param dataStructure Pointer to the new DataStructure owner
   */
  void setDataStructure(DataStructure* dataStructure) override;

private:
  DataMap m_DataMap;
};
} // namespace nx::core
