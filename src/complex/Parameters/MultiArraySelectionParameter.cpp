#include "MultiArraySelectionParameter.hpp"

#include <fmt/core.h>

#include <nlohmann/json.hpp>

#include "complex/Common/Any.hpp"

namespace complex
{
MultiArraySelectionParameter::MultiArraySelectionParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: MutableDataParameter(name, humanName, helpText, Category::Required)
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
  const auto& paths = GetAnyRef<ValueType>(value);
  nlohmann::json json = nlohmann::json::array();
  for(const auto& path : paths)
  {
    json.push_back(path.toString());
  }
  return json;
}

Result<std::any> MultiArraySelectionParameter::fromJson(const nlohmann::json& json) const
{
  if(!json.is_array())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("JSON value for key '{}' is not an array", name()));
  }

  ValueType paths;

  std::vector<Error> errors;

  for(const auto& item : json)
  {
    if(!item.is_string())
    {
      return MakeErrorResult<std::any>(-3, fmt::format("JSON value in array is not a string"));
    }
    auto string = item.get<std::string>();
    auto path = DataPath::FromString(string);
    if(!path.has_value())
    {
      errors.push_back(Error{-4, fmt::format("Failed to parse '{}' as DataPath", string)});
      continue;
    }
    paths.push_back(std::move(*path));
  }

  if(!errors.empty())
  {
    return {{nonstd::make_unexpected(std::move(errors))}};
  }

  return {{std::move(paths)}};
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
  const auto& paths = GetAnyRef<ValueType>(value);

  return validatePaths(dataStructure, paths);
}

Result<> MultiArraySelectionParameter::validatePaths(const DataStructure& dataStructure, const ValueType& value) const
{
  const std::string prefix = fmt::format("FilterParameter '{}' Validation Error: ", humanName());
  for(const auto& path : value)
  {
    if(value.empty())
    {
      return complex::MakeErrorResult(complex::FilterParameter::Constants::k_Validate_Empty_Value, fmt::format("{}DataPath cannot be empty", prefix));
    }
    const DataObject* object = dataStructure.getData(path);
    if(object == nullptr)
    {
      return complex::MakeErrorResult<>(complex::FilterParameter::Constants::k_Validate_Does_Not_Exist, fmt::format("{}Object does not exist at path '{}'", prefix, path.toString()));
    }
  }

  return {};
}

Result<std::any> MultiArraySelectionParameter::resolve(DataStructure& dataStructure, const std::any& value) const
{
  const auto& paths = GetAnyRef<ValueType>(value);
  std::vector<DataObject*> objects;
  for(const auto& path : paths)
  {
    objects.push_back(dataStructure.getData(path));
  }
  return {{objects}};
}
} // namespace complex
