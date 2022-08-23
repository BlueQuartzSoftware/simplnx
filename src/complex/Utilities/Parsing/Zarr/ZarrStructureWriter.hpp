#pragma once

#include <filesystem>
#include <map>
#include <memory>

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/Parsing/Zarr/Zarr.hpp"

namespace complex
{
namespace Zarr
{
class COMPLEX_EXPORT DataStructureWriter
{
  friend class complex::DataObject;

  using DataMapType = std::map<DataObject::IdType, std::filesystem::path>;

public:
  /**
   * @brief Constructs an empty DataStructureWriter.
   */
  DataStructureWriter();

  /**
   * @brief Default destructor.
   */
  virtual ~DataStructureWriter();

  /**
   * @brief Writes the DataObject under the given GroupWriter. If the
   * DataObject has already been written, a link is create instead.
   *
   * If the process encounters an error, the error code is returned. Otherwise,
   * this method returns 0.
   * @param dataObject
   * @param parentId
   * @return ErrorType
   */
  ErrorType writeDataObject(const DataObject* dataObject, FileVec::IGroup& parentGroup);

protected:
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
   * @return std::filesystem::path
   */
  std::filesystem::path getPathForObjectId(DataObject::IdType objectId) const;

  /**
   * @brief Returns the path to the HDF5 object for the provided
   * DataObject ID. Returns an empty string if no HDF5 writer could be found.
   * @param objectId
   * @return std::filesystem::path
   */
  std::filesystem::path getParentPathForObjectId(DataObject::IdType objectId) const;

  /**
   * @brief Returns the path to the HDF5 object for the specified
   * DataObject's sibling under the same parent. Returns an empty string if
   * no HDF5 writer could be found.
   * @param objectId
   * @param siblingName
   * @return std::filesystem::path
   */
  std::filesystem::path getPathForObjectSibling(DataObject::IdType objectId, const std::string& siblingName) const;

  /**
   * @brief Clears the DataObject to HDF5 ID map and resets the HDF5 parent ID.
   */
  void clearIdMap();

  /**
   * @brief Adds the H5::ObjectWriter to the DataStructureWriter for the given DataObject ID
   * @param objectWriter
   * @param objectId
   */
  void addZarrWriter(FileVec::BaseCollection& objectWriter, DataObject::IdType objectId);

private:
  DataStructure m_DataStructure;
  DataMapType m_IdMap;
};
} // namespace Zarr
} // namespace complex
