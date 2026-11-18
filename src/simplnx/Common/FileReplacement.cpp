#include "FileReplacement.hpp"

#include "simplnx/Common/ScopeGuard.hpp"

#include <fmt/std.h>

#include <algorithm>
#include <cassert>
#include <thread>

#ifdef _WIN32
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

FileReplacementAttempt renameOnWindows(const fs::path& replacement, const fs::path& destination, FileReplacementDurability durability)
{
  std::error_code pathError;
  fs::path absoluteDestination = fs::absolute(destination, pathError);
  if(pathError)
  {
    return {.error = pathError};
  }
  absoluteDestination.make_preferred();
  const auto& name = absoluteDestination.native();
  constexpr std::size_t k_MaxPathUnitsWithTerminator = 32767;
  if(name.size() >= k_MaxPathUnitsWithTerminator)
  {
    return {.error = {ERROR_FILENAME_EXCED_RANGE, std::system_category()}};
  }

  const bool writeThrough = durability == FileReplacementDurability::WriteThrough;
  {
    const DWORD access = DELETE | SYNCHRONIZE | (writeThrough ? GENERIC_WRITE : 0u);
    // On NTFS, FILE_FLAG_WRITE_THROUGH also flushes metadata changes from the rename.
    // Open a reparse point itself so the rename acts on the named entry.
    const DWORD flags = FILE_FLAG_OPEN_REPARSE_POINT | (writeThrough ? FILE_FLAG_WRITE_THROUGH : 0u);
    const HANDLE handle = CreateFileW(replacement.c_str(), access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, flags, nullptr);
    if(handle == INVALID_HANDLE_VALUE)
    {
      return {.error = lastWindowsError()};
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

    if(SetFileInformationByHandle(handle, k_FileRenameInfoEx, buffer.data(), static_cast<DWORD>(buffer.size())) != 0)
    {
      if(writeThrough && FlushFileBuffers(handle) == 0)
      {
        return {.error = lastWindowsError(), .renamed = true};
      }
      return {.error = {}, .renamed = true};
    }

    const std::error_code error = lastWindowsError();
    // FAT and exFAT reject this class with invalid parameter, as specified by MS-FSCC for unsupported classes.
    // SMB can return invalid info class (also Win32 invalid parameter) or not supported for this request.
    // Invalid function covers an unsupported device request. Access and sharing errors stay on the POSIX path.
    if(error.value() != ERROR_INVALID_PARAMETER && error.value() != ERROR_NOT_SUPPORTED && error.value() != ERROR_INVALID_FUNCTION)
    {
      return {.error = error};
    }
  }

  const DWORD flags = MOVEFILE_REPLACE_EXISTING | (writeThrough ? MOVEFILE_WRITE_THROUGH : 0u);
  if(MoveFileExW(replacement.c_str(), absoluteDestination.c_str(), flags) == 0)
  {
    return {.error = lastWindowsError()};
  }
  return {.error = {}, .renamed = true};
}

bool isWindowsRenameErrorTransient(const std::error_code& error)
{
  if(error.category() != std::system_category())
  {
    return false;
  }
  const int code = error.value();
  return code == ERROR_ACCESS_DENIED || code == ERROR_SHARING_VIOLATION || code == ERROR_LOCK_VIOLATION;
}
#endif
} // namespace

namespace nx::core
{
FileReplacementSteps PlatformFileReplacementSteps(FileReplacementDurability durability)
{
  const auto wait = [](std::chrono::milliseconds duration) { std::this_thread::sleep_for(duration); };
#ifdef _WIN32
  return {.rename = [durability](const fs::path& replacement, const fs::path& destination) { return renameOnWindows(replacement, destination, durability); },
          .isTransient = isWindowsRenameErrorTransient,
          .wait = wait};
#else
  static_cast<void>(durability);
  return {.rename =
              [](const fs::path& replacement, const fs::path& destination) {
                std::error_code error;
                fs::rename(replacement, destination, error);
                return FileReplacementAttempt{.error = error, .renamed = !error};
              },
          .isTransient = [](const std::error_code&) { return false; },
          .wait = wait};
#endif
}

FileReplacementOutcome InstallReplacementFile(const fs::path& replacement, const fs::path& destination, const FileReplacementSteps& steps)
{
  assert(steps.rename && steps.isTransient && steps.wait);
  FileReplacementOutcome outcome;
  std::chrono::milliseconds nextWait = k_FileReplacementFirstWaitMs;
  while(true)
  {
    const auto attempt = steps.rename(replacement, destination);
    outcome.error = attempt.error;
    outcome.renamed = attempt.renamed;
    ++outcome.attempts;
    if(!outcome.error || outcome.renamed || !steps.isTransient(outcome.error))
    {
      return outcome;
    }
    if(outcome.waited >= k_FileReplacementTotalWaitLimitMs)
    {
      outcome.retriesExhausted = true;
      return outcome;
    }
    const auto wait = (std::min)(nextWait, k_FileReplacementTotalWaitLimitMs - outcome.waited);
    steps.wait(wait);
    outcome.waited += wait;
    nextWait = (std::min)(nextWait * 2, k_FileReplacementLongestWaitMs);
  }
}

FileReplacementOutcome InstallReplacementFile(const fs::path& replacement, const fs::path& destination, FileReplacementDurability durability)
{
  return InstallReplacementFile(replacement, destination, PlatformFileReplacementSteps(durability));
}

std::string DescribeFileReplacementFailure(const FileReplacementOutcome& outcome, const fs::path& destination)
{
  assert(outcome.error);
  std::string description;
  if(outcome.renamed)
  {
    description = fmt::format("Could not flush the replacement at '{}'. Save the output again to confirm it reaches disk.", destination);
  }
  else if(outcome.retriesExhausted)
  {
    description = fmt::format("Could not replace output file '{}'. The file is probably open in another program or in this program, or is read-only. "
                              "Examples include the Windows Search indexer, a virus scanner, and a backup tool. Make sure the file is closed and writable.",
                              destination);
  }
  else
  {
    description = fmt::format("Could not replace output file '{}'. Check that the files and the output folder are accessible.", destination);
  }
  description += fmt::format(" Operating system error: {}", outcome.error.message());
  if(outcome.attempts > 1)
  {
    description += fmt::format(" ({} attempts, {} ms total requested wait).", outcome.attempts, outcome.waited.count());
  }
  return description;
}
} // namespace nx::core
