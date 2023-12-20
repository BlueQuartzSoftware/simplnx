#include "DataGroupCreationParameter.hpp"

#include "simplnx/Common/Any.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

namespace nx::core
{
DataGroupCreationParameter::DataGroupCreationParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: MutableDataParameter(name, humanName, helpText, Category::Created)
, m_DefaultValue(defaultValue)
{
}

Uuid DataGroupCreationParameter::uuid() const
{
  return ParameterTraits<DataGroupCreationParameter>::uuid;
}

IParameter::AcceptedTypes DataGroupCreationParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

nlohmann::json DataGroupCreationParameter::toJson(const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);
  nlohmann::json json = path.toString();
  return json;
}

Result<std::any> DataGroupCreationParameter::fromJson(const nlohmann::json& json) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'DataGroupCreationParameter' JSON Error: ";

  if(!json.is_string())
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_String, fmt::format("{}The JSON data entry for key '{}' is not a string.", prefix.view(), name()));
  }

  auto valueString = json.get<std::string>();
  std::optional<DataPath> path = DataPath::FromString(valueString);
  if(!path.has_value())
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_Value_Type, fmt::format("{}Failed to parse '{}' as DataPath for key '{}'.", prefix.view(), valueString, name()));
  }
  return {{std::move(*path)}};
}

IParameter::UniquePointer DataGroupCreationParameter::clone() const
{
  return std::make_unique<DataGroupCreationParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

std::any DataGroupCreationParameter::defaultValue() const
{
  return defaultPath();
}

typename DataGroupCreationParameter::ValueType DataGroupCreationParameter::defaultPath() const
{
  return m_DefaultValue;
}

Result<> DataGroupCreationParameter::validate(const DataStructure& dataStructure, const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);

  return validatePath(dataStructure, path);
}

Result<> DataGroupCreationParameter::validatePath(const DataStructure& dataStructure, const DataPath& value) const
{
  const std::string prefix = fmt::format("FilterParameter '{}' Validation Error: ", humanName());

  if(value.empty())
  {
    return nx::core::MakeErrorResult(nx::core::FilterParameter::Constants::k_Validate_Empty_Value, fmt::format("{}DataPath cannot be empty", prefix));
  }
  const DataObject* object = dataStructure.getData(value);
  if(object != nullptr)
  {
    return nx::core::MakeErrorResult(nx::core::FilterParameter::Constants::k_Validate_ExistingValue, fmt::format("{}Object exists at path '{}'", prefix, value.toString()));
  }

  return {};
}

Result<std::any> DataGroupCreationParameter::resolve(DataStructure& dataStructure, const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);
  DataObject* object = dataStructure.getData(path);
  return {{object}};
}
} // namespace nx::core
