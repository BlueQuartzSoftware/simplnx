#include "DataObjectNameParameter.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/DataStructure/DataObject.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

namespace nx::core
{
DataObjectNameParameter::DataObjectNameParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
{
}

Uuid DataObjectNameParameter::uuid() const
{
  return ParameterTraits<DataObjectNameParameter>::uuid;
}

IParameter::AcceptedTypes DataObjectNameParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

nlohmann::json DataObjectNameParameter::toJson(const std::any& value) const
{
  const auto& stringValue = GetAnyRef<ValueType>(value);
  nlohmann::json json = stringValue;
  return json;
}

Result<std::any> DataObjectNameParameter::fromJson(const nlohmann::json& json) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'DataObjectNameParameter' JSON Error: ";
  if(!json.is_string())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("{}JSON value for key '{}' is not a string", prefix, name()));
  }
  auto string = json.get<ValueType>();
  return {{string}};
}

IParameter::UniquePointer DataObjectNameParameter::clone() const
{
  return std::make_unique<DataObjectNameParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

std::any DataObjectNameParameter::defaultValue() const
{
  return defaultName();
}

typename DataObjectNameParameter::ValueType DataObjectNameParameter::defaultName() const
{
  return m_DefaultValue;
}

Result<> DataObjectNameParameter::validate(const std::any& value) const
{
  const auto& stringValue = GetAnyRef<ValueType>(value);
  return validateName(stringValue);
}

Result<> DataObjectNameParameter::validateName(const std::string& value) const
{
  const std::string prefix = fmt::format("Parameter Name: '{}'\n    Parameter Key: '{}'\n    Validation Error: ", humanName(), name());

  if(value.empty())
  {
    return nx::core::MakeErrorResult(nx::core::FilterParameter::Constants::k_Validate_Empty_Value, fmt::format("{}DataObjectName cannot be empty", prefix));
  }

  if(!DataObject::IsValidName(value))
  {
    return MakeErrorResult(nx::core::FilterParameter::Constants::k_Validate_InvalidDataObjectName, fmt::format("{}'{}' is not a valid DataObject name", prefix, value));
  }
  return {};
}
} // namespace nx::core
