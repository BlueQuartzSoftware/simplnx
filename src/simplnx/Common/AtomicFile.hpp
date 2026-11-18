#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"

#include <filesystem>
#include <string>

namespace nx::core
{
struct FileReplacementOutcome;

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
  /** @brief The output file was replaced, but the system could not confirm the disk flush. */
  static constexpr int32 k_FlushAfterReplacementError = -15785;

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
   *
   * On Windows, this operation retries transient rename errors and requests write-through.
   * @return Valid result on success or a detailed filesystem error. Error k_FlushAfterReplacementError means the new file is at the destination, but the disk flush failed.
   */
  [[nodiscard]] Result<> commit();

  /**
   * @brief Converts the result of installing the temporary file to a commit result.
   * @param outcome Result of the file replacement and any requested flush.
   * @param temporaryPath Path of the temporary output file before replacement.
   * @param destination Final output path.
   * @return Commit result with the replacement or flush error, if any.
   */
  [[nodiscard]] static Result<> commitResult(const FileReplacementOutcome& outcome, const std::filesystem::path& temporaryPath, const std::filesystem::path& destination);

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
