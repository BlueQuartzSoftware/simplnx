#include "ArrayCreationParameter.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

namespace nx::core
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

//------------------------------------------------------------------------------
IParameter::VersionType ArrayCreationParameter::getVersion() const
{
  return 1;
}

nlohmann::json ArrayCreationParameter::toJsonImpl(const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);
  nlohmann::json json = path.toString();
  return json;
}

Result<std::any> ArrayCreationParameter::fromJsonImpl(const nlohmann::json& json, VersionType version) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'ArrayCreationParameter' JSON Error: ";

  if(!json.is_string())
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_String, fmt::format("{}JSON value for key '{}' is not a string", prefix.view(), name()));
  }
  auto string = json.get<std::string>();
  std::optional<DataPath> path = DataPath::FromString(string);
  if(!path.has_value())
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_Value_Type, fmt::format("{}Failed to parse '{}' as DataPath", prefix.view(), string));
  }
  return {{std::move(*path)}};
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
  const std::string prefix = fmt::format("Parameter Name: '{}'\n    Parameter Key: '{}'\n    Validation Error: ", humanName(), name());

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

Result<std::any> ArrayCreationParameter::resolve(DataStructure& dataStructure, const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);
  DataObject* object = dataStructure.getData(path);
  return {{object}};
}

namespace SIMPLConversion
{
Result<DataArrayCreationFilterParameterConverter::ValueType> DataArrayCreationFilterParameterConverter::convert(const nlohmann::json& json)
{
  auto dataContainerNameResult = ReadDataContainerName(json, "DataArrayCreationFilterParameter");
  if(dataContainerNameResult.invalid())
  {
    return ConvertInvalidResult<ValueType>(std::move(dataContainerNameResult));
  }

  auto attributeMatrixNameResult = ReadAttributeMatrixName(json, "DataArrayCreationFilterParameter");
  if(attributeMatrixNameResult.invalid())
  {
    return ConvertInvalidResult<ValueType>(std::move(attributeMatrixNameResult));
  }

  auto dataArrayNameResult = ReadDataArrayName(json, "DataArrayCreationFilterParameter");
  if(dataArrayNameResult.invalid())
  {
    return ConvertInvalidResult<ValueType>(std::move(dataArrayNameResult));
  }

  DataPath dataPath({std::move(dataContainerNameResult.value()), std::move(attributeMatrixNameResult.value()), std::move(dataArrayNameResult.value())});

  return {std::move(dataPath)};
}
} // namespace SIMPLConversion
} // namespace nx::core
