#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/AbstractDataObject.hpp"
#include "simplnx/DataStructure/DataMap.hpp"
#include "simplnx/DataStructure/LinkedPath.hpp"
#include "simplnx/simplnx_export.hpp"

#include <nod/nod.hpp>
#include <nonstd/expected.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace nx::core
{
class AbstractDataStructureMessage;
class DataPath;

/**
 * @class IDataStructure
 * @brief Pure virtual interface for the DataStructure.
 *
 * Defines the public contract for accessing, querying, and manipulating
 * data objects within the data structure. Template convenience methods
 * (getDataAs, getDataRefAs, etc.) live on the concrete DataStructure class.
 */
class SIMPLNX_EXPORT IDataStructure
{
public:
  using SignalType = nod::signal<void(IDataStructure*, const std::shared_ptr<AbstractDataStructureMessage>&)>;
  using Iterator = DataMap::Iterator;
  using ConstIterator = DataMap::ConstIterator;

  virtual ~IDataStructure() noexcept = default;

  /**
   * @brief Returns the number of unique DataObjects in the DataStructure.
   * @return usize
   */
  virtual usize getSize() const = 0;

  /**
   * @brief Clears the DataStructure by removing all DataObjects.
   */
  virtual void clear() = 0;

  /**
   * @brief Returns the IdType for the AbstractDataObject found at the specified DataPath.
   * @param path
   * @return std::optional<IdType>
   */
  virtual std::optional<AbstractDataObject::IdType> getId(const DataPath& path) const = 0;

  /**
   * @brief Returns true if the DataStructure contains a AbstractDataObject with the given key.
   * @param identifier
   * @return bool
   */
  virtual bool containsData(AbstractDataObject::IdType identifier) const = 0;

  /**
   * @brief Returns true if the DataStructure contains a AbstractDataObject with the given path.
   * @param path
   * @return bool
   */
  virtual bool containsData(const DataPath& path) const = 0;

  /**
   * @brief Returns a pointer to the AbstractDataObject with the specified IdType.
   * @param identifier
   * @return AbstractDataObject*
   */
  virtual AbstractDataObject* getData(AbstractDataObject::IdType identifier) = 0;

  /**
   * @brief Returns a pointer to the AbstractDataObject with the specified IdType.
   * @param identifier
   * @return AbstractDataObject*
   */
  virtual AbstractDataObject* getData(const std::optional<AbstractDataObject::IdType>& identifier) = 0;

  /**
   * @brief Returns a pointer to the AbstractDataObject at the given DataPath.
   * @param path
   * @return AbstractDataObject*
   */
  virtual AbstractDataObject* getData(const DataPath& path) = 0;

  /**
   * @brief Returns a reference to the AbstractDataObject at the given DataPath.
   * @param path
   * @return AbstractDataObject&
   */
  virtual AbstractDataObject& getDataRef(const DataPath& path) = 0;

  /**
   * @brief Returns a reference to the AbstractDataObject with the given identifier.
   * @param identifier
   * @return AbstractDataObject&
   */
  virtual AbstractDataObject& getDataRef(AbstractDataObject::IdType identifier) = 0;

  /**
   * @brief Returns a pointer to the AbstractDataObject found at the specified LinkedPath.
   * @param path
   * @return AbstractDataObject*
   */
  virtual AbstractDataObject* getData(const LinkedPath& path) = 0;

  /**
   * @brief Returns a pointer to the AbstractDataObject with the specified IdType.
   * @param identifier
   * @return const AbstractDataObject*
   */
  virtual const AbstractDataObject* getData(AbstractDataObject::IdType identifier) const = 0;

  /**
   * @brief Returns a pointer to the AbstractDataObject with the specified IdType.
   * @param identifier
   * @return const AbstractDataObject*
   */
  virtual const AbstractDataObject* getData(const std::optional<AbstractDataObject::IdType>& identifier) const = 0;

  /**
   * @brief Returns a pointer to the AbstractDataObject at the given DataPath.
   * @param path
   * @return const AbstractDataObject*
   */
  virtual const AbstractDataObject* getData(const DataPath& path) const = 0;

  /**
   * @brief Returns a reference to the AbstractDataObject at the given DataPath.
   * @param path
   * @return const AbstractDataObject&
   */
  virtual const AbstractDataObject& getDataRef(const DataPath& path) const = 0;

  /**
   * @brief Returns a reference to the AbstractDataObject with the given identifier.
   * @param identifier
   * @return const AbstractDataObject&
   */
  virtual const AbstractDataObject& getDataRef(AbstractDataObject::IdType identifier) const = 0;

  /**
   * @brief Returns a pointer to the AbstractDataObject found at the specified LinkedPath.
   * @param path
   * @return const AbstractDataObject*
   */
  virtual const AbstractDataObject* getData(const LinkedPath& path) const = 0;

  /**
   * @brief Returns the shared pointer for the specified AbstractDataObject.
   * @param id
   * @return std::shared_ptr<AbstractDataObject>
   */
  virtual std::shared_ptr<AbstractDataObject> getSharedData(AbstractDataObject::IdType id) = 0;

  /**
   * @brief Returns the shared pointer for the specified AbstractDataObject.
   * @param id
   * @return std::shared_ptr<const AbstractDataObject>
   */
  virtual std::shared_ptr<const AbstractDataObject> getSharedData(AbstractDataObject::IdType id) const = 0;

  /**
   * @brief Returns the shared pointer for the AbstractDataObject at the target path.
   * @param path
   * @return std::shared_ptr<AbstractDataObject>
   */
  virtual std::shared_ptr<AbstractDataObject> getSharedData(const DataPath& path) = 0;

  /**
   * @brief Returns the shared pointer for the AbstractDataObject at the target path.
   * @param path
   * @return std::shared_ptr<const AbstractDataObject>
   */
  virtual std::shared_ptr<const AbstractDataObject> getSharedData(const DataPath& path) const = 0;

  /**
   * @brief Removes the AbstractDataObject using the specified IdType.
   * @param identifier
   * @return bool
   */
  virtual bool removeData(AbstractDataObject::IdType identifier) = 0;

  /**
   * @brief Removes the AbstractDataObject using the specified IdType.
   * @param identifier
   * @return bool
   */
  virtual bool removeData(const std::optional<AbstractDataObject::IdType>& identifier) = 0;

  /**
   * @brief Removes the AbstractDataObject using the specified DataPath.
   * @param path
   * @return bool
   */
  virtual bool removeData(const DataPath& path) = 0;

  /**
   * @brief Returns a LinkedPath based on the specified DataPath.
   * @param path
   * @return LinkedPath
   */
  virtual LinkedPath getLinkedPath(const DataPath& path) const = 0;

  /**
   * @brief Creates the path in the data structure as a series of DataObjects.
   * @param path
   * @return Result<LinkedPath>
   */
  virtual Result<LinkedPath> makePath(const DataPath& path) = 0;

  /**
   * @brief Returns a vector of DataPaths for the AbstractDataObject with the specified ID.
   * @param identifier
   * @return std::vector<DataPath>
   */
  virtual std::vector<DataPath> getDataPathsForId(AbstractDataObject::IdType identifier) const = 0;

  /**
   * @brief Returns a collection of all DataPaths within the structure.
   * @return std::vector<DataPath>
   */
  virtual std::vector<DataPath> getAllDataPaths() const = 0;

  /**
   * @brief Returns a collection of all AbstractDataObject ids within the structure.
   * @return std::vector<AbstractDataObject::IdType>
   */
  virtual std::vector<AbstractDataObject::IdType> getAllDataObjectIds() const = 0;

  /**
   * @brief Returns the top-level of the DataStructure.
   * @return std::vector<AbstractDataObject*>
   */
  virtual std::vector<AbstractDataObject*> getTopLevelData() const = 0;

  /**
   * @brief Returns a reference to the DataMap backing the top level of the DataStructure.
   * @return const DataMap&
   */
  virtual const DataMap& getDataMap() const = 0;

  /**
   * @brief Inserts a new AbstractDataObject into the DataStructure nested under the given DataPath.
   * @param dataObject
   * @param dataPath
   * @return bool
   */
  virtual bool insert(const std::shared_ptr<AbstractDataObject>& dataObject, const DataPath& dataPath) = 0;

  /**
   * @brief Returns the next ID value to use in the DataStructure.
   * @return AbstractDataObject::IdType
   */
  virtual AbstractDataObject::IdType getNextId() const = 0;

  /**
   * @brief Adds an additional parent to the target AbstractDataObject.
   * @param targetId
   * @param newParent
   * @return bool
   */
  virtual bool setAdditionalParent(AbstractDataObject::IdType targetId, AbstractDataObject::IdType newParent) = 0;

  /**
   * @brief Removes a parent from the target AbstractDataObject.
   * @param targetId
   * @param parent
   * @return bool
   */
  virtual bool removeParent(AbstractDataObject::IdType targetId, AbstractDataObject::IdType parent) = 0;

  /**
   * @brief Returns an iterator for the beginning of the top-level DataMap.
   * @return iterator
   */
  virtual Iterator begin() = 0;

  /**
   * @brief Returns an iterator for the end of the top-level DataMap.
   * @return iterator
   */
  virtual Iterator end() = 0;

  /**
   * @brief Returns an iterator for the beginning of the top-level DataMap.
   * @return
   */
  virtual ConstIterator begin() const = 0;

  /**
   * @brief Returns an iterator for the end of the top-level DataMap.
   * @return
   */
  virtual ConstIterator end() const = 0;

  /**
   * @brief Checks if all IDataArrays at the target paths have the same tuple count.
   * @param dataPaths
   * @return bool
   */
  virtual nonstd::expected<void, std::string> validateNumberOfTuples(const std::vector<DataPath>& dataPaths) const = 0;

  /**
   * @brief Resets AbstractDataObject IDs starting at the provided value.
   * @param startingId
   */
  virtual void resetIds(AbstractDataObject::IdType startingId) = 0;

  /**
   * @brief Outputs data graph in .dot file format.
   * @param outputStream
   */
  virtual void exportHierarchyAsGraphViz(std::ostream& outputStream) const = 0;

  /**
   * @brief Outputs data graph in console readable format.
   * @param outputStream
   */
  virtual void exportHierarchyAsText(std::ostream& outputStream) const = 0;

  /**
   * @brief Sets the next ID to use when constructing a AbstractDataObject.
   * @param nextDataId
   */
  virtual void setNextId(AbstractDataObject::IdType nextDataId) = 0;

  /**
   * @brief Returns a reference to the root DataMap.
   * @return DataMap&
   */
  virtual DataMap& getRootGroup() = 0;

  /**
   * @brief Flushes all DataObjects to their respective target.
   */
  virtual void flush() const = 0;

  /**
   * @brief Returns the memory usage of the DataStructure.
   * @return uint64
   */
  virtual uint64 memoryUsage() const = 0;

  /**
   * @brief Transfers array data to OOC if available.
   * @return Result with warnings and errors
   */
  virtual Result<> transferDataArraysOoc() = 0;

  /**
   * @brief Validates that each Geometry is valid based on shared-list and
   * AttributeMatrix tuple count criteria.
   * @return Result<>
   */
  virtual Result<> validateGeometries() const = 0;

  /**
   * @brief Validates that each AttributeMatrix contains AbstractArray objects with
   * consistent tuple counts.
   * @return Result<>
   */
  virtual Result<> validateAttributeMatrices() const = 0;

protected:
  IDataStructure() = default;
  IDataStructure(const IDataStructure&) = default;
  IDataStructure(IDataStructure&&) = default;
  IDataStructure& operator=(const IDataStructure&) = default;
  IDataStructure& operator=(IDataStructure&&) noexcept = default;
};
} // namespace nx::core
