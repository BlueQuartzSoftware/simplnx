#include "ArrayCreationParameter.hpp"

#include <fmt/core.h>

#include <nlohmann/json.hpp>

namespace complex
{
ArrayCreationParameter::ArrayCreationParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: MutableDataParameter(name, humanName, helpText, Category::Created)
, m_DefaultValue(defaultValue)
{
}

Uuid ArrayCreationParameter::uuid() const
{
  return ParameterTraits<ArrayCreationParameter>::uuid;
}

IParameter::AcceptedTypes ArrayCreationParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

nlohmann::json ArrayCreationParameter::toJson(const std::any& value) const
{
  auto path = std::any_cast<ValueType>(value);
  nlohmann::json json = path.toString();
  return json;
}

Result<std::any> ArrayCreationParameter::fromJson(const nlohmann::json& json) const
{
  const std::string key = name();
  if(!json.contains(key))
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-1, fmt::format("JSON does not contain key \"{}\"", key)}})};
  }
  auto jsonValue = json.at(key);
  if(!jsonValue.is_string())
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-2, fmt::format("JSON value for key \"{}\" is not a string", key)}})};
  }
  auto string = jsonValue.get<std::string>();
  auto path = DataPath::FromString(string);
  if(!path.has_value())
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-3, fmt::format("Failed to parse \"{}\" as DataPath", string)}})};
  }
  return {path};
}

IParameter::UniquePointer ArrayCreationParameter::clone() const
{
  return std::make_unique<ArrayCreationParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

std::any ArrayCreationParameter::defaultValue() const
{
  return defaultPath();
}

typename ArrayCreationParameter::ValueType ArrayCreationParameter::defaultPath() const
{
  return m_DefaultValue;
}

Result<> ArrayCreationParameter::validate(const DataStructure& dataStructure, const std::any& value) const
{
  auto path = std::any_cast<ValueType>(value);

  return validatePath(dataStructure, path);
}

Result<> ArrayCreationParameter::validatePath(const DataStructure& dataStructure, const DataPath& value) const
{
  const DataObject* object = dataStructure.getData(value);
  if(object == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-1, fmt::format("Object does not exist at path \"{}\"", value.toString())}})};
  }

  return {};
}

Result<std::any> ArrayCreationParameter::resolve(DataStructure& dataStructure, const std::any& value) const
{
  auto path = std::any_cast<ValueType>(value);
  DataObject* object = dataStructure.getData(path);
  return {object};
}
} // namespace complex
