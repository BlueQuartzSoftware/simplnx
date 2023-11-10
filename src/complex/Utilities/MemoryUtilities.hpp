#pragma once

#include "complex/Common/Types.hpp"
#include "complex/complex_export.hpp"

#include <filesystem>

namespace complex
{
namespace Memory
{
struct COMPLEX_EXPORT dataStorage
{
  uint64_t total = 0;
  uint64_t free = 0;
};

uint64 COMPLEX_EXPORT GetTotalMemory();
dataStorage COMPLEX_EXPORT GetAvailableStorage();
dataStorage COMPLEX_EXPORT GetAvailableStorageOnDrive(const std::filesystem::path& path);
} // namespace Memory
} // namespace complex
