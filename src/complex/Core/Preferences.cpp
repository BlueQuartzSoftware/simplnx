#include "Preferences.hpp"

#include "complex/Plugin/AbstractPlugin.hpp"

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

namespace complex
{

namespace
{
inline constexpr int64 k_LargeDataSize = 1073741824; // 1 GB
inline constexpr StringLiteral k_LargeDataFormat = "";
inline constexpr StringLiteral k_Plugin_Key = "plugins";
inline constexpr StringLiteral k_DefaultFileName = "preferences.json";

inline constexpr int32 k_FailedToCreateDirectory_Code = -585;
inline constexpr int32 k_FileDoesNotExist_Code = -586;
inline constexpr int32 k_FileCouldNotOpen_Code = -587;

inline constexpr StringLiteral k_FailedToCreateDirectory_Message = "Failed to the parent directory when saving Preferences";
inline constexpr StringLiteral k_FileDoesNotExist_Message = "Preferences file does not exist";
inline constexpr StringLiteral k_FileCouldNotOpen_Message = "Could not open Preferences file";

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
  m_DefaultValues[k_PreferredLargeDataFormat_Key] = k_LargeDataFormat;
}

void Preferences::addDefaultValues(AbstractPlugin& plugin, std::string& valueName, const nlohmann::json& value)
{
  const std::string pluginName = plugin.getName();
  auto pluginGroup = m_DefaultValues[k_Plugin_Key];
  if(!pluginGroup.contains(pluginName))
  {
    pluginGroup[pluginName] = nlohmann::json::object();
  }
  pluginGroup[valueName] = value;
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
    return m_Values[k_Plugin_Key][valueName];
  }
  else if(m_DefaultValues[k_Plugin_Key].contains(valueName))
  {
    return m_DefaultValues[k_Plugin_Key][valueName];
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
  if(!std::filesystem::create_directories(filepath.parent_path()))
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

  return {};
}
} // namespace complex
