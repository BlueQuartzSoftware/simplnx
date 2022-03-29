#pragma once

#include <iostream>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "complex/DataStructure/Metadata.hpp"
#include "complex/Utilities/Parsing/HDF5/H5.hpp"
#include "complex/Utilities/TooltipGenerator.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
class BaseGroup;
class DataPath;
class DataStructure;

namespace H5
{
class DataStructureWriter;
class GroupWriter;
class ObjectWriter;
} // namespace H5

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

    AbstractMontage = 4,
    DataGroup = 5,

    IDataArray = 6,
    DataArray = 7,

    AbstractGeometry = 8,
    VertexGeom = 9,
    EdgeGeom = 10,
    AbstractGeometryGrid = 11,
    RectGridGeom = 12,
    ImageGeom = 13,

    AbstractGeometry2D = 14,
    QuadGeom = 15,
    TriangleGeom = 16,

    AbstractGeometry3D = 17,
    HexahedralGeom = 18,
    TetrahedralGeom = 19,

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
  using IdType = uint64;

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
  virtual DataObject* deepCopy() = 0;

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

  /**
   * @brief Writes the DataObject to the target HDF5 group.
   * @param dataStructureWriter
   * @param parentGroupWriter
   * @param importable = true
   * @return H5::ErrorType
   */
  virtual H5::ErrorType writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable = true) const = 0;

protected:
  /**
   * @brief DataObject constructor takes a reference to the DataStructure and
   * object name.
   * @param ds
   * @param name
   */
  DataObject(DataStructure& ds, std::string name);

  /**
   * @brief DataObject constructor takes a reference to the DataStructure,
   * object name, and object ID.
   * @param ds
   * @param name
   * @param importId
   */
  DataObject(DataStructure& ds, std::string name, IdType importId);

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
   * @param ds
   * @param data
   * @param parentId
   * @return bool
   */
  static bool AttemptToAddObject(DataStructure& ds, const std::shared_ptr<DataObject>& data, const std::optional<IdType>& parentId);

  /**
   * @brief Marks the specified BaseGroup as a parent.
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
   * @param ds
   */
  virtual void setDataStructure(DataStructure* ds);

  /**
   * @brief Writes the dataType as a string attribute for the target HDF5 object.
   * Returns the HDF5 error should one occur.
   * @param dataStructureWriter
   * @param objectWriter
   * @param importable = true
   * @return H5::ErrorType
   */
  H5::ErrorType writeH5ObjectAttributes(H5::DataStructureWriter& dataStructureWriter, H5::ObjectWriter& objectWriter, bool importable = true) const;

private:
  DataStructure* m_DataStructure = nullptr;
  ParentCollectionType m_ParentList;
  IdType m_Id = 0;
  std::string m_Name;
  Metadata m_Metadata;
};
} // namespace complex
