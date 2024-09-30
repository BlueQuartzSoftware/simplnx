#include "DataTypeParameter.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include <optional>

namespace
{
constexpr std::optional<nx::core::DataType> DataTypefromString(std::string_view string) noexcept
{
  if(string == "bool")
  {
    return nx::core::DataType::boolean;
  }
  if(string == "int8")
  {
    return nx::core::DataType::int8;
  }
  if(string == "uint8")
  {
    return nx::core::DataType::uint8;
  }
  if(string == "int16")
  {
    return nx::core::DataType::int16;
  }
  if(string == "uint16")
  {
    return nx::core::DataType::uint16;
  }
  if(string == "int32")
  {
    return nx::core::DataType::int32;
  }
  if(string == "uint32")
  {
    return nx::core::DataType::uint32;
  }
  if(string == "int64")
  {
    return nx::core::DataType::int64;
  }
  if(string == "uint64")
  {
    return nx::core::DataType::uint64;
  }
  if(string == "float32")
  {
    return nx::core::DataType::float32;
  }
  if(string == "float64")
  {
    return nx::core::DataType::float64;
  }

  return {};
}
} // namespace

namespace nx::core
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

//------------------------------------------------------------------------------
IParameter::VersionType DataTypeParameter::getVersion() const
{
  return 1;
}

nlohmann::json DataTypeParameter::toJsonImpl(const std::any& value) const
{
  auto enumValue = std::any_cast<ValueType>(value);
  nlohmann::json json = enumValue;
  return json;
}

Result<std::any> DataTypeParameter::fromJsonImpl(const nlohmann::json& json, VersionType version) const
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

namespace SIMPLConversion
{
/*
enum class Type : int32_t
{
  Int8 = 0,
  UInt8,
  Int16,
  UInt16,
  Int32,
  UInt32,
  Int64,
  UInt64,
  Float,
  Double,
  Bool,
  SizeT
};
*/

Result<ScalarTypeParameterConverter::ValueType> ScalarTypeParameterConverter::convert(const nlohmann::json& json)
{
  if(!json.is_number_integer())
  {
    return MakeErrorResult<ValueType>(-1, fmt::format("ScalarTypeParameter json '{}' is not an integer", json.dump()));
  }

  auto value = json.get<int32>();

  if(value < 0 || value > 11)
  {
    return MakeErrorResult<ValueType>(-2, fmt::format("Invalid ScalarTypeParameter value '{}'", value));
  }

  // Convert size_t to uint64
  if(value == 11)
  {
    return {DataType::uint64};
  }

  return {static_cast<DataType>(value)};
}
} // namespace SIMPLConversion
} // namespace nx::core
