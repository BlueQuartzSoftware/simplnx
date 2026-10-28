#define CATCH_CONFIG_MAIN

#include "BenchmarkPreferenceSnapshot.hpp"

#include <catch2/catch.hpp>

#include <limits>
#include <stdexcept>

using namespace nx::core;

TEST_CASE("Benchmark preference snapshot restores exact JSON and key presence", "[Core][Preferences][OocThreshold]")
{
  for(const auto& original : {nlohmann::json(int64{0}), nlohmann::json(int64{37}), nlohmann::json(uint64{0}), nlohmann::json(uint64{37}), nlohmann::json(std::numeric_limits<uint64>::max())})
  {
    DYNAMIC_SECTION(original.dump() << " type=" << static_cast<int>(original.type()))
    {
      Preferences preferences;
      preferences.setValue(Preferences::k_LargeDataSize_Key, original);
      preferences.setDataStorageMode(DataStorageMode::Adaptive);
      const auto mode = preferences.value(Preferences::k_DataStorageMode_Key);
      {
        ip_bench::BenchmarkPreferenceSnapshot snapshot(preferences, DataStorageMode::ForceInCore, 19);
        CHECK(preferences.valueAs<uint64>(Preferences::k_LargeDataSize_Key) == 19);
        CHECK(preferences.dataStorageMode() == DataStorageMode::ForceInCore);
      }
      const auto restored = preferences.value(Preferences::k_LargeDataSize_Key);
      REQUIRE(preferences.contains(Preferences::k_LargeDataSize_Key));
      REQUIRE(restored.type() == original.type());
      CHECK(restored.get<uint64>() == original.get<uint64>());
      CHECK(preferences.value(Preferences::k_DataStorageMode_Key) == mode);
      CHECK(preferences.value(Preferences::k_DataStorageMode_Key).type() == mode.type());
      CHECK(preferences.contains(Preferences::k_DataStorageMode_Key));
    }
  }

  Preferences absent;
  absent.removeValue(Preferences::k_LargeDataSize_Key);
  absent.removeValue(Preferences::k_DataStorageMode_Key);
  {
    ip_bench::BenchmarkPreferenceSnapshot snapshot(absent, DataStorageMode::ForceOutOfCore);
    CHECK(absent.valueAs<uint64>(Preferences::k_LargeDataSize_Key) == 0);
  }
  CHECK_FALSE(absent.contains(Preferences::k_LargeDataSize_Key));
  CHECK_FALSE(absent.contains(Preferences::k_DataStorageMode_Key));
}

TEST_CASE("Benchmark preference snapshot rejects a negative threshold before mode mutation", "[Core][Preferences][OocThreshold]")
{
  Preferences preferences;
  preferences.setValue(Preferences::k_LargeDataSize_Key, std::numeric_limits<uint64>::max());
  preferences.setDataStorageMode(DataStorageMode::Adaptive);
  const auto originalThreshold = preferences.value(Preferences::k_LargeDataSize_Key);
  const auto originalMode = preferences.value(Preferences::k_DataStorageMode_Key);
  REQUIRE_THROWS_AS(ip_bench::BenchmarkPreferenceSnapshot(preferences, DataStorageMode::ForceOutOfCore, -1), std::invalid_argument);
  CHECK(preferences.value(Preferences::k_LargeDataSize_Key) == originalThreshold);
  CHECK(preferences.value(Preferences::k_LargeDataSize_Key).type() == originalThreshold.type());
  CHECK(preferences.value(Preferences::k_DataStorageMode_Key) == originalMode);
  CHECK(preferences.value(Preferences::k_DataStorageMode_Key).type() == originalMode.type());
  CHECK(preferences.contains(Preferences::k_LargeDataSize_Key));
  CHECK(preferences.contains(Preferences::k_DataStorageMode_Key));

  Preferences absent;
  REQUIRE_THROWS_AS(ip_bench::BenchmarkPreferenceSnapshot(absent, DataStorageMode::ForceOutOfCore, -1), std::invalid_argument);
  CHECK_FALSE(absent.contains(Preferences::k_LargeDataSize_Key));
  CHECK_FALSE(absent.contains(Preferences::k_DataStorageMode_Key));
}
