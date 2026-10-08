#include "Preferences.hpp"

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
#include <stdexcept>
#include <string_view>
#include <utility>

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

constexpr int32 k_FailedToCreateDirectory_Code = -585;
constexpr int32 k_FileDoesNotExist_Code = -586;
constexpr int32 k_FileCouldNotOpen_Code = -587;
constexpr int32 k_JsonParseError_Code = -588;
constexpr int32 k_InvalidRoot_Code = -589;
constexpr int32 k_InvalidOocSize_Code = -590;
constexpr int32 k_RepairedOocSize_Warning = -591;

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
    return MakeErrorResult<uint64>(k_InvalidOocSize_Code, fmt::format("Preferences size key '{}' is not supported for value {} (JSON type {}).", name, boundedJsonValue(value), jsonTypeName(value)));
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
  return MakeErrorResult<uint64>(k_InvalidOocSize_Code, fmt::format("Preferences {} ('{}') value {} (JSON type {}) is invalid. Allowed domain: 0 to {} bytes.", label, name, boundedJsonValue(value),
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
    return MakeErrorResult(k_FailedToCreateDirectory_Code, k_FailedToCreateDirectory_Message);
  }

  std::ofstream fileStream(filepath);
  if(!fileStream.is_open())
  {
    return MakeErrorResult(k_FileCouldNotOpen_Code, k_FileCouldNotOpen_Message);
  }

  fileStream << m_Values;
  return {};
}

Result<> Preferences::loadFromFile(const std::filesystem::path& filepath)
{
  if(!std::filesystem::exists(filepath))
  {
    return MakeErrorResult(k_FileDoesNotExist_Code, k_FileDoesNotExist_Message);
  }

  std::ifstream fileStream(filepath);
  if(!fileStream.is_open())
  {
    return MakeErrorResult(k_FileCouldNotOpen_Code, k_FileCouldNotOpen_Message);
  }

  nlohmann::json parsedResult = nlohmann::json::parse(fileStream, nullptr, false);
  if(parsedResult.is_discarded())
  {
    return MakeErrorResult(k_JsonParseError_Code, k_JsonParseError_Message);
  }

  if(!parsedResult.is_object())
  {
    return MakeErrorResult(k_InvalidRoot_Code, fmt::format("Preferences file '{}' has a JSON {} root. Expected a JSON object.", filepath.string(), jsonTypeName(parsedResult)));
  }

  Result<> result;
  const std::array<std::string, 2> thresholdKeys = {std::string(k_LargeDataSize_Key), std::string(k_LargeDataStructureSize_Key)};
  for(const auto& key : thresholdKeys)
  {
    if(!parsedResult.contains(key))
    {
      continue;
    }
    const auto validation = ValidateOocSizeValue(key, parsedResult.at(key));
    if(validation.valid())
    {
      continue;
    }
    const uint64 replacementBytes = defaultValueAs<uint64>(key);
    result.warnings().push_back(
        Warning{k_RepairedOocSize_Warning,
                fmt::format("Preferences: saved {} ('{}') value {} (JSON type {}) is invalid. Allowed domain: 0 to {} bytes. Using the default: {} bytes. Review this value in "
                            "Preferences > Out of Core.",
                            oocSizeLabel(key), key, boundedJsonValue(parsedResult.at(key)), jsonTypeName(parsedResult.at(key)), std::numeric_limits<uint64>::max(), replacementBytes)});
    parsedResult.erase(key);
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

  m_DefaultValues[k_LargeDataStructureSize_Key] = targetValue;
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
