#include "DataGroupCreationParameter.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include <utility>

namespace nx::core
{
DataGroupCreationParameter::DataGroupCreationParameter(const std::string& name, const std::string& humanName, const std::string& helpText, ValueType defaultValue)
: MutableDataParameter(name, humanName, helpText, Category::Created)
, m_DefaultValue(std::move(defaultValue))
{
}

Uuid DataGroupCreationParameter::uuid() const
{
  return ParameterTraits<DataGroupCreationParameter>::uuid;
}

IParameter::AcceptedTypesType DataGroupCreationParameter::acceptedTypes() const
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

Result<std::any> DataGroupCreationParameter::fromJsonImpl(const nlohmann::json& json, VersionType /*version*/) const
{
  static constexpr StringLiteralType k_Prefix = "FilterParameter 'DataGroupCreationParameter' JSON Error: ";

  if(!json.is_string())
  {
    return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonValueNotString, fmt::format("{}The JSON data entry for key '{}' is not a string.", k_Prefix.view(), name()));
  }

  auto valueString = json.get<std::string>();
  std::optional<DataPath> path = DataPath::FromString(valueString);
  if(!path.has_value())
  {
    return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonValueNotValueType, fmt::format("{}Failed to parse '{}' as DataPath for key '{}'.", k_Prefix.view(), valueString, name()));
  }
  return {{std::move(*path)}};
}

IParameter::UniquePointerType DataGroupCreationParameter::clone() const
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
    return nx::core::MakeErrorResult(nx::core::filter_parameter::constants::k_ValidateEmptyValue, fmt::format("{}DataPath cannot be empty", prefix));
  }
  const DataObject* objectPtr = dataStructure.getData(value);
  if(objectPtr != nullptr)
  {
    return nx::core::MakeErrorResult(nx::core::filter_parameter::constants::k_ValidateExistingValue, fmt::format("{}Object exists at path '{}'", prefix, value.toString()));
  }

  return {};
}

Result<std::any> DataGroupCreationParameter::resolve(DataStructure& dataStructure, const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);
  DataObject* objectPtr = dataStructure.getData(path);
  return {{objectPtr}};
}

namespace simpl_conversion
{
Result<DataContainerCreationFilterParameterConverter::ValueType> DataContainerCreationFilterParameterConverter::Convert(const nlohmann::json& json)
{
  if(!json.is_string())
  {
    return MakeErrorResult<DataContainerCreationFilterParameterConverter::ValueType>(-2, fmt::format("DataContainerCreationFilterParameter value '{}' is not a string", json.dump()));
  }

  auto dataContainerName = json.get<std::string>();
  DataPath dataPath({dataContainerName});
  return {std::move(dataPath)};
}

Result<StringToDataPathFilterParameterConverter::ValueType> StringToDataPathFilterParameterConverter::Convert(const nlohmann::json& json)
{
  if(!json.is_string())
  {
    return MakeErrorResult<ValueType>(-1, fmt::format("StringToDataPathFilterParameterConverter json '{}' is not a string", json.dump()));
  }

  DataPath dataPath({json.get<std::string>()});

  return {std::move(dataPath)};
}

Result<StringsToDataPathFilterParameterConverter::ValueType> StringsToDataPathFilterParameterConverter::Convert(const nlohmann::json& json1, const nlohmann::json& json2)
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

Result<AMPathBuilderFilterParameterConverter::ValueType> AMPathBuilderFilterParameterConverter::Convert(const nlohmann::json& json1, const nlohmann::json& json2)
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

Result<DAPathBuilderFilterParameterConverter::ValueType> DAPathBuilderFilterParameterConverter::Convert(const nlohmann::json& json1, const nlohmann::json& json2, const nlohmann::json& json3)
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

Result<DCPathBuilderFilterParameterConverter::ValueType> DCPathBuilderFilterParameterConverter::Convert(const nlohmann::json& json)
{
  std::string dcName;

  if(json.is_string())
  {
    dcName = json.get<std::string>();
  }
  else
  {
    auto dataContainerNameResult = ReadDataContainerName(json, "DCPathBuilderFilterParameterConverter");
    if(dataContainerNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(dataContainerNameResult));
    }
    dcName = std::move(dataContainerNameResult.value());
  }

  DataPath dataPath({dcName});

  return {std::move(dataPath)};
}

Result<AttributeMatrixCreationFilterParameterConverter::ValueType> AttributeMatrixCreationFilterParameterConverter::Convert(const nlohmann::json& json)
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
} // namespace simpl_conversion
} // namespace nx::core
