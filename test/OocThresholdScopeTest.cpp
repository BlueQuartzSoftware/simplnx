#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/UnitTest/AlgorithmTestScope.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>

using namespace nx::core;

namespace
{
std::filesystem::path requireOwnedProfile()
{
  const char* rootText = std::getenv("SIMPLNX_P1_OWNED_ROOT");
  REQUIRE(rootText != nullptr);
  const std::filesystem::path root(rootText);
  REQUIRE(root.is_absolute());
#if defined(_WIN32)
  const auto expected = root / "AppData/Local/DREAM3DNX/preferences.json";
#elif defined(__APPLE__)
  const auto expected = root / "Library/Preferences/DREAM3DNX/preferences.json";
#else
  const auto expected = root / ".config/DREAM3DNX/preferences.json";
#endif
  REQUIRE(Preferences::DefaultFilePath("DREAM3DNX") == expected);
  REQUIRE(Application::Instance() == nullptr);
  return expected;
}

void requireThreshold(const Preferences& preferences, bool present, nlohmann::json::value_t type, uint64 value)
{
  REQUIRE(preferences.contains(Preferences::k_LargeDataSize_Key) == present);
  const auto actual = preferences.value(Preferences::k_LargeDataSize_Key);
  REQUIRE(actual.type() == type);
  REQUIRE(actual.is_number_integer());
  CHECK(actual.get<uint64>() == value);
}
} // namespace

TEST_CASE("P1 owned shared sentinel restores exact threshold and presence", "[.P1OwnedThreshold]")
{
  requireOwnedProfile();
  auto app = Application::GetOrCreateInstance();
  auto* preferences = app->getPreferences();
  preferences->removeValue(Preferences::k_LargeDataSize_Key);
  preferences->removeValue(Preferences::k_DataStorageMode_Key);

  {
    UnitTest::PreferencesSentinel sentinel(DataStorageMode::ForceInCore, nlohmann::json(uint64{0}));
    REQUIRE(preferences->dataStorageMode() == DataStorageMode::ForceInCore);
    requireThreshold(*preferences, true, nlohmann::json::value_t::number_unsigned, 0);
  }
  CHECK_FALSE(preferences->contains(Preferences::k_LargeDataSize_Key));
  CHECK_FALSE(preferences->contains(Preferences::k_DataStorageMode_Key));

  preferences->setValue(Preferences::k_LargeDataSize_Key, std::numeric_limits<uint64>::max());
  preferences->setDataStorageMode(DataStorageMode::Adaptive);
  const auto originalMode = preferences->value(Preferences::k_DataStorageMode_Key);
  {
    UnitTest::PreferencesSentinel sentinel(DataStorageMode::ForceOutOfCore, nlohmann::json(uint64{17}));
    requireThreshold(*preferences, true, nlohmann::json::value_t::number_unsigned, 17);
  }
  requireThreshold(*preferences, true, nlohmann::json::value_t::number_unsigned, std::numeric_limits<uint64>::max());
  CHECK(preferences->value(Preferences::k_DataStorageMode_Key) == originalMode);

  CHECK_THROWS_AS(UnitTest::PreferencesSentinel(DataStorageMode::ForceInCore, int64{-1}), std::invalid_argument);
  requireThreshold(*preferences, true, nlohmann::json::value_t::number_unsigned, std::numeric_limits<uint64>::max());
  CHECK(preferences->value(Preferences::k_DataStorageMode_Key) == originalMode);

  for(const auto& value : {nlohmann::json(int64{37}), nlohmann::json(uint64{37})})
  {
    preferences->setValue(Preferences::k_LargeDataSize_Key, value);
    {
      UnitTest::PreferencesSentinel sentinel(DataStorageMode::ForceInCore, nlohmann::json(uint64{0}));
    }
    requireThreshold(*preferences, true, value.type(), 37);
  }

  app.reset();
  Application::DeleteInstance();
}

TEST_CASE("P1 owned public algorithm scope retains maximum threshold", "[.P1OwnedThreshold]")
{
  requireOwnedProfile();
  auto app = Application::GetOrCreateInstance();
  auto* preferences = app->getPreferences();
  preferences->setValue(Preferences::k_LargeDataSize_Key, std::numeric_limits<uint64>::max());
  {
    UnitTest::AlgorithmTestScope scope(UnitTest::AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore);
    REQUIRE(preferences->dataStorageMode() == DataStorageMode::ForceInCore);
  }
  requireThreshold(*preferences, true, nlohmann::json::value_t::number_unsigned, std::numeric_limits<uint64>::max());
  preferences->removeValue(Preferences::k_LargeDataSize_Key);
  preferences->removeValue(Preferences::k_DataStorageMode_Key);
  {
    UnitTest::AlgorithmTestScope scope(UnitTest::AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore);
  }
  CHECK_FALSE(preferences->contains(Preferences::k_LargeDataSize_Key));
  CHECK_FALSE(preferences->contains(Preferences::k_DataStorageMode_Key));
  app.reset();
  Application::DeleteInstance();
}

TEST_CASE("P1 owned Application retains startup correction through clean reload", "[.P1OwnedThreshold]")
{
  const auto profile = requireOwnedProfile();
  std::filesystem::create_directories(profile.parent_path());
  {
    std::ofstream stream(profile);
    REQUIRE(stream.is_open());
    stream << "{\"large_data_size\":-1,\"large_datastructure_size\":18446744073709551615,\"plugins\":{\"P1\":{\"keep\":17}}}";
  }
  Preferences freshDefaults;
  const uint64 expectedDefault = freshDefaults.defaultValueAs<uint64>(Preferences::k_LargeDataSize_Key);
  auto app = Application::GetOrCreateInstance();
  auto* preferences = app->getPreferences();
  CHECK(preferences->valueAs<uint64>(Preferences::k_LargeDataSize_Key) == expectedDefault);
  const auto validStructure = preferences->value(Preferences::k_LargeDataStructureSize_Key);
  REQUIRE(preferences->contains(Preferences::k_LargeDataStructureSize_Key));
  REQUIRE(validStructure.is_number_unsigned());
  CHECK(validStructure.get<uint64>() == std::numeric_limits<uint64>::max());
  CHECK(preferences->pluginValue("P1", "keep") == nlohmann::json(17));

  {
    std::ofstream stream(profile);
    REQUIRE(stream.is_open());
    stream << "{\"large_data_size\":0,\"large_datastructure_size\":18446744073709551615,\"plugins\":{\"P1\":{\"keep\":17}}}";
  }
  const auto cleanLoad = app->loadPreferences();
  REQUIRE(cleanLoad.valid());
  CHECK(cleanLoad.warnings().empty());
  const auto first = app->takePreferenceLoadWarnings();
  REQUIRE(first.size() == 1);
  CHECK(first.front().message.find("-1") != std::string::npos);
  CHECK(first.front().message.find(std::to_string(expectedDefault)) != std::string::npos);
  CHECK(app->takePreferenceLoadWarnings().empty());

  {
    std::ofstream stream(profile);
    REQUIRE(stream.is_open());
    stream << "{\"large_datastructure_size\":\"bad\"}";
  }
  REQUIRE(app->loadPreferences().valid());
  {
    std::ofstream stream(profile);
    REQUIRE(stream.is_open());
    stream << "{\"large_data_size\":-2}";
  }
  REQUIRE(app->loadPreferences().valid());
  const auto ordered = app->takePreferenceLoadWarnings();
  REQUIRE(ordered.size() == 2);
  CHECK(ordered[0].message.find("large_datastructure_size") != std::string::npos);
  CHECK(ordered[1].message.find("large_data_size") != std::string::npos);
  CHECK(app->takePreferenceLoadWarnings().empty());

  app.reset();
  Application::DeleteInstance();
}
