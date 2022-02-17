#include "CalculatorParameter.hpp"

#include "complex/DataStructure/DataGroup.hpp"

#include <fmt/core.h>

#include <nlohmann/json.hpp>

#include "complex/Common/Any.hpp"

namespace complex
{
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
  const auto& infix_expression = GetAnyRef<ValueType>(value);
  nlohmann::json json = infix_expression;
  return json;
}

Result<std::any> CalculatorParameter::fromJson(const nlohmann::json& json) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'CalculatorParameter' JSON Error: ";
  if(!json.is_string())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("{}JSON value for key '{}' is not a string", prefix, name()));
  }
  auto infix_expression = json.get<ValueType>();
  return {{infix_expression}};
}

IParameter::UniquePointer CalculatorParameter::clone() const
{
  return std::make_unique<CalculatorParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

std::any CalculatorParameter::defaultValue() const
{
  return m_DefaultValue;
}

Result<> CalculatorParameter::validate(const std::any& value) const
{
  if(value.type() == typeid(std::string))
  {
    return {};
  }

  return {nonstd::make_unexpected(std::vector<Error>{Error{-667, "Bad parameter type"}})};
}
} // namespace complex
