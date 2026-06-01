#include "Preferences.hpp"

#include "simplnx/Common/SimplnxConfig.hpp"
#ifdef SIMPLNX_USE_OOC
#include "SimplnxOoc/OocDataIOManager.hpp"
#endif

#include "simplnx/Core/Application.hpp"
#include "simplnx/Utilities/MemoryBudgetManager.hpp"
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
  checkUseOoc();
}

Preferences::~Preferences() noexcept = default;

void Preferences::setDefaultValues()
{
  m_Values[k_Plugin_Key] = nlohmann::json::object();
  m_DefaultValues[k_Plugin_Key] = nlohmann::json::object();

  m_DefaultValues[k_LargeDataSize_Key] = k_LargeDataSize;

  {
    // Set a default value for out-of-core temp directory.
    std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "simplnx";
    m_DefaultValues[k_OoCTempDirectory_ID] = tempDir.string();
  }

  updateMemoryDefaults();

#ifdef SIMPLNX_FORCE_OUT_OF_CORE_DATA
  m_DefaultValues[k_ForceOocData_Key] = true;
#else
  m_DefaultValues[k_ForceOocData_Key] = false;
#endif

  // Seed the default large-data format. When OOC is compiled in (SIMPLNX_USE_OOC),
  // default to the HDF5 out-of-core backend so large arrays spill to disk
  // automatically. When OOC is not compiled in, default to explicit in-memory
  // storage.
#ifdef SIMPLNX_USE_OOC
  m_DefaultValues[k_PreferredLargeDataFormat_Key] = "HDF5-OOC";
#else
  m_DefaultValues[k_PreferredLargeDataFormat_Key] = k_InMemoryFormat.str();
#endif

  m_DefaultValues[k_AutoRangeComputation_Key] = k_AutoRangeComputationDefault;
}

std::string Preferences::defaultLargeDataFormat() const
{
  return m_DefaultValues[k_PreferredLargeDataFormat_Key].get<std::string>();
}

void Preferences::setDefaultLargeDataFormat(std::string dataFormat)
{
  m_DefaultValues[k_PreferredLargeDataFormat_Key] = dataFormat;
  checkUseOoc();
}

std::string Preferences::largeDataFormat() const
{
  auto formatJson = value(k_PreferredLargeDataFormat_Key);
  if(formatJson.is_null() || !formatJson.is_string())
  {
    return {};
  }
  return formatJson.get<std::string>();
}
void Preferences::setLargeDataFormat(std::string dataFormat)
{
  if(dataFormat.empty())
  {
    // Remove the key so the compiled-in default can take effect.
    // An empty string means "not configured", not "in-core". To explicitly
    // request in-core storage, pass k_InMemoryFormat instead. This distinction
    // matters because an OOC-enabled build seeds a default OOC format
    // (see setDefaultValues), and erasing the user value lets that default
    // take effect.
    m_Values.erase(k_PreferredLargeDataFormat_Key);
  }
  else
  {
    m_Values[k_PreferredLargeDataFormat_Key] = dataFormat;
  }
  // Recompute the cached m_UseOoc flag after any format change
  checkUseOoc();
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

  // Check if out-of-core values need to be updated.
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

  // Migrate legacy format strings from saved preferences files that were
  // written before the OOC architecture was finalized. Two legacy values
  // need cleanup:
  //   - Empty string (""):   Old "not configured" state. Removing the key
  //     lets the compiled-in default (e.g., "HDF5-OOC") take effect.
  //   - "In-Memory":         Old explicit in-core sentinel. Replaced by
  //     k_InMemoryFormat ("Simplnx-Default-In-Memory"). Removing the key
  //     avoids confusion with the new sentinel value.
  if(m_Values.contains(k_PreferredLargeDataFormat_Key) && m_Values[k_PreferredLargeDataFormat_Key].is_string())
  {
    const std::string savedFormat = m_Values[k_PreferredLargeDataFormat_Key].get<std::string>();
    if(savedFormat.empty() || savedFormat == "In-Memory")
    {
      m_Values.erase(k_PreferredLargeDataFormat_Key);
    }
  }

  // Recompute derived state from the loaded (and possibly migrated) values
  checkUseOoc();
  updateMemoryDefaults();
  return {};
}

void Preferences::checkUseOoc()
{
  // Resolve the format from user values first, then default values (via value())
  auto formatJson = value(k_PreferredLargeDataFormat_Key);

  // If no format is configured (null/non-string), OOC is not active
  if(formatJson.is_null() || !formatJson.is_string())
  {
    m_UseOoc = false;
    return;
  }

  // OOC is active when the format is a non-empty string that is NOT the
  // explicit in-memory sentinel. This means an OOC-enabled build has
  // seeded a real OOC format like "HDF5-OOC".
  const std::string format = formatJson.get<std::string>();
  m_UseOoc = !format.empty() && format != k_InMemoryFormat;
}

bool Preferences::useOocData() const
{
  return m_UseOoc;
}

bool Preferences::forceOocData() const
{
  // The force_ooc_data flag is an independent user preference. It must not
  // be gated on m_UseOoc — the whole point of force_ooc_data is to override
  // the "in-memory format" choice when the user wants every eligible array
  // routed to OOC anyway. The OocDataIOManager format resolver explicitly
  // handles the (forceOoc=true, userChoseInMemory=true) case by returning
  // "HDF5-OOC". Gating here defeats that design and silently makes the
  // preference checkbox useless whenever the large-data format is set to
  // in-memory.
  return valueAs<bool>(k_ForceOocData_Key);
}

void Preferences::setForceOocData(bool forceOoc)
{
  // See forceOocData() — the m_UseOoc gate is intentionally absent so a
  // user toggling the Force OOC checkbox in the Preferences dialog always
  // persists, regardless of the currently-selected large-data format.
  setValue(k_ForceOocData_Key, forceOoc);
}

void Preferences::updateMemoryDefaults()
{
  // Reserve headroom equal to 2x the single-array large-data threshold.
  // This leaves room for the OS, the application, and at least one large
  // array being constructed while the DataStructure holds existing data.
  const uint64 minimumRemaining = 2 * defaultValueAs<uint64>(k_LargeDataSize_Key);
  const uint64 totalMemory = Memory::GetTotalMemory();
  uint64 targetValue = totalMemory - minimumRemaining;

  // On low-memory systems where the reservation exceeds total RAM,
  // fall back to using half of total RAM as the threshold
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
#ifdef SIMPLNX_USE_OOC
  // Route the temp directory straight to the OOC subsystem so session working
  // files are created there. When OOC is not compiled in, the preference is
  // simply persisted.
  SimplnxOoc::setBaseDirectory(std::filesystem::path(path));
#endif
}

bool Preferences::autoRangeComputation() const
{
  return value(k_AutoRangeComputation_Key).get<bool>();
}

void Preferences::setAutoRangeComputation(bool enabled)
{
  setValue(k_AutoRangeComputation_Key, enabled);
}

uint64 Preferences::memoryBudgetBytes() const
{
  // When the user has never saved an explicit budget preference, fall back
  // to MemoryBudgetManager's system-aware default (50% of system RAM,
  // clamped to a minimum of 1 GB). Computing this here means every caller
  // gets a system-aware value, rather than a stale hard-coded constant.
  // Using m_Values.value() (not the value() member) reads directly from
  // user-set values with the fallback, bypassing the default-value layer.
  return m_Values.value(k_MemoryBudgetBytes_Key, MemoryBudgetManager::defaultBudgetBytes());
}

void Preferences::setMemoryBudgetBytes(uint64 bytes)
{
  m_Values[k_MemoryBudgetBytes_Key] = bytes;
}
} // namespace nx::core
