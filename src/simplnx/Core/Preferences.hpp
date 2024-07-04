#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/StringLiteral.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>

namespace nx::core
{
class AbstractPlugin;

class SIMPLNX_EXPORT Preferences
{
  friend class AbstractPlugin;

public:
  static inline constexpr StringLiteral k_LargeDataSize_Key = "large_data_size";                   // bytes
  static inline constexpr StringLiteral k_PreferredLargeDataFormat_Key = "large_data_format";      // string
  static inline constexpr StringLiteral k_LargeDataStructureSize_Key = "large_datastructure_size"; // bytes
  static inline constexpr StringLiteral k_ForceOocData_Key = "force_ooc_data";                     // boolean

  static std::filesystem::path DefaultFilePath(const std::string& applicationName);

  Preferences();
  ~Preferences() noexcept;

  bool contains(const std::string& name) const;
  bool pluginContains(const std::string& pluginName, const std::string& name) const;
  bool pluginContainsDefault(const std::string& pluginName, const std::string& name) const;

  nlohmann::json value(const std::string& name) const;

  template <typename T>
  T valueAs(const std::string& name) const
  {
    return value(name).get<T>();
  }

  nlohmann::json defaultValue(const std::string& name) const;

  template <typename T>
  T defaultValueAs(const std::string& name) const
  {
    return defaultValue(name).get<T>();
  }

  void setValue(const std::string& name, const nlohmann::json& value);

  nlohmann::json pluginValue(const std::string& pluginName, const std::string& valueName) const;

  template <typename T>
  T pluginValueAs(const std::string& pluginName, const std::string& valueName) const
  {
    return pluginValue(pluginName, valueName);
  }

  nlohmann::json defaultPluginValue(const std::string& pluginName, const std::string& name) const;

  template <typename T>
  T defaultPluginValueAs(const std::string& pluginName, const std::string& name) const
  {
    return defaultPluginValue(pluginName, name).get<T>();
  }

  void setPluginValue(const std::string& pluginName, const std::string& valueName, const nlohmann::json& value);

  void clear();

  Result<> saveToFile(const std::filesystem::path& filepath) const;
  Result<> loadFromFile(const std::filesystem::path& filepath);

  std::string defaultLargeDataFormat() const;
  void setDefaultLargeDataFormat(std::string dataFormat);

  std::string largeDataFormat() const;
  void setLargeDataFormat(std::string dataFormat);

  bool useOocData() const;
  bool forceOocData() const;

  void setForceOocData(bool forceOoc);

  void updateMemoryDefaults();
  uint64 largeDataStructureSize() const;

protected:
  void setDefaultValues();

  void addDefaultValues(std::string pluginName, std::string valueName, const nlohmann::json& value);

  void checkUseOoc();

private:
  nlohmann::json m_DefaultValues;
  nlohmann::json m_Values;
  bool m_UseOoc = false;
};
} // namespace nx::core
