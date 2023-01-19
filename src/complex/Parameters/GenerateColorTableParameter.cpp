#include "GenerateColorTableParameter.hpp"

#include <fmt/core.h>

#include <nlohmann/json.hpp>

#include "complex/Common/Any.hpp"

namespace complex
{
GenerateColorTableParameter::GenerateColorTableParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
{
}

Uuid GenerateColorTableParameter::uuid() const
{
  return ParameterTraits<GenerateColorTableParameter>::uuid;
}

IParameter::AcceptedTypes GenerateColorTableParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

nlohmann::json GenerateColorTableParameter::toJson(const std::any& value) const
{
  const auto& json = GetAnyRef<ValueType>(value);
  return json;
}

Result<std::any> GenerateColorTableParameter::fromJson(const nlohmann::json& json) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'GenerateColorTableParameter' JSON Error: ";
  if(!json.is_object())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("{}JSON value for key '{}' is not an object", prefix, name()));
  }

  return {{nonstd::make_unexpected(std::make_any<nlohmann::json>(json))}};
}

IParameter::UniquePointer GenerateColorTableParameter::clone() const
{
  return std::make_unique<GenerateColorTableParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

std::any GenerateColorTableParameter::defaultValue() const
{
  return defaultJson();
}

typename GenerateColorTableParameter::ValueType GenerateColorTableParameter::defaultJson() const
{
  return m_DefaultValue;
}

Result<> GenerateColorTableParameter::validate(const std::any& value) const
{
  try
  {
    [[maybe_unused]] const auto& json = GetAnyRef<ValueType>(value);
  } catch(const std::bad_any_cast& exception)
  {
    return MakeErrorResult(-1000, fmt::format("FilterParameter '{}' Validation Error: {}", humanName(), exception.what()));
  }

  return {};
}
} // namespace complex
