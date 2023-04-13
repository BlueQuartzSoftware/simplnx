#include "DataStoreFormatParameter.hpp"

#include "complex/Common/Any.hpp"
#include "complex/Core/Application.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

namespace
{
constexpr complex::int64 k_BadFormatCode = -780;
}

namespace complex
{
DataStoreFormatParameter::DataStoreFormatParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
{
}

Uuid DataStoreFormatParameter::uuid() const
{
  return ParameterTraits<DataStoreFormatParameter>::uuid;
}

IParameter::AcceptedTypes DataStoreFormatParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

nlohmann::json DataStoreFormatParameter::toJson(const std::any& value) const
{
  const auto& stringValue = GetAnyRef<ValueType>(value);
  nlohmann::json json = stringValue;
  return json;
}

Result<std::any> DataStoreFormatParameter::fromJson(const nlohmann::json& json) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'DataStoreFormatParameter' JSON Error: ";
  if(!json.is_string())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("{}JSON value for key '{}' is not a string", prefix, name()));
  }
  auto string = json.get<ValueType>();
  return {{string}};
}

IParameter::UniquePointer DataStoreFormatParameter::clone() const
{
  return std::make_unique<DataStoreFormatParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

std::any DataStoreFormatParameter::defaultValue() const
{
  return defaultString();
}

typename DataStoreFormatParameter::ValueType DataStoreFormatParameter::defaultString() const
{
  return m_DefaultValue;
}

typename DataStoreFormatParameter::AvailableValuesType DataStoreFormatParameter::availableValues() const
{
  return Application::GetOrCreateInstance()->getDataStoreFormats();
}

Result<> DataStoreFormatParameter::validate(const std::any& value) const
{
  [[maybe_unused]] const auto& stringValue = GetAnyRef<ValueType>(value);
  const auto formats = Application::GetOrCreateInstance()->getDataStoreFormats();
  if(std::find(formats.begin(), formats.end(), stringValue) == formats.end())
  {
    std::string ss = fmt::format("DataStore format not known: '{}'", stringValue);
    return MakeErrorResult(k_BadFormatCode, ss);
  }
  return {};
}
} // namespace complex
