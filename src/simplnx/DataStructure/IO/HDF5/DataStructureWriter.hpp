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
class IDataIO;
class ObjectIO;

/**
 * @brief The DataStructureWriter class serves to write DataStructures to HDF5 file or groups.
 */
class SIMPLNX_EXPORT DataStructureWriter
{
  friend class nx::core::DataObject;
  friend SIMPLNX_EXPORT Result<> WriteObjectAttributes(DataStructureWriter&, ObjectIO&, const DataObject*, bool);
  friend class HDF5::IDataIO;

  using DataMapType = std::map<DataObject::IdType, std::string>;

public:
  /**
   * @brief Default constructor.
   */
  DataStructureWriter();
  ~DataStructureWriter() noexcept;

  /**
   * @brief File-write options forwarded from higher layers (e.g. WriteDREAM3DFilter).
   *        Default-constructed instances produce contiguous, uncompressed datasets.
   */
  struct WriteOptions
  {
    /// Gzip/deflate level. 0 = off (contiguous). 1-9 = chunked + deflate at the given level.
    int32 compressionLevel = 0;
  };

  /**
   * @brief Returns the current write options (never null; default-constructed on creation).
   * @return const reference to the options struct.
   */
  const WriteOptions& getWriteOptions() const noexcept;

  /**
   * @brief Stores the given write options for use by the next WriteFile/AppendFile call.
   * @param options Struct describing how datasets should be encoded on disk.
   */
  void setWriteOptions(const WriteOptions& options) noexcept;

  /**
   * @brief Writes a DataStructure to an HDF5 file at the specified path.
   * @param dataStructure The DataStructure to write
   * @param filepath The file path to write to
   * @return Result<> Result with any errors or warnings
   */
  static Result<> WriteFile(const DataStructure& dataStructure, const std::filesystem::path& filepath);

  /**
   * @brief Writes a DataStructure to an HDF5 file at the specified path with the given options.
   * @param dataStructure The DataStructure to write
   * @param filepath The file path to write to
   * @param options Write options (e.g. compression level)
   * @return Result<> Result with any errors or warnings
   */
  static Result<> WriteFile(const DataStructure& dataStructure, const std::filesystem::path& filepath, const WriteOptions& options);

  /**
   * @brief Writes a DataStructure to an open HDF5 file.
   * @param dataStructure The DataStructure to write
   * @param fileWriter The HDF5 file writer to write to
   * @return Result<> Result with any errors or warnings
   */
  static Result<> WriteFile(const DataStructure& dataStructure, FileIO& fileWriter);

  /**
   * @brief Writes a DataStructure to an open HDF5 file with the given options.
   * @param dataStructure The DataStructure to write
   * @param fileWriter The HDF5 file writer to write to
   * @param options Write options (e.g. compression level)
   * @return Result<> Result with any errors or warnings
   */
  static Result<> WriteFile(const DataStructure& dataStructure, FileIO& fileWriter, const WriteOptions& options);

  /**
   * @brief Appends a DataObject at the specified path to an existing HDF5 file.
   * @param filepath The file path to append to
   * @param dataStructure The DataStructure containing the object to append
   * @param dataPath The path to the object to append
   * @return Result<> Result with any errors or warnings
   */
  static Result<> AppendFile(const std::filesystem::path& filepath, const DataStructure& dataStructure, const DataPath& dataPath);

  /**
   * @brief Appends a DataObject at the specified path to an existing HDF5 file with the given options.
   * @param filepath The file path to append to
   * @param dataStructure The DataStructure containing the object to append
   * @param dataPath The path to the object to append
   * @param options Write options (e.g. compression level)
   * @return Result<> Result with any errors or warnings
   */
  static Result<> AppendFile(const std::filesystem::path& filepath, const DataStructure& dataStructure, const DataPath& dataPath, const WriteOptions& options);

  /**
   * @brief Appends a DataObject at the specified path to an open HDF5 file.
   * @param file The HDF5 file to append to
   * @param dataStructure The DataStructure containing the object to append
   * @param dataPath The path to the object to append
   * @return Result<> Result with any errors or warnings
   */
  static Result<> AppendFile(FileIO& file, const DataStructure& dataStructure, const DataPath& dataPath);

  /**
   * @brief Appends a DataObject at the specified path to an open HDF5 file with the given options.
   * @param file The HDF5 file to append to
   * @param dataStructure The DataStructure containing the object to append
   * @param dataPath The path to the object to append
   * @param options Write options (e.g. compression level)
   * @return Result<> Result with any errors or warnings
   */
  static Result<> AppendFile(FileIO& file, const DataStructure& dataStructure, const DataPath& dataPath, const WriteOptions& options);

  /**
   * @brief Writes the DataObject under the given GroupIO. If the
   * DataObject has already been written, a link is created instead.
   *
   * Before using the normal type-factory write path, this method checks
   * two conditions in order:
   *
   * 1. **Deduplication** -- If the DataObject has already been written to
   *    this file, an HDF5 hard link is created instead of a duplicate copy.
   *
   * 2. **OOC recovery write** -- When built with SIMPLNX_USE_OOC and a
   *    recovery write is active, SimplnxOoc::maybeWriteRecoveryArray is given
   *    a chance to write OOC-backed arrays as lightweight placeholder
   *    datasets. If it declines (the object is not OOC-backed), the normal
   *    write path is used.
   *
   * If the process encounters an error, the error code is returned. Otherwise,
   * this method returns 0.
   * @param dataObject The DataObject to write
   * @param parentGroup The HDF5 group to write the object into
   * @return Result<>
   */
  Result<> writeDataObject(const DataObject* dataObject, GroupIO& parentGroup);

  /**
   * @brief Writes the provided dataMap to HDF5 group.
   * @param dataMap
   * @param parentGroup
   * @return Result<>
   */
  Result<> writeDataMap(const DataMap& dataMap, GroupIO& parentGroup);

  /**
   * @brief Writes an entire DataStructure to an HDF5 group.
   * @param dataMap The DataStructure to write
   * @param parentGroup The HDF5 group to write to
   * @return Result<> Result with any errors or warnings
   */
  Result<> writeDataStructure(const DataStructure& dataMap, GroupIO& parentGroup);

protected:
  /**
   * @brief Writes a DataObject link under the given GroupIO.
   *
   * If the process encounters an error, the error code is returned. Otherwise,
   * this method returns 0.
   * @param dataObject
   * @param parentGroup
   * @return Result<>
   */
  Result<> writeDataObjectLink(const DataObject* dataObject, GroupIO& parentGroup);

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
   * @brief Adds the nx::core::HDF5::ObjectWriter to the DataStructureWriter for the given DataObject ID
   * @param objectWriter
   * @param objectId
   */
  void addWriter(ObjectIO& objectWriter, DataObject::IdType objectId);

private:
  WriteOptions m_WriteOptions;
  DataStructure m_DataStructure;
  DataMapType m_IdMap;
  std::shared_ptr<DataIOManager> m_IOManager;
};
} // namespace HDF5
} // namespace nx::core
