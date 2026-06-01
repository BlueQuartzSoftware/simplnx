#include <catch2/catch.hpp>

#include "simplnx/Common/SimplnxConfig.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataIOManager.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/MemoryUtilities.hpp"

using namespace nx::core;

TEST_CASE("Contains HDF5 IO Support", "IOTest")
{
  auto app = Application::GetOrCreateInstance();

  auto& ioCollection = app->getIOCollection();
  auto h5IO = ioCollection.getManager("HDF5");
  REQUIRE(h5IO != nullptr);
}

TEST_CASE("Memory Check", "IOTest")
{
  REQUIRE(Memory::GetTotalMemory() > 0);
  const auto storage = Memory::GetAvailableStorage();
  REQUIRE(storage.total > 0);
  REQUIRE(storage.free > 0);
}

// =============================================================================
// Data Format Preference Tests
//
// These verify the in-core build's large-data-format preference behavior. They
// are compiled only when OOC is NOT built in: when SIMPLNX_USE_OOC is defined,
// Preferences seeds the default large-data format to "HDF5-OOC" (so "not
// configured" resolves to OOC rather than in-memory), and that OOC-build
// behavior is covered separately by SimplnxOoc's DataFormatPreferenceTest.
// =============================================================================
#ifndef SIMPLNX_USE_OOC

TEST_CASE("Data Format: Not configured defaults to InMemory store", "[IOTest][DataFormat]")
{
  auto* prefs = Application::GetOrCreateInstance()->getPreferences();

  // With OOC not compiled in and no format explicitly configured,
  // setLargeDataFormat("") clears the key so the seeded default applies. In the
  // in-core build that default is k_InMemoryFormat, so useOocData() is false.
  std::string savedFormat = prefs->largeDataFormat();
  prefs->setLargeDataFormat("");
  REQUIRE(prefs->largeDataFormat() == Preferences::k_InMemoryFormat);
  REQUIRE_FALSE(prefs->useOocData());

  // CreateDataStore should produce an InMemory store regardless of size
  auto store = DataStoreUtilities::CreateDataStore<float32>({100, 100, 100}, {1}, IDataAction::Mode::Execute);
  REQUIRE(store != nullptr);
  REQUIRE(store->getStoreType() == IDataStore::StoreType::InMemory);

  prefs->setLargeDataFormat(savedFormat);
}

TEST_CASE("Data Format: Explicit InMemory format prevents OOC", "[IOTest][DataFormat]")
{
  auto* prefs = Application::GetOrCreateInstance()->getPreferences();

  std::string savedFormat = prefs->largeDataFormat();
  prefs->setLargeDataFormat(std::string(Preferences::k_InMemoryFormat));

  // k_InMemoryFormat is non-empty but should NOT enable OOC
  REQUIRE_FALSE(prefs->largeDataFormat().empty());
  REQUIRE(prefs->largeDataFormat() == Preferences::k_InMemoryFormat);
  REQUIRE_FALSE(prefs->useOocData());

  // CreateDataStore should produce InMemory even for large arrays
  auto store = DataStoreUtilities::CreateDataStore<float32>({100, 100, 100}, {1}, IDataAction::Mode::Execute);
  REQUIRE(store != nullptr);
  REQUIRE(store->getStoreType() == IDataStore::StoreType::InMemory);

  prefs->setLargeDataFormat(savedFormat);
}

TEST_CASE("Data Format: checkUseOoc returns false for empty string", "[IOTest][DataFormat]")
{
  auto* prefs = Application::GetOrCreateInstance()->getPreferences();

  std::string savedFormat = prefs->largeDataFormat();
  prefs->setLargeDataFormat("");
  REQUIRE_FALSE(prefs->useOocData());
  prefs->setLargeDataFormat(savedFormat);
}

TEST_CASE("Data Format: checkUseOoc returns false for InMemory format", "[IOTest][DataFormat]")
{
  auto* prefs = Application::GetOrCreateInstance()->getPreferences();

  std::string savedFormat = prefs->largeDataFormat();
  prefs->setLargeDataFormat(std::string(Preferences::k_InMemoryFormat));
  REQUIRE_FALSE(prefs->useOocData());
  prefs->setLargeDataFormat(savedFormat);
}

#endif // !SIMPLNX_USE_OOC

TEST_CASE("Data Format: Cannot register IO manager with reserved InMemory name", "[IOTest][DataFormat]")
{
  // Create a dummy IDataIOManager subclass that returns k_InMemoryFormat
  class ReservedNameManager : public IDataIOManager
  {
  public:
    std::string formatName() const override
    {
      return std::string(Preferences::k_InMemoryFormat);
    }
  };

  auto& ioCollection = Application::GetOrCreateInstance()->getIOCollection();
  auto badManager = std::make_shared<ReservedNameManager>();
  auto addResult = ioCollection.addIOManager(badManager);
  SIMPLNX_RESULT_REQUIRE_INVALID(addResult);
}
