#include "StringParameter.hpp"

#include "complex/Common/Any.hpp"

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
  const auto& stringValue = GetAnyRef<ValueType>(value);
  nlohmann::json json = stringValue;
  return json;
}

Result<std::any> StringParameter::fromJson(const nlohmann::json& json) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'StringParameter' JSON Error: ";
  if(!json.is_string())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("{}JSON value for key '{}' is not a string", prefix, name()));
  }
  auto string = json.get<ValueType>();
  return {{string}};
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
  try
  {
    [[maybe_unused]] const auto& stringValue = GetAnyRef<ValueType>(value);
  } catch(const std::bad_any_cast& exception)
  {
    return MakeErrorResult(-1000, fmt::format("FilterParameter '{}' Validation Error: {}", humanName(), exception.what()));
  }

  return {};
}
} // namespace complex
