#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Types.hpp"

#include <chrono>
#include <filesystem>
#include <string>
#include <system_error>

namespace nx::core
{
/**
 * @enum FileReplacementDurability
 * @brief Selects the write-through behavior on Windows.
 *
 * On Windows, WriteThrough flushes the replacement file data before either rename.
 *
 * Transient flush and open failures share the rename retry budget. No rename runs until the flush succeeds.
 *
 * The operation does not flush the rename itself. The conventional Windows rename uses MOVEFILE_WRITE_THROUGH.
 * On POSIX, both values use one rename.
 */
enum class FileReplacementDurability : uint8
{
  Default,     ///< Use the platform's default buffering.
  WriteThrough ///< Request write-through on Windows.
};

/**
 * @struct FileReplacementOutcome
 * @brief Reports the final result and retry history of a replacement.
 */
struct FileReplacementOutcome
{
  /** @brief System error from the final attempt, or an empty code on success. */
  std::error_code error;
  /** @brief Number of loop attempts, at least one. Each tries an unfinished flush, then a rename if the flush succeeded. */
  usize attempts = 0;
  /** @brief Sum of the requested waits between attempts. */
  std::chrono::milliseconds waited{0};
  /** @brief True when a transient error persists after the total wait budget. */
  bool retries_exhausted = false;
  /** @brief False when no rename ran, for example when the prerequisite flush never succeeded. */
  bool rename_attempted = false;
};

/**
 * @brief Installs a complete file at the destination with the platform operations.
 *
 * On NTFS, a POSIX-semantics rename can replace the destination while a handle with delete sharing is open.
 * Windows tries a POSIX-semantics rename first and tries a conventional rename after any failure of that step.
 * File systems without POSIX-semantics support use the conventional rename with the same bounded retry policy.
 * Windows retries transient access-denied, sharing-violation, and lock-violation errors.
 * For access-denied errors, a read-only destination, a destination directory, or a replacement directory prevents a retry.
 * On Windows, waits double from 1 ms to 64 ms, with a total budget of 2000 ms. POSIX uses one rename.
 *
 * @param replacement Path to the complete replacement file.
 * @param destination Path that receives the replacement file. The destination can be absent.
 * @param durability Selects Windows write-through behavior. The default adds no flush.
 * @return Final error and retry history. An empty error code indicates success.
 * @pre The paths refer to different files on the same filesystem. The caller closed the writer of the replacement file.
 * @post Success removes the replacement path. A failure before any rename (including a flush failure) or a definite
 * refusal leaves both paths unchanged. A network filesystem can complete a rename but report an error if its reply
 * is lost. FileReplacementLeftDestinationUnchanged() identifies failures known to preserve the destination.
 * For other failures, check the destination before retrying.
 */
[[nodiscard]] SIMPLNX_EXPORT FileReplacementOutcome InstallReplacementFile(const std::filesystem::path& replacement, const std::filesystem::path& destination,
                                                                           FileReplacementDurability durability = FileReplacementDurability::Default);

/**
 * @brief Reports whether a failed replacement is known to leave the destination unchanged.
 * @param outcome Result of the failed operation.
 * @return True if no rename ran, retries ended with only refusals, or the final error is a definite refusal.
 * Other rename errors can leave the final state uncertain, for example after a lost network reply.
 * @pre outcome.Error contains an error.
 */
[[nodiscard]] SIMPLNX_EXPORT bool FileReplacementLeftDestinationUnchanged(const FileReplacementOutcome& outcome);

/**
 * @brief Describes the system error and retry history for a caller's diagnostic.
 * @param outcome Result of the failed operation.
 * @return System error text. After a retry, the text ends with "(tried N times over M ms)."
 * M is the sum of the retry delays in milliseconds.
 * @pre outcome.Error contains an error.
 */
[[nodiscard]] SIMPLNX_EXPORT std::string DescribeFileReplacementSystemError(const FileReplacementOutcome& outcome);

/**
 * @brief Describes a failed replacement and gives a recovery action.
 * @param outcome Result of the failed operation.
 * @param destination Path that was to receive the replacement file.
 * @return Diagnostic with the path, recovery action and system error. States preservation only when known.
 * @pre outcome.Error contains an error.
 */
[[nodiscard]] SIMPLNX_EXPORT std::string DescribeFileReplacementFailure(const FileReplacementOutcome& outcome, const std::filesystem::path& destination);
} // namespace nx::core
