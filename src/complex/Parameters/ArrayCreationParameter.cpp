#include "ArrayCreationParameter.hpp"

#include <fmt/core.h>

#include <nlohmann/json.hpp>

#include "complex/Common/Any.hpp"

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
  const auto& path = GetAnyRef<ValueType>(value);
  nlohmann::json json = path.toString();
  return json;
}

Result<std::any> ArrayCreationParameter::fromJson(const nlohmann::json& json) const
{
  if(!json.is_string())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("JSON value for key \"{}\" is not a string", name()));
  }
  auto string = json.get<std::string>();
  std::optional<DataPath> path = DataPath::FromString(string);
  if(!path.has_value())
  {
    return MakeErrorResult<std::any>(-3, fmt::format("Failed to parse \"{}\" as DataPath", string));
  }
  return {std::move(*path)};
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
  const auto& path = GetAnyRef<ValueType>(value);

  return validatePath(dataStructure, path);
}

Result<> ArrayCreationParameter::validatePath(const DataStructure& dataStructure, const DataPath& value) const
{
  const std::string prefix = fmt::format("FilterParameter '{}' Validation Error: ", humanName());

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

Result<std::any> ArrayCreationParameter::resolve(DataStructure& dataStructure, const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);
  DataObject* object = dataStructure.getData(path);
  return {object};
}
} // namespace complex
