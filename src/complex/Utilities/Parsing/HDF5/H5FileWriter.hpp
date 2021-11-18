#pragma once

#include "H5GroupWriter.hpp"

#include "complex/Common/Result.hpp"

#include <filesystem>
#include <optional>

namespace complex
{
namespace H5
{
class COMPLEX_EXPORT FileWriter : public GroupWriter
{
public:
  using PointerType = std::unique_ptr<H5::FileWriter>;
  using ResultType = Result<PointerType>;

  /**
   * @brief This static method will ensure that the complete path to the file exists
   * and the file is created.
   * @param filepath The file path to the HDF5 file that should be created
   * @return A standard Result object that wraps a std::unique_ptr<FileWriter> object on success.
   */
  static ResultType CreateFile(const std::filesystem::path& filepath);

  /**
   * @brief This static method will wrap an existing HDF5 fileId value as long as the
   * value is > 0. If the fileId is invalid the the Result object will be `invalid()`
   * @param fileId The HDF5 File ID to wrap
   * @return A standard Result object that wraps a std::unique_ptr<FileWriter> object on success.
   */
  static ResultType WrapHdf5FileId(H5::IdType fileId);

  /**
   * @brief Constructs an invalid FileWriter.
   */
  FileWriter();

  /**
   * @brief Closes the HDF5 file.
   */
  virtual ~FileWriter();

  /**
   * @brief Returns the file's HDF5 ID. Returns 0 if the object is invalid.
   * @return H5::IdType
   */
  H5::IdType getId() const override;

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
  FileWriter(const std::filesystem::path& filepath) noexcept(false);

  /**
   * @brief Constructs a FileWriter and wraps an already open HDF5 file. The
   * FileWriter is invalid if the file ID is 0.
   * @param fileId
   */
  FileWriter(H5::IdType fileId);

  /**
   * @brief Closes the HDF5 ID and resets it to 0.
   */
  void closeHdf5();

private:
  H5::IdType m_FileId = 0;
};
} // namespace H5
} // namespace complex
