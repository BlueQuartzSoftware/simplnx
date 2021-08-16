#include "NumericTypeParameter.hpp"

#include <optional>

#include <fmt/core.h>

#include <nlohmann/json.hpp>

namespace
{
std::optional<complex::NumericType> NumericTypefromString(std::string_view string)
{
  if(string == "i8")
  {
    return complex::NumericType::i8;
  }
  if(string == "u8")
  {
    return complex::NumericType::u8;
  }
  if(string == "i16")
  {
    return complex::NumericType::i8;
  }
  if(string == "u16")
  {
    return complex::NumericType::u8;
  }
  if(string == "i32")
  {
    return complex::NumericType::i32;
  }
  if(string == "u32")
  {
    return complex::NumericType::u32;
  }
  if(string == "i64")
  {
    return complex::NumericType::i64;
  }
  if(string == "u64")
  {
    return complex::NumericType::u64;
  }
  if(string == "f32")
  {
    return complex::NumericType::f32;
  }
  if(string == "f64")
  {
    return complex::NumericType::f64;
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
