#pragma once

#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"
#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Result.hpp"

#include <filesystem>
#include <string>

namespace nx::core::HDF5
{
class SIMPLNX_EXPORT FileIO : public GroupIO
{
public:
  /**
   * @brief This static method will ensure that the complete path to the file
   * exists and the file is created.
   * @param filepath The file path to the HDF5 file that should be created
   * @return A standard Result object that wraps a std::unique_ptr<FileWriter>
   * object on success.
   */
  static Result<FileIO> CreateFile(const std::filesystem::path& filepath);

  /**
   * @brief This static method will ensure that the complete path to the file
   * exists and the file is created.
   * @param filepath The file path to the HDF5 file that should be created
   * @return A standard Result object that wraps a std::unique_ptr<FileWriter>
   * object on success.
   */
  static Result<std::shared_ptr<FileIO>> CreateSharedFile(const std::filesystem::path& filepath);

  /**
   * @brief This static method will wrap an existing HDF5 fileId value as long
   * as the value is > 0. If the fileId is invalid the the Result object will be
   * `invalid()`
   * @param fileId The HDF5 File ID to wrap
   * @return A standard Result object that wraps a std::unique_ptr<FileWriter>
   * object on success.
   */
  static Result<FileIO> WrapHdf5FileId(IdType fileId);

  /**
   * @brief Constructs an invalid FileIO.
   */
  FileIO();

  /**
   * @brief Constructs a FileIO wrapping the HDF5 file at the target
   * filepath. The constructed object will be invalid if the HDF5 file cannot
   * be found or openned.
   * @param filepath
   */
  FileIO(const std::filesystem::path& filepath);

  /**
   * @brief Constructs a FileIO wrapping the target HDF5 file ID. The
   * constructed object will be invalid if the provided HDF5 file ID is 0.
   * @param fileId
   */
  FileIO(IdType fileId);

  /**
   * @brief Move constructor.
   * @param rhs
   */
  FileIO(FileIO&& rhs) noexcept;

  /**
   * @brief Releases the HDF5 file ID.
   */
  virtual ~FileIO() noexcept;

  /**
   * @brief Returns the HDF5 file name. Returns an empty string if the file
   * is invalid.
   * @return std::string
   */
  std::string getName() const override;

protected:
  /**
   * @brief Closes the HDF5 ID and resets it to 0.
   */
  void closeHdf5() override;
};
} // namespace nx::core::HDF5
