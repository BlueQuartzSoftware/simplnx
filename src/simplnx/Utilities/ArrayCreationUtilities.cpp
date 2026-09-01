#include "ArrayCreationUtilities.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataStoreFormatResolver.hpp"
#include "simplnx/Utilities/MemoryUtilities.hpp"

using namespace nx::core;

std::string ArrayCreationUtilities::ResolveStorageFormat(const DataStructure& dataStructure, const DataPath& path, DataType numericType, uint64 dataSizeBytes, const std::string& requestedFormat)
{
  // An explicit format overrides automatic policy.
  if(!requestedFormat.empty())
  {
    return requestedFormat;
  }
  // The DataStructure resolver handles the remaining automatic request.
  return dataStructure.formatResolver().resolveFormat(dataStructure, path, numericType, dataSizeBytes);
}

bool ArrayCreationUtilities::CheckMemoryRequirement(const DataStructure& dataStructure, uint64 requiredMemory)
{
  static const uint64 k_AvailableMemory = Memory::GetTotalMemory();
  const uint64 memoryUsage = dataStructure.memoryUsage() + requiredMemory;
  return memoryUsage < k_AvailableMemory;
}

bool ArrayCreationUtilities::WouldExceedAvailableMemory(uint64 currentUsageBytes, uint64 requiredMemory, uint64 availableBytes)
{
  // Overflow-safe form of (currentUsageBytes + requiredMemory) > availableBytes.
  if(requiredMemory > availableBytes)
  {
    return true;
  }
  return currentUsageBytes > (availableBytes - requiredMemory);
}
