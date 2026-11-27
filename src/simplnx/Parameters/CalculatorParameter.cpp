#include "CalculatorParameter.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Common/StringLiteralFormatting.hpp"
#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include <utility>

namespace nx::core
{

namespace
{
constexpr StringLiteralType k_Equation = "equation";
constexpr StringLiteralType k_Units = "units";
constexpr StringLiteralType k_SelectedGroup = "selected_group";
} // namespace

CalculatorParameter::CalculatorParameter(const std::string& name, const std::string& humanName, const std::string& helpText, ValueType defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(std::move(defaultValue))
{
}

Uuid CalculatorParameter::uuid() const
{
  return ParameterTraits<CalculatorParameter>::uuid;
}

IParameter::AcceptedTypesType CalculatorParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

//------------------------------------------------------------------------------
IParameter::VersionType CalculatorParameter::getVersion() const
{
  return 1;
}

nlohmann::json CalculatorParameter::toJsonImpl(const std::any& value) const
{
  const auto& structValue = GetAnyRef<ValueType>(value);
  nlohmann::json json;
  json[k_Equation] = structValue.equation;
  json[k_Units] = structValue.m_units;
  json[k_SelectedGroup] = structValue.m_selected_group.toString();
  return json;
}

Result<std::any> CalculatorParameter::fromJsonImpl(const nlohmann::json& json, VersionType /*version*/) const
{
  static constexpr StringLiteralType k_Prefix = "FilterParameter 'CalculatorParameter' JSON Error: ";
  if(!json.is_object())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("{}JSON value for key '{}' is not a object", k_Prefix, name()));
  }
  if(!json.contains(k_SelectedGroup))
  {
    return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonMissingEntry,
                                     fmt::format("{}The JSON data does not contain an entry with a key of '{}'", k_Prefix.view(), k_SelectedGroup.view()));
  }
  if(!json.contains(k_Equation))
  {
    return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonMissingEntry, fmt::format("{}The JSON data does not contain an entry with a key of '{}'", k_Prefix.view(), k_Equation.view()));
  }
  if(!json.contains(k_Units))
  {
    return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonMissingEntry, fmt::format("{}The JSON data does not contain an entry with a key of '{}'", k_Prefix.view(), k_Units.view()));
  }
  auto units = json[k_Units].get<std::underlying_type_t<AngleUnits>>();
  if(units != to_underlying(AngleUnits::Radians) && units != to_underlying(AngleUnits::Degrees))
  {
    return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonValueNotEnumeration, fmt::format("{}JSON value for key '{}' was not a valid ordering Value. [{}|{}] allowed.", k_Prefix.view(),
                                                                                                         k_Units.view(), to_underlying(AngleUnits::Radians), to_underlying(AngleUnits::Degrees)));
  }
  ValueType value;
  value.equation = json[k_Equation].get<std::string>();
  value.m_units = static_cast<AngleUnits>(units);
  auto selectedGroupString = json[k_SelectedGroup].get<std::string>();
  auto selectedGroupPath = DataPath::FromString(selectedGroupString);
  if(!selectedGroupPath.has_value())
  {
    return MakeErrorResult<std::any>(filter_parameter::constants::k_JsonValueNotValueType,
                                     fmt::format("{}Failed to parse '{}' as DataPath for key '{}'", k_Prefix, selectedGroupString, k_SelectedGroup));
  }
  value.m_selected_group = selectedGroupPath.value();
  return {{std::move(value)}};
}

IParameter::UniquePointerType CalculatorParameter::clone() const
{
  return std::make_unique<CalculatorParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

std::any CalculatorParameter::defaultValue() const
{
  return defaultString();
}

typename CalculatorParameter::ValueType CalculatorParameter::defaultString() const
{
  return m_DefaultValue;
}

Result<> CalculatorParameter::validate(const std::any& value) const
{
  static constexpr StringLiteralType k_Prefix = "FilterParameter 'CalculatorParameter' JSON Error: ";
  [[maybe_unused]] const auto& structValue = GetAnyRef<ValueType>(value);
  if(string_utilities::Trimmed(structValue.equation).empty())
  {
    return MakeErrorResult(filter_parameter::constants::k_ValidateEmptyValue, fmt::format("{}expression cannot be empty", k_Prefix));
  }

  // m_SelectedGroup is only a resolution hint for array name lookups.
  // An empty path means "use root", and a non-existent or non-BaseGroup
  // path is silently accepted -- the parser will handle any issues.

  return {};
}

namespace simpl_conversion
{
namespace
{
constexpr StringLiteralType k_InfixEquationKey = "InfixEquation";
constexpr StringLiteralType k_UnitsKey = "Units";
constexpr StringLiteralType k_SelectedAttributeMatrixKey = "SelectedAttributeMatrix";
} // namespace

Result<CalculatorFilterParameterConverter::ValueType> CalculatorFilterParameterConverter::Convert(const nlohmann::json& json)
{
  if(!json.contains(k_InfixEquationKey))
  {
    return MakeErrorResult<ValueType>(-2, fmt::format("CalculatorParameter json '{}' does not contain '{}'", json.dump(), k_InfixEquationKey));
  }
  if(!json.contains(k_UnitsKey))
  {
    return MakeErrorResult<ValueType>(-2, fmt::format("CalculatorParameter json '{}' does not contain '{}'", json.dump(), k_UnitsKey));
  }
  if(!json.contains(k_SelectedAttributeMatrixKey))
  {
    return MakeErrorResult<ValueType>(-2, fmt::format("CalculatorParameter json '{}' does not contain '{}'", json.dump(), k_SelectedAttributeMatrixKey));
  }

  if(!json[k_InfixEquationKey].is_string())
  {
    return MakeErrorResult<ValueType>(-2, fmt::format("CalculatorParameter json[InfixEquation] '{}' is not a string", json[k_InfixEquationKey].dump()));
  }
  if(!json[k_UnitsKey].is_number_integer())
  {
    return MakeErrorResult<ValueType>(-1, fmt::format("CalculatorParameter json[ScalarType] '{}' is not an integer", json[k_UnitsKey].dump()));
  }
  if(!json[k_SelectedAttributeMatrixKey].is_object())
  {
    return MakeErrorResult<ValueType>(-1, fmt::format("CalculatorParameter json[SelectedAttributeMatrix] '{}' is not an object", json[k_SelectedAttributeMatrixKey].dump()));
  }

  const auto& amJson = json[k_SelectedAttributeMatrixKey];
  auto dataContainerNameResult = ReadDataContainerName(amJson, "AttributeMatrixCreationFilterParameter");
  if(dataContainerNameResult.invalid())
  {
    return ConvertInvalidResult<ValueType>(std::move(dataContainerNameResult));
  }

  auto attributeMatrixNameResult = ReadAttributeMatrixName(amJson, "AttributeMatrixCreationFilterParameter");
  if(attributeMatrixNameResult.invalid())
  {
    return ConvertInvalidResult<ValueType>(std::move(attributeMatrixNameResult));
  }

  DataPath dataPath({std::move(dataContainerNameResult.value()), std::move(attributeMatrixNameResult.value())});

  ValueType value;
  value.equation = json[k_InfixEquationKey].get<std::string>();
  value.m_units = static_cast<ParameterType::AngleUnits>(json[k_UnitsKey].get<int32>());
  value.m_selected_group = dataPath;

  return {std::move(value)};
}
} // namespace simpl_conversion
} // namespace nx::core
