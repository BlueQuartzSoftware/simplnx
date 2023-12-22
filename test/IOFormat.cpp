#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/Utilities/MemoryUtilities.hpp"

using namespace nx::core;

TEST_CASE("Contains HDF5 IO Support", "IOTest")
{
  auto app = Application::GetOrCreateInstance();

  auto ioCollection = app->getIOCollection();
  auto h5IO = ioCollection->getManager("HDF5");
  REQUIRE(h5IO != nullptr);
}

TEST_CASE("Memory Check", "IOTest")
{
  REQUIRE(Memory::GetTotalMemory() > 0);
  const auto storage = Memory::GetAvailableStorage();
  REQUIRE(storage.total > 0);
  REQUIRE(storage.free > 0);
}

TEST_CASE("Target DataStructure Size", "IOTest")
{
  auto* preferences = Application::GetOrCreateInstance()->getPreferences();
  REQUIRE(preferences->largeDataStructureSize() > 0);

  const uint64 memory = Memory::GetTotalMemory();
  const uint64 largeDataSize = preferences->valueAs<uint64>(Preferences::k_LargeDataSize_Key);
  const uint64 minimumRemaining = 2 * largeDataSize;
  uint64 targetReducedSize = (memory - 2 * largeDataSize);
  if(minimumRemaining >= memory)
  {
    targetReducedSize = memory / 2;
  }
  REQUIRE(preferences->defaultValueAs<uint64>(Preferences::k_LargeDataStructureSize_Key) == targetReducedSize);
}
