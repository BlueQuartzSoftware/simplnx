#include "NumericTypeParameter.hpp"

#include <optional>

#include <fmt/core.h>

#include <nlohmann/json.hpp>

namespace
{
constexpr std::optional<complex::NumericType> NumericTypefromString(std::string_view string) noexcept
{
  if(string == "int8")
  {
    return complex::NumericType::int8;
  }
  if(string == "uint8")
  {
    return complex::NumericType::uint8;
  }
  if(string == "int16")
  {
    return complex::NumericType::int16;
  }
  if(string == "uint16")
  {
    return complex::NumericType::uint16;
  }
  if(string == "int32")
  {
    return complex::NumericType::int32;
  }
  if(string == "uint32")
  {
    return complex::NumericType::uint32;
  }
  if(string == "int64")
  {
    return complex::NumericType::int64;
  }
  if(string == "uint64")
  {
    return complex::NumericType::uint64;
  }
  if(string == "float32")
  {
    return complex::NumericType::float32;
  }
  if(string == "float64")
  {
    return complex::NumericType::float64;
  }

  return {};
}
} // namespace

namespace complex
{
NumericTypeParameter::NumericTypeParameter(const std::string& name, const std::string& humanName, const std::string& helpText, ValueType defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
{
}

Uuid NumericTypeParameter::uuid() const
{
  return ParameterTraits<NumericTypeParameter>::uuid;
}

IParameter::AcceptedTypes NumericTypeParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

nlohmann::json NumericTypeParameter::toJson(const std::any& value) const
{
  auto boolValue = std::any_cast<ValueType>(value);
  nlohmann::json json = boolValue;
  return json;
}

Result<std::any> NumericTypeParameter::fromJson(const nlohmann::json& json) const
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
  auto numbericTypeString = jsonValue.get<std::string>();
  std::optional<NumericType> type = NumericTypefromString(numbericTypeString);
  if(!type.has_value())
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-3, fmt::format("JSON value for key \"{}\" is not a valid NumericType", key)}})};
  }
  return {*type};
}

IParameter::UniquePointer NumericTypeParameter::clone() const
{
  return std::make_unique<NumericTypeParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

std::any NumericTypeParameter::defaultValue() const
{
  return defaultNumericType();
}

typename NumericTypeParameter::ValueType NumericTypeParameter::defaultNumericType() const
{
  return m_DefaultValue;
}

Result<> NumericTypeParameter::validate(const std::any& value) const
{
  [[maybe_unused]] auto castValue = std::any_cast<ValueType>(value);
  return {};
}
} // namespace complex
