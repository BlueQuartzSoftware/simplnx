#include "DataGroupSelectionParameter.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Common/StringLiteralFormatting.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/core.h>
#include <fmt/ranges.h>
#include <nlohmann/json.hpp>
#include <utility>

namespace nx::core
{
DataGroupSelectionParameter::DataGroupSelectionParameter(const std::string& name, const std::string& humanName, const std::string& helpText, ValueType defaultValue, AllowedTypeSetType allowedTypes)
: MutableDataParameter(name, humanName, helpText, Category::Required)
, m_DefaultValue(std::move(defaultValue))
, m_AllowedTypes(std::move(allowedTypes))
{
}

Uuid DataGroupSelectionParameter::uuid() const
{
  return ParameterTraits<DataGroupSelectionParameter>::uuid;
}

IParameter::AcceptedTypesType DataGroupSelectionParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

//------------------------------------------------------------------------------
IParameter::VersionType DataGroupSelectionParameter::getVersion() const
{
  return 1;
}

nlohmann::json DataGroupSelectionParameter::toJsonImpl(const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);
  nlohmann::json json = path.toString();
  return json;
}

Result<std::any> DataGroupSelectionParameter::fromJsonImpl(const nlohmann::json& json, VersionType /*version*/) const
{
  static constexpr StringLiteralType k_Prefix = "FilterParameter 'DataGroupSelectionParameter' JSON Error: ";

  if(!json.is_string())
  {
    return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonValueNotString, fmt::format("{}The JSON data entry for key '{}' is not a string.", k_Prefix.view(), name()));
  }

  auto valueString = json.get<std::string>();
  auto path = DataPath::FromString(valueString);
  if(!path.has_value())
  {
    return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonValueNotValueType, fmt::format("{}Failed to parse '{}' as DataPath for key '{}'", k_Prefix, valueString, name()));
  }
  return {{std::move(*path)}};
}

IParameter::UniquePointerType DataGroupSelectionParameter::clone() const
{
  return std::make_unique<DataGroupSelectionParameter>(name(), humanName(), helpText(), m_DefaultValue, m_AllowedTypes);
}

std::any DataGroupSelectionParameter::defaultValue() const
{
  return defaultPath();
}

const DataGroupSelectionParameter::AllowedTypeSetType& DataGroupSelectionParameter::allowedTypes() const
{
  return m_AllowedTypes;
}

typename DataGroupSelectionParameter::ValueType DataGroupSelectionParameter::defaultPath() const
{
  return m_DefaultValue;
}

Result<> DataGroupSelectionParameter::validate(const DataStructure& dataStructure, const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);

  return validatePath(dataStructure, path);
}

Result<> DataGroupSelectionParameter::validatePath(const DataStructure& dataStructure, const DataPath& value) const
{
  const std::string prefix = fmt::format("Parameter Name: '{}'\n    Parameter Key: '{}'\n    Validation Error: ", humanName(), name());

  if(value.empty())
  {
    return MakeErrorResult(filter_parameter::constants::k_ValidateEmptyValue, fmt::format("{}DataPath cannot be empty", prefix));
  }

  const DataObject* dataObjectPtr = dataStructure.getData(value);
  if(dataObjectPtr == nullptr)
  {
    return MakeErrorResult(filter_parameter::constants::k_ValidateDuplicateValue, fmt::format("{}Object does not exist at path '{}'", prefix, value.toString()));
  }

  const auto* const baseGroupObjPtr = dataStructure.getDataAs<BaseGroup>(value);
  if(baseGroupObjPtr == nullptr)
  {
    return MakeErrorResult(filter_parameter::constants::k_ValidateDuplicateValue, fmt::format("{}Object at path '{}' is not a BaseGroup type", prefix, value.toString()));
  }

  // Look for the actual group type that the user selected in the allowed set
  if(m_AllowedTypes.contains(baseGroupObjPtr->getGroupType()))
  {
    return {};
  }

  return MakeErrorResult(filter_parameter::constants::k_ValidateAllowedTypeError, fmt::format("{}Group at path '{}' was of type '{}', but only {} are allowed", prefix, value.toString(),
                                                                                              dataObjectPtr->getTypeName(), BaseGroup::StringListFromGroupType(m_AllowedTypes)));
}

Result<std::any> DataGroupSelectionParameter::resolve(DataStructure& dataStructure, const std::any& value) const
{
  const auto& path = GetAnyRef<ValueType>(value);
  DataObject* objectPtr = dataStructure.getData(path);
  return {{objectPtr}};
}

namespace simpl_conversion
{
Result<DataContainerSelectionFilterParameterConverter::ValueType> DataContainerSelectionFilterParameterConverter::Convert(const nlohmann::json& json)
{
  // 6.5 stored as a string
  if(json.is_string())
  {
    return {DataPath({json.get<std::string>()})};
  }

  auto dataContainerNameResult = ReadDataContainerName(json, "DataContainerSelectionFilterParameter");
  if(dataContainerNameResult.invalid())
  {
    return ConvertInvalidResult<ValueType>(std::move(dataContainerNameResult));
  }

  DataPath dataPath({std::move(dataContainerNameResult.value())});

  return {std::move(dataPath)};
}

Result<DataContainerFromMultiSelectionFilterParameterConverter::ValueType> DataContainerFromMultiSelectionFilterParameterConverter::Convert(const nlohmann::json& json)
{
  if(!json.is_array())
  {
    return MakeErrorResult<ValueType>(-1, fmt::format("DataContainerFromMultiSelectionFilterParameterConverter json '{}' is not an array", json.dump()));
  }

  for(const auto& iter : json)
  {
    auto dataContainerNameResult = ReadDataContainerName(iter, "DataContainerSelectionFilterParameter");
    if(dataContainerNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(dataContainerNameResult));
    }
    DataPath value({std::move(dataContainerNameResult.value())});

    return {std::move(value)};
  }

  return {DataPath()};
}
} // namespace simpl_conversion
} // namespace nx::core
