#pragma once

#include <iostream>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "complex/DataStructure/Metadata.hpp"
#include "complex/Utilities/TooltipGenerator.hpp"
#include "complex/Utilities/Parsing/HDF5/H5.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class BaseGroup;
class DataPath;
class DataStructure;

/**
 * class DataObject
 *
 */
class COMPLEX_EXPORT DataObject
{
public:
  using IdType = size_t;
  using ParentCollectionType = std::list<BaseGroup*>;

  friend class BaseGroup;
  friend class DataMap;
  friend class DataStructure;

  /**
   * @brief
   * @param other
   */
  DataObject(const DataObject& other);

  /**
   * @brief
   * @param other
   */
  DataObject(DataObject&& other) noexcept;

  /**
   * @brief
   */
  virtual ~DataObject();

  /**
   * @brief
   * @return DataObject*
   */
  virtual DataObject* deepCopy() = 0;

  /**
   * @brief
   * @return DataObject*
   */
  virtual DataObject* shallowCopy() = 0;

  /**
   * @brief
   * @return IdType
   */
  IdType getId() const;

  /**
   * @brief
   * @return DataStructure*
   */
  DataStructure* getDataStructure();

  /**
   * @brief
   * @return DataStructure*
   */
  const DataStructure* getDataStructure() const;

  /**
   * @brief
   * @return std::string
   */
  std::string getName() const;

  /**
   * @brief
   * @param name
   * @return bool
   */
  bool canRename(const std::string& name) const;

  /**
   * @brief
   * @param name
   * @return bool
   */
  bool rename(const std::string& name);

  /**
   * @brief
   * @return ParentCollectionType
   */
  ParentCollectionType getParents() const;

  /**
   * @brief
   * @return
   */
  std::vector<DataPath> getDataPaths() const;

  /**
   * @brief
   * @return Metadata*
   */
  Metadata* getMetadata() const;

  /**
   * @brief Writes the DataObject to the specified HDF5 file
   * @param out
   * @param hdfFileName
   * @return H5::ErrorType
   */
  virtual H5::ErrorType generateXdmfText(std::ostream& out, const std::string& hdfFileName) const = 0;

  /**
   * @brief
   * @param in
   * @param hdfFileName
   * @return H5::ErrorType
   */
  virtual H5::ErrorType readFromXdmfText(std::istream& in, const std::string& hdfFileName) = 0;

  /**
   * @brief Checks if the DataObject has an HDF5 ID assigned to it.
   * @return
   */
  bool hasH5Id() const;

  /**
   * @brief Returns the HDF5 ID used by the DataObject.
   * @return
   */
  H5::IdType getH5Id() const;

protected:
  /**
   * @brief
   * @param name
   */
  DataObject(DataStructure* ds, const std::string& name);

  /**
   * @brief
   * @param parent
   */
  void addParent(BaseGroup* parent);

  /**
   * @brief
   * @param parent
   */
  void removeParent(BaseGroup* parent);

  /**
   * @brief
   * @param oldParent
   * @param newParent
   */
  void replaceParent(BaseGroup* oldParent, BaseGroup* newParent);

  /**
   * @brief
   * @param ds
   */
  virtual void setDataStructure(DataStructure* ds);

private:
  /**
   * @brief
   * @param opId
   * @return
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
