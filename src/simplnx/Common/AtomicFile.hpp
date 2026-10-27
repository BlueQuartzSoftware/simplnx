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
 *
 * Create() runs the file-replace hook (BeforeFileReplace()) on the final path, so a writer that creates its file through
 * AtomicFile needs no other hook call.
 */
class SIMPLNX_EXPORT AtomicFile
{
public:
  ~AtomicFile() noexcept;

  AtomicFile(const AtomicFile&) = delete;
  AtomicFile(AtomicFile&&) noexcept = default;

  AtomicFile& operator=(const AtomicFile&) = delete;
  AtomicFile& operator=(AtomicFile&&) noexcept = default;

  /**
   * @brief Runs the file-replace hook on the final path, then creates the temporary file location for it.
   * @param filename Final path that commit() replaces. A relative path is made absolute against the current working directory.
   * @return The AtomicFile with the warnings of the hook, or an error. A hook error is the unchanged result of
   * BeforeFileReplace(), with its warnings. Other errors have the hook's warnings in front. They are -15780 when the path
   * cannot be made absolute, and the errors of the permission check and of the directory creation.
   *
   * BeforeFileReplace() runs on the absolute final path before the permission check and before the temporary directory
   * exists. Thus a component that reads the final file can stop reading it before commit() replaces the file. A hook error
   * creates no temporary directory. Call Create() only in execution, never in preflight, because the hook can change the
   * state of other components. A consumer that gets an invalid result returns it and does not change that final path. A consumer keeps the
   * warnings of a valid result on its own result.
   */
  static Result<AtomicFile> Create(std::filesystem::path filename);

  /**
   * @brief Creates the temporary file location for a final path whose file-replace hook already ran.
   * @param filename Final path that commit() replaces. A relative path is made absolute against the current working directory.
   * @return The AtomicFile, or the same errors as Create() except the hook errors.
   * @pre The caller ran BeforeFileReplace() on this thread on the same final path. The path was made absolute as Create()
   * does, and the call returned a valid result.
   *
   * Use this function only when the caller must run the hook itself. An example is a caller that selects a
   * FileWriteHookMode and reports a hook error in its own way. Every other caller uses Create().
   */
  static Result<AtomicFile> CreateAfterHook(std::filesystem::path filename);

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

  /**
   * @brief Makes the final path absolute and optionally runs the file-replace hook on it.
   * @param filename Final path.
   * @param runsHook True to run BeforeFileReplace() on the absolute final path before any other check.
   * @return See Create() and CreateAfterHook().
   */
  static Result<AtomicFile> CreateImpl(std::filesystem::path filename, bool runsHook);

  std::filesystem::path m_FilePath;
  std::filesystem::path m_TempFilePath;
};
} // namespace nx::core
