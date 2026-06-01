#include "ArrayCreationUtilities.hpp"

#include "simplnx/Utilities/MemoryUtilities.hpp"

using namespace nx::core;

//-----------------------------------------------------------------------------
bool ArrayCreationUtilities::CheckMemoryRequirement(const DataStructure& dataStructure, uint64 requiredMemory)
{
  static const uint64 k_AvailableMemory = Memory::GetTotalMemory();
  const uint64 memoryUsage = dataStructure.memoryUsage() + requiredMemory;
  return memoryUsage < k_AvailableMemory;
}
