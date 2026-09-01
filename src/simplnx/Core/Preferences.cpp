#include "Preferences.hpp"

#include "simplnx/Common/SimplnxConfig.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"
#include "simplnx/Utilities/MemoryUtilities.hpp"

#include <fstream>

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

constexpr StringLiteral k_FailedToCreateDirectory_Message = "Failed to create the parent directory when saving Preferences. Check that the path is valid and writable.";
constexpr StringLiteral k_FileDoesNotExist_Message = "Preferences file does not exist";
constexpr StringLiteral k_FileCouldNotOpen_Message = "Could not open Preferences file";
constexpr StringLiteral k_JsonParseError_Message = "Parsing the JSON Preferences file failed.";

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
  m_Values[name] = value;

  // The single-array threshold determines the whole-data-structure default.
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

  m_Values = parsedResult;

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
  return {};
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
