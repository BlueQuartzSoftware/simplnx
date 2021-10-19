#include "DataGroupCreationParameter.hpp"

#include <fmt/core.h>

#include <nlohmann/json.hpp>

namespace complex
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
  auto path = std::any_cast<ValueType>(value);
  nlohmann::json json = path.toString();
  return json;
}

Result<std::any> DataGroupCreationParameter::fromJson(const nlohmann::json& json) const
{
  const std::string prefix("FilterParameter 'DataGroupCreationParameter' JSON Error: ");

  if(!json.contains(name()))
  {
    return complex::MakeErrorResult<std::any>(complex::FilterParameter::Constants::k_Json_Missing_Entry, fmt::format("{}JSON Data does not contain an entry with a key of \"{}\"", prefix, name()));
  }

  auto jsonValue = json.at(name());
  if(!jsonValue.is_object())
  {
    return complex::MakeErrorResult<std::any>(complex::FilterParameter::Constants::k_Json_Value_Not_Object,
                                              fmt::format("{}}The JSON data entry for key \"{}\" is not in the form of a JSON Object.", prefix, name()));
  }

  auto valueString = jsonValue.get<std::string>();
  auto path = DataPath::FromString(valueString);
  if(!path.has_value())
  {
    return complex::MakeErrorResult<std::any>(complex::FilterParameter::Constants::k_Json_Value_Not_Value_Type, fmt::format("{}Failed to parse '{}' as DataPath", prefix, name(), valueString));
  }
  return {path};
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
  auto path = std::any_cast<ValueType>(value);

  return validatePath(dataStructure, path);
}

Result<> DataGroupCreationParameter::validatePath(const DataStructure& dataStructure, const DataPath& value) const
{
  const std::string prefix("FilterParameter 'DataGroupCreationParameter' Validation Error: ");

  if(value.empty())
  {
    return complex::MakeErrorResult(complex::FilterParameter::Constants::k_Validate_Empty_Value, fmt::format("{}DataPath cannot be empty", prefix));
  }
  const DataObject* object = dataStructure.getData(value);
  if(object != nullptr)
  {
    return complex::MakeErrorResult(complex::FilterParameter::Constants::k_Validate_ExistingValue, fmt::format("{}Object exists at path '{}'", prefix, value.toString()));
  }

  return {};
}

Result<std::any> DataGroupCreationParameter::resolve(DataStructure& dataStructure, const std::any& value) const
{
  auto path = std::any_cast<ValueType>(value);
  DataObject* object = dataStructure.getData(path);
  return {object};
}
} // namespace complex
