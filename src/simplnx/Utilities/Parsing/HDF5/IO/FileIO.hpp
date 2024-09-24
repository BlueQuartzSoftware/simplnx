#pragma once

#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"
#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Result.hpp"

#include "highfive/H5File.hpp"

#include <filesystem>
#include <string>

namespace nx::core::HDF5
{
class SIMPLNX_EXPORT FileIO : public GroupIO
{
public:
  static FileIO ReadFile(const std::filesystem::path& filepath);
  static FileIO WriteFile(const std::filesystem::path& filepath);

  FileIO() = default;

  /**
   * @brief Constructs a FileIO wrapping the HDF5 file at the target
   * filepath.
   * @param filepath
   * @param file
   */
  FileIO(const std::filesystem::path& filepath, HighFive::File&& file);

  FileIO(const FileIO& rhs) = delete;

  /**
   * @brief Move constructor.
   * @param rhs
   */
  FileIO(FileIO&& rhs) noexcept = default;

  FileIO& operator=(const FileIO& rhs) = delete;
  FileIO& operator=(FileIO&& rhs) noexcept = default;

  /**
   * @brief Releases the HDF5 file ID.
   */
  ~FileIO() noexcept;

  /**
   * @brief Returns the HDF5 file name. Returns an empty string if the file
   * is invalid.
   * @return std::string
   */
  std::string getName() const override;

  /**
   * Returns the HDF5 object path.
   * @return std::string
   */
  std::string getObjectPath() const override;

  /**
   * @brief Returns the IO Classes HighFive ObjectType
   * @return HighFive::ObjectType
   */
  HighFive::ObjectType getObjectType() const override;

  /**
   * @brief Deletes the attribute with the specified name;
   * @param name
   */
  void deleteAttribute(const std::string& name) override;

  /**
   * @brief Returns the number of attributes in the object. Returns 0 if the
   * object is not valid.
   * @return usize
   */
  usize getNumAttributes() const override;

  /**
   * @brief Returns a vector with each attribute name.
   * @return std::vector<std::string>
   */
  std::vector<std::string> getAttributeNames() const override;

  /**
   * @brief Returns the number of children objects within the group.
   *
   * Returns 0 if the GroupIO is invalid.
   * @return size_t
   */
  usize getNumChildren() const override;

  /**
   * @brief Returns a vector with the names of each child object.
   *
   * This will return an empty vector if the GroupIO is invalid.
   * @return std::vector<std::string>
   */
  std::vector<std::string> getChildNames() const override;

  /**
   * @brief Returns true if the target child is a group. Returns false
   * otherwise.
   *
   * This will always return false if the GroupIO is invalid.
   * @param childName
   * @return bool
   */
  bool isGroup(const std::string& childName) const override;

  /**
   * @brief Returns true if the target child is a dataset. Returns false
   * otherwise.
   *
   * This will always return false if the GroupIO is invalid.
   * @param childName
   * @return bool
   */
  bool isDataset(const std::string& childName) const override;

  /**
   * @brief Attempts to open a nested HDF5 group with the specified name.
   * The created GroupIO is returned. Returns an error result if the group
   * cannot be created.
   * @param name
   * @return GroupIO
   */
  Result<GroupIO> openGroup(const std::string& name) const override;

  std::shared_ptr<GroupIO> openGroupPtr(const std::string& name) const override;

  /**
   * @brief Creates a GroupIO for writing to a child group with the
   * target name. Returns an error result if the group cannot be
   * created.
   * @param childName
   * @return Result<GroupIO>
   */
  Result<GroupIO> createGroup(const std::string& childName) override;

  /**
   * @brief Creates a GroupIO for writing to a child group with the
   * target name. Returns an empty pointer if the group cannot be
   * created.
   * @param childName
   * @return std::shared_ptr<GroupIO>
   */
  std::shared_ptr<GroupIO> createGroupPtr(const std::string& childName) override;

  /**
   * @brief Returns the HighFive::File for the current IO handler.
   * Returns an empty optional if the file could not be determined.
   *
   * This method should only be called by simplnx HDF5 IO wrapper classes.
   * @return std::optional<HighFive::File>
   */
  std::optional<HighFive::File> h5File() const override;

  /**
   * @brief Opens and returns the HDF5 dataset with the given name.
   *
   * This method should only be called by simplnx HDF5 IO wrapper classes.
   * @param name
   * @return HighFive::DataSet
   */
  HighFive::DataSet openH5Dataset(const std::string& name) const override;

  /**
   * @brief Creates or opens an HDF5 dataset with the given name, dimensions, and datatype.
   *
   * This method should only be called by simplnx HDF5 IO wrapper classes.
   * @param name
   * @param dims
   * @param dataType
   * @return HighFive::DataSet
   */
  HighFive::DataSet createOrOpenH5Dataset(const std::string& name, const HighFive::DataSpace& dims, HighFive::DataType dataType) override;

  /**
   * @param Returns the HDF5 object id.
   * Should only be called by HDF5 IO wrapper classes.
   * @return hid_t
   */
  hid_t getH5Id() const override;

private:
  std::optional<HighFive::File> m_File;
};
} // namespace nx::core::HDF5
