#include "DataTypeParameter.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include <optional>

namespace
{
constexpr std::optional<complex::DataType> DataTypefromString(std::string_view string) noexcept
{
  if(string == "bool")
  {
    return complex::DataType::boolean;
  }
  if(string == "int8")
  {
    return complex::DataType::int8;
  }
  if(string == "uint8")
  {
    return complex::DataType::uint8;
  }
  if(string == "int16")
  {
    return complex::DataType::int16;
  }
  if(string == "uint16")
  {
    return complex::DataType::uint16;
  }
  if(string == "int32")
  {
    return complex::DataType::int32;
  }
  if(string == "uint32")
  {
    return complex::DataType::uint32;
  }
  if(string == "int64")
  {
    return complex::DataType::int64;
  }
  if(string == "uint64")
  {
    return complex::DataType::uint64;
  }
  if(string == "float32")
  {
    return complex::DataType::float32;
  }
  if(string == "float64")
  {
    return complex::DataType::float64;
  }

  return {};
}
} // namespace

namespace complex
{
DataTypeParameter::DataTypeParameter(const std::string& name, const std::string& humanName, const std::string& helpText, ValueType defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
{
}

Uuid DataTypeParameter::uuid() const
{
  return ParameterTraits<DataTypeParameter>::uuid;
}

IParameter::AcceptedTypes DataTypeParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

nlohmann::json DataTypeParameter::toJson(const std::any& value) const
{
  auto enumValue = std::any_cast<ValueType>(value);
  nlohmann::json json = enumValue;
  return json;
}

Result<std::any> DataTypeParameter::fromJson(const nlohmann::json& json) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'DataTypeParameter' JSON Error: ";
  if(!json.is_number())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("{}JSON value for key '{}' is not a number", prefix, name()));
  }
  auto type = json.get<ValueType>();
  return {{type}};
}

IParameter::UniquePointer DataTypeParameter::clone() const
{
  return std::make_unique<DataTypeParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

std::any DataTypeParameter::defaultValue() const
{
  return defaultDataType();
}

typename DataTypeParameter::ValueType DataTypeParameter::defaultDataType() const
{
  return m_DefaultValue;
}

Result<> DataTypeParameter::validate(const std::any& value) const
{
  [[maybe_unused]] auto castValue = std::any_cast<ValueType>(value);
  return {};
}
} // namespace complex
