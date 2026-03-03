#include "MemoryUtilities.hpp"

#if defined(_WIN32)
#include <cstdlib>
#include <windows.h>

#include <psapi.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <sys/sysctl.h>
#include <unistd.h>
#else
#include <fstream>
#include <sstream>
#include <unistd.h>
#endif

namespace nx::core::Memory
{
dataStorage GetAvailableStorage()
{
  return GetAvailableStorageOnDrive(std::filesystem::temp_directory_path());
}

#if defined(_WIN32)
uint64 GetTotalMemory()
{
  uint64 totalKilos = 0;
  GetPhysicallyInstalledSystemMemory(&totalKilos);

  return totalKilos * 1024;
}

dataStorage GetAvailableStorageOnDrive(const std::filesystem::path& path)
{
  const std::filesystem::path rootDirectory = path.root_directory();
  const std::filesystem::path driveDirectory = path.root_name();

  dataStorage storage;
  const std::string driveName = driveDirectory.string();

  if(GetDriveType(driveName.data()) != DRIVE_FIXED)
  {
    std::printf("not a fixed drive, skipping");
  }
  else
  {
    GetDiskFreeSpaceEx(driveName.data(), NULL, (PULARGE_INTEGER)&storage.total, (PULARGE_INTEGER)&storage.free);
  }

  return storage;
}
#else
uint64 GetTotalMemory()
{
  long pages = sysconf(_SC_PHYS_PAGES);
  long page_size = sysconf(_SC_PAGE_SIZE);
  return pages * page_size;
}

dataStorage GetAvailableStorageOnDrive(const std::filesystem::path& directory)
{
  std::filesystem::space_info info = std::filesystem::space(directory);
  dataStorage storage;
  storage.free = info.available;
  storage.total = info.capacity;
  return storage;
}
#endif

// =============================================================================
// GetSystemMemoryInfo — platform implementations
// =============================================================================

#if defined(_WIN32)

SystemMemoryInfo GetSystemMemoryInfo()
{
  SystemMemoryInfo info;

  MEMORYSTATUSEX memStatus;
  memStatus.dwLength = sizeof(MEMORYSTATUSEX);
  if(GlobalMemoryStatusEx(&memStatus))
  {
    info.totalGB = static_cast<double>(memStatus.ullTotalPhys) / (1024.0 * 1024.0 * 1024.0);
    const double availableGB = static_cast<double>(memStatus.ullAvailPhys) / (1024.0 * 1024.0 * 1024.0);
    info.usedGB = info.totalGB - availableGB;
    info.loadPercent = static_cast<double>(memStatus.dwMemoryLoad);
  }

  PROCESS_MEMORY_COUNTERS_EX pmc;
  const HANDLE hProcess = GetCurrentProcess();
  if(GetProcessMemoryInfo(hProcess, reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc)))
  {
    info.processGB = static_cast<double>(pmc.WorkingSetSize) / (1024.0 * 1024.0 * 1024.0);
  }

  return info;
}

#elif defined(__APPLE__)

namespace
{
// Helper: read a 64-bit integer sysctl value by name.
int sysctlInt64(const char* name, int64_t* value)
{
  size_t len = sizeof(int64_t);
  return sysctlbyname(name, value, &len, nullptr, 0);
}
} // namespace

SystemMemoryInfo GetSystemMemoryInfo()
{
  SystemMemoryInfo info;

  int64_t tempInt64 = 0;

  if(sysctlInt64("hw.memsize", &tempInt64) != 0)
  {
    return info;
  }
  info.totalGB = static_cast<double>(tempInt64) / (1024.0 * 1024.0 * 1024.0);

  vm_statistics64_data_t vmstat;
  mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
  if(host_statistics64(mach_host_self(), HOST_VM_INFO64, reinterpret_cast<host_info64_t>(&vmstat), &count) == KERN_SUCCESS)
  {
    if(sysctlInt64("hw.pagesize", &tempInt64) == 0)
    {
      const int64_t availableBytes = (static_cast<int64_t>(vmstat.free_count) + static_cast<int64_t>(vmstat.inactive_count)) * tempInt64;
      const double availableGB = static_cast<double>(availableBytes) / (1024.0 * 1024.0 * 1024.0);
      info.usedGB = info.totalGB - availableGB;
      if(info.totalGB > 0.0)
      {
        info.loadPercent = info.usedGB * 100.0 / info.totalGB;
      }
    }
  }

  task_vm_info_data_t taskInfo;
  mach_msg_type_number_t taskCount = TASK_VM_INFO_COUNT;
  if(task_info(mach_task_self(), TASK_VM_INFO, reinterpret_cast<task_info_t>(&taskInfo), &taskCount) == KERN_SUCCESS)
  {
    info.processGB = static_cast<double>(taskInfo.phys_footprint) / (1024.0 * 1024.0 * 1024.0);
  }

  return info;
}

#else // Linux

SystemMemoryInfo GetSystemMemoryInfo()
{
  SystemMemoryInfo info;

  // Parse /proc/meminfo for system-wide memory figures
  {
    std::ifstream file("/proc/meminfo");
    if(!file.is_open())
    {
      return info;
    }

    uint64_t memTotal = 0;
    uint64_t memFree = 0;
    uint64_t buffers = 0;
    uint64_t cached = 0;
    uint64_t sReclaimable = 0;

    std::string line;
    while(std::getline(file, line))
    {
      std::istringstream iss(line);
      std::string key;
      uint64_t value = 0;
      std::string unit;
      iss >> key >> value >> unit;

      if(key == "MemTotal:")
        memTotal = value;
      else if(key == "MemFree:")
        memFree = value;
      else if(key == "Buffers:")
        buffers = value;
      else if(key == "Cached:")
        cached = value;
      else if(key == "SReclaimable:")
        sReclaimable = value;
    }

    // Match the 'used' calculation from the 'free' command:
    // used = total - (free + buffers + cached + SReclaimable)
    const uint64_t usedKB = memTotal - (memFree + buffers + cached + sReclaimable);
    info.totalGB = static_cast<double>(memTotal) / (1024.0 * 1024.0);
    info.usedGB = static_cast<double>(usedKB) / (1024.0 * 1024.0);
    if(info.totalGB > 0.0)
    {
      info.loadPercent = info.usedGB * 100.0 / info.totalGB;
    }
  }

  // Read current process resident set size from /proc/self/statm
  {
    std::ifstream statm("/proc/self/statm");
    if(statm.is_open())
    {
      unsigned long long totalPages = 0;
      unsigned long long residentPages = 0;
      statm >> totalPages >> residentPages;
      const long pageSizeBytes = sysconf(_SC_PAGESIZE);
      info.processGB = static_cast<double>(residentPages) * static_cast<double>(pageSizeBytes) / (1024.0 * 1024.0 * 1024.0);
    }
  }

  return info;
}

#endif

} // namespace nx::core::Memory
