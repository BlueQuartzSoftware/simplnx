#pragma once

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Common/Types.hpp"
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
 * @class DataObject
 * @brief The DataObject class is the base class for all items stored in the
 * DataStructure. DataObjects have a name and ID value for looking them up.
 * Concrete implementations need to implement shallowCopy, deepCopy,
 * generateXdmfText, and readFromXdmfText.
 * DataObjects can have multiple parents and are deleted when they are removed
 * from their last remaining parent.
 */
class SIMPLNX_EXPORT DataObject
{
public:
  using EnumType = uint32;
  enum class Type : EnumType
  {
    DataObject = 0,

    DynamicListArray = 1,
    ScalarData = 2,

    BaseGroup = 3,

    AttributeMatrix = 4,
    DataGroup = 5,

    IDataArray = 6,
    DataArray = 7,

    IGeometry = 8,

    IGridGeometry = 9,
    RectGridGeom = 10,
    ImageGeom = 11,

    INodeGeometry0D = 12,
    VertexGeom = 13,

    INodeGeometry1D = 14,
    EdgeGeom = 15,

    INodeGeometry2D = 16,
    QuadGeom = 17,
    TriangleGeom = 18,

    INodeGeometry3D = 19,
    HexahedralGeom = 20,
    TetrahedralGeom = 21,

    INeighborList = 22,
    NeighborList = 23,

    StringArray = 24,

    AbstractMontage = 25,
    GridMontage = 26,

    Unknown = 999,
    Any = 4294967295U
  };

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return The DataObject type enumeration value
   */
  virtual Type getDataObjectType() const;

  /**
   * @brief Returns true if this object is derived from BaseGroup.
   * @return True if the object is a group, false otherwise
   */
  virtual bool isGroup() const;

  /**
   * @brief The IdType alias serves as an ID type for DataObjects within their
   * respective DataStructure.
   */
  using IdType = types::uint64;

  /**
   * @brief The OptionalId alias specifies that the target DataObject is not required.
   */
  using OptionalId = std::optional<IdType>;

  /**
   * @brief The ParentCollectionType alias describes the format by which parent
   * collections are returned via public API.
   */
  using ParentCollectionType = std::list<IdType>;

  friend class BaseGroup;
  friend class DataMap;
  friend class DataStructure;

  /**
   * @brief Returns true if the given string is a valid name for a DataObject.
   * @param name The string to validate as a DataObject name
   * @return True if the name is valid, false otherwise
   */
  static bool IsValidName(std::string_view name);

  /**
   * @brief Converts a set of DataObject types to a set of their string representations.
   * @param dataObjectTypes Set of DataObject type enumerations
   * @return Set of strings representing the DataObject type names
   */
  static std::set<std::string> StringListFromDataObjectType(const std::set<Type>& dataObjectTypes);

  /**
   * @brief Copy constructor.
   * @param rhs The DataObject to copy from
   */
  DataObject(const DataObject& rhs);

  /**
   * @brief Move constructor.
   * @param rhs The DataObject to move from
   */
  DataObject(DataObject&& rhs);

  /**
   * @brief Copy assignment operator.
   * @param rhs The DataObject to copy from
   * @return Reference to this DataObject after assignment
   */
  DataObject& operator=(const DataObject& rhs);

  /**
   * @brief Move assignment operator.
   * @param rhs The DataObject to move from
   * @return Reference to this DataObject after assignment
   */
  DataObject& operator=(DataObject&& rhs) noexcept;

  /**
   * @brief Destructor.
   */
  virtual ~DataObject() noexcept;

  /**
   * @brief Returns a deep copy of the DataObject.
   * @param copyPath The DataPath where the deep copy will be placed
   * @return Shared pointer to the deep copy of the DataObject
   */
  virtual std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) = 0;

  /**
   * @brief Returns a shallow copy of the DataObject.
   * @return Pointer to the shallow copy of the DataObject
   */
  virtual DataObject* shallowCopy() = 0;

  /**
   * @brief Returns typename of the DataObject as a std::string.
   * @return String representation of the DataObject type name
   */
  virtual std::string getTypeName() const = 0;

  /**
   * @brief Returns the DataObject's ID value.
   * @return The unique identifier for this DataObject
   */
  IdType getId() const;

  /**
   * @brief Returns a pointer to the DataStructure this DataObject belongs to.
   * @return Pointer to the owning DataStructure
   */
  DataStructure* getDataStructure();

  /**
   * @brief Returns a read-only pointer to the DataStructure this DataObject
   * belongs to.
   * @return Const pointer to the owning DataStructure
   */
  const DataStructure* getDataStructure() const;

  /**
   * @brief Returns a reference to the DataStructure this DataObject belongs to.
   * @return Reference to the owning DataStructure
   */
  DataStructure& getDataStructureRef();

  /**
   * @brief Returns a read-only reference to the DataStructure this DataObject
   * belongs to.
   * @return Const reference to the owning DataStructure
   */
  const DataStructure& getDataStructureRef() const;

  /**
   * @brief Returns the DataObject's name.
   * @return String containing the name of this DataObject
   */
  std::string getName() const;

  /**
   * @brief Checks and returns if the DataObject can be renamed to the provided
   * value.
   * @param name The new name to check for validity
   * @return True if the DataObject can be renamed to the provided value, false otherwise
   */
  bool canRename(const std::string& name) const;

  /**
   * @brief Attempts to rename the DataObject to the provided value.
   * @param name The new name for the DataObject
   * @return True if the rename operation succeeded, false otherwise
   */
  bool rename(const std::string& name);

  /**
   * @brief Returns a collection of the parent containers that store the DataObject.
   * @return List of parent IDs that contain this DataObject
   */
  ParentCollectionType getParentIds() const;

  /**
   * @brief Clears the list of parent IDs.
   */
  void clearParents();

  /**
   * @brief Returns a vector of DataPaths to the object.
   * @return Vector containing all DataPaths that reference this object
   */
  std::vector<DataPath> getDataPaths() const;

  /**
   * @brief Returns a reference to the object's Metadata.
   * @return Reference to the Metadata object
   */
  Metadata& getMetadata();

  /**
   * @brief Returns a const reference to the object's Metadata.
   * @return Const reference to the Metadata object
   */
  const Metadata& getMetadata() const;

  /**
   * @brief Checks if the DataObject has a parent at the specified path.
   * @param parentPath The DataPath to check for parent relationship
   * @return True if the specified path is a parent of this object, false otherwise
   */
  bool hasParent(const DataPath& parentPath) const;

  /**
   * @brief Flushes the DataObject to its respective target.
   * In-memory DataObjects are not affected.
   */
  virtual void flush() const;

  /**
   * @brief Calculates and returns the memory usage of this DataObject in bytes.
   * @return Memory usage in bytes
   */
  virtual uint64 memoryUsage() const;

protected:
  /**
   * @brief Protected constructor takes a reference to the DataStructure and
   * object name.
   * @param dataStructure The DataStructure that will own this DataObject
   * @param name The name for the DataObject
   */
  DataObject(DataStructure& dataStructure, std::string name);

  /**
   * @brief Protected constructor takes a reference to the DataStructure,
   * object name, and object ID.
   * @param dataStructure The DataStructure that will own this DataObject
   * @param name The name for the DataObject
   * @param importId The ID to assign to this DataObject
   */
  DataObject(DataStructure& dataStructure, std::string name, IdType importId);

  /**
   * @brief Updates the data ID for lookup within the DataStructure.
   * This method should only be called from within the DataStructure.
   * @param newId The new ID value to assign to this DataObject
   */
  void setId(IdType newId);

  /**
   * @brief Notifies the DataObject of DataObject IDs that have been changed by the DataStructure.
   * @param updatedIdsMap std::unordered_map containing the mappings between the old IDs and the new IDs
   */
  void checkUpdatedIds(const std::unordered_map<DataObject::IdType, DataObject::IdType>& updatedIdsMap);

  /**
   * @brief Calls specialized checks for derived classes. Should only be called by checkUpdatedIds.
   * @param updatedIds
   */
  virtual void checkUpdatedIdsImpl(const std::unordered_map<DataObject::IdType, DataObject::IdType>& updatedIdsMap);

  /**
   * @brief Attempts to add the specified DataObject to the target DataStructure.
   * If a parentId is provided, then the DataObject will be added as a child to
   * the target DataObject. Otherwise, the DataObject will be added directly
   * under the DataStructure. If the DataObject is added successfully, the
   * target parent will take ownership of the added DataObject.
   *
   * Returns true if the operation succeeds. Returns false otherwise.
   * @param ds The DataStructure to add the object to
   * @param data Shared pointer to the DataObject to add
   * @param parentId Optional parent ID; if provided, object is added as a child to that parent
   * @return True if the object was added successfully, false otherwise
   */
  static bool AttemptToAddObject(DataStructure& ds, const std::shared_ptr<DataObject>& data, const OptionalId& parentId);

  /**
   * @brief Marks the specified BaseGroup as a parent.
   * If this object is already parented to the given group, this function does nothing.
   * @param parent Pointer to the BaseGroup to add as a parent
   */
  void addParent(BaseGroup* parent);

  /**
   * @brief Removes the specified parent.
   * @param parent Pointer to the BaseGroup to remove as a parent
   */
  void removeParent(BaseGroup* parent);

  /**
   * @brief Replaces the specified parent with another BaseGroup.
   * @param oldParent Pointer to the BaseGroup to replace
   * @param newParent Pointer to the new BaseGroup parent
   */
  void replaceParent(BaseGroup* oldParent, BaseGroup* newParent);

  /**
   * @brief Sets a new DataStructure for the DataObject.
   * @param dataStructure Pointer to the new DataStructure owner
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
