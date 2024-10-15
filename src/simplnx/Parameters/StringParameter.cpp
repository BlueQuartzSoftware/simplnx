#include "StringParameter.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Common/StringLiteralFormatting.hpp"

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

//------------------------------------------------------------------------------
IParameter::VersionType StringParameter::getVersion() const
{
  return 1;
}

nlohmann::json StringParameter::toJsonImpl(const std::any& value) const
{
  const auto& stringValue = GetAnyRef<ValueType>(value);
  nlohmann::json json = stringValue;
  return json;
}

Result<std::any> StringParameter::fromJsonImpl(const nlohmann::json& json, VersionType version) const
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

namespace SIMPLConversion
{
template <typename T>
Result<typename NumberToStringFilterParameterConverter<T>::ValueType> NumberToStringFilterParameterConverter<T>::convert(const nlohmann::json& json)
{
  if(!json.is_number())
  {
    return MakeErrorResult<ValueType>(-1, fmt::format("NumberToStringFilterParameterConverter json '{}' is not a number", json.dump()));
  }

  auto value = json.get<T>();

  return {std::to_string(value)};
}

template struct SIMPLNX_TEMPLATE_EXPORT NumberToStringFilterParameterConverter<int8>;
template struct SIMPLNX_TEMPLATE_EXPORT NumberToStringFilterParameterConverter<uint8>;

template struct SIMPLNX_TEMPLATE_EXPORT NumberToStringFilterParameterConverter<int16>;
template struct SIMPLNX_TEMPLATE_EXPORT NumberToStringFilterParameterConverter<uint16>;

template struct SIMPLNX_TEMPLATE_EXPORT NumberToStringFilterParameterConverter<int32>;
template struct SIMPLNX_TEMPLATE_EXPORT NumberToStringFilterParameterConverter<uint32>;

template struct SIMPLNX_TEMPLATE_EXPORT NumberToStringFilterParameterConverter<int64>;
template struct SIMPLNX_TEMPLATE_EXPORT NumberToStringFilterParameterConverter<uint64>;

template struct SIMPLNX_TEMPLATE_EXPORT NumberToStringFilterParameterConverter<float32>;
template struct SIMPLNX_TEMPLATE_EXPORT NumberToStringFilterParameterConverter<float64>;

Result<StringFilterParameterConverter::ValueType> StringFilterParameterConverter::convert(const nlohmann::json& json)
{
  if(!json.is_string())
  {
    return MakeErrorResult<ValueType>(-1, fmt::format("StringFilterParameter json '{}' is not a string", json.dump()));
  }

  auto value = json.get<std::string>();

  return {std::move(value)};
}
} // namespace SIMPLConversion
} // namespace nx::core
