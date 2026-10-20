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
 * Only objects of the DataStructure being written enter that map, so companion
 * objects built in temporary structures never take over another object's links.
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
   * @return File-creation or write errors, -6056 when the file does not close completely (see FileIO::closeChecked()), a
   * file-write hook error, or success.
   * @post Replaces an existing file at filepath before creation.
   *
   * Forwards to the overload with options, which runs the file-write hook.
   */
  static Result<> WriteFile(const DataStructure& dataStructure, const std::filesystem::path& filepath);

  /**
   * @brief Writes a DataStructure to a new HDF5 file with options.
   * @param dataStructure Source data structure.
   * @param filepath Destination file path.
   * @param options Dataset encoding options.
   * @return File-creation or write errors, -6056 when the file does not close completely (see FileIO::closeChecked()), a
   * file-write hook error, or success.
   * @post Replaces an existing file at filepath before creation.
   *
   * The file-write hook (DataIOCollection::beforeFileWrite() in ReleaseReaders mode) runs on filepath before the file is
   * created, so out-of-core stores that read it are copied to session files first. A hook error is returned without a change to
   * filepath. The hook's warnings are part of every returned result.
   */
  static Result<> WriteFile(const DataStructure& dataStructure, const std::filesystem::path& filepath, const WriteOptions& options);

  /**
   * @brief Writes a DataStructure to an open HDF5 file with default options.
   * @param dataStructure Source data structure.
   * @param fileWriter Open destination file.
   * @return Write errors, or success.
   * @pre No out-of-core store in dataStructure reads the file that fileWriter holds open. This overload does not run the file-write
   * hook; the path-based overload that opens the file runs it.
   */
  static Result<> WriteFile(const DataStructure& dataStructure, FileIO& fileWriter);

  /**
   * @brief Writes a DataStructure to an open HDF5 file with options.
   * @param dataStructure Source data structure.
   * @param fileWriter Open destination file.
   * @param options Dataset encoding options.
   * @return Write errors, or success.
   * @pre No out-of-core store in dataStructure reads the file that fileWriter holds open. This overload does not run the file-write
   * hook; the path-based overload that opens the file runs it.
   */
  static Result<> WriteFile(const DataStructure& dataStructure, FileIO& fileWriter, const WriteOptions& options);

  /**
   * @brief Appends one top-level object to an existing HDF5 file with default options.
   * @param filepath Existing destination file.
   * @param dataStructure Source data structure.
   * @param dataPath Top-level object path.
   * @return Open, validation or write errors, -6056 when the file does not close completely (see FileIO::closeChecked()), a
   * file-write hook error, or success.
   *
   * Forwards to the overload with options, which runs the file-write hook.
   */
  static Result<> AppendFile(const std::filesystem::path& filepath, const DataStructure& dataStructure, const DataPath& dataPath);

  /**
   * @brief Opens an existing HDF5 file, appends one top-level object with options, and closes the file with a checked close.
   * @param filepath Existing destination file.
   * @param dataStructure Source data structure.
   * @param dataPath Top-level object path.
   * @param options Dataset encoding options.
   * @return Open, validation or write errors, -6056 when the file does not close completely (see FileIO::closeChecked()), a
   * file-write hook error, or success. With -6056 the file may already hold the appended object; the error does not promise a
   * rollback.
   *
   * The file-write hook (DataIOCollection::beforeFileWrite() in ReleaseReaders mode) runs on filepath before the file is opened,
   * so out-of-core stores that read it are copied to session files first; an HDF5 file that a store holds open read-only cannot
   * be opened for appending. A hook error is returned without opening the file. The hook's warnings are part of every returned
   * result.
   */
  static Result<> AppendFile(const std::filesystem::path& filepath, const DataStructure& dataStructure, const DataPath& dataPath, const WriteOptions& options);

  /**
   * @brief Appends one top-level object to an open HDF5 file with default options.
   * @param file Open destination file.
   * @param dataStructure Source data structure.
   * @param dataPath Top-level object path.
   * @return Validation or write errors.
   * @pre No out-of-core store in dataStructure reads the file that file holds open. This overload does not run the file-write hook;
   * the path-based overloads (and DREAM3D::AppendFile()) run it before they open the file.
   */
  static Result<> AppendFile(FileIO& file, const DataStructure& dataStructure, const DataPath& dataPath);

  /**
   * @brief Appends one top-level object to an open HDF5 file with options.
   * @param file Open destination file.
   * @param dataStructure Source data structure.
   * @param dataPath Top-level object path.
   * @param options Dataset encoding options.
   * @return Validation or write errors.
   * @pre dataPath has exactly one component.
   * @pre No out-of-core store in dataStructure reads the file that file holds open. This overload does not run the file-write hook;
   * the path-based overloads (and DREAM3D::AppendFile()) run it before they open the file.
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
   *
   * When no DataStructure is set (no writeDataStructure() call on this writer),
   * a call made from outside the writer writes the DataStructure that owns
   * dataObject: that structure is set for the duration of the call and cleared
   * when it returns. Nested calls keep the structure already set.
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
   * dataMap becomes the DataStructure this writer writes for the rest of its
   * lifetime; only its objects are recorded for hard links. A writer writes one
   * DataStructure, so dataMap must outlive every later call on this writer.
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

  /**
   * @brief Records the HDF5 path of a written object so later parents of that object receive hard links to it.
   * @param objectWriter HDF5 object the DataObject was written to; its object path is recorded.
   * @param dataObject DataObject that was written.
   * @return -6055 when the object's identifier is already recorded at a different path (the first path is kept), otherwise success.
   *
   * Only objects whose DataStructure is the one this writer writes are recorded. Objects built in
   * temporary structures, such as the NeighborList counts array, can share an identifier with an
   * object of the written structure; recording them would point that object's later links at the
   * wrong dataset. With no structure set, nothing is recorded. Recording the same path again succeeds.
   */
  [[nodiscard]] Result<> addWriter(ObjectIO& objectWriter, const DataObject& dataObject);

private:
  WriteOptions m_WriteOptions;
  /// DataStructure whose objects are recorded for hard links; set by writeDataStructure() or for one top-level writeDataObject() call.
  const DataStructure* m_WrittenStructure = nullptr;
  DataMapType m_IdMap;
  std::shared_ptr<DataIOManager> m_IOManager;
};

} // namespace HDF5
} // namespace nx::core
