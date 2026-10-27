#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace nx::core::ImageProcessing::detail
{
/** @brief Reports the final conversion without allocating diagnostics inside a pixel loop. */
enum class TerminalImageConversionStatus : std::uint8_t
{
  Valid,
  Clipped,
  NonFinite
};

/**
 * @brief Retains floating behavior and explicitly maps finite narrowing overflow to signed infinity.
 * The input must already be float32 or float64. Preserve existing direct integer-to-float32 conversions at callers;
 * passing those integers through float64 first can introduce double rounding.
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
 * @brief Converts one final image value. Finite integers truncate before clipping; nonfinite integers fail.
 * The binary exclusive upper bound is exact at 64-bit widths, unlike a floating conversion of the native maximum.
 * This function does not change intermediate image precision or aggregate warnings.
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
