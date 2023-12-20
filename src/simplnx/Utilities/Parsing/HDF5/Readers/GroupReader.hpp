#pragma once

#include "simplnx/Utilities/Parsing/HDF5/Readers/DatasetReader.hpp"

namespace nx::core::HDF5
{
class SIMPLNX_EXPORT GroupReader : public ObjectReader
{
public:
  /**
   * @brief Constructs an invalid GroupReader.
   */
  GroupReader();

  /**
   * @brief Opens and wraps an HDF5 group found within the specified parent.
   * @param parentId
   * @param groupName
   */
  GroupReader(IdType parentId, const std::string& groupName);

  /**
   * @brief Releases the wrapped HDF5 group.
   */
  virtual ~GroupReader() noexcept;

  /**
   * @brief Attempts to open a nested HDF5 group with the specified name.
   * The created GroupReader is returned. If the process fails, the returned
   * GroupReader is invalid.
   * @param name
   * @return GroupReader
   */
  GroupReader openGroup(const std::string& name) const;

  /**
   * @brief Attempts to open a nested HDF5 dataset with the specified name.
   * The created DatasetReader is returned. If the process fails, the returned
   * DatasetReader is invalid.
   * @param name
   * @return DatasetReader
   */
  DatasetReader openDataset(const std::string& name) const;

  /**
   * @brief Attempts to open a nested HDF5 object with the specified name.
   * The created ObjectReader is returned. If the process fails, the returned
   * ObjectReader is invalid.
   * @param name
   * @return ObjectReader
   */
  ObjectReader openObject(const std::string& name) const;

  /**
   * @brief Returns the number of children objects within the group.
   *
   * Returns 0 if the GroupReader is invalid.
   * @return size_t
   */
  size_t getNumChildren() const;

  /**
   * @brief Returns a vector with the names of each child object.
   *
   * This will return an empty vector if the GroupReader is invalid.
   * @return std::vector<std::string>
   */
  std::vector<std::string> getChildNames() const;

  /**
   * @brief Returns true if the target child is a group. Returns false
   * otherwise.
   *
   * This will always return false if the GroupReader is invalid.
   * @param childName
   * @return bool
   */
  bool isGroup(const std::string& childName) const;

  /**
   * @brief Returns true if the target child is a dataset. Returns false
   * otherwise.
   *
   * This will always return false if the GroupReader is invalid.
   * @param childName
   * @return bool
   */
  bool isDataset(const std::string& childName) const;

protected:
  /**
   * @brief Closes the HDF5 ID and resets it to 0.
   */
  void closeHdf5() override;

  /**
   * @brief Constructs a GroupReader for use in derived classes. This
   * constructor only accepts the parent ID and the (object) ID. Derived classes
   * are required to open their own target and provide the ID.
   * @param parentId
   * @param objectId
   */
  GroupReader(IdType parentId, IdType objectId);
};
} // namespace nx::core::HDF5
