#include "simplnx/Core/Preferences.hpp"

#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
struct TemporaryPreferenceFiles
{
  fs::path input = fs::temp_directory_path() / "simplnx_cache_memory_budget_migration_input.json";
  fs::path output = fs::temp_directory_path() / "simplnx_cache_memory_budget_migration_output.json";

  ~TemporaryPreferenceFiles()
  {
    std::error_code errorCode;
    fs::remove(input, errorCode);
    fs::remove(output, errorCode);
  }
};

void writeJson(const fs::path& path, const nlohmann::json& json)
{
  std::ofstream stream(path);
  REQUIRE(stream.is_open());
  stream << json;
}

nlohmann::json readJson(const fs::path& path)
{
  std::ifstream stream(path);
  REQUIRE(stream.is_open());
  return nlohmann::json::parse(stream);
}
} // namespace

TEST_CASE("DataStorageMode migrates from legacy keys", "[Core][Preferences]")
{
  Preferences prefs;
  prefs.setValue(Preferences::k_ForceOocData_Key, true);
  REQUIRE(prefs.dataStorageMode() == DataStorageMode::ForceOutOfCore);

  Preferences p2;
  p2.setValue(Preferences::k_ForceOocData_Key, false);
  p2.setValue(Preferences::k_PreferredLargeDataFormat_Key, std::string(Preferences::k_InMemoryFormat));
  REQUIRE(p2.dataStorageMode() == DataStorageMode::ForceInCore);

  Preferences p3;
  p3.setValue(Preferences::k_ForceOocData_Key, false);
  p3.setValue(Preferences::k_PreferredLargeDataFormat_Key, std::string("HDF5-OOC"));
  REQUIRE(p3.dataStorageMode() == DataStorageMode::Adaptive);

  // Fresh prefs with no legacy user values default to Adaptive.
  Preferences p4;
  REQUIRE(p4.dataStorageMode() == DataStorageMode::Adaptive);

  // The canonical key round-trips through set/get.
  Preferences p5;
  p5.setDataStorageMode(DataStorageMode::ForceOutOfCore);
  REQUIRE(p5.dataStorageMode() == DataStorageMode::ForceOutOfCore);

  // An explicit canonical mode takes precedence over any legacy keys present.
  Preferences p6;
  p6.setValue(Preferences::k_ForceOocData_Key, false); // legacy would imply in-core
  p6.setValue(Preferences::k_PreferredLargeDataFormat_Key, std::string(Preferences::k_InMemoryFormat));
  p6.setDataStorageMode(DataStorageMode::ForceOutOfCore);
  REQUIRE(p6.dataStorageMode() == DataStorageMode::ForceOutOfCore);
}

TEST_CASE("Cache memory budget preference migrates to the unambiguous key", "[Core][Preferences]")
{
  constexpr uint64 k_LegacyBudget = 3ULL * 1024 * 1024 * 1024;
  constexpr uint64 k_CanonicalBudget = 5ULL * 1024 * 1024 * 1024;
  TemporaryPreferenceFiles files;

  const auto verifyMigration = [&](const nlohmann::json& input, uint64 expectedBudget) {
    writeJson(files.input, input);

    Preferences preferences;
    const Result<> loadResult = preferences.loadFromFile(files.input);
    REQUIRE(loadResult.valid());
    REQUIRE(preferences.cacheMemoryBudgetBytes() == expectedBudget);
    REQUIRE(preferences.contains(std::string(Preferences::k_CacheMemoryBudgetBytes_Key)));
    REQUIRE_FALSE(preferences.contains(std::string(Preferences::k_LegacyMemoryBudgetBytes_Key)));

    const Result<> saveResult = preferences.saveToFile(files.output);
    REQUIRE(saveResult.valid());
    const nlohmann::json saved = readJson(files.output);
    REQUIRE(saved.at(std::string(Preferences::k_CacheMemoryBudgetBytes_Key)).get<uint64>() == expectedBudget);
    REQUIRE_FALSE(saved.contains(std::string(Preferences::k_LegacyMemoryBudgetBytes_Key)));
  };

  SECTION("Legacy-only value migrates")
  {
    verifyMigration({{"memory_budget_bytes", k_LegacyBudget}}, k_LegacyBudget);
  }

  SECTION("Canonical value remains authoritative")
  {
    verifyMigration({{"cache_memory_budget_bytes", k_CanonicalBudget}}, k_CanonicalBudget);
  }

  SECTION("Canonical value wins when both keys exist")
  {
    verifyMigration({{"memory_budget_bytes", k_LegacyBudget}, {"cache_memory_budget_bytes", k_CanonicalBudget}}, k_CanonicalBudget);
  }
}
