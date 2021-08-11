#include "InputFileParameter.hpp"

#include <fmt/core.h>

#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

namespace complex
{
InputFileParameter::InputFileParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
{
}

Uuid InputFileParameter::uuid() const
{
  constexpr Uuid uuid = *Uuid::FromString("f9a93f3d-21ef-43a1-a958-e57cbf3b2909");
  return uuid;
}

IParameter::AcceptedTypes InputFileParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

nlohmann::json InputFileParameter::toJson(const std::any& value) const
{
  auto path = std::any_cast<ValueType>(value);
  nlohmann::json json = path.string();
  return json;
}

Result<std::any> InputFileParameter::fromJson(const nlohmann::json& json) const
{
  const std::string key = name();
  if(!json.contains(key))
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-1, fmt::format("JSON does not contain key \"{}\"", key)}})};
  }
  auto jsonValue = json.at(key);
  if(!jsonValue.is_string())
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-2, fmt::format("JSON value for key \"{}\" is not a string", key)}})};
  }
  auto pathString = jsonValue.get<std::string>();
  std::filesystem::path path = pathString;
  return {pathString};
}

IParameter::UniquePointer InputFileParameter::clone() const
{
  return std::make_unique<InputFileParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

std::any InputFileParameter::defaultValue() const
{
  return defaultPath();
}

typename InputFileParameter::ValueType InputFileParameter::defaultPath() const
{
  return m_DefaultValue;
}

Result<> InputFileParameter::validate(const std::any& value) const
{
  auto path = std::any_cast<ValueType>(value);
  return validatePath(path);
}

Result<> InputFileParameter::validatePath(const ValueType& path) const
{
  if(path.empty())
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-1, "Path must not be empty"}})};
  }

  if(!fs::exists(path))
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-1, fmt::format("Path \"{}\" does not exist", path.string())}})};
  }

  if(!fs::is_regular_file(path))
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-1, fmt::format("Path \"{}\" is not a file", path.string())}})};
  }

  return {};
}
} // namespace complex
