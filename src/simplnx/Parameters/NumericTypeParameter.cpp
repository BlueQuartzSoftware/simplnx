#include "NumericTypeParameter.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include <optional>

namespace
{
constexpr std::optional<nx::core::NumericType> NumericTypefromString(std::string_view string) noexcept
{
  if(string == "int8")
  {
    return nx::core::NumericType::int8;
  }
  if(string == "uint8")
  {
    return nx::core::NumericType::uint8;
  }
  if(string == "int16")
  {
    return nx::core::NumericType::int16;
  }
  if(string == "uint16")
  {
    return nx::core::NumericType::uint16;
  }
  if(string == "int32")
  {
    return nx::core::NumericType::int32;
  }
  if(string == "uint32")
  {
    return nx::core::NumericType::uint32;
  }
  if(string == "int64")
  {
    return nx::core::NumericType::int64;
  }
  if(string == "uint64")
  {
    return nx::core::NumericType::uint64;
  }
  if(string == "float32")
  {
    return nx::core::NumericType::float32;
  }
  if(string == "float64")
  {
    return nx::core::NumericType::float64;
  }

  return {};
}
} // namespace

namespace nx::core
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
  auto enumValue = std::any_cast<ValueType>(value);
  nlohmann::json json = enumValue;
  return json;
}

Result<std::any> NumericTypeParameter::fromJson(const nlohmann::json& json) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'NumericTypeParameter' JSON Error: ";
  if(!json.is_number())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("{}JSON value for key '{}' is not a number", prefix, name()));
  }
  auto type = json.get<ValueType>();
  return {{type}};
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
} // namespace nx::core
