#include "StringParameter.hpp"

#include <fmt/core.h>

#include <nlohmann/json.hpp>

namespace complex
{
StringParameter::StringParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
{
}

Uuid StringParameter::uuid() const
{
  return ParameterTraits<StringParameter>::uuid;
}

IParameter::AcceptedTypes StringParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

nlohmann::json StringParameter::toJson(const std::any& value) const
{
  auto boolValue = std::any_cast<ValueType>(value);
  nlohmann::json json = boolValue;
  return json;
}

Result<std::any> StringParameter::fromJson(const nlohmann::json& json) const
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
  auto string = jsonValue.get<ValueType>();
  return {string};
}

IParameter::UniquePointer StringParameter::clone() const
{
  return std::make_unique<StringParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

std::any StringParameter::defaultValue() const
{
  return defaultString();
}

typename StringParameter::ValueType StringParameter::defaultString() const
{
  return m_DefaultValue;
}

Result<> StringParameter::validate(const std::any& value) const
{
  [[maybe_unused]] auto castValue = std::any_cast<ValueType>(value);
  return {};
}
} // namespace complex
