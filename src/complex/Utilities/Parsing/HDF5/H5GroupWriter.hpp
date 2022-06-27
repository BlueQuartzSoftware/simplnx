#pragma once

#include <memory>

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
   * @return H5::ErrorType
   */
  H5::ErrorType createLink(const std::string& objectPath);

protected:
  /**
   * @brief Closes the HDF5 ID and resets it to 0.
   */
  void closeHdf5() override;

  /**
   * @brief Constructs a GroupWriter for use in derived classes. This
   * constructor only accepts the parent ID and the (object) ID. Derived classes are required to
   * open their own target and provide the ID.
   * @param parentId
   * @param objectId
   */
  GroupWriter(H5::IdType parentId, H5::IdType objectId);
};
} // namespace H5
} // namespace complex
