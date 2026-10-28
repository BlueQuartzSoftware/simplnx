#include "Preferences.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include <nlohmann/json.hpp>
#include <string>

#include "simplnx/Common/SimplnxConfig.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/MemoryUtilities.hpp"

#include <fmt/format.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <stdio.h>
#include <stdlib.h>
#else
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#endif

namespace nx::core
{

namespace
{
constexpr int64 k_LargeDataSize = 1073741824; // 1 GB
constexpr StringLiteral k_Plugin_Key = "plugins";
constexpr StringLiteral k_DefaultFileName = "preferences.json";
constexpr int64 k_ReducedDataStructureSize = 3221225472; // 3 GB
constexpr bool k_AutoRangeComputationDefault = false;

constexpr int32 k_InvalidRootCode = -589;
constexpr int32 k_InvalidOocSizeCode = -590;
constexpr int32 k_RepairedOocSizeWarning = -591;
constexpr int32 k_RepairedOocSizeGBWarning = -592;

// Correctly rounded division gives the double nearest to 17179869183.99, which is also the parsed JSON value.
constexpr double k_MaxOocSizeGB = static_cast<double>(Preferences::k_MaxOocSizeHundredths) / 100.0;

constexpr StringLiteral k_FailedToCreateDirectory_Message = "Failed to create the parent directory when saving Preferences. Check that the path is valid and writable.";
constexpr StringLiteral k_FileDoesNotExist_Message = "Preferences file does not exist";
constexpr StringLiteral k_FileCouldNotOpen_Message = "Could not open Preferences file";
constexpr StringLiteral k_JsonParseError_Message = "Parsing the JSON Preferences file failed.";

std::string_view oocSizeLabel(const std::string& name)
{
  if(name == Preferences::k_LargeDataSize_Key)
  {
    return "Large Data Size";
  }
  if(name == Preferences::k_LargeDataStructureSize_Key)
  {
    return "Large Data Structure Size";
  }
  return {};
}

/**
 * @struct OocSizeKeys
 * @brief Names the in-memory byte key and the file GB key of one out-of-core size threshold.
 */
struct OocSizeKeys
{
  std::string bytesKey;
  std::string gbKey;
};

/**
 * @brief Returns the byte and GB keys of both out-of-core size thresholds, Large Data Size first.
 * @return Key pairs in the order that load warnings use.
 */
std::array<OocSizeKeys, 2> oocSizeKeys()
{
  return {OocSizeKeys{std::string(Preferences::k_LargeDataSize_Key), std::string(Preferences::k_LargeDataSizeGB_Key)},
          OocSizeKeys{std::string(Preferences::k_LargeDataStructureSize_Key), std::string(Preferences::k_LargeDataStructureSizeGB_Key)}};
}

/**
 * @brief Formats hundredths of a GB as exact text with two decimals.
 * @param hundredths Value in hundredths of a GB.
 * @return Text such as "1.00" or "17179869183.99".
 */
std::string formatHundredthsAsGB(uint64 hundredths)
{
  return fmt::format("{}.{:02}", hundredths / 100, hundredths % 100);
}

/**
 * @brief Converts a saved GB value to hundredths of a GB.
 * @param value Saved JSON value.
 * @return Hundredths of a GB, or std::nullopt when the value is not a finite JSON number from 0 to k_MaxOocSizeGB.
 *
 * A value above 0 that rounds to 0 hundredths becomes 1 (0.01 GB).
 */
std::optional<uint64> savedGBToHundredths(const nlohmann::json& value)
{
  if(!value.is_number())
  {
    return std::nullopt;
  }
  const double gigabytes = value.get<double>();
  // NaN fails both comparisons. The range check also rejects infinity before the integer conversion.
  if(!(gigabytes >= 0.0 && gigabytes <= k_MaxOocSizeGB))
  {
    return std::nullopt;
  }
  auto hundredths = static_cast<uint64>(std::llround(gigabytes * 100.0));
  if(gigabytes > 0.0 && hundredths == 0)
  {
    hundredths = 1;
  }
  return hundredths;
}

std::string_view jsonTypeName(const nlohmann::json& value)
{
  if(value.is_number_unsigned())
  {
    return "number_unsigned";
  }
  if(value.is_number_integer())
  {
    return "number_integer";
  }
  if(value.is_number_float())
  {
    return "number_float";
  }
  return value.type_name();
}

bool canRenderExact(const nlohmann::json& value, std::size_t& remaining, int depth)
{
  if(depth > 3 || remaining < 32)
  {
    return false;
  }
  if(value.is_string())
  {
    const auto& contents = value.get_ref<const std::string&>();
    if(contents.size() > (remaining - 2) / 6)
    {
      return false;
    }
    remaining -= 2 + 6 * contents.size();
    return true;
  }
  if(value.is_array() || value.is_object())
  {
    if(value.size() > 8 || remaining < 2 + 2 * value.size())
    {
      return false;
    }
    remaining -= 2 + 2 * value.size();
    for(auto iter = value.begin(); iter != value.end(); ++iter)
    {
      if(value.is_object())
      {
        if(iter.key().size() > remaining / 6)
        {
          return false;
        }
        remaining -= 6 * iter.key().size();
      }
      if(!canRenderExact(iter.value(), remaining, depth + 1))
      {
        return false;
      }
    }
    return true;
  }
  if(value.is_binary())
  {
    return false;
  }
  if(value.is_number_float() && !std::isfinite(value.get<double>()))
  {
    return false;
  }
  remaining -= 32;
  return true;
}

std::string boundedPreview(const nlohmann::json& value, int depth)
{
  if(value.is_number_float())
  {
    const double number = value.get<double>();
    if(std::isnan(number))
    {
      return "NaN";
    }
    if(std::isinf(number))
    {
      return std::signbit(number) ? "-Infinity" : "+Infinity";
    }
  }
  if(value.is_string())
  {
    const auto& contents = value.get_ref<const std::string&>();
    std::string preview = nlohmann::json(contents.substr(0, 24)).dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
    if(contents.size() > 24)
    {
      preview += "...";
    }
    return preview;
  }
  if(value.is_array() || value.is_object())
  {
    if(depth >= 2)
    {
      return fmt::format("[{} with {} entries]", jsonTypeName(value), value.size());
    }
    std::string preview = value.is_object() ? "{" : "[";
    std::size_t shown = 0;
    for(auto iter = value.begin(); iter != value.end() && shown < 2; ++iter, ++shown)
    {
      if(shown != 0)
      {
        preview += ",";
      }
      if(value.is_object())
      {
        const auto& key = iter.key();
        preview += nlohmann::json(key.substr(0, 24)).dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
        if(key.size() > 24)
        {
          preview += "...";
        }
        preview += ":";
      }
      preview += boundedPreview(iter.value(), depth + 1);
    }
    if(value.size() > shown)
    {
      preview += ",...";
    }
    preview += value.is_object() ? "}" : "]";
    if(preview.size() > 256)
    {
      preview.resize(253);
      preview += "...";
    }
    return preview;
  }
  if(value.is_binary())
  {
    return "[binary value]";
  }
  return value.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
}

std::string boundedJsonValue(const nlohmann::json& value)
{
  std::size_t remaining = 256;
  if(canRenderExact(value, remaining, 0))
  {
    return value.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
  }
  if(value.is_string())
  {
    const auto& contents = value.get_ref<const std::string&>();
    return nlohmann::json(contents.substr(0, 64)).dump(-1, ' ', false, nlohmann::json::error_handler_t::replace) + "...";
  }
  if(value.is_array() || value.is_object())
  {
    return fmt::format("{} with {} entries; preview {} [truncated]", jsonTypeName(value), value.size(), boundedPreview(value, 0));
  }
  return boundedPreview(value, 0);
}

/**
 * @brief Reads one saved out-of-core size threshold and converts it to bytes on the 0.01-GB grid.
 * @param document Parsed preferences file. It contains keys.gbKey, keys.bytesKey, or both.
 * @param keys Byte and GB keys of the threshold.
 * @param defaultBytes Default threshold in bytes, which the warning names.
 * @param warnings Receives one repair warning when the selected value is invalid.
 * @return Threshold in bytes, or std::nullopt when the selected value is invalid.
 *
 * The GB key takes precedence, and the function does not validate a byte key next to it.
 * A legacy byte value that ValidateOocSizeValue() accepts moves to the nearest grid value.
 */
std::optional<uint64> readSavedOocSize(const nlohmann::json& document, const OocSizeKeys& keys, uint64 defaultBytes, std::vector<Warning>& warnings)
{
  const std::string_view label = oocSizeLabel(keys.bytesKey);
  if(document.contains(keys.gbKey))
  {
    const nlohmann::json& saved = document.at(keys.gbKey);
    const std::optional<uint64> hundredths = savedGBToHundredths(saved);
    if(hundredths.has_value())
    {
      return Preferences::OocSizeHundredthsToBytes(*hundredths);
    }
    warnings.push_back(Warning{.code = k_RepairedOocSizeGBWarning,
                               .message = fmt::format("Preferences: saved {} ('{}') value {} (JSON type {}) is invalid. Allowed domain: 0 to {} GB. Using the default: {} GB. Review this "
                                                      "value in Preferences > Out of Core.",
                                                      label, keys.gbKey, boundedJsonValue(saved), jsonTypeName(saved), formatHundredthsAsGB(Preferences::k_MaxOocSizeHundredths),
                                                      formatHundredthsAsGB(Preferences::OocSizeBytesToHundredths(defaultBytes)))});
    return std::nullopt;
  }

  const nlohmann::json& saved = document.at(keys.bytesKey);
  const auto validation = Preferences::ValidateOocSizeValue(keys.bytesKey, saved);
  if(validation.valid())
  {
    return Preferences::OocSizeHundredthsToBytes(Preferences::OocSizeBytesToHundredths(validation.value()));
  }
  warnings.push_back(Warning{.code = k_RepairedOocSizeWarning,
                             .message = fmt::format("Preferences: saved {} ('{}') value {} (JSON type {}) is invalid. Allowed domain: 0 to {} bytes. Using the default: {} bytes. Review this value in "
                                                    "Preferences > Out of Core.",
                                                    label, keys.bytesKey, boundedJsonValue(saved), jsonTypeName(saved), std::numeric_limits<uint64>::max(), defaultBytes)});
  return std::nullopt;
}

/**
 * @brief Returns the current user's home directory.
 * @return Home directory from the platform environment or account database.
 */
std::filesystem::path getHomeDirectory()
{
#ifdef _WIN32
  return getenv("USERPROFILE");
#else
  const char* homedir;
  if((homedir = getenv("HOME")) == NULL)
  {
    homedir = getpwuid(getuid())->pw_dir;
  }
  return std::filesystem::path(homedir);
#endif
}
} // namespace

std::filesystem::path Preferences::DefaultFilePath(const std::string& applicationName)
{
#if defined(__APPLE__)
  return getHomeDirectory() / "Library/Preferences" / applicationName / k_DefaultFileName.str();
#elif defined(_WIN32)
  return getHomeDirectory() / "AppData/Local" / applicationName / k_DefaultFileName.str();
#else
  return getHomeDirectory() / ".config/" / applicationName / k_DefaultFileName.str();
#endif
}

Result<uint64> Preferences::ValidateOocSizeValue(const std::string& name, const nlohmann::json& value)
{
  const std::string_view label = oocSizeLabel(name);
  if(label.empty())
  {
    return MakeErrorResult<uint64>(k_InvalidOocSizeCode, fmt::format("Preferences size key '{}' is not supported for value {} (JSON type {}).", name, boundedJsonValue(value), jsonTypeName(value)));
  }
  if(value.is_number_unsigned())
  {
    return {value.get<uint64>()};
  }
  if(value.is_number_integer())
  {
    const int64 signedValue = value.get<int64>();
    if(signedValue >= 0)
    {
      return {static_cast<uint64>(signedValue)};
    }
  }
  return MakeErrorResult<uint64>(k_InvalidOocSizeCode, fmt::format("Preferences {} ('{}') value {} (JSON type {}) is invalid. Allowed domain: 0 to {} bytes.", label, name, boundedJsonValue(value),
                                                                   jsonTypeName(value), std::numeric_limits<uint64>::max()));
}

Preferences::Preferences()
{
  setDefaultValues();
}

Preferences::~Preferences() noexcept = default;

void Preferences::setDefaultValues()
{
  m_Values[k_Plugin_Key] = nlohmann::json::object();
  m_DefaultValues[k_Plugin_Key] = nlohmann::json::object();

  m_DefaultValues[k_LargeDataSize_Key] = k_LargeDataSize;

  {
    std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "simplnx";
    m_DefaultValues[k_OoCTempDirectory_ID] = tempDir.string();
  }

  updateMemoryDefaults();

  // Adaptive keeps core storage intent independent of a concrete OOC format.
  m_DefaultValues[k_DataStorageMode_Key] = static_cast<int>(DataStorageMode::Adaptive);

  m_DefaultValues[k_AutoRangeComputation_Key] = k_AutoRangeComputationDefault;
}

void Preferences::addDefaultValues(std::string pluginName, std::string valueName, const nlohmann::json& value)
{
  auto& pluginGroup = m_DefaultValues[k_Plugin_Key];
  if(!pluginGroup.contains(pluginName))
  {
    pluginGroup[pluginName] = nlohmann::json::object();
  }
  pluginGroup[pluginName][valueName] = value;
}

void Preferences::clear()
{
  m_Values.clear();
  m_Values[k_Plugin_Key] = nlohmann::json::object();
  updateMemoryDefaults();
}

bool Preferences::contains(const std::string& name) const
{
  return m_Values.contains(name);
}

void Preferences::removeValue(std::string_view name)
{
  m_Values.erase(std::string(name));
}

bool Preferences::pluginContains(const std::string& pluginName, const std::string& name) const
{
  if(!m_Values[k_Plugin_Key].contains(pluginName))
  {
    return false;
  }

  return m_Values[k_Plugin_Key][pluginName].contains(name);
}

bool Preferences::pluginContainsDefault(const std::string& pluginName, const std::string& name) const
{
  if(!m_DefaultValues[k_Plugin_Key].contains(pluginName))
  {
    return false;
  }

  return m_DefaultValues[k_Plugin_Key][pluginName].contains(name);
}

nlohmann::json Preferences::value(const std::string& name) const
{
  if(contains(name))
  {
    return m_Values[name];
  }
  else if(m_DefaultValues.contains(name))
  {
    return m_DefaultValues[name];
  }
  return {};
}

nlohmann::json Preferences::defaultValue(const std::string& name) const
{
  if(m_DefaultValues.contains(name))
  {
    return m_DefaultValues[name];
  }
  return {};
}

void Preferences::setValue(const std::string& name, const nlohmann::json& value)
{
  if(name == k_LargeDataSize_Key || name == k_LargeDataStructureSize_Key)
  {
    const auto validation = ValidateOocSizeValue(name, value);
    if(validation.invalid())
    {
      throw std::invalid_argument(validation.errors().front().message);
    }
  }
  m_Values[name] = value;

  // The default whole-data-structure threshold uses the default array threshold.
  if(name == k_LargeDataSize_Key)
  {
    updateMemoryDefaults();
  }
}

nlohmann::json Preferences::pluginValue(const std::string& pluginName, const std::string& valueName) const
{
  if(pluginContains(pluginName, valueName))
  {
    return m_Values[k_Plugin_Key][pluginName][valueName];
  }
  else if(pluginContainsDefault(pluginName, valueName))
  {
    return m_DefaultValues[k_Plugin_Key][pluginName][valueName];
  }

  return {};
}
nlohmann::json Preferences::defaultPluginValue(const std::string& pluginName, const std::string& valueName) const
{
  if(m_DefaultValues[k_Plugin_Key].contains(valueName))
  {
    return m_DefaultValues[k_Plugin_Key][valueName];
  }

  return {};
}

void Preferences::setPluginValue(const std::string& pluginName, const std::string& valueName, const nlohmann::json& value)
{
  m_Values[k_Plugin_Key][pluginName][valueName] = value;
}

Result<> Preferences::saveToFile(const std::filesystem::path& filepath) const
{
  if(!std::filesystem::exists(filepath.parent_path()) && !std::filesystem::create_directories(filepath.parent_path()))
  {
    return MakeErrorResult(Preferences::k_FailedToCreateDirectory_Code, k_FailedToCreateDirectory_Message);
  }

  std::ofstream fileStream(filepath);
  if(!fileStream.is_open())
  {
    return MakeErrorResult(Preferences::k_FileCouldNotOpen_Code, k_FileCouldNotOpen_Message);
  }

  // The file stores the out-of-core size thresholds in GB. Memory keeps them in bytes.
  nlohmann::json document = m_Values;
  for(const auto& keys : oocSizeKeys())
  {
    if(!document.contains(keys.bytesKey))
    {
      continue;
    }
    const uint64 bytes = document.at(keys.bytesKey).get<uint64>();
    document.erase(keys.bytesKey);
    document[keys.gbKey] = static_cast<double>(OocSizeBytesToHundredths(bytes)) / 100.0;
  }

  fileStream << document;
  return {};
}

Result<> Preferences::loadFromFile(const std::filesystem::path& filepath)
{
  if(!std::filesystem::exists(filepath))
  {
    return MakeErrorResult(Preferences::k_FileDoesNotExist_Code, k_FileDoesNotExist_Message);
  }

  std::ifstream fileStream(filepath);
  if(!fileStream.is_open())
  {
    return MakeErrorResult(Preferences::k_FileCouldNotOpen_Code, k_FileCouldNotOpen_Message);
  }

  nlohmann::json parsedResult = nlohmann::json::parse(fileStream, nullptr, false);
  if(parsedResult.is_discarded())
  {
    return MakeErrorResult(Preferences::k_JsonParseError_Code, k_JsonParseError_Message);
  }

  if(!parsedResult.is_object())
  {
    return MakeErrorResult(k_InvalidRootCode, fmt::format("Preferences file '{}' has a JSON {} root. Expected a JSON object.", filepath.string(), jsonTypeName(parsedResult)));
  }

  Result<> result;
  for(const auto& keys : oocSizeKeys())
  {
    if(!parsedResult.contains(keys.gbKey) && !parsedResult.contains(keys.bytesKey))
    {
      continue;
    }
    // Validate the selected representation before the keys are removed. An invalid value stores
    // nothing, so the default applies.
    const std::optional<uint64> gridBytes = readSavedOocSize(parsedResult, keys, defaultValueAs<uint64>(keys.bytesKey), result.warnings());
    parsedResult.erase(keys.gbKey);
    parsedResult.erase(keys.bytesKey);
    if(gridBytes.has_value())
    {
      parsedResult[keys.bytesKey] = *gridBytes;
    }
  }

  m_Values = std::move(parsedResult);

  // Preserve a canonical value when both keys exist. Remove the legacy key so
  // later saves retain only the cache-specific preference.
  if(!m_Values.contains(k_CacheMemoryBudgetBytes_Key) && m_Values.contains(k_LegacyMemoryBudgetBytes_Key))
  {
    m_Values[k_CacheMemoryBudgetBytes_Key] = m_Values[k_LegacyMemoryBudgetBytes_Key];
  }
  m_Values.erase(k_LegacyMemoryBudgetBytes_Key);

  // Remove empty legacy values so migration does not infer ForceInCore. Preserve
  // the in-memory sentinel and concrete format values for compatible migration.
  if(m_Values.contains(k_PreferredLargeDataFormat_Key) && m_Values[k_PreferredLargeDataFormat_Key].is_string())
  {
    const std::string savedFormat = m_Values[k_PreferredLargeDataFormat_Key].get<std::string>();
    if(savedFormat.empty() || savedFormat == "In-Memory")
    {
      m_Values.erase(k_PreferredLargeDataFormat_Key);
    }
  }

  updateMemoryDefaults();
  return result;
}

bool Preferences::useOocData() const
{
  return dataStorageMode() != DataStorageMode::ForceInCore;
}

DataStorageMode Preferences::dataStorageMode() const
{
  // Resolve storage intent in priority order: canonical value, legacy values,
  // then the Adaptive default.
  if(m_Values.contains(k_DataStorageMode_Key))
  {
    // An unrecognized persisted value uses Adaptive to avoid an invalid enum.
    const int raw = valueAs<int>(k_DataStorageMode_Key);
    if(raw < static_cast<int>(DataStorageMode::Adaptive) || raw > static_cast<int>(DataStorageMode::ForceOutOfCore))
    {
      return DataStorageMode::Adaptive;
    }
    return static_cast<DataStorageMode>(raw);
  }

  if(m_Values.contains(k_ForceOocData_Key) || m_Values.contains(k_PreferredLargeDataFormat_Key))
  {
    // The saved force-out-of-core flag overrides legacy format selection.
    if(m_Values.contains(k_ForceOocData_Key) && m_Values[k_ForceOocData_Key].is_boolean() && m_Values[k_ForceOocData_Key].get<bool>())
    {
      return DataStorageMode::ForceOutOfCore;
    }

    // Empty and in-memory values select ForceInCore. Other formats select Adaptive.
    std::string format;
    if(m_Values.contains(k_PreferredLargeDataFormat_Key) && m_Values[k_PreferredLargeDataFormat_Key].is_string())
    {
      format = m_Values[k_PreferredLargeDataFormat_Key].get<std::string>();
    }
    if(format.empty() || format == k_InMemoryFormat)
    {
      return DataStorageMode::ForceInCore;
    }
    return DataStorageMode::Adaptive;
  }

  return static_cast<DataStorageMode>(m_DefaultValues[k_DataStorageMode_Key].get<int>());
}

void Preferences::setDataStorageMode(DataStorageMode mode)
{
  setValue(k_DataStorageMode_Key, static_cast<int>(mode));
}

void Preferences::updateMemoryDefaults()
{
  // Reserve two single-array thresholds for the operating system and application.
  const uint64 minimumRemaining = 2 * defaultValueAs<uint64>(k_LargeDataSize_Key);
  const uint64 totalMemory = Memory::GetTotalMemory();
  uint64 targetValue = totalMemory - minimumRemaining;

  // Low-memory systems use half of RAM when the reservation is too large.
  if(minimumRemaining >= totalMemory)
  {
    targetValue = totalMemory / 2;
  }

  // A default on the 0.01-GB grid survives the GB file format and the Preferences dialog without a change.
  m_DefaultValues[k_LargeDataStructureSize_Key] = OocSizeHundredthsToBytes(OocSizeBytesToHundredths(targetValue));
}

uint64 Preferences::largeDataStructureSize() const
{
  return value(k_LargeDataStructureSize_Key).get<uint64>();
}

std::string Preferences::oocTempDirectory() const
{
  return value(k_OoCTempDirectory_ID).get<std::string>();
}

void Preferences::setOocTempDirectory(const std::string& path)
{
  setValue(k_OoCTempDirectory_ID, path);
  // Registered managers use this base directory for session backing files. In-core
  // builds have no out-of-core manager, so the collection update has no effect.
  Application::GetOrCreateInstance()->getIOCollection().setBaseDirectory(std::filesystem::path(path));
}

bool Preferences::autoRangeComputation() const
{
  return value(k_AutoRangeComputation_Key).get<bool>();
}

void Preferences::setAutoRangeComputation(bool enabled)
{
  setValue(k_AutoRangeComputation_Key, enabled);
}

uint64 Preferences::cacheMemoryBudgetBytes() const
{
  // Read m_Values directly so the fallback uses the current system-RAM default.
  return m_Values.value(k_CacheMemoryBudgetBytes_Key, CacheMemoryBudgetManager::defaultBudgetBytes());
}

void Preferences::setCacheMemoryBudgetBytes(uint64 bytes)
{
  m_Values[k_CacheMemoryBudgetBytes_Key] = bytes;
}
} // namespace nx::core
