#pragma once

#include "simplnx/Utilities/Parsing/HDF5/IO/ObjectIO.hpp"

#include <H5Gpublic.h>
#include <H5Ppublic.h>

#include "fmt/format.h"

#include <string>

namespace nx::core::HDF5
{
class FileIO;
class DatasetIO;

class SIMPLNX_EXPORT GroupIO : public ObjectIO
{
public:
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
   * @brief Attempts to open a nested HDF5 group with the specified name.
   * The created GroupIO is returned. If the process fails, the returned
   * GroupIO is invalid.
   * @param name
   * @return GroupIO
   */
  GroupIO openGroup(const std::string& name) const;

  /**
   * @brief Attempts to open a nested HDF5 dataset with the specified name.
   * The created DatasetReader is returned. If the process fails, the returned
   * DatasetReader is invalid.
   * @param name
   * @return DatasetReader
   */
  DatasetIO openDataset(const std::string& name) const;

  /**
   * @brief Creates a GroupIO for writing to a child group with the
   * target name. Returns an invalid GroupIO if the group cannot be
   * created.
   * @param childName
   * @return GroupIO
   */
  GroupIO createGroup(const std::string& childName);

  /**
   * @brief Opens a DatasetIO for writing to a child group with the
   * target name. Returns an invalid DatasetIO Result if the dataset cannot be
   * created.
   * @param childName
   * @return DatasetIO
   */
  DatasetIO openDataset(const std::string& childName);

  /**
   * @brief Opens a DatasetIO for writing to a child group with the
   * target name. Returns a null pointer if the dataset cannot be
   * created.
   * @param childName
   * @return std::shared_ptr<DatasetIO>
   */
  std::shared_ptr<DatasetIO> openDatasetPtr(const std::string& childName);

  /**
   * @brief Creates a DatasetIO for writing to a child group with the
   * target name. Returns an invalid DatasetIO if the dataset cannot be
   * created.
   * @param childName
   * @return DatasetIO
   */
  DatasetIO createDataset(const std::string& childName);

  std::shared_ptr<DatasetIO> createDatasetPtr(const std::string& childName);

  /**
   * @brief Creates a link within the group to another HDF5 object specified
   * by an HDF5 object path.
   * Returns an error code if one occurs. Otherwise, this method returns 0.
   * @param objectPath
   * @return Result<>
   */
  Result<> createLink(const std::string& objectPath);

  /**
   * @brief Returns the number of children objects within the group.
   *
   * Returns 0 if the GroupIO is invalid.
   * @return size_t
   */
  virtual usize getNumChildren() const;

  /**
   * @brief Returns a vector with the names of each child object.
   *
   * This will return an empty vector if the GroupIO is invalid.
   * @return std::vector<std::string>
   */
  virtual std::vector<std::string> getChildNames() const;

  virtual std::string getChildNameByIdx(hsize_t idx) const;

  /**
   * @brief Returns true if the target child is a group. Returns false
   * otherwise.
   *
   * This will always return false if the GroupIO is invalid.
   * @param childName
   * @return bool
   */
  virtual bool isGroup(const std::string& childName) const;

  /**
   * @brief Returns true if the target child is a dataset. Returns false
   * otherwise.
   *
   * This will always return false if the GroupIO is invalid.
   * @param childName
   * @return bool
   */
  virtual bool isDataset(const std::string& childName) const;

  bool exists(const std::string& childName) const;

  ObjectType getObjectType(const std::string& childName) const;

protected:
  /**
   * @brief Opens and wraps an HDF5 group.
   * @param filepath
   * @param groupPath
   */
  GroupIO(hid_t parentId, const std::string& groupName, hid_t groupId);

  hid_t open() const override;
  void close() override;

private:
};

// -----------------------------------------------------------------------------
// Declare our extern templates
} // namespace nx::core::HDF5
