#include "CalculatorParameter.hpp"

#include <fmt/core.h>

#include <nlohmann/json.hpp>

#include "complex/Common/Any.hpp"
#include "complex/Common/TypeTraits.hpp"
#include "complex/Utilities/StringUtilities.hpp"

namespace complex
{

namespace
{
constexpr StringLiteral k_Equation = "equation";
constexpr StringLiteral k_Units = "units";
} // namespace

CalculatorParameter::CalculatorParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: ValueParameter(name, humanName, helpText)
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
  return json;
}

Result<std::any> CalculatorParameter::fromJson(const nlohmann::json& json) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'CalculatorParameter' JSON Error: ";
  if(!json.is_object())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("{}JSON value for key '{}' is not a object", prefix, name()));
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

Result<> CalculatorParameter::validate(const std::any& value) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'CalculatorParameter' JSON Error: ";
  [[maybe_unused]] const auto& stringValue = GetAnyRef<ValueType>(value);
  if(StringUtilities::trimmed(stringValue.m_Equation).empty())
  {
    return MakeErrorResult(FilterParameter::Constants::k_Validate_Empty_Value, fmt::format("{}expression cannot be empty", prefix));
  }
  return {};
}
} // namespace complex
