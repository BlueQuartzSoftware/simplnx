#include "SIMPLConversion.hpp"

namespace nx::core::SIMPLConversion
{
/*
"Attribute Matrix Name": "AttributeMatrix",
"Data Array Name": "DataArray",
"Data Container Name": "DataContainer"
*/

namespace
{
constexpr StringLiteral k_DataContainerNameKey = "Data Container Name";
constexpr StringLiteral k_AttributeMatrixNameKey = "Attribute Matrix Name";
constexpr StringLiteral k_DataArrayNameKey = "Data Array Name";
constexpr StringLiteral k_LinkedDataArrayNameKey = "OutputArrayName";
} // namespace

//------------------------------------------------------------------------------
Result<std::string> ReadDataContainerName(const nlohmann::json& json, std::string_view parameterName)
{
  if(!json.contains(k_DataContainerNameKey))
  {
    return MakeErrorResult<std::string>(-1, fmt::format("{} json '{}' does not contain '{}'", parameterName, json.dump(), k_DataContainerNameKey));
  }

  const auto& dataContainerNameJson = json[k_DataContainerNameKey];

  if(!dataContainerNameJson.is_string())
  {
    return MakeErrorResult<std::string>(-2, fmt::format("{} '{}' value '{}' is not a string", parameterName, k_DataContainerNameKey, dataContainerNameJson.dump()));
  }

  auto dataContainerName = dataContainerNameJson.get<std::string>();

  return {std::move(dataContainerName)};
}

//------------------------------------------------------------------------------
Result<std::string> ReadAttributeMatrixName(const nlohmann::json& json, std::string_view parameterName)
{
  if(!json.contains(k_AttributeMatrixNameKey))
  {
    return MakeErrorResult<std::string>(-3, fmt::format("{} json '{}' does not contain '{}'", parameterName, json.dump(), k_AttributeMatrixNameKey));
  }

  const auto& attributeMatrixNameJson = json[k_AttributeMatrixNameKey];

  if(!attributeMatrixNameJson.is_string())
  {
    return MakeErrorResult<std::string>(-4, fmt::format("{} '{}' value '{}' is not a string", parameterName, k_AttributeMatrixNameKey, attributeMatrixNameJson.dump()));
  }

  auto attributeMatrixName = attributeMatrixNameJson.get<std::string>();

  return {std::move(attributeMatrixName)};
}

//------------------------------------------------------------------------------
Result<std::string> ReadDataArrayName(const nlohmann::json& json, std::string_view parameterName)
{
  if(!json.contains(k_DataArrayNameKey))
  {
    return MakeErrorResult<std::string>(-5, fmt::format("{} json '{}' does not contain '{}'", parameterName, json.dump(), k_DataArrayNameKey));
  }

  const auto& dataArrayNameJson = json[k_DataArrayNameKey];

  if(!dataArrayNameJson.is_string())
  {
    return MakeErrorResult<std::string>(-6, fmt::format("{} '{}' value '{}' is not a string", parameterName, k_DataArrayNameKey, dataArrayNameJson.dump()));
  }

  auto dataArrayName = dataArrayNameJson.get<std::string>();

  return {std::move(dataArrayName)};
}

//------------------------------------------------------------------------------
Result<ChoicesParameter::ValueType> ConvertChoicesParameter(const nlohmann::json& json, std::string_view parameterName)
{
  if(!json.is_number_integer())
  {
    return MakeErrorResult<ChoicesParameter::ValueType>(-1, fmt::format("{} json '{}' is not an integer", parameterName, json.dump()));
  }

  auto value = json.get<int32>();

  if(value < 0)
  {
    return MakeErrorResult<ChoicesParameter::ValueType>(-1, fmt::format("{} value '{}' is negative", parameterName, value));
  }

  return {static_cast<ChoicesParameter::ValueType>(value)};
}

//------------------------------------------------------------------------------
Result<FileSystemPathParameter::ValueType> ReadInputFilePath(const nlohmann::json& json, std::string_view parameterName)
{
  using ParameterType = FileSystemPathParameter;

  if(!json.is_string())
  {
    return MakeErrorResult<ParameterType::ValueType>(-1, fmt::format("{} json '{}' is not a string", parameterName, json.dump()));
  }

  auto value = json.get<std::string>();

  return {ParameterType::ValueType(value)};
}
} // namespace nx::core::SIMPLConversion
