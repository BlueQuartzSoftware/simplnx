#include "MultiPathSelectionParameter.hpp"

#include "simplnx/Common/Any.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

namespace nx::core
{
MultiPathSelectionParameter::MultiPathSelectionParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: MutableDataParameter(name, humanName, helpText, Category::Required)
, m_DefaultValue(defaultValue)
{
}

Uuid MultiPathSelectionParameter::uuid() const
{
  return ParameterTraits<MultiPathSelectionParameter>::uuid;
}

IParameter::AcceptedTypes MultiPathSelectionParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

nlohmann::json MultiPathSelectionParameter::toJson(const std::any& value) const
{
  const auto& paths = GetAnyRef<ValueType>(value);
  nlohmann::json json = nlohmann::json::array();
  for(const auto& path : paths)
  {
    json.push_back(std::move(path.toString()));
  }
  return json;
}

Result<std::any> MultiPathSelectionParameter::fromJson(const nlohmann::json& json) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'MultiPathSelectionParameter' JSON Error: ";
  if(!json.is_array())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("{}JSON value for key '{}' is not an array", prefix, name()));
  }

  ValueType paths;

  std::vector<Error> errors;

  for(const auto& item : json)
  {
    if(!item.is_string())
    {
      return MakeErrorResult<std::any>(-3, fmt::format("{}JSON value in array is not a string", prefix));
    }
    auto string = item.get<std::string>();
    auto path = DataPath::FromString(string);
    if(!path.has_value())
    {
      errors.push_back(Error{FilterParameter::Constants::k_Json_Value_Not_Value_Type, fmt::format("{}Failed to parse '{}' as DataPath", prefix.view(), string)});
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

IParameter::UniquePointer MultiPathSelectionParameter::clone() const
{
  return std::make_unique<MultiPathSelectionParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

std::any MultiPathSelectionParameter::defaultValue() const
{
  return defaultPath();
}

typename MultiPathSelectionParameter::ValueType MultiPathSelectionParameter::defaultPath() const
{
  return m_DefaultValue;
}

Result<> MultiPathSelectionParameter::validate(const DataStructure& dataStructure, const std::any& value) const
{
  const auto& paths = GetAnyRef<ValueType>(value);

  return validatePaths(dataStructure, paths);
}

Result<> MultiPathSelectionParameter::validatePaths(const DataStructure& dataStructure, const ValueType& value) const
{
  const std::string prefix = fmt::format("FilterParameter '{}' Validation Error: ", humanName());

  std::vector<Error> errors;
  for(usize i = 0; i < value.size(); i++)
  {
    const auto& path = value[i];

    if(path.empty())
    {
      errors.push_back(Error{FilterParameter::Constants::k_Validate_Empty_Value, fmt::format("{}DataPath cannot be empty at index {}", prefix, i)});
      continue;
    }
    const DataObject* object = dataStructure.getData(path);
    if(object == nullptr)
    {
      errors.push_back(Error{FilterParameter::Constants::k_Validate_Does_Not_Exist, fmt::format("{}Object does not exist at path '{}'", prefix, path.toString())});
      continue;
    }
  }

  if(!errors.empty())
  {
    return {nonstd::make_unexpected(std::move(errors))};
  }

  return {};
}

Result<std::any> MultiPathSelectionParameter::resolve(DataStructure& dataStructure, const std::any& value) const
{
  const auto& paths = GetAnyRef<ValueType>(value);
  std::vector<DataObject*> objects;
  for(const auto& path : paths)
  {
    objects.push_back(dataStructure.getData(path));
  }
  return {{objects}};
}
} // namespace nx::core
