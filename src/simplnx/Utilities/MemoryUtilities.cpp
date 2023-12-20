#include "MemoryUtilities.hpp"

#if defined(_WIN32)
#include <cstdlib>
#include <windows.h>
#else
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
} // namespace nx::core::Memory
