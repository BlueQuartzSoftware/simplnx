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
  /**
   * @brief The IdType alias serves as an ID type for DataObjects within their
   * respective DataStructure.
   */
  using IdType = uint64_t;

  /**
   * @brief The ParentCollectionType alias describes the format by which parent
   * collections are returned via public API.
   */
  using ParentCollectionType = std::list<BaseGroup*>;

  friend class BaseGroup;
  friend class DataMap;
  friend class DataStructure;

  /**
   * @brief Copy constructor
   * @param other
   */
  DataObject(const DataObject& other);

  /**
   * @brief Move constructor
   * @param other
   */
  DataObject(DataObject&& other) noexcept;

  virtual ~DataObject();

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
  ParentCollectionType getParents() const;

  /**
   * @brief Returns a vector of DataPaths.
   * @return std::vector<DataPath>
   */
  std::vector<DataPath> getDataPaths() const;

  /**
   * @brief Returns a pointer to the object's Metadata.
   * @return Metadata*
   */
  Metadata* getMetadata() const;

  /**
   * @brief Checks if the DataObject has an HDF5 ID assigned to it.
   * @return bool
   */
  bool hasH5Id() const;

  /**
   * @brief Returns the HDF5 ID used by the DataObject.
   * @return H5::IdType
   */
  H5::IdType getH5Id() const;

protected:
  /**
   * @brief DataObject constructor takes a pointer to the DataStructure and
   * object name.
   * @param ds
   * @param name
   */
  DataObject(DataStructure* ds, const std::string& name);

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

private:
  /**
   * @brief Generates an IdType for the DataObject constructor.
   * @param opId
   * @return IdType
   */
  static IdType generateId(const std::optional<IdType>& opId = {});

  ////////////
  // Variables
  DataStructure* m_DataStructure = nullptr;
  ParentCollectionType m_ParentList;
  IdType m_Id;
  std::string m_Name;
  std::unique_ptr<Metadata> m_Metadata = nullptr;
  H5::IdType m_H5Id;
};
} // namespace complex
