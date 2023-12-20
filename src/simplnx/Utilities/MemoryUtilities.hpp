#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

#include <filesystem>

namespace nx::core
{
namespace Memory
{
struct SIMPLNX_EXPORT dataStorage
{
  uint64_t total = 0;
  uint64_t free = 0;
};

uint64 SIMPLNX_EXPORT GetTotalMemory();
dataStorage SIMPLNX_EXPORT GetAvailableStorage();
dataStorage SIMPLNX_EXPORT GetAvailableStorageOnDrive(const std::filesystem::path& path);
} // namespace Memory
} // namespace nx::core
