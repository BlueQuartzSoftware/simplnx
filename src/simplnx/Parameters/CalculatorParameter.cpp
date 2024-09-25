#include "CalculatorParameter.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

namespace nx::core
{

namespace
{
constexpr StringLiteral k_Equation = "equation";
constexpr StringLiteral k_Units = "units";
constexpr StringLiteral k_SelectedGroup = "selected_group";
} // namespace

CalculatorParameter::CalculatorParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: MutableDataParameter(name, humanName, helpText, Category::Required)
, m_DefaultValue(defaultValue)
{
}

Uuid CalculatorParameter::uuid() const
{
  return ParameterTraits<CalculatorParameter>::uuid;
}

IParameter::AcceptedTypes CalculatorParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

nlohmann::json CalculatorParameter::toJson(const std::any& value) const
{
  const auto& structValue = GetAnyRef<ValueType>(value);
  nlohmann::json json;
  json[k_Equation] = structValue.m_Equation;
  json[k_Units] = structValue.m_Units;
  json[k_SelectedGroup] = structValue.m_SelectedGroup.toString();
  return json;
}

Result<std::any> CalculatorParameter::fromJson(const nlohmann::json& json) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'CalculatorParameter' JSON Error: ";
  if(!json.is_object())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("{}JSON value for key '{}' is not a object", prefix, name()));
  }
  if(!json.contains(k_SelectedGroup))
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Missing_Entry,
                                     fmt::format("{}The JSON data does not contain an entry with a key of '{}'", prefix.view(), k_SelectedGroup.view()));
  }
  if(!json.contains(k_Equation))
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Missing_Entry, fmt::format("{}The JSON data does not contain an entry with a key of '{}'", prefix.view(), k_Equation.view()));
  }
  if(!json.contains(k_Units))
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Missing_Entry, fmt::format("{}The JSON data does not contain an entry with a key of '{}'", prefix.view(), k_Units.view()));
  }
  auto units = json[k_Units].get<std::underlying_type_t<AngleUnits>>();
  if(units != to_underlying(AngleUnits::Radians) && units != to_underlying(AngleUnits::Degrees))
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_Enumeration, fmt::format("{}JSON value for key '{}' was not a valid ordering Value. [{}|{}] allowed.", prefix.view(),
                                                                                                           k_Units.view(), to_underlying(AngleUnits::Radians), to_underlying(AngleUnits::Degrees)));
  }
  ValueType value;
  value.m_Equation = json[k_Equation].get<std::string>();
  value.m_Units = static_cast<AngleUnits>(units);
  auto selectedGroupString = json[k_SelectedGroup].get<std::string>();
  auto selectedGroupPath = DataPath::FromString(selectedGroupString);
  if(!selectedGroupPath.has_value())
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_Value_Type,
                                     fmt::format("{}Failed to parse '{}' as DataPath for key '{}'", prefix, selectedGroupString, k_SelectedGroup));
  }
  value.m_SelectedGroup = selectedGroupPath.value();
  return {{std::move(value)}};
}

IParameter::UniquePointer CalculatorParameter::clone() const
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

Result<> CalculatorParameter::validate(const DataStructure& dataStructure, const std::any& value) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'CalculatorParameter' JSON Error: ";
  [[maybe_unused]] const auto& structValue = GetAnyRef<ValueType>(value);
  if(StringUtilities::trimmed(structValue.m_Equation).empty())
  {
    return MakeErrorResult(FilterParameter::Constants::k_Validate_Empty_Value, fmt::format("{}expression cannot be empty", prefix));
  }
  if(!structValue.m_SelectedGroup.empty()) // if empty then using root group
  {
    const DataObject* dataObject = dataStructure.getData(structValue.m_SelectedGroup);
    if(dataObject == nullptr)
    {
      return nx::core::MakeErrorResult(nx::core::FilterParameter::Constants::k_Validate_DuplicateValue,
                                       fmt::format("{}Object does not exist at path '{}'", prefix, structValue.m_SelectedGroup.toString()));
    }
    const auto baseGroupObj = dataStructure.getDataAs<BaseGroup>(structValue.m_SelectedGroup);
    if(baseGroupObj == nullptr)
    {
      return MakeErrorResult(FilterParameter::Constants::k_Validate_DuplicateValue, fmt::format("{}Object at path '{}' is not a BaseGroup type", prefix, structValue.m_SelectedGroup.toString()));
    }
  }

  return {};
}

Result<std::any> CalculatorParameter::resolve(DataStructure& dataStructure, const std::any& value) const
{
  const auto& structValue = GetAnyRef<ValueType>(value);
  DataObject* object = dataStructure.getData(structValue.m_SelectedGroup);
  return {{object}};
}

namespace SIMPLConversion
{
namespace
{
constexpr StringLiteral k_InfixEquationKey = "InfixEquation";
constexpr StringLiteral k_UnitsKey = "Units";
constexpr StringLiteral k_SelectedAttributeMatrixKey = "SelectedAttributeMatrix";
} // namespace

Result<CalculatorFilterParameterConverter::ValueType> CalculatorFilterParameterConverter::convert(const nlohmann::json& json)
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
  value.m_Equation = json[k_InfixEquationKey].get<std::string>();
  value.m_Units = static_cast<ParameterType::AngleUnits>(json[k_UnitsKey].get<int32>());
  value.m_SelectedGroup = dataPath;

  return {std::move(value)};
}
} // namespace SIMPLConversion
} // namespace nx::core
