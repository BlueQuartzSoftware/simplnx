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
 * On Windows, WriteThrough flushes the replacement file data to the device before a POSIX-semantics rename.
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
  std::error_code Error;
  /** @brief Number of attempts, at least one when returned by InstallReplacementFile(). */
  usize Attempts = 0;
  /** @brief Sum of the requested waits between attempts. */
  std::chrono::milliseconds Waited{0};
  /** @brief True when a transient error persists after the total wait budget. */
  bool RetriesExhausted = false;
};

/**
 * @brief Installs a complete file at the destination with the platform operations.
 *
 * On NTFS, a POSIX-semantics rename can replace the destination while a handle with delete sharing is open.
 * Windows tries a POSIX-semantics rename first and tries a conventional rename after any failure of that step.
 * File systems without POSIX-semantics support use the conventional rename with the same bounded retry policy.
 * Windows retries transient access-denied, sharing-violation, and lock-violation errors.
 * On Windows, waits double from 1 ms to 64 ms, with a total budget of 2000 ms. POSIX uses one rename.
 *
 * @param replacement Path to the complete replacement file.
 * @param destination Path that receives the replacement file. The destination can be absent.
 * @param durability Selects Windows write-through behavior. The default adds no flush.
 * @return Final error and retry history. An empty error code indicates success.
 * @pre The paths refer to different files on the same filesystem. The caller closed the writer of the replacement file.
 * @post Success removes the replacement path. Failure leaves the destination and replacement paths unchanged.
 */
[[nodiscard]] SIMPLNX_EXPORT FileReplacementOutcome InstallReplacementFile(const std::filesystem::path& replacement, const std::filesystem::path& destination,
                                                                           FileReplacementDurability durability = FileReplacementDurability::Default);

/**
 * @brief Describes the system error and retry history for a caller's diagnostic.
 * @param outcome Result of the failed operation.
 * @return System error text and, after a retry, the attempts and total requested wait.
 * @pre outcome.Error contains an error.
 */
[[nodiscard]] SIMPLNX_EXPORT std::string DescribeFileReplacementSystemError(const FileReplacementOutcome& outcome);

/**
 * @brief Describes a failed replacement and gives a recovery action.
 * @param outcome Result of the failed operation.
 * @param destination Path that was to receive the replacement file.
 * @return Diagnostic with the destination path, recovery action and system error details.
 * @pre outcome.Error contains an error.
 */
[[nodiscard]] SIMPLNX_EXPORT std::string DescribeFileReplacementFailure(const FileReplacementOutcome& outcome, const std::filesystem::path& destination);
} // namespace nx::core
