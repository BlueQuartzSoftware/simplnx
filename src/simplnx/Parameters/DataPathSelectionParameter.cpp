#include "DataPathSelectionParameter.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Common/StringLiteralFormatting.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

namespace nx::core
{
DataPathSelectionParameter::DataPathSelectionParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: MutableDataParameter(name, humanName, helpText, Category::Required)
, m_DefaultValue(defaultValue)
{
}

Uuid DataPathSelectionParameter::uuid() const
{
  return ParameterTraits<DataPathSelectionParameter>::uuid;
}

IParameter::AcceptedTypes DataPathSelectionParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

//------------------------------------------------------------------------------
IParameter::VersionType DataPathSelectionParameter::getVersion() const
{
  return 1;
}

nlohmann::json DataPathSelectionParameter::toJsonImpl(const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);
  nlohmann::json json = path.toString();
  return json;
}

Result<std::any> DataPathSelectionParameter::fromJsonImpl(const nlohmann::json& json, VersionType version) const
{
  static constexpr StringLiteral prefix("FilterParameter 'DataPathSelectionParameter' JSON Error: ");
  if(!json.is_string())
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_String, fmt::format("{}The JSON data entry for key '{}' is not a string.", prefix.view(), name()));
  }

  auto valueString = json.get<std::string>();
  auto path = DataPath::FromString(valueString);
  if(!path.has_value())
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_Value_Type, fmt::format("{}Failed to parse '{}' as DataPath for key '{}'", prefix, valueString, name()));
  }
  return {{std::move(*path)}};
}

IParameter::UniquePointer DataPathSelectionParameter::clone() const
{
  return std::make_unique<DataPathSelectionParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

std::any DataPathSelectionParameter::defaultValue() const
{
  return defaultPath();
}

typename DataPathSelectionParameter::ValueType DataPathSelectionParameter::defaultPath() const
{
  return m_DefaultValue;
}

Result<> DataPathSelectionParameter::validate(const DataStructure& dataStructure, const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);

  return validatePath(dataStructure, path);
}

Result<> DataPathSelectionParameter::validatePath(const DataStructure& dataStructure, const DataPath& value) const
{
  const std::string prefix = fmt::format("Parameter Name: '{}'\n    Parameter Key: '{}'\n    Validation Error: ", humanName(), name());

  if(value.empty())
  {
    return nx::core::MakeErrorResult(nx::core::FilterParameter::Constants::k_Validate_Empty_Value, fmt::format("{}DataPath cannot be empty", prefix));
  }

  const DataObject* dataObject = dataStructure.getData(value);
  if(dataObject == nullptr)
  {
    return nx::core::MakeErrorResult(nx::core::FilterParameter::Constants::k_Validate_DuplicateValue, fmt::format("{}Object does not exist at path '{}'", prefix, value.toString()));
  }

  return {};
}

Result<std::any> DataPathSelectionParameter::resolve(DataStructure& dataStructure, const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);
  DataObject* object = dataStructure.getData(path);
  return {{object}};
}
} // namespace nx::core
