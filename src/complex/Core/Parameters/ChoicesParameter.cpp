#include "ChoicesParameter.hpp"

#include <fmt/core.h>

#include <nlohmann/json.hpp>

namespace complex
{
ChoicesParameter::ChoicesParameter(const std::string& name, const std::string& humanName, const std::string& helpText, ValueType defaultValue, const Choices& choices)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
, m_Choices(choices)
{
}

Uuid ChoicesParameter::uuid() const
{
  return ParameterTraits<ChoicesParameter>::uuid;
}

IParameter::AcceptedTypes ChoicesParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

nlohmann::json ChoicesParameter::toJson(const std::any& value) const
{
  auto index = std::any_cast<ValueType>(value);
  nlohmann::json json = index;
  return json;
}

Result<std::any> ChoicesParameter::fromJson(const nlohmann::json& json) const
{
  const std::string key = name();
  if(!json.contains(key))
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-1, fmt::format("JSON does not contain key \"{}\"", key)}})};
  }
  auto jsonValue = json.at(key);
  if(!jsonValue.is_number())
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-2, fmt::format("JSON value for key \"{}\" is not a number", key)}})};
  }
  auto index = jsonValue.get<ValueType>();
  return {index};
}

IParameter::UniquePointer ChoicesParameter::clone() const
{
  return std::make_unique<ChoicesParameter>(name(), humanName(), helpText(), m_DefaultValue, m_Choices);
}

std::any ChoicesParameter::defaultValue() const
{
  return defaultIndex();
}

typename ChoicesParameter::ValueType ChoicesParameter::defaultIndex() const
{
  return m_DefaultValue;
}

Result<> ChoicesParameter::validate(const std::any& value) const
{
  auto index = std::any_cast<ValueType>(value);
  return validateIndex(index);
}

Result<> ChoicesParameter::validateIndex(ValueType index) const
{
  if(index >= m_Choices.size())
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-1, fmt::format("Index \"{}\" must be less than {}", index, m_Choices.size())}})};
  }

  return {};
}

ChoicesParameter::Choices ChoicesParameter::choices() const
{
  return m_Choices;
}
} // namespace complex
