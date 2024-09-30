#include "DataGroupCreationParameter.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

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

//------------------------------------------------------------------------------
IParameter::VersionType DataGroupCreationParameter::getVersion() const
{
  return 1;
}

nlohmann::json DataGroupCreationParameter::toJsonImpl(const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);
  nlohmann::json json = path.toString();
  return json;
}

Result<std::any> DataGroupCreationParameter::fromJsonImpl(const nlohmann::json& json, VersionType version) const
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

Result<std::any> DataGroupCreationParameter::resolve(DataStructure& dataStructure, const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);
  DataObject* object = dataStructure.getData(path);
  return {{object}};
}

namespace SIMPLConversion
{
Result<DataContainerCreationFilterParameterConverter::ValueType> DataContainerCreationFilterParameterConverter::convert(const nlohmann::json& json)
{
  auto dataContainerNameResult = ReadDataContainerName(json, "DataContainerCreationFilterParameter");
  if(dataContainerNameResult.invalid())
  {
    return ConvertInvalidResult<ValueType>(std::move(dataContainerNameResult));
  }

  DataPath dataPath({std::move(dataContainerNameResult.value())});

  return {std::move(dataPath)};
}

Result<StringToDataPathFilterParameterConverter::ValueType> StringToDataPathFilterParameterConverter::convert(const nlohmann::json& json)
{
  if(!json.is_string())
  {
    return MakeErrorResult<ValueType>(-1, fmt::format("StringToDataPathFilterParameterConverter json '{}' is not a string", json.dump()));
  }

  DataPath dataPath({json.get<std::string>()});

  return {std::move(dataPath)};
}

Result<StringsToDataPathFilterParameterConverter::ValueType> StringsToDataPathFilterParameterConverter::convert(const nlohmann::json& json1, const nlohmann::json& json2)
{
  if(!json1.is_string())
  {
    return MakeErrorResult<ValueType>(-1, fmt::format("StringsToDataPathFilterParameterConverter json '{}' is not a string", json1.dump()));
  }
  if(!json2.is_string())
  {
    return MakeErrorResult<ValueType>(-1, fmt::format("StringsToDataPathFilterParameterConverter json '{}' is not a string", json2.dump()));
  }

  DataPath dataPath({json1.get<std::string>(), json2.get<std::string>()});

  return {std::move(dataPath)};
}

Result<AMPathBuilderFilterParameterConverter::ValueType> AMPathBuilderFilterParameterConverter::convert(const nlohmann::json& json1, const nlohmann::json& json2)
{
  std::string dcName;
  std::string amName;

  if(json1.is_string())
  {
    dcName = json1.get<std::string>();
  }
  else
  {
    auto dataContainerNameResult = ReadDataContainerName(json1, "AMPathBuilderFilterParameterConverter");
    if(dataContainerNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(dataContainerNameResult));
    }
    dcName = std::move(dataContainerNameResult.value());
  }

  if(json2.is_string())
  {
    amName = json2.get<std::string>();
  }
  else
  {
    auto attributeMatrixNameResult = ReadAttributeMatrixName(json2, "AMPathBuilderFilterParameterConverter");
    if(attributeMatrixNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(attributeMatrixNameResult));
    }
    amName = std::move(attributeMatrixNameResult.value());
  }

  DataPath dataPath({dcName, amName});

  return {std::move(dataPath)};
}

Result<DAPathBuilderFilterParameterConverter::ValueType> DAPathBuilderFilterParameterConverter::convert(const nlohmann::json& json1, const nlohmann::json& json2, const nlohmann::json& json3)
{
  std::string dcName;
  std::string amName;
  std::string daName;

  if(json1.is_string())
  {
    dcName = json1.get<std::string>();
  }
  else
  {
    auto dataContainerNameResult = ReadDataContainerName(json1, "DAPathBuilderFilterParameterConverter");
    if(dataContainerNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(dataContainerNameResult));
    }
    dcName = std::move(dataContainerNameResult.value());
  }

  if(json2.is_string())
  {
    amName = json2.get<std::string>();
  }
  else
  {
    auto attributeMatrixNameResult = ReadAttributeMatrixName(json2, "DAPathBuilderFilterParameterConverter");
    if(attributeMatrixNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(attributeMatrixNameResult));
    }
    amName = std::move(attributeMatrixNameResult.value());
  }

  if(json3.is_string())
  {
    daName = json3.get<std::string>();
  }
  else
  {
    auto dataArrayNameResult = ReadDataArrayName(json3, "DAPathBuilderFilterParameterConverter");
    if(dataArrayNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(dataArrayNameResult));
    }
    daName = std::move(dataArrayNameResult.value());
  }

  DataPath dataPath({dcName, amName, daName});

  return {std::move(dataPath)};
}

Result<AttributeMatrixCreationFilterParameterConverter::ValueType> AttributeMatrixCreationFilterParameterConverter::convert(const nlohmann::json& json)
{
  auto dataContainerNameResult = ReadDataContainerName(json, "AttributeMatrixCreationFilterParameter");
  if(dataContainerNameResult.invalid())
  {
    return ConvertInvalidResult<ValueType>(std::move(dataContainerNameResult));
  }

  auto attributeMatrixNameResult = ReadAttributeMatrixName(json, "AttributeMatrixCreationFilterParameter");
  if(attributeMatrixNameResult.invalid())
  {
    return ConvertInvalidResult<ValueType>(std::move(attributeMatrixNameResult));
  }

  DataPath dataPath({std::move(dataContainerNameResult.value()), std::move(attributeMatrixNameResult.value())});

  return {std::move(dataPath)};
}
} // namespace SIMPLConversion
} // namespace nx::core
