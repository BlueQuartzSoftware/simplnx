#include "DataStoreFormatParameter.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Core/Application.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

namespace
{
constexpr nx::core::int64 k_BadFormatCode = -780;
}

namespace nx::core
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

//------------------------------------------------------------------------------
IParameter::VersionType DataStoreFormatParameter::getVersion() const
{
  return 1;
}

nlohmann::json DataStoreFormatParameter::toJsonImpl(const std::any& value) const
{
  return GetAnyRef<ValueType>(value);
}

Result<std::any> DataStoreFormatParameter::fromJsonImpl(const nlohmann::json& json, VersionType version) const
{
  if(!json.is_string())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("FilterParameter 'DataStoreFormatParameter' JSON Error: JSON value for key '{}' is not a string", name()));
  }
  return {{json.get<ValueType>()}};
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
} // namespace nx::core
