#pragma once

#include <filesystem>
#include <map>
#include <memory>

#include "complex/DataStructure/DataStructure.hpp"

namespace complex
{
namespace H5
{
class GroupWriter;

class DataStructureWriter
{
  using DataMapType = std::map<DataObject::IdType, std::shared_ptr<H5::ObjectWriter>>;

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
   * @return H5::ErrorType
   */
  H5::ErrorType writeDataObject(const DataObject* dataObject, GroupWriter& parentGroup);

protected:
  /**
   * @brief Writes a DataObject link under the given GroupWriter.
   * 
   * If the process encounters an error, the error code is returned. Otherwise,
   * this method returns 0.
   * @param dataObject
   * @param parentGroup
   * @return H5::ErrorType
  */
  H5::ErrorType writeDataObjectLink(const DataObject* dataObject, H5::GroupWriter& parentGroup);

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
   * @brief Returns a pointer to the H5::ObjectWriter for the provided
   * DataObject ID. Returns nullptr if no HDF5 writer could be found.
   * @param objectId
   * @return H5::IdType
   */
  std::shared_ptr<H5::ObjectWriter> getH5WriterForObjectId(DataObject::IdType objectId) const;

  /**
   * @brief Clears the DataObject to HDF5 ID map and resets the HDF5 parent ID.
   */
  void clearIdMap();

private:
  H5::IdType m_ParentId = 0;
  DataStructure m_DataStructure;
  DataMapType m_IdMap;
};
} // namespace H5
} // namespace complex
