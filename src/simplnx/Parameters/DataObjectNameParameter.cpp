#include "DataObjectNameParameter.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Common/StringLiteralFormatting.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

namespace nx::core
{
DataObjectNameParameter::DataObjectNameParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
{
}

Uuid DataObjectNameParameter::uuid() const
{
  return ParameterTraits<DataObjectNameParameter>::uuid;
}

IParameter::AcceptedTypes DataObjectNameParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

//------------------------------------------------------------------------------
IParameter::VersionType DataObjectNameParameter::getVersion() const
{
  return 1;
}

nlohmann::json DataObjectNameParameter::toJsonImpl(const std::any& value) const
{
  const auto& stringValue = GetAnyRef<ValueType>(value);
  nlohmann::json json = stringValue;
  return json;
}

Result<std::any> DataObjectNameParameter::fromJsonImpl(const nlohmann::json& json, VersionType version) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'DataObjectNameParameter' JSON Error: ";
  if(!json.is_string())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("{}JSON value for key '{}' is not a string", prefix, name()));
  }
  auto string = json.get<ValueType>();
  return {{string}};
}

IParameter::UniquePointer DataObjectNameParameter::clone() const
{
  return std::make_unique<DataObjectNameParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

std::any DataObjectNameParameter::defaultValue() const
{
  return defaultName();
}

typename DataObjectNameParameter::ValueType DataObjectNameParameter::defaultName() const
{
  return m_DefaultValue;
}

Result<> DataObjectNameParameter::validate(const std::any& value) const
{
  const auto& stringValue = GetAnyRef<ValueType>(value);
  return validateName(stringValue);
}

Result<> DataObjectNameParameter::validateName(const std::string& value) const
{
  const std::string prefix = fmt::format("Parameter Name: '{}'\n    Parameter Key: '{}'\n    Validation Error: ", humanName(), name());

  if(value.empty())
  {
    return nx::core::MakeErrorResult(nx::core::FilterParameter::Constants::k_Validate_Empty_Value, fmt::format("{}DataObjectName cannot be empty", prefix));
  }

  if(!DataObject::IsValidName(value))
  {
    return MakeErrorResult(nx::core::FilterParameter::Constants::k_Validate_InvalidDataObjectName, fmt::format("{}'{}' is not a valid DataObject name", prefix, value));
  }
  return {};
}

namespace SIMPLConversion
{
Result<LinkedPathCreationFilterParameterConverter::ValueType> LinkedPathCreationFilterParameterConverter::convert(const nlohmann::json& json)
{
  if(!json.is_string())
  {
    return MakeErrorResult<ValueType>(-1, fmt::format("LinkedPathCreationFilterParameter json '{}' is not a string", json.dump()));
  }

  auto value = json.get<std::string>();

  return {std::move(value)};
}

Result<DataArrayCreationToDataObjectNameFilterParameterConverter::ValueType> DataArrayCreationToDataObjectNameFilterParameterConverter::convert(const nlohmann::json& json)
{
  auto dataArrayNameResult = ReadDataArrayName(json, "DataArrayCreationFilterParameter");
  if(dataArrayNameResult.invalid())
  {
    return ConvertInvalidResult<ValueType>(std::move(dataArrayNameResult));
  }

  return {std::move(dataArrayNameResult.value())};
}

Result<DataContainerNameFilterParameterConverter::ValueType> DataContainerNameFilterParameterConverter::convert(const nlohmann::json& json)
{
  auto dataContainerNameResult = ReadDataContainerName(json, "DataContainerNameFilterParameterConverter");
  if(dataContainerNameResult.invalid())
  {
    return ConvertInvalidResult<ValueType>(std::move(dataContainerNameResult));
  }

  return {std::move(dataContainerNameResult.value())};
}

Result<AttributeMatrixNameFilterParameterConverter::ValueType> AttributeMatrixNameFilterParameterConverter::convert(const nlohmann::json& json)
{
  auto nameResult = ReadAttributeMatrixName(json, "AttributeMatrixNameFilterParameterConverter");
  if(nameResult.invalid())
  {
    return ConvertInvalidResult<ValueType>(std::move(nameResult));
  }

  return {std::move(nameResult.value())};
}

Result<DataArrayNameFilterParameterConverter::ValueType> DataArrayNameFilterParameterConverter::convert(const nlohmann::json& json)
{
  auto nameResult = ReadDataArrayName(json, "DataArrayNameFilterParameterConverter");
  if(nameResult.invalid())
  {
    return ConvertInvalidResult<ValueType>(std::move(nameResult));
  }

  return {std::move(nameResult.value())};
}
} // namespace SIMPLConversion
} // namespace nx::core
