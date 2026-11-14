#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace nx::core::ImageProcessing::detail
{
/**
 * @enum TerminalImageConversionStatus
 * @brief Reports the final conversion without allocating diagnostics inside a pixel loop.
 */
enum class TerminalImageConversionStatus : std::uint8_t
{
  Valid,    ///< A converted result is available without integer clipping.
  Clipped,  ///< A finite integer result was clipped after truncation.
  NonFinite ///< A nonfinite integer result was rejected; output is unchanged.
};

/**
 * @brief Converts floating image values and maps finite narrowing overflow to signed infinity.
 * @tparam OutputType float or double destination type.
 * @tparam InputType float or double source type.
 * @param value Floating value to convert.
 * @return The converted value, retaining NaN, infinity, and signed zero behavior.
 *
 * Preserve direct integer-to-float32 conversions at callers. Passing those integers through float64 first can introduce double rounding.
 */
template <class OutputType, class InputType>
OutputType NarrowFloatingImageValue(InputType value)
{
  static_assert(std::is_floating_point_v<OutputType> && std::is_floating_point_v<InputType>);
  static_assert(std::is_same_v<InputType, float> || std::is_same_v<InputType, double>);
  static_assert(std::is_same_v<OutputType, float> || std::is_same_v<OutputType, double>);
  if constexpr(std::is_same_v<OutputType, InputType>)
  {
    return value;
  }
  if(std::isfinite(value) && std::abs(static_cast<double>(value)) > static_cast<double>(std::numeric_limits<OutputType>::max()))
  {
    return std::signbit(value) ? -std::numeric_limits<OutputType>::infinity() : std::numeric_limits<OutputType>::infinity();
  }
  return static_cast<OutputType>(value);
}

/**
 * @brief Converts one final image value without changing intermediate image precision.
 * @tparam OutputType Integer, float, or double output type.
 * @tparam InputType float or double working-value type.
 * @param value Final working value to convert.
 * @param output Receives the converted value. Remains unchanged when NonFinite is returned.
 * @return Valid for a floating conversion or an integer conversion within range. Clipped for a finite integer result outside the range after truncation.
 * NonFinite only for a nonfinite value with an integer output type.
 *
 * A Valid floating result can be NaN or infinity. Finite integer values truncate toward zero before clipping.
 * The exact binary exclusive upper bound avoids rounding a native 64-bit maximum into an unsafe cast.
 * This helper does not aggregate warnings.
 */
template <class OutputType, class InputType>
TerminalImageConversionStatus ConvertTerminalImageValue(InputType value, OutputType& output)
{
  static_assert(std::is_floating_point_v<InputType>);
  static_assert(std::is_same_v<InputType, float> || std::is_same_v<InputType, double>);
  if constexpr(std::is_integral_v<OutputType>)
  {
    if(!std::isfinite(value))
    {
      return TerminalImageConversionStatus::NonFinite;
    }
    const double truncated = std::trunc(static_cast<double>(value));
    const double upperExclusive = std::ldexp(1.0, std::numeric_limits<OutputType>::digits);
    const double lowerInclusive = std::is_signed_v<OutputType> ? -upperExclusive : 0.0;
    if(truncated < lowerInclusive)
    {
      output = std::numeric_limits<OutputType>::lowest();
      return TerminalImageConversionStatus::Clipped;
    }
    if(truncated >= upperExclusive)
    {
      output = std::numeric_limits<OutputType>::max();
      return TerminalImageConversionStatus::Clipped;
    }
    output = static_cast<OutputType>(truncated);
    return TerminalImageConversionStatus::Valid;
  }
  else
  {
    output = NarrowFloatingImageValue<OutputType>(value);
    return TerminalImageConversionStatus::Valid;
  }
}
} // namespace nx::core::ImageProcessing::detail
