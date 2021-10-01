#pragma once

#include "complex/Utilities/Parsing/HDF5/H5DatasetWriter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5ObjectWriter.hpp"

namespace complex
{
namespace H5
{
class COMPLEX_EXPORT GroupWriter : public ObjectWriter
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
  GroupWriter(H5::IdType parentId, const std::string& objectName);

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
   * @brief Returns the group's HDF5 ID. Returns 0 if the object is invalid.
   * @return H5::IdType
   */
  H5::IdType getId() const override;

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

protected:
  /**
   * @brief Closes the HDF5 ID and resets it to 0.
   */
  void closeHdf5();

private:
  H5::IdType m_GroupId;
};
} // namespace H5
} // namespace complex
