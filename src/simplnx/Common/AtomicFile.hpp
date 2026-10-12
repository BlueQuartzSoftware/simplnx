#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Result.hpp"

#include <filesystem>
#include <string>

namespace nx::core
{
/**
 * @class AtomicFile
 * @brief Provides transactional replacement of one output file.
 *
 * Create() makes a temporary sibling path. Callers write to tempFilePath() and
 * call commit() only after the complete output is valid. Destruction removes any
 * remaining temporary file and directory.
 */
class SIMPLNX_EXPORT AtomicFile
{
public:
  /**
   * @brief Makes a best-effort removal of any uncommitted temporary output.
   */
  ~AtomicFile() noexcept;

  AtomicFile(const AtomicFile&) = delete;

  /**
   * @brief Moves ownership of the temporary output path.
   * @param other Atomic file whose temporary path is transferred.
   */
  AtomicFile(AtomicFile&& other) noexcept;

  AtomicFile& operator=(const AtomicFile&) = delete;

  /**
   * @brief Replaces this temporary output with another object's temporary output.
   * @param other Atomic file whose temporary path is transferred.
   * @return Reference to this object.
   */
  AtomicFile& operator=(AtomicFile&& other) noexcept;

  /**
   * @brief Creates an atomic-file transaction for a destination path.
   * @param filename Final output path.
   * @return Atomic file transaction or a filesystem validation error.
   */
  [[nodiscard]] static Result<AtomicFile> Create(std::filesystem::path filename); // NOLINT(readability-identifier-naming) Established public API.

  /**
   * @brief Gets the path that receives output before commit.
   * @return Temporary output path.
   */
  [[nodiscard]] std::filesystem::path tempFilePath() const;

  /**
   * @brief Atomically replaces the destination with the temporary file.
   * @return Valid result on success or a detailed filesystem error.
   */
  [[nodiscard]] Result<> commit();

  /**
   * @brief Makes a best-effort purge of the temporary file without throwing.
   */
  void removeTempFile() const noexcept;

private:
  /**
   * @brief Constructs an inactive transaction for the destination path.
   * @param filename Final output path.
   */
  explicit AtomicFile(std::filesystem::path filename);

  std::filesystem::path m_FilePath;
  std::filesystem::path m_TempFilePath;
};
} // namespace nx::core
