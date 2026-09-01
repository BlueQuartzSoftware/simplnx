#pragma once

#include "simplnx/Utilities/Parsing/HDF5/IO/ObjectIO.hpp"

#include <H5Gpublic.h>
#include <H5Ppublic.h>

#include <H5Gpublic.h>
#include <H5Ppublic.h>

#include <fmt/format.h>

#include <string>

namespace nx::core::HDF5
{
class FileIO;
class DatasetIO;

/**
 * @class GroupIO
 * @brief Owns or lazily opens one move-only HDF5 group identifier.
 *
 * Methods serialize HDF5 calls with the process-wide Support::ApiLock(). The
 * lock is non-recursive, so methods resolve operations that can acquire the
 * lock before they enter a leaf HDF5 critical section. Concurrent access to
 * the same GroupIO object is not supported.
 */
class SIMPLNX_EXPORT GroupIO : public ObjectIO
{
public:
  /**
   * @brief Opens a group from an HDF5 file.
   * @param filepath Identifies the HDF5 file.
   * @param objectPath Identifies the group in the file.
   * @return Owning group wrapper, or null when the group cannot be opened.
   */
  static std::shared_ptr<GroupIO> Open(const std::filesystem::path& filepath, const std::string& objectPath);

  /**
   * @brief Constructs an invalid GroupIO.
   */
  GroupIO();

  GroupIO(const GroupIO& other) = delete;
  GroupIO(GroupIO&& other) noexcept = default;

  GroupIO& operator=(const GroupIO& other) = delete;
  GroupIO& operator=(GroupIO&& other) noexcept = default;

  /**
   * @brief Releases the wrapped HDF5 group.
   */
  ~GroupIO() noexcept override;

  /**
   * @brief Opens an existing child group.
   * @param name Identifies the child group.
   * @return Owning child wrapper, or an invalid wrapper on failure.
   */
  GroupIO openGroup(const std::string& name) const;

  /**
   * @brief Gets a lazy wrapper for an existing child dataset.
   * @param name Identifies the child dataset.
   * @return Dataset wrapper, or an invalid wrapper when the child is not a dataset.
   */
  DatasetIO openDataset(const std::string& name) const;

  /**
   * @brief Opens an existing child group or creates a missing child group.
   * @param childName Identifies the child group.
   * @return Owning child wrapper, or an invalid wrapper on failure.
   *
   * An existing child of another HDF5 object type causes failure.
   */
  GroupIO createGroup(const std::string& childName);

  /**
   * @brief Gets a lazy writer for an existing or missing child dataset.
   * @param childName Identifies the child dataset.
   * @return Dataset wrapper, or an invalid wrapper when an existing child has another type.
   *
   * This method does not create the physical dataset. A later dataset write
   * opens an existing dataset or creates a missing dataset with its type and shape.
   */
  DatasetIO openDataset(const std::string& childName);

  /**
   * @brief Gets a lazy wrapper for an existing child dataset.
   * @param childName Identifies the child dataset.
   * @return Dataset wrapper, or null when the child is not a dataset.
   */
  std::shared_ptr<DatasetIO> openDatasetPtr(const std::string& childName) const;

  /**
   * @brief Opens an existing child group.
   * @param childName Identifies the child group.
   * @return Owning child wrapper, or null on failure.
   */
  std::shared_ptr<GroupIO> openGroupPtr(const std::string& childName) const;

  /**
   * @brief Gets a lazy writer for a child dataset.
   * @param childName Identifies the child dataset.
   * @return Dataset wrapper, or an invalid wrapper when this group is invalid.
   *
   * The physical dataset is created by a later write operation.
   */
  DatasetIO createDataset(const std::string& childName);

  /**
   * @brief Gets a shared lazy writer for a child dataset.
   * @param childName Identifies the child dataset.
   * @return Dataset wrapper, or null when this group is invalid.
   *
   * The physical dataset is created by a later write operation.
   */
  std::shared_ptr<DatasetIO> createDatasetPtr(const std::string& childName);

  /**
   * @brief Creates a hard link in this group to an object under the parent identifier.
   * @param objectPath Identifies the source object. Its final path component becomes the link name.
   * @return Valid result on success. Returns an error for an empty path or an HDF5 failure.
   */
  Result<> createLink(const std::string& objectPath);

  /**
   * @brief Gets the number of child objects.
   * @return Child count, or zero when this wrapper is invalid.
   */
  virtual usize getNumChildren() const;

  /**
   * @brief Gets the names of all child objects.
   * @return Child names, or an empty vector when this wrapper is invalid.
   */
  virtual std::vector<std::string> getChildNames() const;

  /**
   * @brief Gets one child name by its HDF5 index.
   * @param idx Selects a child in the range [0, getNumChildren()).
   * @return Child name.
   * @pre This wrapper is valid, the index is in range, and the name has at most 1023 bytes.
   *
   * The current API does not report HDF5 query failures or truncated names.
   */
  virtual std::string getChildNameByIdx(hsize_t idx) const;

  /**
   * @brief Tests whether a child is an HDF5 group.
   * @param childName Identifies the child.
   * @return True when the child is a group. Returns false for invalid wrappers and query failures.
   */
  virtual bool isGroup(const std::string& childName) const;

  /**
   * @brief Tests whether a child is an HDF5 dataset.
   * @param childName Identifies the child.
   * @return True when the child is a dataset. Returns false for invalid wrappers and query failures.
   */
  virtual bool isDataset(const std::string& childName) const;

  /**
   * @brief Tests whether a supported child object exists.
   * @param childName Identifies the child.
   * @return True for groups and datasets. Returns false for named datatypes and query failures.
   */
  bool exists(const std::string& childName) const;

  /**
   * @brief Gets the supported HDF5 object type of a child.
   * @param childName Identifies the child.
   * @return Group or Dataset when detected. Returns Unknown for other types and query failures.
   */
  ObjectType getObjectType(const std::string& childName) const;

protected:
  /**
   * @brief Takes ownership of an open HDF5 group identifier.
   * @param parentId Identifies the parent object. It must outlive this wrapper.
   * @param groupName Stores the child name relative to the parent.
   * @param groupId Supplies the identifier to close during destruction.
   */
  GroupIO(hid_t parentId, const std::string& groupName, hid_t groupId);

  /**
   * @brief Opens this group when it is not open.
   * @return Open group identifier, or a negative HDF5 identifier on failure.
   */
  hid_t open() const override;

  /**
   * @brief Closes the owned group identifier when it is open.
   */
  void close() override;

private:
};
} // namespace nx::core::HDF5
