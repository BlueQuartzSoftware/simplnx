#pragma once

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/IO/HDF5/IOUtilities.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

namespace complex
{
class DataFactoryManager;

namespace HDF5
{
class IDataFactory;
class DataIOManager;
class IDataIO;

/**
 * @brief The DataStructureWriter class serves to write DataStructures to HDF5 file or groups.
 */
class COMPLEX_EXPORT DataStructureWriter
{
  friend class complex::DataObject;
  friend Result<> complex::HDF5::WriteObjectAttributes(DataStructureWriter&, H5::ObjectWriter&, const DataObject*, bool);
  friend class complex::HDF5::IDataIO;

  using DataMapType = std::map<DataObject::IdType, std::string>;

public:
  DataStructureWriter();
  ~DataStructureWriter() noexcept;

  /**
   * @brief Writes the DataObject under the given GroupWriter. If the
   * DataObject has already been written, a link is create instead.
   *
   * If the process encounters an error, the error code is returned. Otherwise,
   * this method returns 0.
   * @param dataObject
   * @param parentGroup
   * @return Result<>
   */
  Result<> writeDataObject(const DataObject* dataObject, H5::GroupWriter& parentGroup);

  /**
   * @brief Writes the provided dataMap to HDF5 group.
   * @param dataMap
   * @param parentGroup
   * @return Result<>
   */
  Result<> writeDataMap(const DataMap& dataMap, H5::GroupWriter& parentGroup);

protected:
  /**
   * @brief Writes a DataObject link under the given GroupWriter.
   *
   * If the process encounters an error, the error code is returned. Otherwise,
   * this method returns 0.
   * @param dataObject
   * @param parentGroup
   * @return Result<>
   */
  Result<> writeDataObjectLink(const DataObject* dataObject, H5::GroupWriter& parentGroup);

  /**
   * @brief Returns true if the DataObject has been written to the current
   * file. Returns false otherwise.
   *
   * This will always return false if the target DataObject is null.
   * @param targetObject
   * @return bool
   */
  bool hasDataBeenWritten(const DataObject* targetObject) const;

  /**
   * @brief Returns true if the DataObject ID has been written to the current
   * file. Returns false otherwise.
   * @param targetObject
   * @return bool
   */
  bool hasDataBeenWritten(DataObject::IdType targetId) const;

  /**
   * @brief Returns the path to the HDF5 object for the provided
   * DataObject ID. Returns an empty string if no HDF5 writer could be found.
   * @param objectId
   * @return std::string
   */
  std::string getPathForObjectId(DataObject::IdType objectId) const;

  /**
   * @brief Returns the path to the HDF5 object for the provided
   * DataObject ID. Returns an empty string if no HDF5 writer could be found.
   * @param objectId
   * @return std::string
   */
  std::string getParentPathForObjectId(DataObject::IdType objectId) const;

  /**
   * @brief Returns the path to the HDF5 object for the specified
   * DataObject's sibling under the same parent. Returns an empty string if
   * no HDF5 writer could be found.
   * @param objectId
   * @param siblingName
   * @return std::string
   */
  std::string getPathForObjectSibling(DataObject::IdType objectId, const std::string& siblingName) const;

  /**
   * @brief Clears the DataObject to HDF5 ID map and resets the HDF5 parent ID.
   */
  void clearIdMap();

  /**
   * @brief Adds the H5::ObjectWriter to the DataStructureWriter for the given DataObject ID
   * @param objectWriter
   * @param objectId
   */
  void addWriter(H5::ObjectWriter& objectWriter, DataObject::IdType objectId);

private:
  H5::IdType m_ParentId = 0;
  DataStructure m_DataStructure;
  DataMapType m_IdMap;
  std::shared_ptr<DataIOManager> m_IOManager;
};
} // namespace HDF5
} // namespace complex
