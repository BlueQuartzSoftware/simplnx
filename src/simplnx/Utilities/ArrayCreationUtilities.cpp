#include "ArrayCreationUtilities.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Utilities/MemoryUtilities.hpp"

using namespace nx::core;

//-----------------------------------------------------------------------------
bool ArrayCreationUtilities::CheckMemoryRequirement(DataStructure& dataStructure, uint64 requiredMemory, std::string& format)
{
  static const uint64 k_AvailableMemory = Memory::GetTotalMemory();

  // Only check if format is set to in-memory
  if(!format.empty())
  {
    return true;
  }

  Preferences* preferencesPtr = Application::GetOrCreateInstance()->getPreferences();

  const uint64 memoryUsage = dataStructure.memoryUsage() + requiredMemory;
  const uint64 largeDataStructureSize = preferencesPtr->largeDataStructureSize();
  const std::string largeDataFormat = preferencesPtr->largeDataFormat();

  if(memoryUsage >= largeDataStructureSize)
  {
    // Check if out-of-core is available / enabled
    if(largeDataFormat.empty() && memoryUsage >= k_AvailableMemory)
    {
      return false;
    }
    // Use out-of-core
    format = largeDataFormat;
  }

  return true;
}
