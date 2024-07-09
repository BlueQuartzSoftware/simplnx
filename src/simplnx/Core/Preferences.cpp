#include "Preferences.hpp"

#include "simplnx/Plugin/AbstractPlugin.hpp"
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
constexpr StringLiteral k_LargeDataFormat = "";
constexpr StringLiteral k_Plugin_Key = "plugins";
constexpr StringLiteral k_DefaultFileName = "preferences.json";
constexpr int64 k_ReducedDataStructureSize = 3221225472; // 3 GB

constexpr int32 k_FailedToCreateDirectory_Code = -585;
constexpr int32 k_FileDoesNotExist_Code = -586;
constexpr int32 k_FileCouldNotOpen_Code = -587;

constexpr StringLiteral k_FailedToCreateDirectory_Message = "Failed to the parent directory when saving Preferences";
constexpr StringLiteral k_FileDoesNotExist_Message = "Preferences file does not exist";
constexpr StringLiteral k_FileCouldNotOpen_Message = "Could not open Preferences file";

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
  m_DefaultValues[k_PreferredLargeDataFormat_Key] = k_LargeDataFormat;

  updateMemoryDefaults();

#ifdef SIMPLNX_FORCE_OUT_OF_CORE_DATA
  m_DefaultValues[k_ForceOocData_Key] = true;
#else
  m_DefaultValues[k_ForceOocData_Key] = false;
#endif
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
  return valueAs<std::string>(k_PreferredLargeDataFormat_Key);
}
void Preferences::setLargeDataFormat(std::string dataFormat)
{
  m_Values[k_PreferredLargeDataFormat_Key] = dataFormat;
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
}

bool Preferences::contains(const std::string& name) const
{
  return m_Values.contains(name);
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
  m_Values = nlohmann::json::parse(fileStream);

  checkUseOoc();
  return {};
}

void Preferences::checkUseOoc()
{
  m_UseOoc = !value(k_PreferredLargeDataFormat_Key).get<std::string>().empty();
}

bool Preferences::useOocData() const
{
  return m_UseOoc;
}

bool Preferences::forceOocData() const
{
  if(!m_UseOoc)
  {
    return false;
  }
  return valueAs<bool>(k_ForceOocData_Key);
}

void Preferences::setForceOocData(bool forceOoc)
{
  if(!m_UseOoc)
  {
    return;
  }
  setValue(k_ForceOocData_Key, forceOoc);
}

void Preferences::updateMemoryDefaults()
{
  const uint64 minimumRemaining = 2 * defaultValueAs<uint64>(k_LargeDataSize_Key);
  const uint64 totalMemory = Memory::GetTotalMemory();
  uint64 targetValue = totalMemory - minimumRemaining;
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
} // namespace nx::core
