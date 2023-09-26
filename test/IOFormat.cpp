#include <catch2/catch.hpp>

#include "complex/Core/Application.hpp"
#include "complex/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "complex/Utilities/MemoryUtilities.hpp"

using namespace complex;

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
  const uint64 targetReducedSize = (memory - largeDataSize);
  REQUIRE(preferences->defaultValueAs<uint64>(Preferences::k_LargeDataStructureSize_Key) == targetReducedSize);
}