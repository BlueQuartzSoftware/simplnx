#pragma once

#include <memory>

#include "simplnx/Utilities/Parsing/HDF5/Writers/DatasetWriter.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Writers/ObjectWriter.hpp"

namespace nx::core::HDF5
{
class SIMPLNX_EXPORT GroupWriter : public ObjectWriter
{
public:
  /**
   * @brief Constructs an invalid GroupWriter.
   */
  GroupWriter();

  /**
   * @brief Constructs a GroupWriter that wraps a target HDF5 group. If no
   * group exists, one is created for the caller to write to. If creating an
   * HDF5 group fails, this writer is invalid.
   * @param parentId
   * @param objectName
   */
  GroupWriter(IdType parentId, const std::string& objectName);

  GroupWriter(const GroupWriter& other) = delete;
  GroupWriter(GroupWriter&& other) noexcept = default;

  /**
   * @brief Closes the HDF5 group.
   */
  virtual ~GroupWriter();

  /**
   * @brief Returns true if the GroupWriter is valid. Returns false otherwise.
   * @return bool
   */
  bool isValid() const override;

  /**
   * @brief Creates a GroupWriter for writing to a child group with the
   * target name. Returns an invalid GroupWriter if the group cannot be
   * created.
   * @param childName
   * @return GroupWriter
   */
  GroupWriter createGroupWriter(const std::string& childName);

  /**
   * @brief Creates a DatasetWriter for writing to a child group with the
   * target name. Returns an invalid DatasetWriter if the dataset cannot be
   * created.
   * @param childName
   * @return DatasetWriter
   */
  DatasetWriter createDatasetWriter(const std::string& childName);

  /**
   * @brief Creates a link within the group to another HDF5 object specified
   * by an HDF5 object path.
   * Returns an error code if one occurs. Otherwise, this method returns 0.
   * @param objectPath
   * @return Result<>
   */
  Result<> createLink(const std::string& objectPath);

  GroupWriter& operator=(const GroupWriter& other) = delete;
  GroupWriter& operator=(GroupWriter&& other) noexcept = default;

protected:
  /**
   * @brief Closes the HDF5 ID and resets it to 0.
   */
  void closeHdf5() override;

  /**
   * @brief Constructs a GroupWriter for use in derived classes. This
   * constructor only accepts the parent ID and the (object) ID. Derived classes
   * are required to open their own target and provide the ID.
   * @param parentId
   * @param objectId
   */
  GroupWriter(IdType parentId, IdType objectId);
};
} // namespace nx::core::HDF5
