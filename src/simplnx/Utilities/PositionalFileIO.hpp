#pragma once

#include "simplnx/Common/Types.hpp"

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
 * @brief Reads bytes after a synchronous absolute seek on Windows.
 * @param h Supplies a valid private read handle.
 * @param buf Receives up to bytes bytes.
 * @param bytes Specifies the requested byte count.
 * @param offset Specifies the absolute file offset.
 * @return Number of bytes read, or -1 on validation or system-call failure.
 * @pre buf points to at least bytes writable bytes.
 *
 * A short read can occur at end of file. The function changes h's file pointer.
 * Do not share h between concurrent calls.
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
 * @brief Closes a valid Windows file handle.
 * @param h Supplies the handle to close.
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
 * @brief Reads bytes from an absolute offset with POSIX pread().
 * @param h Supplies a valid read descriptor.
 * @param buf Receives up to bytes bytes.
 * @param bytes Specifies the requested byte count.
 * @param offset Specifies the absolute file offset.
 * @return Number of bytes read, or -1 on failure.
 * @pre buf points to at least bytes writable bytes. offset fits off_t.
 *
 * A short read can occur at end of file. pread() does not change the descriptor's
 * file position, so concurrent calls can share h.
 */
inline std::ptrdiff_t positionalRead(FileHandle h, void* buf, std::size_t bytes, uint64_t offset)
{
  return ::pread(h, buf, bytes, static_cast<off_t>(offset));
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
