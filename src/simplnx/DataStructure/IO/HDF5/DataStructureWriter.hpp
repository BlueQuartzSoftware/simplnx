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
 * @class DataStructureWriter
 * @brief Serializes a DataStructure to HDF5 files and groups.
 *
 * The writer maps object identifiers to HDF5 paths. It uses hard links for
 * repeated objects and lets registered I/O managers override recovery writes.
 */
class SIMPLNX_EXPORT DataStructureWriter
{
  friend class nx::core::DataObject;
  friend SIMPLNX_EXPORT Result<> WriteObjectAttributes(DataStructureWriter&, ObjectIO&, const DataObject*, bool);
  friend class HDF5::IDataIO;

  using DataMapType = std::map<DataObject::IdType, std::string>;

public:
  DataStructureWriter();

  ~DataStructureWriter() noexcept;

  /**
   * @struct WriteOptions
   * @brief Defines HDF5 dataset encoding options.
   */
  struct WriteOptions
  {
    /**
     * @brief Requests an HDF5 deflate level.
     *
     * Zero does not request deflate. Levels one through nine request deflate.
     * Small arrays can still use the default contiguous layout.
     */
    int32 compressionLevel = 0;
  };

  /**
   * @brief Returns the configured write options.
   * @return Options reference owned by this writer.
   *
   * The reference remains valid until setWriteOptions() or writer destruction.
   */
  const WriteOptions& getWriteOptions() const noexcept;

  void setWriteOptions(const WriteOptions& options) noexcept;

  /**
   * @brief Writes a DataStructure to a new HDF5 file.
   * @param dataStructure Source data structure.
   * @param filepath Destination file path.
   * @return File-creation or write errors.
   * @post Replaces an existing file at filepath before creation.
   */
  static Result<> WriteFile(const DataStructure& dataStructure, const std::filesystem::path& filepath);

  /**
   * @brief Writes a DataStructure to a new HDF5 file with options.
   * @param dataStructure Source data structure.
   * @param filepath Destination file path.
   * @param options Dataset encoding options.
   * @return File-creation or write errors.
   * @post Replaces an existing file at filepath before creation.
   */
  static Result<> WriteFile(const DataStructure& dataStructure, const std::filesystem::path& filepath, const WriteOptions& options);

  static Result<> WriteFile(const DataStructure& dataStructure, FileIO& fileWriter);

  static Result<> WriteFile(const DataStructure& dataStructure, FileIO& fileWriter, const WriteOptions& options);

  static Result<> AppendFile(const std::filesystem::path& filepath, const DataStructure& dataStructure, const DataPath& dataPath);

  static Result<> AppendFile(const std::filesystem::path& filepath, const DataStructure& dataStructure, const DataPath& dataPath, const WriteOptions& options);

  static Result<> AppendFile(FileIO& file, const DataStructure& dataStructure, const DataPath& dataPath);

  /**
   * @brief Appends one top-level object to an open HDF5 file with options.
   * @param file Open destination file.
   * @param dataStructure Source data structure.
   * @param dataPath Top-level object path.
   * @param options Dataset encoding options.
   * @return Validation or write errors.
   * @pre dataPath has exactly one component.
   *
   * The method copies only dataPath, renumbers it from the file's next object
   * identifier, and advances the file attribute before writing.
   */
  static Result<> AppendFile(FileIO& file, const DataStructure& dataStructure, const DataPath& dataPath, const WriteOptions& options);

  /**
   * @brief Writes an object under an HDF5 group.
   * @param dataObject Object to write.
   * @param parentGroup Destination HDF5 group.
   * @return Write or recovery-override errors.
   * @pre dataObject is non-null.
   *
   * A repeated object becomes an HDF5 hard link. Recovery writers can replace
   * disk-backed arrays with placeholder datasets and backing metadata. The
   * normal type factory writes objects that no override handles.
   */
  Result<> writeDataObject(const DataObject* dataObject, GroupIO& parentGroup);

  Result<> writeDataMap(const DataMap& dataMap, GroupIO& parentGroup);

  /**
   * @brief Writes a data structure below an HDF5 group.
   * @param dataMap Source data structure.
   * @param parentGroup Destination HDF5 group.
   * @return Group validation or write errors.
   *
   * The writer stores the next object identifier before writing the root map.
   */
  Result<> writeDataStructure(const DataStructure& dataMap, GroupIO& parentGroup);

protected:
  /**
   * @brief Writes a hard link to an existing object path.
   * @param dataObject Object with a mapped HDF5 path.
   * @param parentGroup Destination HDF5 group.
   * @return Link-creation errors.
   * @pre dataObject has a path in this writer's identifier map.
   *
   * NeighborList links also require their NumNeighbors companion link.
   */
  Result<> writeDataObjectLink(const DataObject* dataObject, GroupIO& parentGroup);

  bool hasDataBeenWritten(const DataObject* targetObject) const;

  bool hasDataBeenWritten(DataObject::IdType targetId) const;

  std::string getPathForObjectId(DataObject::IdType objectId) const;

  /**
   * @brief Returns the parent path for a written object identifier.
   * @param objectId Object identifier.
   * @return Parent HDF5 path, an empty string when absent, or the full stored
   * path when that path has no slash.
   * @pre The stored path contains a slash when a parent path is required.
   */
  std::string getParentPathForObjectId(DataObject::IdType objectId) const;

  std::string getPathForObjectSibling(DataObject::IdType objectId, const std::string& siblingName) const;

  void clearIdMap();

  void addWriter(ObjectIO& objectWriter, DataObject::IdType objectId);

private:
  WriteOptions m_WriteOptions;
  DataStructure m_DataStructure;
  DataMapType m_IdMap;
  std::shared_ptr<DataIOManager> m_IOManager;
};

} // namespace HDF5
} // namespace nx::core
