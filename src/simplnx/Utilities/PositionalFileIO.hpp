#pragma once

#include "simplnx/Common/Types.hpp"

#include <cerrno>
#include <cstddef>
#include <limits>
#include <string>

// This platform abstraction reads bytes at an absolute file offset without HDF5.
// POSIX pread() permits concurrent reads through one descriptor. The Windows path
// changes a synchronous handle's file pointer. Each concurrent Windows read must
// use a private handle. This header contains no HDF5 or compression policy, so
// other raw readers can reuse it.
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace nx::core::detail
{
#ifdef _WIN32
using FileHandle = HANDLE;
inline FileHandle invalidFileHandle()
{
  return INVALID_HANDLE_VALUE;
}
inline bool isValidFileHandle(FileHandle h)
{
  return h != INVALID_HANDLE_VALUE;
}

/**
 * @brief Opens a Windows file for positional reads.
 * @param path Supplies the narrow native file path.
 * @return Read handle, or invalidFileHandle() on failure.
 *
 * Read and write sharing permits access while HDF5 has the same file open.
 */
inline FileHandle openFileForRead(const std::string& path)
{
  // FILE_SHARE_READ | FILE_SHARE_WRITE so HDF5's own open handle on the same file
  // does not conflict with our independent read handle.
  return CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
}

/**
 * @brief Opens a Windows file for positional reads and writes.
 * @param path Native file path.
 * @return Read-write handle, or `invalidFileHandle()` on failure.
 */
inline FileHandle openFileForReadWrite(const std::string& path)
{
  return CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
}

/**
 * @brief Reads bytes after a synchronous absolute seek on Windows.
 * @param h Valid private read handle.
 * @param buf Receives up to `bytes` bytes.
 * @param bytes Requested byte count.
 * @param offset Absolute file offset.
 * @return Number of bytes read, or -1 on validation or system-call failure.
 * @pre `buf` points to at least `bytes` writable bytes.
 * @note The function changes the handle position. Concurrent calls require separate handles.
 */
inline std::ptrdiff_t positionalRead(FileHandle h, void* buf, std::size_t bytes, uint64_t offset)
{
  if(bytes > static_cast<std::size_t>(std::numeric_limits<DWORD>::max()) || offset > static_cast<uint64_t>(std::numeric_limits<LONGLONG>::max()))
  {
    return -1;
  }

  LARGE_INTEGER fileOffset{};
  fileOffset.QuadPart = static_cast<LONGLONG>(offset);
  if(!SetFilePointerEx(h, fileOffset, nullptr, FILE_BEGIN))
  {
    return -1;
  }

  DWORD bytesRead = 0;
  if(!ReadFile(h, buf, static_cast<DWORD>(bytes), &bytesRead, nullptr))
  {
    return -1;
  }
  return static_cast<std::ptrdiff_t>(bytesRead);
}

/**
 * @brief Writes bytes after a synchronous absolute seek on Windows.
 * @param h Valid private read-write handle.
 * @param buf Supplies `bytes` bytes.
 * @param bytes Requested byte count.
 * @param offset Absolute file offset.
 * @return Number of bytes written, or -1 on validation or system-call failure.
 * @pre `buf` points to at least `bytes` readable bytes.
 * @note The function changes the handle position. Concurrent calls require separate handles.
 */
inline std::ptrdiff_t positionalWrite(FileHandle h, const void* buf, std::size_t bytes, uint64_t offset)
{
  if(bytes > static_cast<std::size_t>(std::numeric_limits<DWORD>::max()) || offset > static_cast<uint64_t>(std::numeric_limits<LONGLONG>::max()))
  {
    return -1;
  }

  LARGE_INTEGER fileOffset{};
  fileOffset.QuadPart = static_cast<LONGLONG>(offset);
  if(!SetFilePointerEx(h, fileOffset, nullptr, FILE_BEGIN))
  {
    return -1;
  }

  DWORD bytesWritten = 0;
  if(!WriteFile(h, buf, static_cast<DWORD>(bytes), &bytesWritten, nullptr))
  {
    return -1;
  }
  return static_cast<std::ptrdiff_t>(bytesWritten);
}

/**
 * @brief Closes a valid Windows file handle.
 * @param h Handle to close.
 */
inline void closeFileHandle(FileHandle h)
{
  CloseHandle(h);
}
#else
using FileHandle = int;
inline FileHandle invalidFileHandle()
{
  return -1;
}
inline bool isValidFileHandle(FileHandle h)
{
  return h >= 0;
}

/**
 * @brief Opens a POSIX file descriptor for positional reads.
 * @param path Supplies the native file path.
 * @return Read descriptor, or invalidFileHandle() on failure.
 */
inline FileHandle openFileForRead(const std::string& path)
{
  return ::open(path.c_str(), O_RDONLY);
}

/**
 * @brief Opens a POSIX file descriptor for positional reads and writes.
 * @param path Native file path.
 * @return Read-write descriptor, or `invalidFileHandle()` on failure.
 */
inline FileHandle openFileForReadWrite(const std::string& path)
{
  return ::open(path.c_str(), O_RDWR);
}

/**
 * @brief Reads bytes from an absolute offset with `pread()`.
 * @param h Valid read descriptor.
 * @param buf Receives up to `bytes` bytes.
 * @param bytes Requested byte count.
 * @param offset Absolute file offset.
 * @return Number of bytes read, or -1 on failure.
 * @pre `buf` points to at least `bytes` writable bytes. `offset` fits in `off_t`.
 */
inline std::ptrdiff_t positionalRead(FileHandle h, void* buf, std::size_t bytes, uint64_t offset)
{
  std::ptrdiff_t result = 0;
  do
  {
    result = ::pread(h, buf, bytes, static_cast<off_t>(offset));
  } while(result < 0 && errno == EINTR);
  return result;
}

/**
 * @brief Writes bytes at an absolute offset with `pwrite()`.
 * @param h Valid read-write descriptor.
 * @param buf Supplies `bytes` bytes.
 * @param bytes Requested byte count.
 * @param offset Absolute file offset.
 * @return Number of bytes written, or -1 on failure.
 * @pre `buf` points to at least `bytes` readable bytes. `offset` fits in `off_t`.
 */
inline std::ptrdiff_t positionalWrite(FileHandle h, const void* buf, std::size_t bytes, uint64_t offset)
{
  std::ptrdiff_t result = 0;
  do
  {
    result = ::pwrite(h, buf, bytes, static_cast<off_t>(offset));
  } while(result < 0 && errno == EINTR);
  return result;
}

/**
 * @brief Closes a valid POSIX file descriptor.
 * @param h Supplies the descriptor to close.
 */
inline void closeFileHandle(FileHandle h)
{
  ::close(h);
}
#endif
} // namespace nx::core::detail
