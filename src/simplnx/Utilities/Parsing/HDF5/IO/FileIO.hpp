#pragma once

#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"
#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Result.hpp"

//#include "highfive/H5File.hpp"
#include <H5Fpublic.h>

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

  #if 0
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
  #endif

  /**
   * @brief Creates or opens an HDF5 dataset with the given name, dimensions, and datatype.
   *
   * This method should only be called by simplnx HDF5 IO wrapper classes.
   * @param name
   * @param dims
   * @param dataType
   * @return HighFive::DataSet
   */
  //hid_t createOrOpenH5Dataset(const std::string& name, const DimsType& dims, DataType dataType) override;

protected:
  /**
   * @brief Constructs a FileIO wrapping the HDF5 file at the target
   * filepath.
   * @param filepath
   * @param fileId
   */
  FileIO(const std::filesystem::path& filepath, hid_t fileId);

  hid_t open() const override;
  void close() override;

private:
};
} // namespace nx::core::HDF5
