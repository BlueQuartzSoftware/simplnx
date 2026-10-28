#include "simplnx/Core/Preferences.hpp"

#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
// Expected values are hand calculations with 1 GB = 1024^3 = 1073741824 bytes.
constexpr uint64 k_OneHundredthBytes = 10737418;           // floor(1073741824 / 100 + 0.5)
constexpr uint64 k_OneAndAHalfGBBytes = 1610612736;        // 1.5 * 1073741824
constexpr uint64 k_MaxGridBytes = 18446744073698814198ULL; // 17179869183.99 GB
constexpr uint64 k_MaxHundredths = 1717986918399;          // 17179869183.99 GB
constexpr int32 k_LegacyRepairCode = -591;
constexpr int32 k_GBRepairCode = -592;

/**
 * @struct SizeGBFiles
 * @brief Owns a unique temporary directory with an input and an output preferences file.
 */
struct SizeGBFiles
{
  fs::path root;
  fs::path input;
  fs::path output;

  SizeGBFiles()
  {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    for(usize attempt = 0; attempt < 1000; ++attempt)
    {
      const auto candidate = fs::temp_directory_path() / ("simplnx_size_gb_preferences_" + std::to_string(stamp) + "_" + std::to_string(attempt));
      if(fs::create_directory(candidate))
      {
        root = candidate;
        input = root / "input.json";
        output = root / "output.json";
        return;
      }
    }
    throw std::runtime_error("Could not create a unique temporary directory for GB preference tests.");
  }

  ~SizeGBFiles()
  {
    std::error_code errorCode;
    fs::remove_all(root, errorCode);
  }

  SizeGBFiles(const SizeGBFiles&) = delete;
  SizeGBFiles(SizeGBFiles&&) = delete;
  SizeGBFiles& operator=(const SizeGBFiles&) = delete;
  SizeGBFiles& operator=(SizeGBFiles&&) = delete;
};

void writeText(const fs::path& path, const std::string& text)
{
  std::ofstream stream(path, std::ios::binary);
  REQUIRE(stream.is_open());
  stream << text;
  stream.close();
  REQUIRE_FALSE(stream.fail());
}

std::string readText(const fs::path& path)
{
  std::ifstream stream(path, std::ios::binary);
  REQUIRE(stream.is_open());
  std::stringstream buffer;
  buffer << stream.rdbuf();
  return buffer.str();
}

/**
 * @brief Loads a preferences document from a new file into a fresh Preferences object.
 * @param files Temporary file owner.
 * @param document JSON document to write.
 * @param preferences Receives the loaded values.
 * @return Load result.
 */
Result<> loadDocument(const SizeGBFiles& files, const nlohmann::json& document, Preferences& preferences)
{
  writeText(files.input, document.dump());
  return preferences.loadFromFile(files.input);
}

const std::array<std::string, 2> k_ByteKeys = {std::string(Preferences::k_LargeDataSize_Key), std::string(Preferences::k_LargeDataStructureSize_Key)};
const std::array<std::string, 2> k_GBKeys = {std::string(Preferences::k_LargeDataSizeGB_Key), std::string(Preferences::k_LargeDataStructureSizeGB_Key)};
const std::array<std::string, 2> k_Labels = {"Large Data Size", "Large Data Structure Size"};
} // namespace

TEST_CASE("Preferences OOC size grid converts between bytes and hundredths of a GB", "[Core][Preferences][OocThreshold][PreferencesSizeGB]")
{
  CHECK(Preferences::k_BytesPerGiB == 1073741824);
  CHECK(Preferences::k_MaxOocSizeHundredths == k_MaxHundredths);

  CHECK(Preferences::OocSizeHundredthsToBytes(0) == 0);
  CHECK(Preferences::OocSizeHundredthsToBytes(1) == k_OneHundredthBytes);
  CHECK(Preferences::OocSizeHundredthsToBytes(100) == 1073741824);
  CHECK(Preferences::OocSizeHundredthsToBytes(150) == k_OneAndAHalfGBBytes);
  CHECK(Preferences::OocSizeHundredthsToBytes(k_MaxHundredths) == k_MaxGridBytes);
  // A value above the domain converts as the maximum.
  CHECK(Preferences::OocSizeHundredthsToBytes(k_MaxHundredths + 1) == k_MaxGridBytes);
  CHECK(Preferences::OocSizeHundredthsToBytes(std::numeric_limits<uint64>::max()) == k_MaxGridBytes);

  CHECK(Preferences::OocSizeBytesToHundredths(0) == 0);
  CHECK(Preferences::OocSizeBytesToHundredths(1073741824) == 100);
  CHECK(Preferences::OocSizeBytesToHundredths(123456789) == 11);
  CHECK(Preferences::OocSizeBytesToHundredths(k_MaxGridBytes) == k_MaxHundredths);
  CHECK(Preferences::OocSizeBytesToHundredths(std::numeric_limits<uint64>::max()) == k_MaxHundredths);
  // A positive byte count never becomes 0 GB.
  CHECK(Preferences::OocSizeBytesToHundredths(1) == 1);
  CHECK(Preferences::OocSizeBytesToHundredths(10000) == 1);
  // 0.005 GB is 5368709.12 bytes: 5368709 rounds down to 0 (then becomes 1); 5368710 rounds up to 1.
  CHECK(Preferences::OocSizeBytesToHundredths(5368709) == 1);
  CHECK(Preferences::OocSizeBytesToHundredths(5368710) == 1);
  // 1.005 GB is 1079110533.12 bytes.
  CHECK(Preferences::OocSizeBytesToHundredths(1079110533) == 100);
  CHECK(Preferences::OocSizeBytesToHundredths(1079110534) == 101);

  for(const uint64 hundredths : {uint64{0}, uint64{1}, uint64{99}, uint64{100}, uint64{101}, k_MaxHundredths - 1, k_MaxHundredths})
  {
    CAPTURE(hundredths);
    CHECK(Preferences::OocSizeBytesToHundredths(Preferences::OocSizeHundredthsToBytes(hundredths)) == hundredths);
  }
  usize mismatches = 0;
  for(uint64 hundredths = 0; hundredths <= 200000; ++hundredths)
  {
    mismatches += Preferences::OocSizeBytesToHundredths(Preferences::OocSizeHundredthsToBytes(hundredths)) == hundredths ? 0 : 1;
  }
  constexpr uint64 k_Stride = 17179869; // About 100000 samples across the domain.
  for(uint64 hundredths = 7; hundredths <= k_MaxHundredths; hundredths += k_Stride)
  {
    mismatches += Preferences::OocSizeBytesToHundredths(Preferences::OocSizeHundredthsToBytes(hundredths)) == hundredths ? 0 : 1;
  }
  CHECK(mismatches == 0);
}

TEST_CASE("Preferences loads valid GB thresholds on the 0.01-GB grid", "[Core][Preferences][OocThreshold][PreferencesSizeGB]")
{
  struct ValidCase
  {
    const char* description;
    nlohmann::json value;
    uint64 expectedBytes;
  };
  const std::vector<ValidCase> cases = {{"integer 1", nlohmann::json(int64{1}), 1073741824},
                                        {"unsigned 2", nlohmann::json(uint64{2}), 2147483648},
                                        {"1.5", nlohmann::json(1.5), k_OneAndAHalfGBBytes},
                                        {"integer 0", nlohmann::json(int64{0}), 0},
                                        {"0.0", nlohmann::json(0.0), 0},
                                        {"-0.0", nlohmann::json(-0.0), 0},
                                        {"0.01", nlohmann::json(0.01), k_OneHundredthBytes},
                                        {"0.001", nlohmann::json(0.001), k_OneHundredthBytes},
                                        {"0.004", nlohmann::json(0.004), k_OneHundredthBytes},
                                        {"13.85", nlohmann::json(13.85), 14871324262},
                                        {"maximum", nlohmann::json(17179869183.99), k_MaxGridBytes}};
  for(usize keyIndex = 0; keyIndex < k_GBKeys.size(); ++keyIndex)
  {
    for(const auto& validCase : cases)
    {
      DYNAMIC_SECTION(k_GBKeys[keyIndex] << "=" << validCase.description)
      {
        SizeGBFiles files;
        Preferences preferences;
        const auto result = loadDocument(files, {{k_GBKeys[keyIndex], validCase.value}, {"unrelated", "preserved"}}, preferences);
        REQUIRE(result.valid());
        CHECK(result.warnings().empty());
        REQUIRE(preferences.contains(k_ByteKeys[keyIndex]));
        const auto stored = preferences.value(k_ByteKeys[keyIndex]);
        REQUIRE(stored.is_number_unsigned());
        CHECK(stored.get<uint64>() == validCase.expectedBytes);
        CHECK_FALSE(preferences.contains(k_GBKeys[keyIndex]));
        CHECK_FALSE(preferences.contains(k_ByteKeys[1 - keyIndex]));
        CHECK(preferences.value("unrelated") == nlohmann::json("preserved"));
      }
    }
  }
}

TEST_CASE("Preferences repairs invalid GB thresholds with their own warning", "[Core][Preferences][OocThreshold][PreferencesSizeGB]")
{
  struct InvalidCase
  {
    const char* description;
    nlohmann::json value;
    std::string shownValue;
    std::string shownType;
  };
  const std::vector<InvalidCase> cases = {{"negative integer", nlohmann::json(int64{-1}), "-1", "number_integer"},
                                          {"negative float", nlohmann::json(-0.5), "-0.5", "number_float"},
                                          {"string", nlohmann::json("1.5"), "\"1.5\"", "string"},
                                          {"boolean", nlohmann::json(true), "true", "boolean"},
                                          {"null", nlohmann::json(nullptr), "null", "null"},
                                          {"array", nlohmann::json::array({1}), "[1]", "array"},
                                          {"1e300", nlohmann::json(1e300), "1e+300", "number_float"},
                                          {"above maximum", nlohmann::json(17179869184.0), "17179869184.0", "number_float"},
                                          {"UINT64_MAX", nlohmann::json(std::numeric_limits<uint64>::max()), "18446744073709551615", "number_unsigned"}};
  for(usize keyIndex = 0; keyIndex < k_GBKeys.size(); ++keyIndex)
  {
    for(const auto& invalidCase : cases)
    {
      DYNAMIC_SECTION(k_GBKeys[keyIndex] << "=" << invalidCase.description)
      {
        SizeGBFiles files;
        Preferences preferences;
        const uint64 defaultBytes = preferences.defaultValueAs<uint64>(k_ByteKeys[keyIndex]);
        const uint64 defaultHundredths = (defaultBytes / 1073741824) * 100 + ((defaultBytes % 1073741824) * 100 + 536870912) / 1073741824;
        const std::string defaultText = std::to_string(defaultHundredths / 100) + "." + (defaultHundredths % 100 < 10 ? "0" : "") + std::to_string(defaultHundredths % 100);
        const auto document = nlohmann::json::object({{k_GBKeys[keyIndex], invalidCase.value}, {"unrelated", "preserved"}});
        const auto result = loadDocument(files, document, preferences);
        REQUIRE(result.valid());
        REQUIRE(result.warnings().size() == 1);
        CHECK(result.warnings().front().code == k_GBRepairCode);
        const std::string expected = "Preferences: saved " + k_Labels[keyIndex] + " ('" + k_GBKeys[keyIndex] + "') value " + invalidCase.shownValue + " (JSON type " + invalidCase.shownType +
                                     ") is invalid. Allowed domain: 0 to 17179869183.99 GB. Using the default: " + defaultText + " GB. Review this value in Preferences > Out of Core.";
        CHECK(result.warnings().front().message == expected);
        CHECK_FALSE(preferences.contains(k_ByteKeys[keyIndex]));
        CHECK_FALSE(preferences.contains(k_GBKeys[keyIndex]));
        CHECK(preferences.valueAs<uint64>(k_ByteKeys[keyIndex]) == defaultBytes);
        CHECK(preferences.value("unrelated") == nlohmann::json("preserved"));
        CHECK(nlohmann::json::parse(readText(files.input)) == document);
      }
    }
  }
  SECTION("Large Data Size default text")
  {
    SizeGBFiles files;
    Preferences preferences;
    const auto result = loadDocument(files, {{k_GBKeys[0], int64{-1}}}, preferences);
    REQUIRE(result.warnings().size() == 1);
    CHECK(result.warnings().front().message.find("Using the default: 1.00 GB.") != std::string::npos);
  }
}

TEST_CASE("Preferences converts legacy byte thresholds to the 0.01-GB grid", "[Core][Preferences][OocThreshold][PreferencesSizeGB]")
{
  struct LegacyCase
  {
    const char* description;
    nlohmann::json value;
    uint64 expectedBytes;
  };
  const std::vector<LegacyCase> cases = {{"0", nlohmann::json(uint64{0}), 0},
                                         {"10000 bytes", nlohmann::json(int64{10000}), k_OneHundredthBytes},
                                         {"123456789 bytes", nlohmann::json(uint64{123456789}), 118111601},
                                         {"1 GiB", nlohmann::json(uint64{1073741824}), 1073741824},
                                         {"UINT64_MAX", nlohmann::json(std::numeric_limits<uint64>::max()), k_MaxGridBytes}};
  for(usize keyIndex = 0; keyIndex < k_ByteKeys.size(); ++keyIndex)
  {
    for(const auto& legacyCase : cases)
    {
      DYNAMIC_SECTION(k_ByteKeys[keyIndex] << "=" << legacyCase.description)
      {
        SizeGBFiles files;
        Preferences preferences;
        const auto result = loadDocument(files, {{k_ByteKeys[keyIndex], legacyCase.value}}, preferences);
        REQUIRE(result.valid());
        CHECK(result.warnings().empty());
        const auto stored = preferences.value(k_ByteKeys[keyIndex]);
        REQUIRE(stored.is_number_unsigned());
        CHECK(stored.get<uint64>() == legacyCase.expectedBytes);
        CHECK_FALSE(preferences.contains(k_GBKeys[keyIndex]));
      }
    }
    DYNAMIC_SECTION(k_ByteKeys[keyIndex] << " invalid keeps the legacy warning")
    {
      SizeGBFiles files;
      Preferences preferences;
      const uint64 defaultBytes = preferences.defaultValueAs<uint64>(k_ByteKeys[keyIndex]);
      const auto result = loadDocument(files, {{k_ByteKeys[keyIndex], int64{-1}}}, preferences);
      REQUIRE(result.valid());
      REQUIRE(result.warnings().size() == 1);
      CHECK(result.warnings().front().code == k_LegacyRepairCode);
      CHECK(result.warnings().front().message == "Preferences: saved " + k_Labels[keyIndex] + " ('" + k_ByteKeys[keyIndex] +
                                                     "') value -1 (JSON type number_integer) is invalid. Allowed domain: 0 to 18446744073709551615 bytes. Using the default: " +
                                                     std::to_string(defaultBytes) + " bytes. Review this value in Preferences > Out of Core.");
      CHECK_FALSE(preferences.contains(k_ByteKeys[keyIndex]));
    }
  }
}

TEST_CASE("Preferences gives a saved GB threshold precedence over a byte threshold", "[Core][Preferences][OocThreshold][PreferencesSizeGB]")
{
  for(usize keyIndex = 0; keyIndex < k_GBKeys.size(); ++keyIndex)
  {
    DYNAMIC_SECTION(k_GBKeys[keyIndex] << " valid GB and valid bytes")
    {
      SizeGBFiles files;
      Preferences preferences;
      const auto result = loadDocument(files, {{k_GBKeys[keyIndex], 2}, {k_ByteKeys[keyIndex], uint64{1073741824}}}, preferences);
      REQUIRE(result.valid());
      CHECK(result.warnings().empty());
      CHECK(preferences.valueAs<uint64>(k_ByteKeys[keyIndex]) == 2147483648);
      CHECK_FALSE(preferences.contains(k_GBKeys[keyIndex]));
    }
    DYNAMIC_SECTION(k_GBKeys[keyIndex] << " valid GB and invalid bytes")
    {
      SizeGBFiles files;
      Preferences preferences;
      const auto result = loadDocument(files, {{k_GBKeys[keyIndex], 1.5}, {k_ByteKeys[keyIndex], "bad"}}, preferences);
      REQUIRE(result.valid());
      CHECK(result.warnings().empty());
      CHECK(preferences.valueAs<uint64>(k_ByteKeys[keyIndex]) == k_OneAndAHalfGBBytes);
      CHECK_FALSE(preferences.contains(k_GBKeys[keyIndex]));
    }
    DYNAMIC_SECTION(k_GBKeys[keyIndex] << " invalid GB and valid bytes")
    {
      SizeGBFiles files;
      Preferences preferences;
      const uint64 defaultBytes = preferences.defaultValueAs<uint64>(k_ByteKeys[keyIndex]);
      const auto result = loadDocument(files, {{k_GBKeys[keyIndex], "bad"}, {k_ByteKeys[keyIndex], uint64{1073741824 * 3ULL}}}, preferences);
      REQUIRE(result.valid());
      REQUIRE(result.warnings().size() == 1);
      CHECK(result.warnings().front().code == k_GBRepairCode);
      CHECK(result.warnings().front().message.find(k_GBKeys[keyIndex]) != std::string::npos);
      CHECK_FALSE(preferences.contains(k_ByteKeys[keyIndex]));
      CHECK_FALSE(preferences.contains(k_GBKeys[keyIndex]));
      CHECK(preferences.valueAs<uint64>(k_ByteKeys[keyIndex]) == defaultBytes);
    }
  }
}

TEST_CASE("Preferences GB conversion keeps the legacy cache budget migration", "[Core][Preferences][OocThreshold][PreferencesSizeGB]")
{
  SizeGBFiles files;
  Preferences preferences;
  const auto result = loadDocument(files, {{k_ByteKeys[0], uint64{10000}}, {k_GBKeys[1], 2.5}, {"memory_budget_bytes", uint64{123456}}}, preferences);
  REQUIRE(result.valid());
  CHECK(result.warnings().empty());
  CHECK(preferences.valueAs<uint64>(k_ByteKeys[0]) == k_OneHundredthBytes);
  CHECK(preferences.valueAs<uint64>(k_ByteKeys[1]) == 2684354560);
  CHECK(preferences.cacheMemoryBudgetBytes() == 123456);
  CHECK(preferences.contains(Preferences::k_CacheMemoryBudgetBytes_Key));
  CHECK_FALSE(preferences.contains(Preferences::k_LegacyMemoryBudgetBytes_Key));
  REQUIRE(preferences.saveToFile(files.output).valid());
  const auto saved = nlohmann::json::parse(readText(files.output));
  CHECK(saved.at(std::string(Preferences::k_CacheMemoryBudgetBytes_Key)).get<uint64>() == 123456);
  CHECK(saved.at(k_GBKeys[0]).get<double>() == 0.01);
  CHECK(saved.at(k_GBKeys[1]).get<double>() == 2.5);
}

TEST_CASE("Preferences repeated GB loads keep no stale threshold state", "[Core][Preferences][OocThreshold][PreferencesSizeGB]")
{
  SizeGBFiles files;
  Preferences preferences;
  const uint64 defaultBytes = preferences.defaultValueAs<uint64>(k_ByteKeys[0]);

  const auto failed = loadDocument(files, {{k_GBKeys[0], -3}}, preferences);
  REQUIRE(failed.valid());
  REQUIRE(failed.warnings().size() == 1);
  CHECK(failed.warnings().front().code == k_GBRepairCode);
  CHECK(preferences.valueAs<uint64>(k_ByteKeys[0]) == defaultBytes);

  const auto good = loadDocument(files, {{k_GBKeys[0], 1.5}}, preferences);
  REQUIRE(good.valid());
  CHECK(good.warnings().empty());
  CHECK(preferences.valueAs<uint64>(k_ByteKeys[0]) == k_OneAndAHalfGBBytes);

  const auto absent = loadDocument(files, nlohmann::json::object({{"plugins", nlohmann::json::object()}}), preferences);
  REQUIRE(absent.valid());
  CHECK(absent.warnings().empty());
  CHECK_FALSE(preferences.contains(k_ByteKeys[0]));
  CHECK(preferences.valueAs<uint64>(k_ByteKeys[0]) == defaultBytes);
}

TEST_CASE("Preferences saves OOC thresholds only as GB keys", "[Core][Preferences][OocThreshold][PreferencesSizeGB]")
{
  struct SaveCase
  {
    uint64 bytes;
    std::string text;
  };
  const std::vector<SaveCase> cases = {{0, "0.0"}, {10000, "0.01"}, {1073741824, "1.0"}, {k_OneAndAHalfGBBytes, "1.5"}, {14871324262, "13.85"}, {std::numeric_limits<uint64>::max(), "17179869183.99"}};
  for(usize keyIndex = 0; keyIndex < k_ByteKeys.size(); ++keyIndex)
  {
    for(const auto& saveCase : cases)
    {
      DYNAMIC_SECTION(k_ByteKeys[keyIndex] << "=" << saveCase.bytes)
      {
        SizeGBFiles files;
        Preferences preferences;
        preferences.setValue(k_ByteKeys[keyIndex], saveCase.bytes);
        preferences.setValue("unrelated", "preserved");
        preferences.setPluginValue("SizeGBTest", "keep", 17);
        // The in-memory value stays exact. Only the file uses the 0.01-GB grid.
        CHECK(preferences.valueAs<uint64>(k_ByteKeys[keyIndex]) == saveCase.bytes);
        REQUIRE(preferences.saveToFile(files.output).valid());
        const std::string text = readText(files.output);
        CHECK(text.find("\"" + k_GBKeys[keyIndex] + "\":" + saveCase.text) != std::string::npos);
        CHECK(text.find("\"" + k_ByteKeys[0] + "\"") == std::string::npos);
        CHECK(text.find("\"" + k_ByteKeys[1] + "\"") == std::string::npos);
        const auto saved = nlohmann::json::parse(text);
        CHECK(saved.at("unrelated") == nlohmann::json("preserved"));
        CHECK(saved.at("plugins").at("SizeGBTest").at("keep") == nlohmann::json(17));
        CHECK(preferences.valueAs<uint64>(k_ByteKeys[keyIndex]) == saveCase.bytes);
      }
    }
  }
}

TEST_CASE("Preferences converts a legacy file only when it saves", "[Core][Preferences][OocThreshold][PreferencesSizeGB]")
{
  SizeGBFiles files;
  const std::string legacyText = R"({"large_data_size":10000,"large_datastructure_size":18446744073709551615,"plugins":{},"unrelated":"preserved"})";
  writeText(files.input, legacyText);
  const auto modifiedBefore = fs::last_write_time(files.input);

  Preferences preferences;
  const auto result = preferences.loadFromFile(files.input);
  REQUIRE(result.valid());
  CHECK(result.warnings().empty());
  CHECK(readText(files.input) == legacyText);
  // Evaluate to bool before Catch2 tries to stream libc++'s file-clock time point.
  CHECK((fs::last_write_time(files.input) == modifiedBefore));
  CHECK(preferences.valueAs<uint64>(k_ByteKeys[0]) == k_OneHundredthBytes);
  CHECK(preferences.valueAs<uint64>(k_ByteKeys[1]) == k_MaxGridBytes);

  REQUIRE(preferences.saveToFile(files.input).valid());
  const auto converted = nlohmann::json::parse(readText(files.input));
  CHECK_FALSE(converted.contains(k_ByteKeys[0]));
  CHECK_FALSE(converted.contains(k_ByteKeys[1]));
  CHECK(converted.at(k_GBKeys[0]).get<double>() == 0.01);
  CHECK(converted.at(k_GBKeys[1]).get<double>() == 17179869183.99);
  CHECK(converted.at("unrelated") == nlohmann::json("preserved"));
}

TEST_CASE("Preferences GB save and load round trip keeps exact grid bytes", "[Core][Preferences][OocThreshold][PreferencesSizeGB]")
{
  const uint64 ramDefault = Preferences().defaultValueAs<uint64>(k_ByteKeys[1]);
  for(const uint64 bytes : {uint64{0}, k_OneHundredthBytes, uint64{1073741824}, ramDefault, k_MaxGridBytes})
  {
    for(usize keyIndex = 0; keyIndex < k_ByteKeys.size(); ++keyIndex)
    {
      DYNAMIC_SECTION(k_ByteKeys[keyIndex] << "=" << bytes)
      {
        SizeGBFiles files;
        Preferences preferences;
        preferences.setValue(k_ByteKeys[keyIndex], bytes);
        REQUIRE(preferences.saveToFile(files.output).valid());
        Preferences loaded;
        const auto result = loaded.loadFromFile(files.output);
        REQUIRE(result.valid());
        CHECK(result.warnings().empty());
        CHECK(loaded.valueAs<uint64>(k_ByteKeys[keyIndex]) == bytes);
      }
    }
  }
}

TEST_CASE("Preferences threshold defaults are on the 0.01-GB grid", "[Core][Preferences][OocThreshold][PreferencesSizeGB]")
{
  Preferences preferences;
  const uint64 arrayDefault = preferences.defaultValueAs<uint64>(k_ByteKeys[0]);
  const uint64 structureDefault = preferences.defaultValueAs<uint64>(k_ByteKeys[1]);
  CHECK(arrayDefault == 1073741824);
  CHECK(Preferences::OocSizeHundredthsToBytes(Preferences::OocSizeBytesToHundredths(arrayDefault)) == arrayDefault);
  CHECK(Preferences::OocSizeHundredthsToBytes(Preferences::OocSizeBytesToHundredths(structureDefault)) == structureDefault);
  // clear() recomputes the defaults and keeps them on the grid.
  preferences.clear();
  CHECK(preferences.defaultValueAs<uint64>(k_ByteKeys[1]) == structureDefault);
}
