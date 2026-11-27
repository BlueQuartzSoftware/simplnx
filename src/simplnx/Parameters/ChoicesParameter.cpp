#include "ChoicesParameter.hpp"

#include "simplnx/Common/StringLiteralFormatting.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/core.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <utility>

template <>
struct fmt::formatter<nx::core::Error>
{
  static constexpr format_parse_context::iterator parse(format_parse_context& ctx)
  {
    return ctx.begin();
  }

  static format_context::iterator format(const nx::core::Error& error, format_context& ctx)
  {
    return fmt::format_to(ctx.out(), "Error({}, {})", error.code, error.message);
  }
};

namespace nx::core
{
ChoicesParameter::ChoicesParameter(const std::string& name, const std::string& humanName, const std::string& helpText, ValueType defaultValue, ChoicesType choices)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
, m_Choices(std::move(choices))
{
}

Uuid ChoicesParameter::uuid() const
{
  return ParameterTraits<ChoicesParameter>::uuid;
}

IParameter::AcceptedTypesType ChoicesParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

//------------------------------------------------------------------------------
IParameter::VersionType ChoicesParameter::getVersion() const
{
  return 1;
}

nlohmann::json ChoicesParameter::toJsonImpl(const std::any& value) const
{
  auto index = std::any_cast<ValueType>(value);
  nlohmann::json json = index;
  return json;
}

Result<std::any> ChoicesParameter::fromJsonImpl(const nlohmann::json& json, VersionType /*version*/) const
{
  static constexpr StringLiteralType k_Prefix = "FilterParameter 'ChoicesParameter' JSON Error: ";
  if(!json.is_number())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("{}JSON value for key '{}' is not a number", k_Prefix, name()));
  }
  auto index = json.get<ValueType>();
  return {{index}};
}

IParameter::UniquePointerType ChoicesParameter::clone() const
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
  const std::string prefix = fmt::format("Parameter Name: '{}'\n    Parameter Key: '{}'\n    Validation Error: ", humanName(), name());

  if(index >= m_Choices.size())
  {
    return nx::core::MakeErrorResult(nx::core::filter_parameter::constants::k_ValidateOutOfRangeError,
                                     fmt::format("{}Value '{}' must be less than {}. Valid choices are: [{}]", prefix, index, m_Choices.size(), fmt::join(m_Choices, ", ")));
  }

  return {};
}

ChoicesParameter::ChoicesType ChoicesParameter::choices() const
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

namespace simpl_conversion
{
Result<LinkedChoicesFilterParameterConverter::ValueType> LinkedChoicesFilterParameterConverter::Convert(const nlohmann::json& json)
{
  return ConvertChoicesParameter(json, "LinkedChoicesFilterParameter");
}

Result<ChoiceFilterParameterConverter::ValueType> ChoiceFilterParameterConverter::Convert(const nlohmann::json& json)
{
  return ConvertChoicesParameter(json, "ChoiceFilterParameter");
}
} // namespace simpl_conversion
} // namespace nx::core
