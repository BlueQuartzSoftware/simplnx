#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/HDF5/IOUtilities.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <filesystem>
#include <map>
#include <string>

namespace nx::core
{
class DataStructure;
class DataFactoryManager;

namespace HDF5
{
class IDataFactory;
class DataIOManager;
class AbstractDataIO;
class ObjectIO;

/**
 * @brief The DataStructureWriter class serves to write DataStructures to HDF5 file or groups.
 */
class SIMPLNX_EXPORT DataStructureWriter
{
  friend class nx::core::AbstractDataObject;
  friend SIMPLNX_EXPORT Result<> WriteObjectAttributes(DataStructureWriter&, ObjectIO&, const AbstractDataObject*, bool);
  friend class HDF5::AbstractDataIO;

  using DataMapType = std::map<AbstractDataObject::IdType, std::string>;

public:
  DataStructureWriter();
  ~DataStructureWriter() noexcept;

  static Result<> WriteFile(const DataStructure& dataStructure, const std::filesystem::path& filepath);
  static Result<> WriteFile(const DataStructure& dataStructure, FileIO& fileWriter);
  static Result<> AppendFile(const std::filesystem::path& filepath, const DataStructure& dataStructure, const DataPath& dataPath);
  static Result<> AppendFile(FileIO& file, const DataStructure& dataStructure, const DataPath& dataPath);

  /**
   * @brief Writes the AbstractDataObject under the given GroupIO. If the
   * AbstractDataObject has already been written, a link is create instead.
   *
   * If the process encounters an error, the error code is returned. Otherwise,
   * this method returns 0.
   * @param dataObject
   * @param parentGroup
   * @return Result<>
   */
  Result<> writeDataObject(const AbstractDataObject* dataObject, GroupIO& parentGroup);

  /**
   * @brief Writes the provided dataMap to HDF5 group.
   * @param dataMap
   * @param parentGroup
   * @return Result<>
   */
  Result<> writeDataMap(const DataMap& dataMap, GroupIO& parentGroup);

  Result<> writeDataStructure(const DataStructure& dataMap, GroupIO& parentGroup);

protected:
  /**
   * @brief Writes a AbstractDataObject link under the given GroupIO.
   *
   * If the process encounters an error, the error code is returned. Otherwise,
   * this method returns 0.
   * @param dataObject
   * @param parentGroup
   * @return Result<>
   */
  Result<> writeDataObjectLink(const AbstractDataObject* dataObject, GroupIO& parentGroup);

  /**
   * @brief Returns true if the AbstractDataObject has been written to the current
   * file. Returns false otherwise.
   *
   * This will always return false if the target AbstractDataObject is null.
   * @param targetObject
   * @return bool
   */
  bool hasDataBeenWritten(const AbstractDataObject* targetObject) const;

  /**
   * @brief Returns true if the AbstractDataObject ID has been written to the current
   * file. Returns false otherwise.
   * @param targetObject
   * @return bool
   */
  bool hasDataBeenWritten(AbstractDataObject::IdType targetId) const;

  /**
   * @brief Returns the path to the HDF5 object for the provided
   * AbstractDataObject ID. Returns an empty string if no HDF5 writer could be found.
   * @param objectId
   * @return std::string
   */
  std::string getPathForObjectId(AbstractDataObject::IdType objectId) const;

  /**
   * @brief Returns the path to the HDF5 object for the provided
   * AbstractDataObject ID. Returns an empty string if no HDF5 writer could be found.
   * @param objectId
   * @return std::string
   */
  std::string getParentPathForObjectId(AbstractDataObject::IdType objectId) const;

  /**
   * @brief Returns the path to the HDF5 object for the specified
   * AbstractDataObject's sibling under the same parent. Returns an empty string if
   * no HDF5 writer could be found.
   * @param objectId
   * @param siblingName
   * @return std::string
   */
  std::string getPathForObjectSibling(AbstractDataObject::IdType objectId, const std::string& siblingName) const;

  /**
   * @brief Clears the AbstractDataObject to HDF5 ID map and resets the HDF5 parent ID.
   */
  void clearIdMap();

  /**
   * @brief Adds the nx::core::HDF5::ObjectWriter to the DataStructureWriter for the given AbstractDataObject ID
   * @param objectWriter
   * @param objectId
   */
  void addWriter(ObjectIO& objectWriter, AbstractDataObject::IdType objectId);

private:
  DataStructure m_DataStructure;
  DataMapType m_IdMap;
  std::shared_ptr<DataIOManager> m_IOManager;
};
} // namespace HDF5
} // namespace nx::core
