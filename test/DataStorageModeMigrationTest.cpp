#include "simplnx/Core/Preferences.hpp"

#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

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

TEST_CASE("DataStorageMode file migration normalizes legacy formats", "[Core][Preferences]")
{
  TemporaryPreferenceFiles files;

  const auto verifyMigration = [&](const nlohmann::json& input, DataStorageMode expectedMode, bool expectsLegacyFormat) {
    writeJson(files.input, input);

    Preferences preferences;
    const Result<> loadResult = preferences.loadFromFile(files.input);
    REQUIRE(loadResult.valid());
    REQUIRE(preferences.dataStorageMode() == expectedMode);
    REQUIRE(preferences.contains(std::string(Preferences::k_PreferredLargeDataFormat_Key)) == expectsLegacyFormat);

    const Result<> saveResult = preferences.saveToFile(files.output);
    REQUIRE(saveResult.valid());
    const nlohmann::json saved = readJson(files.output);
    REQUIRE(saved.contains(std::string(Preferences::k_PreferredLargeDataFormat_Key)) == expectsLegacyFormat);

    Preferences reloaded;
    const Result<> reloadResult = reloaded.loadFromFile(files.output);
    REQUIRE(reloadResult.valid());
    REQUIRE(reloaded.dataStorageMode() == expectedMode);
    REQUIRE(reloaded.contains(std::string(Preferences::k_PreferredLargeDataFormat_Key)) == expectsLegacyFormat);
  };

  SECTION("Empty legacy format is removed and selects Adaptive")
  {
    verifyMigration({{std::string(Preferences::k_PreferredLargeDataFormat_Key), ""}}, DataStorageMode::Adaptive, false);
  }

  SECTION("Hyphenated legacy in-memory format is removed and selects Adaptive")
  {
    verifyMigration({{std::string(Preferences::k_PreferredLargeDataFormat_Key), "In-Memory"}}, DataStorageMode::Adaptive, false);
  }

  SECTION("Explicit InMemory sentinel remains and selects ForceInCore")
  {
    verifyMigration({{std::string(Preferences::k_PreferredLargeDataFormat_Key), std::string(Preferences::k_InMemoryFormat)}}, DataStorageMode::ForceInCore, true);
  }

  SECTION("Canonical mode overrides a retained legacy format")
  {
    verifyMigration({{std::string(Preferences::k_DataStorageMode_Key), static_cast<int>(DataStorageMode::ForceOutOfCore)},
                     {std::string(Preferences::k_PreferredLargeDataFormat_Key), std::string(Preferences::k_InMemoryFormat)}},
                    DataStorageMode::ForceOutOfCore, true);
  }

  SECTION("Out-of-range canonical mode selects Adaptive")
  {
    verifyMigration({{std::string(Preferences::k_DataStorageMode_Key), 99}}, DataStorageMode::Adaptive, false);
  }
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

namespace
{
/**
 * @struct ThresholdPreferenceFiles
 * @brief Owns unique files without reading or changing the process preferences.
 */
struct ThresholdPreferenceFiles
{
  std::filesystem::path root;
  std::filesystem::path input;
  std::filesystem::path output;

  ThresholdPreferenceFiles()
  {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    for(usize attempt = 0; attempt < 1000; ++attempt)
    {
      const auto candidate = std::filesystem::temp_directory_path() / ("simplnx_threshold_preferences_" + std::to_string(stamp) + "_" + std::to_string(attempt));
      if(std::filesystem::create_directory(candidate))
      {
        root = candidate;
        input = root / "input.json";
        output = root / "output.json";
        return;
      }
    }
    throw std::runtime_error("Could not create a unique temporary directory for threshold preference tests.");
  }

  ~ThresholdPreferenceFiles()
  {
    std::error_code errorCode;
    std::filesystem::remove(input, errorCode);
    std::filesystem::remove(output, errorCode);
    std::filesystem::remove(root, errorCode);
  }

  ThresholdPreferenceFiles(const ThresholdPreferenceFiles&) = delete;
  ThresholdPreferenceFiles& operator=(const ThresholdPreferenceFiles&) = delete;
};

const std::array<std::string, 2> k_ThresholdKeys = {std::string(Preferences::k_LargeDataSize_Key), std::string(Preferences::k_LargeDataStructureSize_Key)};
const std::array<std::string, 2> k_ThresholdLabels = {"Large Data Size", "Large Data Structure Size"};

std::vector<nlohmann::json> invalidThresholdValues()
{
  return {nlohmann::json(int64{-1}),
          nlohmann::json(1.25),
          nlohmann::json(1.0),
          nlohmann::json("0"),
          nlohmann::json(true),
          nlohmann::json(nullptr),
          nlohmann::json::array({1}),
          nlohmann::json::object({{"value", 1}}),
          nlohmann::json(18446744073709551616.0)};
}

void writeThresholdJson(const std::filesystem::path& path, const nlohmann::json& value)
{
  std::ofstream stream(path);
  REQUIRE(stream.is_open());
  stream << value;
  stream.close();
  REQUIRE_FALSE(stream.fail());
}

nlohmann::json readThresholdJson(const std::filesystem::path& path)
{
  std::ifstream stream(path);
  REQUIRE(stream.is_open());
  return nlohmann::json::parse(stream);
}
} // namespace

TEST_CASE("Preferences rejects invalid new OOC thresholds before mutation", "[Core][Preferences][OocThreshold]")
{
  for(const auto& key : k_ThresholdKeys)
  {
    for(const auto& invalidValue : invalidThresholdValues())
    {
      DYNAMIC_SECTION(key << "=" << invalidValue.dump())
      {
        Preferences preferences;
        preferences.setValue(key, uint64{17});
        preferences.setDataStorageMode(DataStorageMode::ForceInCore);
        preferences.setPluginValue("ThresholdTest", "nested", nlohmann::json::object({{"keep", "unchanged"}}));
        const auto originalDefault = preferences.defaultValue(Preferences::k_LargeDataStructureSize_Key);

        CHECK_THROWS_AS(preferences.setValue(key, invalidValue), std::invalid_argument);
        CHECK(preferences.value(key) == nlohmann::json(uint64{17}));
        CHECK(preferences.defaultValue(Preferences::k_LargeDataStructureSize_Key) == originalDefault);
        CHECK(preferences.dataStorageMode() == DataStorageMode::ForceInCore);
        CHECK(preferences.pluginValue("ThresholdTest", "nested") == nlohmann::json::object({{"keep", "unchanged"}}));
      }
    }
  }
}

TEST_CASE("Preferences preserves valid OOC threshold values", "[Core][Preferences][OocThreshold]")
{
  const std::vector<nlohmann::json> validValues = {nlohmann::json(int64{0}), nlohmann::json(int64{37}), nlohmann::json(uint64{0}), nlohmann::json(uint64{37}),
                                                   nlohmann::json(std::numeric_limits<uint64>::max())};
  for(const auto& key : k_ThresholdKeys)
  {
    for(const auto& validValue : validValues)
    {
      DYNAMIC_SECTION(key << "=" << validValue.dump() << " type=" << static_cast<int>(validValue.type()))
      {
        ThresholdPreferenceFiles files;
        Preferences preferences;
        const auto originalDefault = preferences.defaultValue(Preferences::k_LargeDataStructureSize_Key);
        REQUIRE_NOTHROW(preferences.setValue(key, validValue));
        CHECK(preferences.value(key) == validValue);
        CHECK(preferences.value(key).type() == validValue.type());
        CHECK(preferences.defaultValue(Preferences::k_LargeDataStructureSize_Key) == originalDefault);
        REQUIRE(preferences.saveToFile(files.input).valid());

        Preferences loaded;
        const auto result = loaded.loadFromFile(files.input);
        REQUIRE(result.valid());
        CHECK(result.warnings().empty());
        REQUIRE(loaded.contains(key));
        const auto loadedValue = loaded.value(key);
        REQUIRE(loadedValue.is_number_integer());
        if(validValue.get<uint64>() == std::numeric_limits<uint64>::max())
        {
          REQUIRE(loadedValue.is_number_unsigned());
        }
        CHECK(loadedValue.get<uint64>() == validValue.get<uint64>());
        REQUIRE(loaded.saveToFile(files.output).valid());
        const auto saved = readThresholdJson(files.output);
        REQUIRE(saved.contains(key));
        const auto& savedValue = saved.at(key);
        REQUIRE(savedValue.is_number_integer());
        if(validValue.get<uint64>() == std::numeric_limits<uint64>::max())
        {
          REQUIRE(savedValue.is_number_unsigned());
        }
        CHECK(savedValue.get<uint64>() == validValue.get<uint64>());
      }
    }
  }
}

TEST_CASE("Preferences preserves absent OOC threshold keys", "[Core][Preferences][OocThreshold]")
{
  ThresholdPreferenceFiles files;
  writeThresholdJson(files.input, nlohmann::json::object({{"plugins", nlohmann::json::object()}}));
  Preferences preferences;
  const auto result = preferences.loadFromFile(files.input);
  REQUIRE(result.valid());
  CHECK(result.warnings().empty());
  REQUIRE(preferences.saveToFile(files.output).valid());
  const auto saved = readThresholdJson(files.output);
  for(const auto& key : k_ThresholdKeys)
  {
    CHECK_FALSE(preferences.contains(key));
    CHECK(preferences.value(key) == preferences.defaultValue(key));
    CHECK_FALSE(saved.contains(key));
  }
}

TEST_CASE("Preferences repairs only invalid saved OOC threshold fields", "[Core][Preferences][OocThreshold]")
{
  for(usize keyIndex = 0; keyIndex < k_ThresholdKeys.size(); ++keyIndex)
  {
    const auto& invalidKey = k_ThresholdKeys[keyIndex];
    const auto& validKey = k_ThresholdKeys[1 - keyIndex];
    for(const auto& invalidValue : invalidThresholdValues())
    {
      DYNAMIC_SECTION(invalidKey << "=" << invalidValue.dump())
      {
        ThresholdPreferenceFiles files;
        const auto nested = nlohmann::json::object({{"keep", nlohmann::json::array({"value", 43})}});
        const auto original = nlohmann::json::object({{invalidKey, invalidValue},
                                                      {validKey, std::numeric_limits<uint64>::max()},
                                                      {"plugins", {{"ThresholdTest", nested}}},
                                                      {"cache_memory_budget_bytes", uint64{123456}},
                                                      {"unrelated", "preserved"}});
        writeThresholdJson(files.input, original);

        Preferences preferences;
        const auto defaultBytes = preferences.defaultValueAs<uint64>(invalidKey);
        const auto result = preferences.loadFromFile(files.input);
        REQUIRE(result.valid());
        REQUIRE(result.warnings().size() == 1);
        const auto& message = result.warnings().front().message;
        CHECK(message.find(k_ThresholdLabels[keyIndex]) != std::string::npos);
        CHECK(message.find(invalidKey) != std::string::npos);
        CHECK(message.find(invalidValue.dump()) != std::string::npos);
        CHECK(message.find(invalidValue.type_name()) != std::string::npos);
        CHECK(message.find(std::to_string(std::numeric_limits<uint64>::max())) != std::string::npos);
        CHECK(message.find(std::to_string(defaultBytes)) != std::string::npos);
        CHECK(message.find("Preferences > Out of Core") != std::string::npos);
        CHECK(preferences.value(invalidKey) == preferences.defaultValue(invalidKey));
        REQUIRE(preferences.contains(validKey));
        const auto retainedValue = preferences.value(validKey);
        REQUIRE(retainedValue.is_number_unsigned());
        CHECK(retainedValue.get<uint64>() == std::numeric_limits<uint64>::max());
        CHECK(preferences.pluginValue("ThresholdTest", "keep") == nested.at("keep"));
        CHECK(preferences.cacheMemoryBudgetBytes() == uint64{123456});
        CHECK(preferences.value("unrelated") == nlohmann::json("preserved"));
        CHECK(readThresholdJson(files.input) == original);
      }
    }
  }
}

TEST_CASE("Preferences rejects non-object documents without changing existing state", "[Core][Preferences][OocThreshold]")
{
  const std::vector<nlohmann::json> invalidRoots = {nlohmann::json::array({1}), nlohmann::json(nullptr), nlohmann::json("text"), nlohmann::json(7), nlohmann::json(true)};
  for(const auto& invalidRoot : invalidRoots)
  {
    DYNAMIC_SECTION(invalidRoot.dump())
    {
      ThresholdPreferenceFiles files;
      writeThresholdJson(files.input, invalidRoot);
      Preferences preferences;
      preferences.setValue(Preferences::k_LargeDataSize_Key, uint64{19});
      preferences.setValue(Preferences::k_LargeDataStructureSize_Key, std::numeric_limits<uint64>::max());
      preferences.setDataStorageMode(DataStorageMode::ForceInCore);
      preferences.setPluginValue("ThresholdTest", "keep", "unchanged");
      Result<> result;
      CHECK_NOTHROW(result = preferences.loadFromFile(files.input));
      CHECK(result.invalid());
      CHECK(preferences.value(Preferences::k_LargeDataSize_Key) == nlohmann::json(uint64{19}));
      CHECK(preferences.value(Preferences::k_LargeDataStructureSize_Key) == nlohmann::json(std::numeric_limits<uint64>::max()));
      CHECK(preferences.dataStorageMode() == DataStorageMode::ForceInCore);
      CHECK(preferences.pluginValue("ThresholdTest", "keep") == nlohmann::json("unchanged"));
    }
  }
}

TEST_CASE("Preferences validates OOC size values without mutation", "[Core][Preferences][OocThreshold]")
{
  for(const auto& key : k_ThresholdKeys)
  {
    for(const nlohmann::json value : {nlohmann::json(int64{0}), nlohmann::json(int64{17}), nlohmann::json(uint64{0}), nlohmann::json(uint64{17}), nlohmann::json(std::numeric_limits<uint64>::max())})
    {
      DYNAMIC_SECTION(key << "=" << value.dump() << " type=" << static_cast<int>(value.type()))
      {
        const auto result = Preferences::ValidateOocSizeValue(key, value);
        REQUIRE(result.valid());
        CHECK(result.value() == value.get<uint64>());
      }
    }
    for(const auto& value : invalidThresholdValues())
    {
      DYNAMIC_SECTION(key << "=" << value.dump() << " type=" << static_cast<int>(value.type()))
      {
        const auto result = Preferences::ValidateOocSizeValue(key, value);
        REQUIRE(result.invalid());
        REQUIRE(result.errors().size() == 1);
        CHECK(result.errors().front().message.find(key) != std::string::npos);
      }
    }
  }
  const auto unknown = Preferences::ValidateOocSizeValue("unrelated", nlohmann::json(uint64{17}));
  REQUIRE(unknown.invalid());

  Preferences preferences;
  const nlohmann::json invalidUtf8(std::string(1, static_cast<char>(0xff)));
  REQUIRE_THROWS_AS(preferences.setValue(k_ThresholdKeys[0], invalidUtf8), std::invalid_argument);
  CHECK_FALSE(preferences.contains(k_ThresholdKeys[0]));
}

TEST_CASE("Preferences names nonfinite OOC thresholds without changing values", "[Core][Preferences][OocThreshold]")
{
  struct NonfiniteCase
  {
    double number;
    const char* description;
  };
  const std::array<NonfiniteCase, 3> values = {
      {{std::numeric_limits<double>::quiet_NaN(), "NaN"}, {std::numeric_limits<double>::infinity(), "+Infinity"}, {-std::numeric_limits<double>::infinity(), "-Infinity"}}};
  for(const auto& key : k_ThresholdKeys)
  {
    for(const auto& nonfinite : values)
    {
      DYNAMIC_SECTION(key << "=" << nonfinite.description)
      {
        const nlohmann::json invalidValue(nonfinite.number);
        const auto validation = Preferences::ValidateOocSizeValue(key, invalidValue);
        REQUIRE(validation.invalid());
        REQUIRE(validation.errors().size() == 1);
        CHECK(validation.errors().front().message.find(nonfinite.description) != std::string::npos);

        Preferences preferences;
        preferences.setValue(key, uint64{17});
        try
        {
          preferences.setValue(key, invalidValue);
          FAIL("A nonfinite threshold must be rejected.");
        } catch(const std::invalid_argument& error)
        {
          CHECK(std::string(error.what()).find(nonfinite.description) != std::string::npos);
        }
        CHECK(preferences.value(key).is_number_unsigned());
        CHECK(preferences.valueAs<uint64>(key) == uint64{17});
      }
    }
  }
}

TEST_CASE("Preferences repairs both invalid saved OOC fields and reloads cleanly", "[Core][Preferences][OocThreshold]")
{
  ThresholdPreferenceFiles files;
  const auto original = nlohmann::json::object({{k_ThresholdKeys[0], int64{-1}}, {k_ThresholdKeys[1], 1.25}, {"plugins", {{"ThresholdTest", {{"keep", "unchanged"}}}}}});
  writeThresholdJson(files.input, original);
  Preferences preferences;
  const auto expectedArray = preferences.defaultValueAs<uint64>(k_ThresholdKeys[0]);
  const auto expectedStructure = preferences.defaultValueAs<uint64>(k_ThresholdKeys[1]);
  const auto result = preferences.loadFromFile(files.input);
  REQUIRE(result.valid());
  REQUIRE(result.warnings().size() == 2);
  CHECK(preferences.valueAs<uint64>(k_ThresholdKeys[0]) == expectedArray);
  CHECK(preferences.valueAs<uint64>(k_ThresholdKeys[1]) == expectedStructure);
  CHECK(preferences.pluginValue("ThresholdTest", "keep") == nlohmann::json("unchanged"));
  CHECK(readThresholdJson(files.input) == original);
  REQUIRE(preferences.saveToFile(files.output).valid());
  Preferences reloaded;
  const auto reload = reloaded.loadFromFile(files.output);
  REQUIRE(reload.valid());
  CHECK(reload.warnings().empty());
  CHECK(reloaded.valueAs<uint64>(k_ThresholdKeys[0]) == expectedArray);
  CHECK(reloaded.valueAs<uint64>(k_ThresholdKeys[1]) == expectedStructure);
}

TEST_CASE("Preferences bounds invalid saved OOC value diagnostics", "[Core][Preferences][OocThreshold]")
{
  const std::array<nlohmann::json, 3> values = {nlohmann::json(std::string(4096, 'x')), nlohmann::json::object({{"nested", std::string(4096, 'y')}}),
                                                nlohmann::json::array({"first-value", std::string(4096, 'z')})};
  for(const auto& invalidValue : values)
  {
    ThresholdPreferenceFiles files;
    writeThresholdJson(files.input, nlohmann::json::object({{k_ThresholdKeys[0], invalidValue}}));
    Preferences preferences;
    const auto result = preferences.loadFromFile(files.input);
    REQUIRE(result.valid());
    REQUIRE(result.warnings().size() == 1);
    const auto& message = result.warnings().front().message;
    CHECK(message.size() < 1024);
    CHECK(message.find(k_ThresholdKeys[0]) != std::string::npos);
    CHECK(message.find("Preferences > Out of Core") != std::string::npos);
    if(invalidValue.is_object())
    {
      CHECK(message.find("nested") != std::string::npos);
      CHECK(message.find("yyyy") != std::string::npos);
    }
    if(invalidValue.is_array())
    {
      CHECK(message.find("first-value") != std::string::npos);
    }
  }
}

TEST_CASE("Preferences threshold repair preserves cache and storage migrations", "[Core][Preferences][OocThreshold]")
{
  ThresholdPreferenceFiles files;
  writeThresholdJson(files.input, nlohmann::json::object({{std::string(Preferences::k_LargeDataSize_Key), int64{-1}},
                                                          {"memory_budget_bytes", uint64{123456}},
                                                          {std::string(Preferences::k_PreferredLargeDataFormat_Key), "In-Memory"},
                                                          {"plugins", {{"ThresholdTest", {{"keep", "unchanged"}}}}}}));

  Preferences preferences;
  const uint64 defaultBytes = preferences.defaultValueAs<uint64>(Preferences::k_LargeDataSize_Key);
  const auto result = preferences.loadFromFile(files.input);
  REQUIRE(result.valid());
  REQUIRE(result.warnings().size() == 1);
  CHECK(preferences.valueAs<uint64>(Preferences::k_LargeDataSize_Key) == defaultBytes);
  CHECK(preferences.cacheMemoryBudgetBytes() == uint64{123456});
  CHECK(preferences.contains(Preferences::k_CacheMemoryBudgetBytes_Key));
  CHECK_FALSE(preferences.contains(Preferences::k_LegacyMemoryBudgetBytes_Key));
  CHECK_FALSE(preferences.contains(Preferences::k_PreferredLargeDataFormat_Key));
  CHECK(preferences.dataStorageMode() == DataStorageMode::Adaptive);
  CHECK(preferences.pluginValue("ThresholdTest", "keep") == nlohmann::json("unchanged"));
}

TEST_CASE("Preferences syntax errors leave existing OOC values unchanged", "[Core][Preferences][OocThreshold]")
{
  ThresholdPreferenceFiles files;
  std::ofstream stream(files.input);
  REQUIRE(stream.is_open());
  stream << "{invalid";
  stream.close();

  Preferences preferences;
  preferences.setValue(Preferences::k_LargeDataSize_Key, std::numeric_limits<uint64>::max());
  preferences.setDataStorageMode(DataStorageMode::ForceInCore);
  const auto result = preferences.loadFromFile(files.input);
  REQUIRE(result.invalid());
  const auto retained = preferences.value(Preferences::k_LargeDataSize_Key);
  REQUIRE(retained.is_number_unsigned());
  CHECK(retained.get<uint64>() == std::numeric_limits<uint64>::max());
  CHECK(preferences.dataStorageMode() == DataStorageMode::ForceInCore);
}
