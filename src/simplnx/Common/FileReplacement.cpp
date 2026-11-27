#include "FileReplacement.hpp"

#include "FileReplacementDetails.hpp"

#include <fmt/format.h>
#include <fmt/std.h> // IWYU pragma: keep (filesystem path formatter)

#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <functional>
#include <string>
#include <system_error>
#include <thread>

#ifdef _WIN32
#include "simplnx/Common/ScopeGuard.hpp"

#include <cstddef>
#include <cstring>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

using namespace nx::core;

namespace fs = std::filesystem;

namespace
{
#ifdef _WIN32
// These Win32 SDK values also work with headers that predate FileRenameInfoEx and FILE_RENAME_FLAG_*.
constexpr auto k_FileRenameInfoEx = static_cast<FILE_INFO_BY_HANDLE_CLASS>(22);
constexpr DWORD k_RenameReplaceIfExists = 0x00000001;
constexpr DWORD k_RenamePosixSemantics = 0x00000002;

std::error_code lastWindowsError()
{
  return {static_cast<int>(GetLastError()), std::system_category()};
}

std::error_code flushOnWindows(const fs::path& replacement)
{
  const HANDLE handle = CreateFileW(replacement.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
  if(handle == INVALID_HANDLE_VALUE)
  {
    return lastWindowsError();
  }
  const auto closeHandle = MakeScopeGuard([handle]() noexcept { CloseHandle(handle); });
  return FlushFileBuffers(handle) == 0 ? lastWindowsError() : std::error_code{};
}

std::error_code tryWindowsPosixRename(const fs::path& replacement, const fs::path& absoluteDestination)
{
  const auto& name = absoluteDestination.native();
  const DWORD access = DELETE | SYNCHRONIZE;
  // Open a reparse point itself so the rename acts on the named entry.
  const HANDLE handle = CreateFileW(replacement.c_str(), access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
  if(handle == INVALID_HANDLE_VALUE)
  {
    return lastWindowsError();
  }
  const auto closeHandle = MakeScopeGuard([handle]() noexcept { CloseHandle(handle); });

  constexpr std::size_t k_NameOffset = offsetof(FILE_RENAME_INFO, FileName);
  const std::size_t nameBytes = name.size() * sizeof(wchar_t);
  const std::size_t bufferSize = (std::max)(sizeof(FILE_RENAME_INFO), k_NameOffset + nameBytes + sizeof(wchar_t));
  FILE_RENAME_INFO info{};
  info.RootDirectory = nullptr;
  info.FileNameLength = static_cast<DWORD>(nameBytes);
  std::vector<std::byte> buffer(bufferSize);
  std::memcpy(buffer.data(), &info, k_NameOffset);
  // The extended layout uses a DWORD where old SDK headers expose a BOOLEAN and padding.
  constexpr DWORD k_RenameFlags = k_RenameReplaceIfExists | k_RenamePosixSemantics;
  std::memcpy(buffer.data(), &k_RenameFlags, sizeof(k_RenameFlags));
  std::memcpy(buffer.data() + k_NameOffset, name.c_str(), nameBytes + sizeof(wchar_t));

  if(SetFileInformationByHandle(handle, k_FileRenameInfoEx, buffer.data(), static_cast<DWORD>(buffer.size())) == 0)
  {
    return lastWindowsError();
  }
  return {};
}

bool isWindowsRenameErrorTransient(const std::error_code& error, const fs::path& replacement, const fs::path& destination)
{
  if(error.category() != std::system_category())
  {
    return false;
  }
  const int code = error.value();
  if(code == ERROR_ACCESS_DENIED)
  {
    const DWORD destinationAttributes = GetFileAttributesW(destination.c_str());
    if(destinationAttributes != INVALID_FILE_ATTRIBUTES && (destinationAttributes & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_DIRECTORY)) != 0)
    {
      return false;
    }
    const DWORD replacementAttributes = GetFileAttributesW(replacement.c_str());
    // A conventional rename can return ERROR_ACCESS_DENIED while any destination handle is open.
    // A mapped file section can also cause this error. Both conditions can be temporary.
    return replacementAttributes == INVALID_FILE_ATTRIBUTES || (replacementAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
  }
  return code == ERROR_SHARING_VIOLATION || code == ERROR_LOCK_VIOLATION;
}

std::error_code renameOnWindows(const fs::path& replacement, const fs::path& destination, FileReplacementDurability durability)
{
  std::error_code pathError;
  fs::path absoluteDestination = fs::absolute(destination, pathError);
  if(pathError)
  {
    return pathError;
  }
  absoluteDestination.make_preferred();
  constexpr std::size_t k_MaxPathUnitsWithTerminator = 32767;
  if(absoluteDestination.native().size() >= k_MaxPathUnitsWithTerminator)
  {
    return {ERROR_FILENAME_EXCED_RANGE, std::system_category()};
  }

  const bool writeThrough = durability == FileReplacementDurability::WriteThrough;
  return detail::RenameWithFallback([&] { return tryWindowsPosixRename(replacement, absoluteDestination); },
                                    [&] {
                                      const DWORD flags = MOVEFILE_REPLACE_EXISTING | (writeThrough ? MOVEFILE_WRITE_THROUGH : 0u);
                                      return MoveFileExW(replacement.c_str(), absoluteDestination.c_str(), flags) == 0 ? lastWindowsError() : std::error_code{};
                                    },
                                    [&replacement, &absoluteDestination](const std::error_code& error) { return isWindowsRenameErrorTransient(error, replacement, absoluteDestination); });
}
#endif
} // namespace

namespace nx::core::detail
{
std::error_code RenameWithFallback(const std::function<std::error_code()>& primaryRename, const std::function<std::error_code()>& fallbackRename,
                                   const std::function<bool(const std::error_code&)>& isTransient)
{
  const auto primaryError = primaryRename();
  if(!primaryError)
  {
    return {};
  }

  const auto fallbackError = fallbackRename();
  return fallbackError && isTransient(primaryError) ? primaryError : fallbackError;
}

FileReplacementSteps PlatformFileReplacementSteps(FileReplacementDurability durability)
{
  const auto wait = [](std::chrono::milliseconds duration) { std::this_thread::sleep_for(duration); };
#ifdef _WIN32
  return {.Rename = [durability](const fs::path& replacement, const fs::path& destination) { return renameOnWindows(replacement, destination, durability); },
          .IsTransient = isWindowsRenameErrorTransient,
          .Wait = wait,
          .Flush = durability == FileReplacementDurability::WriteThrough ? flushOnWindows : std::function<std::error_code(const fs::path&)>{}};
#else
  static_cast<void>(durability);
  return {.rename =
              [](const fs::path& replacement, const fs::path& destination) {
                std::error_code error;
                fs::rename(replacement, destination, error);
                return error;
              },
          .is_transient = [](const std::error_code&, const fs::path&, const fs::path&) { return false; },
          .wait = wait};
#endif
}

FileReplacementOutcome InstallReplacementFile(const fs::path& replacement, const fs::path& destination, const FileReplacementSteps& steps)
{
  assert(steps.Rename && steps.IsTransient && steps.Wait);
  FileReplacementOutcome outcome;
  bool flushed = !steps.flush;
  std::chrono::milliseconds nextWait = k_FileReplacementFirstWaitMs;
  while(true)
  {
    ++outcome.attempts;
    if(!flushed)
    {
      outcome.error = steps.flush(replacement);
      flushed = !outcome.error;
    }
    if(flushed)
    {
      outcome.rename_attempted = true;
      outcome.error = steps.rename(replacement, destination);
    }
    if(!outcome.error || !steps.is_transient(outcome.error, replacement, destination))
    {
      return outcome;
    }
    if(outcome.waited >= k_FileReplacementTotalWaitLimitMs)
    {
      outcome.retries_exhausted = true;
      return outcome;
    }
    const auto wait = (std::min)(nextWait, k_FileReplacementTotalWaitLimitMs - outcome.waited);
    steps.wait(wait);
    outcome.waited += wait;
    nextWait = (std::min)(nextWait * 2, k_FileReplacementLongestWaitMs);
  }
}
} // namespace nx::core::detail

namespace nx::core
{
FileReplacementOutcome InstallReplacementFile(const fs::path& replacement, const fs::path& destination, FileReplacementDurability durability)
{
  return detail::InstallReplacementFile(replacement, destination, detail::PlatformFileReplacementSteps(durability));
}

std::string DescribeFileReplacementSystemError(const FileReplacementOutcome& outcome)
{
  assert(outcome.Error);
  if(outcome.attempts > 1)
  {
    return fmt::format("Operating system error: {} (tried {} times over {} ms).", outcome.error.message(), outcome.attempts, outcome.waited.count());
  }
  return fmt::format("Operating system error: {}", outcome.error.message());
}

bool FileReplacementLeftDestinationUnchanged(const FileReplacementOutcome& outcome)
{
  assert(outcome.Error);
  if(!outcome.rename_attempted || outcome.retries_exhausted)
  {
    return true;
  }
  const auto& error = outcome.error;
  if(error == std::errc::permission_denied || error == std::errc::operation_not_permitted || error == std::errc::read_only_file_system || error == std::errc::is_a_directory ||
     error == std::errc::not_a_directory || error == std::errc::directory_not_empty || error == std::errc::file_exists || error == std::errc::no_such_file_or_directory ||
     error == std::errc::cross_device_link || error == std::errc::filename_too_long || error == std::errc::device_or_resource_busy || error == std::errc::text_file_busy)
  {
    return true;
  }
#ifdef _WIN32
  // MSVC maps access denied and sharing violation to permission_denied, but lock violation to no_lock_available.
  // Check the native codes too, without treating unrelated no_lock_available errors as definite refusals.
  return error.category() == std::system_category() && (error.value() == ERROR_ACCESS_DENIED || error.value() == ERROR_SHARING_VIOLATION || error.value() == ERROR_LOCK_VIOLATION);
#else
  return false;
#endif
}

std::string DescribeFileReplacementFailure(const FileReplacementOutcome& outcome, const fs::path& destination)
{
  std::string description;
  if(!FileReplacementLeftDestinationUnchanged(outcome))
  {
    description = fmt::format("Could not confirm that output file '{}' was replaced. Check the file before you run again.", destination);
  }
  else
  {
    description = fmt::format("Could not replace output file '{}'. The destination is unchanged. ", destination);
    if(outcome.retries_exhausted)
    {
      description += "It may be open in another program, or you may not have permission to replace it. Close it and run again.";
    }
    else
    {
      description += "The file or the output folder may be read-only, or you may not have permission to replace the file.";
    }
  }
  return fmt::format("{} {}", description, DescribeFileReplacementSystemError(outcome));
}
} // namespace nx::core
