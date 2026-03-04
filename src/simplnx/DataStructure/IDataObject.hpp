#pragma once

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/Metadata.hpp"
#include "simplnx/simplnx_export.hpp"

#include <list>
#include <optional>
#include <string>
#include <vector>

namespace nx::core
{
class DataPath;
class DataStructure;

/**
 * @class IDataObject
 * @brief Pure virtual interface for all items stored in the DataStructure.
 *
 * Defines the public contract for querying AbstractDataObject identity, name,
 * parentage, metadata, and memory usage. Concrete implementations live
 * in AbstractDataObject and its subclasses.
 */
class SIMPLNX_EXPORT IDataObject
{
public:
  using EnumType = uint32;
  enum class Type : EnumType
  {
    AbstractDataObject = 0,

    DynamicListArray = 1,
    ScalarData = 2,

    BaseGroup = 3,

    AttributeMatrix = 4,
    DataGroup = 5,

    AbstractDataArray = 6,
    DataArray = 7,

    AbstractGeometry = 8,

    AbstractGridGeometry = 9,
    RectGridGeom = 10,
    ImageGeom = 11,

    AbstractNodeGeometry0D = 12,
    VertexGeom = 13,

    AbstractNodeGeometry1D = 14,
    EdgeGeom = 15,

    AbstractNodeGeometry2D = 16,
    QuadGeom = 17,
    TriangleGeom = 18,

    AbstractNodeGeometry3D = 19,
    HexahedralGeom = 20,
    TetrahedralGeom = 21,

    AbstractNeighborList = 22,
    NeighborList = 23,

    StringArray = 24,

    AbstractMontage = 25,
    GridMontage = 26,

    Unknown = 999,
    Any = 4294967295U
  };

  /**
   * @brief The IdType alias serves as an ID type for DataObjects within their
   * respective DataStructure.
   */
  using IdType = types::uint64;

  /**
   * @brief The OptionalId alias specifies that the target AbstractDataObject is not required.
   */
  using OptionalId = std::optional<IdType>;

  /**
   * @brief The ParentCollectionType alias describes the format by which parent
   * collections are returned via public API.
   */
  using ParentCollectionType = std::list<IdType>;

  virtual ~IDataObject() noexcept = default;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction.
   * @return Type
   */
  virtual Type getDataObjectType() const = 0;

  /**
   * @brief Returns true if this object is derived from BaseGroup.
   * @return bool
   */
  virtual bool isGroup() const = 0;

  /**
   * @brief Returns typename of the AbstractDataObject as a std::string.
   * @return std::string
   */
  virtual std::string getTypeName() const = 0;

  /**
   * @brief Returns the AbstractDataObject's ID value.
   * @return IdType
   */
  virtual IdType getId() const = 0;

  /**
   * @brief Returns a pointer to the DataStructure this AbstractDataObject belongs to.
   * @return DataStructure*
   */
  virtual DataStructure* getDataStructure() = 0;

  /**
   * @brief Returns a read-only pointer to the DataStructure this AbstractDataObject belongs to.
   * @return const DataStructure*
   */
  virtual const DataStructure* getDataStructure() const = 0;

  /**
   * @brief Returns a reference to the DataStructure this AbstractDataObject belongs to.
   * @return DataStructure&
   */
  virtual DataStructure& getDataStructureRef() = 0;

  /**
   * @brief Returns a read-only reference to the DataStructure this AbstractDataObject belongs to.
   * @return const DataStructure&
   */
  virtual const DataStructure& getDataStructureRef() const = 0;

  /**
   * @brief Returns the AbstractDataObject's name.
   * @return std::string
   */
  virtual std::string getName() const = 0;

  /**
   * @brief Checks and returns if the AbstractDataObject can be renamed to the provided value.
   * @param name
   * @return bool
   */
  virtual bool canRename(const std::string& name) const = 0;

  /**
   * @brief Attempts to rename the AbstractDataObject to the provided value.
   * @param name
   * @return bool
   */
  virtual bool rename(const std::string& name) = 0;

  /**
   * @brief Returns a collection of the parent container IDs that store the AbstractDataObject.
   * @return ParentCollectionType
   */
  virtual ParentCollectionType getParentIds() const = 0;

  /**
   * @brief Clears the list of parent IDs.
   */
  virtual void clearParents() = 0;

  /**
   * @brief Returns a vector of DataPaths to the object.
   * @return std::vector<DataPath>
   */
  virtual std::vector<DataPath> getDataPaths() const = 0;

  /**
   * @brief Returns a reference to the object's Metadata.
   * @return Metadata&
   */
  virtual Metadata& getMetadata() = 0;

  /**
   * @brief Returns a reference to the object's Metadata.
   * @return const Metadata&
   */
  virtual const Metadata& getMetadata() const = 0;

  /**
   * @brief Returns true if the AbstractDataObject has the specified parent path.
   * @param parentPath
   * @return bool
   */
  virtual bool hasParent(const DataPath& parentPath) const = 0;

  /**
   * @brief Flushes the AbstractDataObject to its respective target.
   * In-memory DataObjects are not affected.
   */
  virtual void flush() const = 0;

  /**
   * @brief Returns the memory usage of the AbstractDataObject.
   * @return uint64
   */
  virtual uint64 memoryUsage() const = 0;

protected:
  IDataObject() = default;
  IDataObject(const IDataObject&) = default;
  IDataObject(IDataObject&&) = default;
  IDataObject& operator=(const IDataObject&) = default;
  IDataObject& operator=(IDataObject&&) noexcept = default;
};
} // namespace nx::core
