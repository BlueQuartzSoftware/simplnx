#pragma once

#include "complex/Common/StringLiteral.hpp"
#include "complex/Common/Types.hpp"
#include "complex/DataStructure/Metadata.hpp"
#include "complex/complex_export.hpp"

#include <iostream>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace complex
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
class COMPLEX_EXPORT DataObject
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
   * @return
   */
  virtual Type getDataObjectType() const;

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
   * @param name
   * @return
   */
  static bool IsValidName(std::string_view name);

  static std::set<std::string> StringListFromDataObjectType(const std::set<Type>& dataObjectTypes);

  /**
   * @brief Copy constructor.
   * @param rhs
   */
  DataObject(const DataObject& rhs);

  /**
   * @brief Move constructor.
   * @param rhs
   */
  DataObject(DataObject&& rhs);

  /**
   * @brief Copy assignment.
   * @param rhs
   * @return
   */
  DataObject& operator=(const DataObject& rhs);

  /**
   * @brief Move assignment.
   * @param rhs
   * @return
   */
  DataObject& operator=(DataObject&& rhs) noexcept;

  /**
   * @brief Destructor.
   */
  virtual ~DataObject() noexcept;

  /**
   * @brief Returns a deep copy of the DataObject.
   * @return DataObject*
   */
  virtual std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) = 0;

  /**
   * @brief Returns a shallow copy of the DataObject.
   * @return DataObject*
   */
  virtual DataObject* shallowCopy() = 0;

  /**
   * @brief Returns typename of the DataObject as a std::string.
   * @return std::string
   */
  virtual std::string getTypeName() const = 0;

  /**
   * @brief Returns the DataObject's ID value.
   * @return IdType
   */
  IdType getId() const;

  /**
   * @brief Returns a pointer to the DataStructure this DataObject belongs to.
   * @return DataStructure*
   */
  DataStructure* getDataStructure();

  /**
   * @brief Returns a read-only pointer to the DataStructure this DataObject
   * belongs to.
   * @return const DataStructure*
   */
  const DataStructure* getDataStructure() const;

  /**
   * @brief Returns a pointer to the DataStructure this DataObject belongs to.
   * @return DataStructure&
   */
  DataStructure& getDataStructureRef();

  /**
   * @brief Returns a read-only pointer to the DataStructure this DataObject
   * belongs to.
   * @return const DataStructure&
   */
  const DataStructure& getDataStructureRef() const;

  /**
   * @brief Returns the DataObject's name.
   * @return std::string
   */
  std::string getName() const;

  /**
   * @brief Checks and returns if the DataObject can be renamed to the provided
   * value.
   * @param name
   * @return bool
   */
  bool canRename(const std::string& name) const;

  /**
   * @brief Attempts to rename the DataObject to the provided value.
   * @param name
   * @return bool
   */
  bool rename(const std::string& name);

  /**
   * @brief Returns a collection of the parent containers that store the DataObject.
   * @return ParentCollectionType
   */
  ParentCollectionType getParentIds() const;

  /**
   * @brief Clears the list of parent IDs.
   */
  void clearParents();

  /**
   * @brief Returns a vector of DataPaths to the object.
   * @return std::vector<DataPath>
   */
  std::vector<DataPath> getDataPaths() const;

  /**
   * @brief Returns a reference to the object's Metadata.
   * @return Metadata&
   */
  Metadata& getMetadata();

  /**
   * @brief Returns a reference to the object's Metadata.
   * @return const Metadata&
   */
  const Metadata& getMetadata() const;

  bool hasParent(const DataPath& parentPath) const;

  /**
   * @brief Flushes the DataObject to its respective target.
   * In-memory DataObjects are not affected.
   */
  virtual void flush() const;

protected:
  /**
   * @brief DataObject constructor takes a reference to the DataStructure and
   * object name.
   * @param dataStructure
   * @param name
   */
  DataObject(DataStructure& dataStructure, std::string name);

  /**
   * @brief DataObject constructor takes a reference to the DataStructure,
   * object name, and object ID.
   * @param dataStructure
   * @param name
   * @param importId
   */
  DataObject(DataStructure& dataStructure, std::string name, IdType importId);

  /**
   * @brief Updates the data ID for lookup within the DataStructure.
   * This method should only be called from within the DataStructure.
   * @param newId
   */
  void setId(IdType newId);

  /**
   * @brief Notifies the DataObject of DataObject IDs that have been changed by the DataStructure.
   * @param updatedIds std::pair ordered as {old ID, new ID}
   */
  void checkUpdatedIds(const std::vector<std::pair<IdType, IdType>>& updatedIds);

  /**
   * @brief Calls specialized checks for derived classes. Should only be called by checkUpdatedIds.
   * @param updatedIds
   */
  virtual void checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds);

  /**
   * @brief Attempts to add the specified DataObject to the target DataStructure.
   * If a parentId is provided, then the DataObject will be added as a child to
   * the target DataObject. Otherwise, the DataObject will be added directly
   * under the DataStructure. If the DataObject is added successfully, the
   * target parent will take ownership of the added DataObject.
   *
   * Returns true if the operation succeeds. Returns false otherwise.
   * @param dataStructure
   * @param data
   * @param parentId
   * @return bool
   */
  static bool AttemptToAddObject(DataStructure& ds, const std::shared_ptr<DataObject>& data, const OptionalId& parentId);

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
   * @brief Sets a new DataStructure for the DataObject.
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
} // namespace complex
