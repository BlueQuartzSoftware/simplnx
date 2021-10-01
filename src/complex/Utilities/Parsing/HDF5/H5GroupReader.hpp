#pragma once

#include "complex/Utilities/Parsing/HDF5/H5DatasetReader.hpp"

namespace complex
{
namespace H5
{
class COMPLEX_EXPORT GroupReader : public ObjectReader
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
  GroupReader(H5::IdType parentId, const std::string& groupName);

  /**
   * @brief Releases the wrapped HDF5 group.
   */
  virtual ~GroupReader();

  /**
   * @brief Returns the wrapped HDF5 group ID.
   * @return H5::IdType
   */
  H5::IdType getId() const override;

  /**
   * @brief Attempts to open a nested HDF5 group with the specified name.
   * @param name
   * @return GroupReader
   */
  GroupReader openGroup(const std::string& name) const;

  /**
   * @brief Attempts to open a nested HDF5 dataset with the specified name.
   * @param name
   * @return DatasetReader
   */
  DatasetReader openDataset(const std::string& name) const;

  /**
   * @brief Attempts to open a nested HDF5 object with the specified name.
   * @param name
   * @return ObjectReader
   */
  ObjectReader openObject(const std::string& name) const;

  /**
   * @brief Returns the number of children objects within the group.
   * @return size_t
   */
  size_t getNumChildren() const;

  /**
   * @brief Returns a vector with the names of each child object.
   * @return std::vector<std::string>
   */
  std::vector<std::string> getChildNames() const;

  /**
   * @brief Returns true if the target child is a group. Returns false otherwise.
   * @param childName
   * @return bool
   */
  bool isGroup(const std::string& childName) const;

  /**
   * @brief Returns true if the target child is a dataset. Returns false otherwise.
   * @param childName
   * @return bool
   */
  bool isDataset(const std::string& childName) const;

protected:
  /**
   * @brief Closes the HDF5 ID and resets it to 0.
   */
  void closeHdf5() override;

private:
  H5::IdType m_GroupId = 0;
};
} // namespace H5
} // namespace complex
