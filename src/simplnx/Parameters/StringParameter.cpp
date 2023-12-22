#include "StringParameter.hpp"

#include "simplnx/Common/Any.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

namespace nx::core
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
  [[maybe_unused]] const auto& stringValue = GetAnyRef<ValueType>(value);
  return {};
}
} // namespace nx::core
