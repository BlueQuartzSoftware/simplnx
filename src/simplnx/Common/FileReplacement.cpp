#include "FileReplacement.hpp"

#include <fmt/format.h>
#include <fmt/std.h> // IWYU pragma: keep (filesystem path formatter)

#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
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

FileReplacementAttempt tryWindowsPosixRename(const fs::path& replacement, const fs::path& absoluteDestination, bool writeThrough)
{
  const auto& name = absoluteDestination.native();
  const DWORD access = DELETE | SYNCHRONIZE | (writeThrough ? GENERIC_WRITE : 0u);
  // On NTFS, FILE_FLAG_WRITE_THROUGH also flushes metadata changes from the rename.
  // Open a reparse point itself so the rename acts on the named entry.
  const DWORD flags = FILE_FLAG_OPEN_REPARSE_POINT | (writeThrough ? FILE_FLAG_WRITE_THROUGH : 0u);
  const HANDLE handle = CreateFileW(replacement.c_str(), access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, flags, nullptr);
  if(handle == INVALID_HANDLE_VALUE)
  {
    return {.Error = lastWindowsError()};
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
    return {.Error = lastWindowsError()};
  }
  if(writeThrough && FlushFileBuffers(handle) == 0)
  {
    return {.Error = lastWindowsError(), .Renamed = true};
  }
  return {.Error = {}, .Renamed = true};
}

FileReplacementAttempt renameOnWindows(const fs::path& replacement, const fs::path& destination, FileReplacementDurability durability)
{
  std::error_code pathError;
  fs::path absoluteDestination = fs::absolute(destination, pathError);
  if(pathError)
  {
    return {.Error = pathError};
  }
  absoluteDestination.make_preferred();
  constexpr std::size_t k_MaxPathUnitsWithTerminator = 32767;
  if(absoluteDestination.native().size() >= k_MaxPathUnitsWithTerminator)
  {
    return {.Error = {ERROR_FILENAME_EXCED_RANGE, std::system_category()}};
  }

  const bool writeThrough = durability == FileReplacementDurability::WriteThrough;
  const auto attempt = tryWindowsPosixRename(replacement, absoluteDestination, writeThrough);
  // A file system or redirector can reject the flags on open or the extended rename request.
  const int code = attempt.Error.value();
  if(attempt.Renamed || (code != ERROR_INVALID_PARAMETER && code != ERROR_NOT_SUPPORTED && code != ERROR_INVALID_FUNCTION))
  {
    return attempt;
  }

  const DWORD flags = MOVEFILE_REPLACE_EXISTING | (writeThrough ? MOVEFILE_WRITE_THROUGH : 0u);
  if(MoveFileExW(replacement.c_str(), absoluteDestination.c_str(), flags) == 0)
  {
    return {.Error = lastWindowsError()};
  }
  return {.Error = {}, .Renamed = true};
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
    // A scanner without delete sharing, a mapped file section, and the MoveFileExW fallback can all give a temporary access-denied error.
    return replacementAttributes == INVALID_FILE_ATTRIBUTES || (replacementAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
  }
  return code == ERROR_SHARING_VIOLATION || code == ERROR_LOCK_VIOLATION;
}
#endif
} // namespace

namespace nx::core
{
FileReplacementSteps PlatformFileReplacementSteps(FileReplacementDurability durability)
{
  const auto wait = [](std::chrono::milliseconds duration) { std::this_thread::sleep_for(duration); };
#ifdef _WIN32
  return {.Rename = [durability](const fs::path& replacement, const fs::path& destination) { return renameOnWindows(replacement, destination, durability); },
          .IsTransient = isWindowsRenameErrorTransient,
          .Wait = wait};
#else
  static_cast<void>(durability);
  return {.Rename =
              [](const fs::path& replacement, const fs::path& destination) {
                std::error_code error;
                fs::rename(replacement, destination, error);
                return FileReplacementAttempt{.Error = error, .Renamed = !error};
              },
          .IsTransient = [](const std::error_code&, const fs::path&, const fs::path&) { return false; },
          .Wait = wait};
#endif
}

FileReplacementOutcome InstallReplacementFile(const fs::path& replacement, const fs::path& destination, const FileReplacementSteps& steps)
{
  assert(steps.Rename && steps.IsTransient && steps.Wait);
  FileReplacementOutcome outcome;
  std::chrono::milliseconds nextWait = k_FileReplacementFirstWaitMs;
  while(true)
  {
    const auto attempt = steps.Rename(replacement, destination);
    outcome.Error = attempt.Error;
    outcome.Renamed = attempt.Renamed;
    ++outcome.Attempts;
    if(!outcome.Error || outcome.Renamed || !steps.IsTransient(outcome.Error, replacement, destination))
    {
      return outcome;
    }
    if(outcome.Waited >= k_FileReplacementTotalWaitLimitMs)
    {
      outcome.RetriesExhausted = true;
      return outcome;
    }
    const auto wait = (std::min)(nextWait, k_FileReplacementTotalWaitLimitMs - outcome.Waited);
    steps.Wait(wait);
    outcome.Waited += wait;
    nextWait = (std::min)(nextWait * 2, k_FileReplacementLongestWaitMs);
  }
}

FileReplacementOutcome InstallReplacementFile(const fs::path& replacement, const fs::path& destination, FileReplacementDurability durability)
{
  return InstallReplacementFile(replacement, destination, PlatformFileReplacementSteps(durability));
}

std::string DescribeFileReplacementSystemError(const FileReplacementOutcome& outcome)
{
  assert(outcome.Error);
  if(outcome.Attempts > 1)
  {
    return fmt::format("Operating system error: {} ({} attempts, {} ms total requested wait).", outcome.Error.message(), outcome.Attempts, outcome.Waited.count());
  }
  return fmt::format("Operating system error: {}", outcome.Error.message());
}

std::string DescribeFileReplacementFailure(const FileReplacementOutcome& outcome, const fs::path& destination)
{
  std::string description;
  if(outcome.Renamed)
  {
    description = fmt::format("Output file '{}' was replaced, but the system could not confirm that the data reached the disk. Save the output again.", destination);
  }
  else if(outcome.RetriesExhausted)
  {
    description = fmt::format("Could not replace output file '{}'. The destination is unchanged. The file may be read-only or you may not have permission to replace it. "
                              "It may also be open in another program. Make sure the file is closed and writable.",
                              destination);
  }
  else
  {
    description = fmt::format("Could not replace output file '{}'. The destination is unchanged. Check that the files and the output folder are accessible.", destination);
  }
  return fmt::format("{} {}", description, DescribeFileReplacementSystemError(outcome));
}
} // namespace nx::core
