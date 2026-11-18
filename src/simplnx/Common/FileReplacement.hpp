#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Types.hpp"

#include <chrono>
#include <filesystem>
#include <functional>
#include <string>
#include <system_error>

namespace nx::core
{
/** @brief Wait before the second rename attempt. */
inline constexpr std::chrono::milliseconds k_FileReplacementFirstWaitMs{1};
/** @brief Maximum wait between two rename attempts. */
inline constexpr std::chrono::milliseconds k_FileReplacementLongestWaitMs{64};
/** @brief Total wait budget before the last rename attempt. */
inline constexpr std::chrono::milliseconds k_FileReplacementTotalWaitLimitMs{2000};

/**
 * @enum FileReplacementDurability
 * @brief Selects the write-through behavior on Windows. Both values use one rename on POSIX.
 */
enum class FileReplacementDurability
{
  Default,     ///< Use the platform's default buffering.
  WriteThrough ///< Request a disk flush before reporting success on Windows.
};

/**
 * @struct FileReplacementAttempt
 * @brief Reports the result of one rename and any requested flush.
 */
struct FileReplacementAttempt
{
  /** @brief System error from the rename or flush, or an empty code on success. */
  std::error_code error;
  /** @brief True when the rename succeeded, including when a subsequent flush failed. */
  bool renamed = false;
};

/**
 * @struct FileReplacementSteps
 * @brief Supplies the operations for the bounded retry loop.
 * @pre All three steps are callable before passing this struct to InstallReplacementFile().
 */
struct FileReplacementSteps
{
  /**
   * @var FileReplacementSteps::rename
   * @brief Renames the replacement to the destination once. The arguments are the replacement and destination paths in that order.
   */
  std::function<FileReplacementAttempt(const std::filesystem::path&, const std::filesystem::path&)> rename;
  /**
   * @var FileReplacementSteps::isTransient
   * @brief Returns true for a rename error that permits a retry.
   */
  std::function<bool(const std::error_code&)> isTransient;
  /**
   * @var FileReplacementSteps::wait
   * @brief Waits for the specified duration before the next attempt.
   */
  std::function<void(std::chrono::milliseconds)> wait;
};

/**
 * @struct FileReplacementOutcome
 * @brief Reports the final result and retry history of a replacement.
 */
struct FileReplacementOutcome
{
  /** @brief System error from the final attempt, or an empty code on success. */
  std::error_code error;
  /** @brief Number of attempts, at least one when returned by InstallReplacementFile(). */
  usize attempts = 0;
  /** @brief Sum of the requested waits between attempts. */
  std::chrono::milliseconds waited{0};
  /** @brief True when the rename succeeded, including when a subsequent flush failed. */
  bool renamed = false;
  /** @brief True when a transient error persists after the total wait budget. */
  bool retriesExhausted = false;
};

/**
 * @brief Returns operations that rename files on the current platform.
 *
 * Windows tries a POSIX-semantics rename first. A conventional Windows replace fails if any handle to the destination is open.
 * The Windows Search indexer can hold such a handle for a few microseconds.
 * Windows permits retries for access denied, sharing violation and lock violation errors. POSIX does not retry.
 * @param durability Selects Windows write-through behavior. The default adds no flush.
 * @return Rename, error classification and wait operations.
 */
[[nodiscard]] SIMPLNX_EXPORT FileReplacementSteps PlatformFileReplacementSteps(FileReplacementDurability durability = FileReplacementDurability::Default);

/**
 * @brief Installs a complete file at the destination with a bounded retry loop.
 *
 * Waits double from 1 ms to 64 ms, with a total budget of 2000 ms. A failed flush after a successful rename ends the operation.
 * @param replacement Path to the complete replacement file.
 * @param destination Path that receives the replacement file. The destination can be absent.
 * @param steps Supplies the rename, error classification and wait operations.
 * @return Final error and retry history. If only the flush fails, the new file is already at the destination.
 * @pre All three steps are callable.
 * @pre The paths refer to different files on the same filesystem. The caller closed the writer of the replacement file.
 * @post A successful rename removes the replacement path.
 */
[[nodiscard]] SIMPLNX_EXPORT FileReplacementOutcome InstallReplacementFile(const std::filesystem::path& replacement, const std::filesystem::path& destination, const FileReplacementSteps& steps);

/**
 * @brief Installs a complete file at the destination with the platform operations.
 * @param replacement Path to the complete replacement file.
 * @param destination Path that receives the replacement file. The destination can be absent.
 * @param durability Selects Windows write-through behavior. The default adds no flush.
 * @return Final error and retry history. If only the flush fails, the new file is already at the destination.
 * @pre The paths refer to different files on the same filesystem. The caller closed the writer of the replacement file.
 * @post A successful rename removes the replacement path.
 */
[[nodiscard]] SIMPLNX_EXPORT FileReplacementOutcome InstallReplacementFile(const std::filesystem::path& replacement, const std::filesystem::path& destination,
                                                                           FileReplacementDurability durability = FileReplacementDurability::Default);

/**
 * @brief Describes a failed replacement or flush and gives a recovery action.
 * @param outcome Result of the failed operation.
 * @param destination Path that was to receive the replacement file.
 * @return Diagnostic with the destination path, recovery action and system error details.
 * @pre outcome.error contains an error.
 */
[[nodiscard]] SIMPLNX_EXPORT std::string DescribeFileReplacementFailure(const FileReplacementOutcome& outcome, const std::filesystem::path& destination);
} // namespace nx::core
