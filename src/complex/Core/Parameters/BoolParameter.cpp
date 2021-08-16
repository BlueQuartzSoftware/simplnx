#include "BoolParameter.hpp"

#include <fmt/core.h>

#include <nlohmann/json.hpp>

namespace complex
{
BoolParameter::BoolParameter(const std::string& name, const std::string& humanName, const std::string& helpText, ValueType defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
{
}

Uuid BoolParameter::uuid() const
{
  return ParameterTraits<BoolParameter>::uuid;
}

IParameter::AcceptedTypes BoolParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

nlohmann::json BoolParameter::toJson(const std::any& value) const
{
  auto boolValue = std::any_cast<ValueType>(value);
  nlohmann::json json = boolValue;
  return json;
}

Result<std::any> BoolParameter::fromJson(const nlohmann::json& json) const
{
  const std::string key = name();
  if(!json.contains(key))
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-1, fmt::format("JSON does not contain key \"{}\"", key)}})};
  }
  auto jsonValue = json.at(key);
  if(!jsonValue.is_boolean())
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-2, fmt::format("JSON value for key \"{}\" is not a bool", key)}})};
  }
  auto boolValue = jsonValue.get<ValueType>();
  return {boolValue};
}

IParameter::UniquePointer BoolParameter::clone() const
{
  return std::make_unique<BoolParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

std::any BoolParameter::defaultValue() const
{
  return defaultBool();
}

typename BoolParameter::ValueType BoolParameter::defaultBool() const
{
  return m_DefaultValue;
}

Result<> BoolParameter::validate(const std::any& value) const
{
  [[maybe_unused]] auto castValue = std::any_cast<ValueType>(value);
  return {};
}
} // namespace complex
