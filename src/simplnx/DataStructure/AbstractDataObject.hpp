#pragma once

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/IDataObject.hpp"
#include "simplnx/DataStructure/Metadata.hpp"
#include "simplnx/simplnx_export.hpp"

#include <iostream>
#include <list>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace nx::core
{
class BaseGroup;
class DataPath;
class DataStructure;

/**
 * @class AbstractDataObject
 * @brief The AbstractDataObject class is the abstract base class for all items stored in the
 * DataStructure. AbstractDataObjects have a name and ID value for looking them up.
 * Concrete implementations need to implement shallowCopy, deepCopy, and getTypeName.
 * AbstractDataObjects can have multiple parents and are deleted when they are removed
 * from their last remaining parent.
 */
class SIMPLNX_EXPORT AbstractDataObject : public IDataObject
{
public:
  using IDataObject::EnumType;
  using IDataObject::IdType;
  using IDataObject::OptionalId;
  using IDataObject::ParentCollectionType;
  using IDataObject::Type;

  friend class BaseGroup;
  friend class DataMap;
  friend class DataStructure;

  /**
   * @brief Returns true if the given string is a valid name for an AbstractDataObject.
   * @param name
   * @return
   */
  static bool IsValidName(std::string_view name);

  static std::set<std::string> StringListFromDataObjectType(const std::set<Type>& dataObjectTypes);

  /**
   * @brief Copy constructor.
   * @param rhs
   */
  AbstractDataObject(const AbstractDataObject& rhs);

  /**
   * @brief Move constructor.
   * @param rhs
   */
  AbstractDataObject(AbstractDataObject&& rhs);

  /**
   * @brief Copy assignment.
   * @param rhs
   * @return
   */
  AbstractDataObject& operator=(const AbstractDataObject& rhs);

  /**
   * @brief Move assignment.
   * @param rhs
   * @return
   */
  AbstractDataObject& operator=(AbstractDataObject&& rhs) noexcept;

  /**
   * @brief Destructor.
   */
  ~AbstractDataObject() noexcept override;

  /**
   * @brief Returns a deep copy of the AbstractDataObject.
   * @return AbstractDataObject*
   */
  virtual std::shared_ptr<AbstractDataObject> deepCopy(const DataPath& copyPath) = 0;

  /**
   * @brief Returns a shallow copy of the AbstractDataObject.
   * @return AbstractDataObject*
   */
  virtual AbstractDataObject* shallowCopy() = 0;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return
   */
  Type getDataObjectType() const override;

  /**
   * @brief Returns true if this object is derived from BaseGroup.
   * @return bool
   */
  bool isGroup() const override;

  /**
   * @brief Returns the AbstractDataObject's ID value.
   * @return IdType
   */
  IdType getId() const override;

  /**
   * @brief Returns a pointer to the DataStructure this AbstractDataObject belongs to.
   * @return DataStructure*
   */
  DataStructure* getDataStructure() override;

  /**
   * @brief Returns a read-only pointer to the DataStructure this AbstractDataObject
   * belongs to.
   * @return const DataStructure*
   */
  const DataStructure* getDataStructure() const override;

  /**
   * @brief Returns a reference to the DataStructure this AbstractDataObject belongs to.
   * @return DataStructure&
   */
  DataStructure& getDataStructureRef() override;

  /**
   * @brief Returns a read-only reference to the DataStructure this AbstractDataObject
   * belongs to.
   * @return const DataStructure&
   */
  const DataStructure& getDataStructureRef() const override;

  /**
   * @brief Returns the AbstractDataObject's name.
   * @return std::string
   */
  std::string getName() const override;

  /**
   * @brief Checks and returns if the AbstractDataObject can be renamed to the provided
   * value.
   * @param name
   * @return bool
   */
  bool canRename(const std::string& name) const override;

  /**
   * @brief Attempts to rename the AbstractDataObject to the provided value.
   * @param name
   * @return bool
   */
  bool rename(const std::string& name) override;

  /**
   * @brief Returns a collection of the parent containers that store the AbstractDataObject.
   * @return ParentCollectionType
   */
  ParentCollectionType getParentIds() const override;

  /**
   * @brief Clears the list of parent IDs.
   */
  void clearParents() override;

  /**
   * @brief Returns a vector of DataPaths to the object.
   * @return std::vector<DataPath>
   */
  std::vector<DataPath> getDataPaths() const override;

  /**
   * @brief Returns a reference to the object's Metadata.
   * @return Metadata&
   */
  Metadata& getMetadata() override;

  /**
   * @brief Returns a reference to the object's Metadata.
   * @return const Metadata&
   */
  const Metadata& getMetadata() const override;

  bool hasParent(const DataPath& parentPath) const override;

  /**
   * @brief Flushes the AbstractDataObject to its respective target.
   * In-memory AbstractDataObjects are not affected.
   */
  void flush() const override;

  uint64 memoryUsage() const override;

protected:
  /**
   * @brief AbstractDataObject constructor takes a reference to the DataStructure and
   * object name.
   * @param dataStructure
   * @param name
   */
  AbstractDataObject(DataStructure& dataStructure, std::string name);

  /**
   * @brief AbstractDataObject constructor takes a reference to the DataStructure,
   * object name, and object ID.
   * @param dataStructure
   * @param name
   * @param importId
   */
  AbstractDataObject(DataStructure& dataStructure, std::string name, IdType importId);

  /**
   * @brief Updates the data ID for lookup within the DataStructure.
   * This method should only be called from within the DataStructure.
   * @param newId
   */
  void setId(IdType newId);

  /**
   * @brief Notifies the AbstractDataObject of IDs that have been changed by the DataStructure.
   * @param updatedIdsMap std::unordered_map containing the mappings between the old IDs and the new IDs
   */
  void checkUpdatedIds(const std::unordered_map<AbstractDataObject::IdType, AbstractDataObject::IdType>& updatedIdsMap);

  /**
   * @brief Calls specialized checks for derived classes. Should only be called by checkUpdatedIds.
   * @param updatedIds
   */
  virtual void checkUpdatedIdsImpl(const std::unordered_map<AbstractDataObject::IdType, AbstractDataObject::IdType>& updatedIdsMap);

  /**
   * @brief Attempts to add the specified AbstractDataObject to the target DataStructure.
   * If a parentId is provided, then the AbstractDataObject will be added as a child to
   * the target AbstractDataObject. Otherwise, the AbstractDataObject will be added directly
   * under the DataStructure. If the AbstractDataObject is added successfully, the
   * target parent will take ownership of the added AbstractDataObject.
   *
   * Returns true if the operation succeeds. Returns false otherwise.
   * @param dataStructure
   * @param data
   * @param parentId
   * @return bool
   */
  static bool AttemptToAddObject(DataStructure& ds, const std::shared_ptr<AbstractDataObject>& data, const OptionalId& parentId);

  /**
   * @brief Marks the specified BaseGroup as a parent.
   * If this object is already parented to the given group, this function does nothing.
   * @param parent
   */
  void addParent(BaseGroup* parent);

  /**
   * @brief Removes the specified parent.
   * @param parent
   */
  void removeParent(BaseGroup* parent);

  /**
   * @brief Replaces the specified parent with another BaseGroup.
   * @param oldParent
   * @param newParent
   */
  void replaceParent(BaseGroup* oldParent, BaseGroup* newParent);

  /**
   * @brief Sets a new DataStructure for the AbstractDataObject.
   * @param dataStructure
   */
  virtual void setDataStructure(DataStructure* dataStructure);

private:
  DataStructure* m_DataStructure = nullptr;
  ParentCollectionType m_ParentList;
  IdType m_Id = 0;
  std::string m_Name;
  Metadata m_Metadata;
};
} // namespace nx::core
