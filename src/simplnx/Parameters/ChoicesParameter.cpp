#include "ChoicesParameter.hpp"

#include <fmt/core.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

template <>
struct fmt::formatter<nx::core::Error>
{
  constexpr format_parse_context::iterator parse(format_parse_context& ctx)
  {
    return ctx.begin();
  }

  format_context::iterator format(const nx::core::Error& error, format_context& ctx) const
  {
    return format_to(ctx.out(), "Error({}, {})", error.code, error.message);
  }
};

namespace nx::core
{
ChoicesParameter::ChoicesParameter(const std::string& name, const std::string& humanName, const std::string& helpText, ValueType defaultValue, const Choices& choices)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
, m_Choices(choices)
{
}

Uuid ChoicesParameter::uuid() const
{
  return ParameterTraits<ChoicesParameter>::uuid;
}

IParameter::AcceptedTypes ChoicesParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

nlohmann::json ChoicesParameter::toJson(const std::any& value) const
{
  auto index = std::any_cast<ValueType>(value);
  nlohmann::json json = index;
  return json;
}

Result<std::any> ChoicesParameter::fromJson(const nlohmann::json& json) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'ChoicesParameter' JSON Error: ";
  if(!json.is_number())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("{}JSON value for key '{}' is not a number", prefix, name()));
  }
  auto index = json.get<ValueType>();
  return {{index}};
}

IParameter::UniquePointer ChoicesParameter::clone() const
{
  return std::make_unique<ChoicesParameter>(name(), humanName(), helpText(), m_DefaultValue, m_Choices);
}

std::any ChoicesParameter::defaultValue() const
{
  return defaultIndex();
}

typename ChoicesParameter::ValueType ChoicesParameter::defaultIndex() const
{
  return m_DefaultValue;
}

Result<> ChoicesParameter::validate(const std::any& value) const
{
  auto index = std::any_cast<ValueType>(value);
  return validateIndex(index);
}

Result<> ChoicesParameter::validateIndex(ValueType index) const
{
  const std::string prefix = fmt::format("FilterParameter '{}' Validation Error: ", humanName());

  if(index >= m_Choices.size())
  {
    return nx::core::MakeErrorResult(nx::core::FilterParameter::Constants::k_Validate_OutOfRange_Error, fmt::format("{}Value '{}' must be less than {}", prefix, index, m_Choices.size()));
  }

  return {};
}

ChoicesParameter::Choices ChoicesParameter::choices() const
{
  return m_Choices;
}

bool ChoicesParameter::checkActive(const std::any& parameterValue, const std::any& associatedValue) const
{
  auto value = std::any_cast<ValueType>(parameterValue);
  Result<> result = validateIndex(value);
  if(result.invalid())
  {
    throw std::runtime_error(fmt::format("Parameter value is not valid: {}", fmt::join(result.errors(), " | ")));
  }

  auto index = std::any_cast<ValueType>(associatedValue);
  return value == index;
}
} // namespace nx::core
