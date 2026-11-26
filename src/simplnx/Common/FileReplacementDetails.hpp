#pragma once

#include "simplnx/Common/FileReplacement.hpp"
#include "simplnx/simplnx_export.hpp"

#include <chrono>
#include <filesystem>
#include <functional>
#include <system_error>

namespace nx::core::detail
{
/** @brief Wait before the second attempt. */
inline constexpr std::chrono::milliseconds k_FileReplacementFirstWaitMs{1};
/** @brief Maximum wait between two attempts. */
inline constexpr std::chrono::milliseconds k_FileReplacementLongestWaitMs{64};
/** @brief Total wait budget before the last attempt. */
inline constexpr std::chrono::milliseconds k_FileReplacementTotalWaitLimitMs{2000};

/**
 * @struct FileReplacementSteps
 * @brief Supplies callable operations for the bounded retry loop.
 */
struct FileReplacementSteps
{
  /** @brief Attempts the rename with the replacement and destination paths, in that order. Returns an empty error code on success. */
  std::function<std::error_code(const std::filesystem::path&, const std::filesystem::path&)> Rename;
  /** @brief Tests if a definite refusal permits a retry for the replacement and destination paths, in that order. */
  std::function<bool(const std::error_code&, const std::filesystem::path&, const std::filesystem::path&)> IsTransient;
  /** @brief Waits for the specified duration before the next attempt. */
  std::function<void(std::chrono::milliseconds)> Wait;
  /** @brief Optional prerequisite. Transient errors share the rename retry budget. Never called again after success. */
  std::function<std::error_code(const std::filesystem::path&)> Flush;
};

/**
 * @brief Tries the fallback once if the primary rename fails.
 * @param primaryRename First rename operation.
 * @param fallbackRename Rename operation to use if the primary rename returns an error.
 * @param isTransient Tests if the primary error permits a retry when both operations fail.
 * @return An empty code if either operation succeeds. If both fail, returns the primary error when it permits a retry and the fallback error otherwise.
 * @pre Both operations and the predicate are callable.
 */
[[nodiscard]] SIMPLNX_EXPORT std::error_code RenameWithFallback(const std::function<std::error_code()>& primaryRename, const std::function<std::error_code()>& fallbackRename,
                                                                const std::function<bool(const std::error_code&)>& isTransient);

/**
 * @brief Returns operations that rename files on the current platform.
 * @param durability Selects Windows write-through behavior. The default adds no flush.
 * @return Rename, error classification and wait operations.
 */
[[nodiscard]] SIMPLNX_EXPORT FileReplacementSteps PlatformFileReplacementSteps(FileReplacementDurability durability = FileReplacementDurability::Default);

/**
 * @brief Installs a complete file at the destination with a bounded retry loop.
 * @param replacement Path to the complete replacement file.
 * @param destination Path that receives the replacement file. The destination can be absent.
 * @param steps Supplies the rename, error classification, wait and optional prerequisite flush operations.
 * @return Final error and retry history. An empty error code indicates success.
 * @pre The paths refer to different files on the same filesystem. The caller closed the writer of the replacement file.
 * @pre Rename, IsTransient and Wait are callable. IsTransient accepts only errors that preserve both paths.
 * Flush is optional and does not change either path.
 * @post Success removes the replacement path. No rename runs until the flush succeeds.
 * Pre-rename failures and definite refusals preserve both paths. Other rename errors can leave the final state uncertain.
 * FileReplacementLeftDestinationUnchanged() identifies failures known to preserve the destination.
 */
[[nodiscard]] SIMPLNX_EXPORT FileReplacementOutcome InstallReplacementFile(const std::filesystem::path& replacement, const std::filesystem::path& destination, const FileReplacementSteps& steps);
} // namespace nx::core::detail
