#pragma once

#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"

#include <string>

namespace nx::core::HDF5
{
class SIMPLNX_EXPORT GroupIO : public ObjectIO
{
public:
  /**
   * @brief Constructs an invalid GroupIO.
   */
  GroupIO();

  /**
   * @brief Opens and wraps an HDF5 group found within the specified parent.
   * @param parentId
   * @param groupName
   */
  GroupIO(IdType parentId, const std::string& groupName);

  GroupIO(const GroupIO& other) = delete;
  GroupIO(GroupIO&& other) noexcept = default;

  /**
   * @brief Releases the wrapped HDF5 group.
   */
  virtual ~GroupIO() noexcept;

  /**
   * @brief Attempts to open a nested HDF5 group with the specified name.
   * The created GroupIO is returned. If the process fails, the returned
   * GroupIO is invalid.
   * @param name
   * @return GroupIO
   */
  GroupIO openGroup(const std::string& name) const;

  std::shared_ptr<GroupIO> openGroupPtr(const std::string& name) const;

  /**
   * @brief Attempts to open a nested HDF5 dataset with the specified name.
   * The created DatasetReader is returned. If the process fails, the returned
   * DatasetReader is invalid.
   * @param name
   * @return DatasetReader
   */
  DatasetIO openDataset(const std::string& name) const;

  std::shared_ptr<DatasetIO> openDatasetPtr(const std::string& name) const;

  /**
   * @brief Attempts to open a nested HDF5 object with the specified name.
   * The created ObjectIO is returned. If the process fails, the returned
   * ObjectIO is invalid.
   * @param name
   * @return ObjectIO
   */
  ObjectIO openObject(const std::string& name) const;

  /**
   * @brief Creates a GroupIO for writing to a child group with the
   * target name. Returns an invalid GroupIO if the group cannot be
   * created.
   * @param childName
   * @return GroupIO
   */
  GroupIO createGroup(const std::string& childName);

  std::shared_ptr<GroupIO> createGroupPtr(const std::string& childName);

  /**
   * @brief Opens a DatasetIO for writing to a child group with the
   * target name. Returns an invalid DatasetIO if the dataset cannot be
   * created.
   * @param childName
   * @return DatasetIO
   */
  DatasetIO openDataset(const std::string& childName);

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
  size_t getNumChildren() const;

  /**
   * @brief Returns a vector with the names of each child object.
   *
   * This will return an empty vector if the GroupIO is invalid.
   * @return std::vector<std::string>
   */
  std::vector<std::string> getChildNames() const;

  /**
   * @brief Returns true if the target child is a group. Returns false
   * otherwise.
   *
   * This will always return false if the GroupIO is invalid.
   * @param childName
   * @return bool
   */
  bool isGroup(const std::string& childName) const;

  /**
   * @brief Returns true if the target child is a dataset. Returns false
   * otherwise.
   *
   * This will always return false if the GroupIO is invalid.
   * @param childName
   * @return bool
   */
  bool isDataset(const std::string& childName) const;

  GroupIO& operator=(const GroupIO& other) = delete;
  GroupIO& operator=(GroupIO&& other) noexcept = default;

protected:
  /**
   * @brief Constructs a GroupWriter for use in derived classes. This
   * constructor only accepts the parent ID and the (object) ID. Derived classes
   * are required to open their own target and provide the ID.
   * @param parentId
   * @param objectId
   */
  GroupIO(IdType parentId, IdType objectId);

  /**
   * @brief Closes the HDF5 ID and resets it to 0.
   */
  void closeHdf5() override;
};

// -----------------------------------------------------------------------------
// Declare our extern templates
} // namespace nx::core::HDF5
