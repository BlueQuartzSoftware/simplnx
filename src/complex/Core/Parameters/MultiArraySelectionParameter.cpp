#include "MultiArraySelectionParameter.hpp"

#include <fmt/core.h>

#include <nlohmann/json.hpp>

namespace complex
{
MultiArraySelectionParameter ::MultiArraySelectionParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: MutableDataParameter(name, humanName, helpText, Category::Created)
, m_DefaultValue(defaultValue)
{
}

Uuid MultiArraySelectionParameter::uuid() const
{
  return ParameterTraits<MultiArraySelectionParameter>::uuid;
}

IParameter::AcceptedTypes MultiArraySelectionParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

nlohmann::json MultiArraySelectionParameter::toJson(const std::any& value) const
{
  auto paths = std::any_cast<ValueType>(value);
  nlohmann::json json = nlohmann::json::array();
  for(const auto& path : paths)
  {
    json.push_back(path.toString());
  }
  return json;
}

Result<std::any> MultiArraySelectionParameter::fromJson(const nlohmann::json& json) const
{
  const std::string key = name();
  if(!json.contains(key))
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-1, fmt::format("JSON does not contain key \"{}\"", key)}})};
  }
  auto jsonValue = json.at(key);
  if(!jsonValue.is_array())
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-2, fmt::format("JSON value for key \"{}\" is not an array", key)}})};
  }

  ValueType paths;

  for(const auto& item : jsonValue)
  {
    if(!item.is_string())
    {
      return {nonstd::make_unexpected(std::vector<Error>{{-3, "JSON value in array is not a string"}})};
    }
    auto string = jsonValue.get<std::string>();
    auto path = DataPath::FromString(string);
    if(!path.has_value())
    {
      return {nonstd::make_unexpected(std::vector<Error>{{-4, fmt::format("Failed to parse \"{}\" as DataPath", string)}})};
    }
    paths.push_back(std::move(*path));
  }

  return {std::move(paths)};
}

IParameter::UniquePointer MultiArraySelectionParameter::clone() const
{
  return std::make_unique<MultiArraySelectionParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

std::any MultiArraySelectionParameter::defaultValue() const
{
  return defaultPath();
}

typename MultiArraySelectionParameter::ValueType MultiArraySelectionParameter::defaultPath() const
{
  return m_DefaultValue;
}

Result<> MultiArraySelectionParameter::validate(const DataStructure& dataStructure, const std::any& value) const
{
  auto paths = std::any_cast<ValueType>(value);

  return validatePaths(dataStructure, paths);
}

Result<> MultiArraySelectionParameter::validatePaths(const DataStructure& dataStructure, const ValueType& value) const
{
  for(const auto& path : value)
  {
    if(value.empty())
    {
      return complex::MakeErrorResult<>(-1, "DataPath cannot be empty");
    }
    const DataObject* object = dataStructure.getData(path);
    if(object == nullptr)
    {
      return complex::MakeErrorResult<>(-2, fmt::format("Object does not exists at path '{}'", path.toString()));
    }
  }

  return {};
}

Result<std::any> MultiArraySelectionParameter::resolve(DataStructure& dataStructure, const std::any& value) const
{
  auto paths = std::any_cast<ValueType>(value);
  std::vector<DataObject*> objects;
  for(const auto& path : paths)
  {
    objects.push_back(dataStructure.getData(path));
  }
  return {objects};
}
} // namespace complex
