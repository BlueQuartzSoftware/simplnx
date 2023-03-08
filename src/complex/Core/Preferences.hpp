#pragma once

#include "complex/complex_export.hpp"

#include "complex/Common/Result.hpp"
#include "complex/Common/StringLiteral.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>

namespace complex
{
class AbstractPlugin;

class COMPLEX_EXPORT Preferences
{
  friend class AbstractPlugin;

public:
  static inline constexpr StringLiteral k_LargeDataSize_Key = "large_data_size";
  static inline constexpr StringLiteral k_PreferredLargeDataFormat_Key = "large_data_format";

  static std::filesystem::path DefaultFilePath(const std::string& applicationName);

  Preferences();
  ~Preferences() noexcept;

  bool contains(const std::string& name) const;
  bool pluginContains(const std::string& pluginName, const std::string& name) const;

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
    return pluginValue(pluginName, valueName).get<T>();
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

protected:
  void setDefaultValues();
  void addDefaultValues(AbstractPlugin& plugin, std::string& valueName, const nlohmann::json& value);

private:
  nlohmann::json m_DefaultValues;
  nlohmann::json m_Values;
};
} // namespace complex
