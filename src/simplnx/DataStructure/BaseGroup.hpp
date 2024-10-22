#pragma once

#include "simplnx/Common/Result.hpp"
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
   * @param other
   */
  BaseGroup(const BaseGroup& other);

  /**
   * @brief Move constructor creates a BaseGroup using the existing values of
   * the provided BaseGroup.
   * @param other
   */
  BaseGroup(BaseGroup&& other);

  /**
   * @brief Destroys the BaseGroup and removes it from the list of it's
   * children's known parents. If a child no longer has any parents, the
   * DataObject is destroyed.
   */
  ~BaseGroup() override;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return
   */
  DataObject::Type getDataObjectType() const override;

  /**
   * @brief Returns true if this object is derived from BaseGroup.
   * @return bool
   */
  bool isGroup() const override;

  /**
   * @brief Returns an enumeration of the class or subclass GroupType. Used for quick comparison or type deduction
   * @return
   */
  virtual GroupType getGroupType() const;

  /**
   * @brief Returns the number of DataObjects in the group.
   *
   * BaseGroups found among the container's children are not expanded during
   * the operation.
   * @return usize
   */
  usize getSize() const;

  /**
   * @brief Returns if there are any DataObjects in the group
   * @return bool
   */
  bool empty() const;

  /**
   * @brief Returns the underlying DataMap by value.
   * @return const DataMap&
   */
  const DataMap& getDataMap() const;

  /**
   * @brief Returns the underlying DataMap by reference.
   * @return DataMap&
   */
  DataMap& getDataMap();

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
   * Returns a pointer to the DataObject child with the specified name.
   * Throws if no child exists with the specified name exists.
   *
   * BaseGroups found among the container's children are not expanded during
   * the operation.
   * @param name
   * @return DataObject&
   */
  DataObject& at(const std::string& name);

  /**
   * Returns a pointer to the DataObject child with the specified name.
   * Throws if no child exists with the specified name exists.
   *
   * BaseGroups found among the container's children are not expanded during
   * the operation.
   * @param name
   * @return const DataObject&
   */
  const DataObject& at(const std::string& name) const;

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
   * @brief Searches the group for all DataObjects of the templated type.
   * If the optional parameter recursive is set to true, this will recursively
   * search any child groups as well.
   * @tparam T
   * @param recursive = false
   * @return std::vector<std::shared_ptr<T>>
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
   * @brief Returns true if this group is a parent of the given DataObject
   * @return bool
   */
  bool isParentOf(const DataObject* dataObj) const;

  /**
   * @brief Attempts to insert the specified DataObject into the container. If the
   * DataObject passes 'canInsert(obj: weak_ptr<DataObject>): bool', the DataObject
   * will be inserted into the container and the method returns true. Otherwise,
   * returns false.
   * @param obj
   * @return bool
   */
  Result<> insert(const std::weak_ptr<DataObject>& obj);

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
   * @brief Clears the group of all children.
   */
  void clear();

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

  /**
   * @brief Creates a set of all the BaseGroup GroupTypes.
   * @return std::set<GroupType>
   */
  static const std::set<GroupType>& GetAllGroupTypes();

  /**
   * @brief Creates a set of all the BaseGroup GroupTypes.
   * @return std::set<GroupType>
   */
  static const std::set<GroupType>& GetAllGeometryGroupTypes();

  /**
   * @brief Converts the set of BaseGroup GroupTypes to strings.
   * @param groupTypes
   * @return std::set<std::string>
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
   * @brief Creates a BaseGroup with the target DataStructure and name.
   * @param dataStructure
   * @param name
   */
  BaseGroup(DataStructure& dataStructure, std::string name);

  /**
   * @brief Creates a BaseGroup with the target DataStructure and name.
   * @param dataStructure
   * @param name
   * @param importId
   */
  BaseGroup(DataStructure& dataStructure, std::string name, IdType importId);

  /**
   * @brief Updates the DataMap IDs. Should only be called by DataObject::checkUpdatedIds.
   * @param updatedIdsMap
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
   * @param obj
   * @return bool
   */
  virtual Result<> canInsert(const DataObject* obj) const;

  /**
   * @brief Sets a new DataStructure for the BaseGroup. Updates the DataMap
   * and its contained DataObjects as well.
   * @param dataStructure
   */
  void setDataStructure(DataStructure* dataStructure) override;

private:
  DataMap m_DataMap;
};
} // namespace nx::core
