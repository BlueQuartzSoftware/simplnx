#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Writers/GroupWriter.hpp"

#include <filesystem>
#include <optional>
#include <string>

namespace nx::core::HDF5
{
class SIMPLNX_EXPORT FileWriter : public GroupWriter
{
public:
  /**
   * @brief This static method will ensure that the complete path to the file
   * exists and the file is created.
   * @param filepath The file path to the HDF5 file that should be created
   * @return A standard Result object that wraps a std::unique_ptr<FileWriter>
   * object on success.
   */
  static Result<FileWriter> CreateFile(const std::filesystem::path& filepath);

  /**
   * @brief This static method will wrap an existing HDF5 fileId value as long
   * as the value is > 0. If the fileId is invalid the the Result object will be
   * `invalid()`
   * @param fileId The HDF5 File ID to wrap
   * @return A standard Result object that wraps a std::unique_ptr<FileWriter>
   * object on success.
   */
  static Result<FileWriter> WrapHdf5FileId(HDF5::IdType fileId);

  /**
   * @brief Constructs an invalid FileWriter.
   */
  FileWriter();

  /**
   * @brief Move constructor.
   * @param rhs
   */
  FileWriter(FileWriter&& rhs) noexcept;

  /**
   * @brief Closes the HDF5 file.
   */
  ~FileWriter() override;

  /**
   * @brief Returns the HDF5 file name. Returns an empty string if the writer
   * is invalid.
   * @return std::string
   */
  std::string getName() const override;

protected:
  /**
   * @brief Constructs a FileWriter that creates and wraps an HDF5 file at the
   * specified path. If the file exists, the file is truncated to delete all
   * existing data. If the file cannot be created, the writer is invalid.
   * @param filepath
   */
  FileWriter(const std::filesystem::path& filepath);

  /**
   * @brief Constructs a FileWriter and wraps an already open HDF5 file. The
   * FileWriter is invalid if the file ID is 0.
   * @param fileId
   */
  FileWriter(IdType fileId);

  /**
   * @brief Closes the HDF5 ID and resets it to 0.
   */
  void closeHdf5() override;
};
} // namespace nx::core::HDF5
