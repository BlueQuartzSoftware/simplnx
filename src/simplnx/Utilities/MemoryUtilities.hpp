#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

#include <filesystem>

namespace nx::core
{
namespace Memory
{

/**
 * @brief Holds a total capacity and free-space measurement for a storage
 * device or filesystem, in bytes.
 */
struct SIMPLNX_EXPORT dataStorage
{
  uint64_t total = 0; ///< Total capacity of the storage device in bytes.
  uint64_t free = 0;  ///< Available (free) space on the storage device in bytes.
};

/**
 * @brief Returns the total amount of physical RAM installed in the system.
 *
 * @return Total physical memory in bytes.
 */
uint64 SIMPLNX_EXPORT GetTotalMemory();

/**
 * @brief Returns the total and free space on the default temporary storage
 * device (equivalent to calling GetAvailableStorageOnDrive() with the
 * platform's temporary directory path).
 *
 * @return dataStorage containing total and free bytes on the temp drive.
 * @see GetAvailableStorageOnDrive()
 */
dataStorage SIMPLNX_EXPORT GetAvailableStorage();

/**
 * @brief Returns the total and free space on the filesystem that contains
 * @p path.
 *
 * @param path Any path that resides on the drive or filesystem to query.
 *             On Windows this determines the drive letter; on POSIX systems
 *             it determines the mount point.
 * @return dataStorage containing total and free bytes on the queried drive.
 * @see GetAvailableStorage()
 */
dataStorage SIMPLNX_EXPORT GetAvailableStorageOnDrive(const std::filesystem::path& path);

/**
 * @brief Snapshot of system-wide and per-process memory statistics.
 *
 * All memory quantities are expressed in gigabytes (GB).
 */
struct SIMPLNX_EXPORT SystemMemoryInfo
{
  double totalGB = 0.0;     ///< Total installed physical RAM in GB.
  double usedGB = 0.0;      ///< Currently used physical RAM in GB (total - available).
  double loadPercent = 0.0; ///< System memory load as a percentage in the range [0, 100].
  double processGB = 0.0;   ///< Resident memory used by the current process in GB.
};

/**
 * @brief Query the operating system for a current snapshot of system and
 * process memory usage.
 *
 * Supported platforms: Windows, macOS, Linux (/proc/meminfo).
 * Returns a zero-initialised struct on any platform error.
 *
 * @return SystemMemoryInfo containing total RAM, used RAM, memory load
 *         percentage, and the calling process's resident memory.
 * @see SystemMemoryInfo
 */
SIMPLNX_EXPORT SystemMemoryInfo GetSystemMemoryInfo();

} // namespace Memory
} // namespace nx::core
