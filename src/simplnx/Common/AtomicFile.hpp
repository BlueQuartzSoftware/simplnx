#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Result.hpp"

#include <filesystem>
#include <string>

namespace nx::core
{
/**
 * @class AtomicFile
 * @brief The AtomicFile class accepts a filepath which it stores and
 * used to create a temporary file. This temporary can be written to in place
 * of the original file path. Upon commit() the temporary file will be moved to
 * the end location passed in originally. The temporary file will always be deleted
 * upon object destruction.
 */
class SIMPLNX_EXPORT AtomicFile
{
public:
  ~AtomicFile() noexcept;

  AtomicFile(const AtomicFile&) = delete;
  AtomicFile(AtomicFile&&) noexcept = default;

  AtomicFile& operator=(const AtomicFile&) = delete;
  AtomicFile& operator=(AtomicFile&&) noexcept = default;

  static Result<AtomicFile> Create(std::filesystem::path filename);

  /**
   * @brief Fetches the file path object to work from
   * @return fs::path the working file path modifications should be made to
   */
  [[nodiscard]] std::filesystem::path tempFilePath() const;

  /**
   * @brief Attempts to move the temp file to its final path
   * @return Result<>
   */
  [[nodiscard]] Result<> commit();

  /**
   * @brief Purges the temporary file
   */
  void removeTempFile() const;

private:
  /**
   * @brief Constructs the object.
   */
  explicit AtomicFile(std::filesystem::path filename);

  std::filesystem::path m_FilePath;
  std::filesystem::path m_TempFilePath;
};
} // namespace nx::core
