#include "StringParameter.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Common/StringLiteralFormatting.hpp"
#include "simplnx/Common/Types.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

namespace nx::core
{
StringParameter::StringParameter(const std::string& name, const std::string& humanName, const std::string& helpText, ValueType  defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(std::move(defaultValue))
{
}

Uuid StringParameter::uuid() const
{
  return ParameterTraits<StringParameter>::uuid;
}

IParameter::AcceptedTypes StringParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

//------------------------------------------------------------------------------
IParameter::VersionType StringParameter::getVersion() const
{
  return 1;
}

nlohmann::json StringParameter::toJsonImpl(const std::any& value) const
{
  const auto& stringValue = GetAnyRef<ValueType>(value);
  nlohmann::json json = stringValue;
  return json;
}

Result<std::any> StringParameter::fromJsonImpl(const nlohmann::json& json, VersionType /*version*/) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'StringParameter' JSON Error: ";
  if(!json.is_string())
  {
    return MakeErrorResult<std::any>(-2, fmt::format("{}JSON value for key '{}' is not a string", prefix, name()));
  }
  auto string = json.get<ValueType>();
  return {{string}};
}

IParameter::UniquePointer StringParameter::clone() const
{
  return std::make_unique<StringParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

std::any StringParameter::defaultValue() const
{
  return defaultString();
}

typename StringParameter::ValueType StringParameter::defaultString() const
{
  return m_DefaultValue;
}

Result<> StringParameter::validate(const std::any& value) const
{
  [[maybe_unused]] const auto& stringValue = GetAnyRef<ValueType>(value);
  return {};
}

namespace SIMPLConversion
{
namespace
{
std::optional<int32> float32MidpointPrecision(float64 value)
{
  if(!std::isfinite(value) || value == 0.0)
  {
    return std::nullopt;
  }

  using Float32LimitsType = std::numeric_limits<float32>;
  const int32 exponent = std::ilogb(value);
  if(exponent >= Float32LimitsType::max_exponent)
  {
    return std::nullopt;
  }

  // A midpoint lies an odd number of half steps from zero. Subnormals use the smallest normal spacing.
  const int32 halfStepPower = Float32LimitsType::digits - std::max(exponent, Float32LimitsType::min_exponent - 1);
  const float64 halfSteps = std::ldexp(std::abs(value), halfStepPower);
  if(std::fmod(halfSteps, 2.0) == 1.0)
  {
    // A binary denominator of 2^n needs at most n fixed decimal places for an exact value.
    return std::max(0, halfStepPower);
  }
  return std::nullopt;
}
} // namespace

template <typename T>
Result<typename NumberToStringFilterParameterConverter<T>::ValueType> NumberToStringFilterParameterConverter<T>::convert(const nlohmann::json& json)
{
  if(!json.is_number())
  {
    return MakeErrorResult<ValueType>(-1, fmt::format("NumberToStringFilterParameterConverter json '{}' is not a number", json.dump()));
  }

  auto value = json.get<T>();

  if constexpr(std::is_floating_point_v<T>)
  {
    if constexpr(std::is_same_v<T, float64>)
    {
      // Shortest Float64 text can move a Float32 rounding tie to either side of the midpoint.
      if(const auto precision = float32MidpointPrecision(value); precision.has_value())
      {
        return {fmt::format("{:.{}f}", value, *precision)};
      }
    }

    auto text = fmt::format("{}", value);
    const auto exponentPosition = text.find_first_of("eE");
    if(exponentPosition != std::string::npos)
    {
      // Legacy integer consumers read decimal prefixes and cannot interpret an exponent.
      const int32 exponent = std::stoi(text.substr(exponentPosition + 1));
      // One extra significant digit covers a shortest exponent that rounds across a power of ten.
      const int32 precision = std::max<int32>(0, std::numeric_limits<T>::max_digits10 - exponent);
      // Fixed formatting of the original value preserves exact large integers and enough fractional digits to round-trip.
      text = fmt::format("{:.{}f}", value, precision);
    }
    return {std::move(text)};
  }
  else
  {
    return {std::to_string(value)};
  }
}

template struct SIMPLNX_TEMPLATE_EXPORT NumberToStringFilterParameterConverter<int8>;
template struct SIMPLNX_TEMPLATE_EXPORT NumberToStringFilterParameterConverter<uint8>;

template struct SIMPLNX_TEMPLATE_EXPORT NumberToStringFilterParameterConverter<int16>;
template struct SIMPLNX_TEMPLATE_EXPORT NumberToStringFilterParameterConverter<uint16>;

template struct SIMPLNX_TEMPLATE_EXPORT NumberToStringFilterParameterConverter<int32>;
template struct SIMPLNX_TEMPLATE_EXPORT NumberToStringFilterParameterConverter<uint32>;

template struct SIMPLNX_TEMPLATE_EXPORT NumberToStringFilterParameterConverter<int64>;
template struct SIMPLNX_TEMPLATE_EXPORT NumberToStringFilterParameterConverter<uint64>;

template struct SIMPLNX_TEMPLATE_EXPORT NumberToStringFilterParameterConverter<float32>;
template struct SIMPLNX_TEMPLATE_EXPORT NumberToStringFilterParameterConverter<float64>;

Result<StringFilterParameterConverter::ValueType> StringFilterParameterConverter::convert(const nlohmann::json& json)
{
  if(!json.is_string())
  {
    return MakeErrorResult<ValueType>(-1, fmt::format("StringFilterParameter json '{}' is not a string", json.dump()));
  }

  auto value = json.get<std::string>();

  return {std::move(value)};
}
} // namespace SIMPLConversion
} // namespace nx::core
