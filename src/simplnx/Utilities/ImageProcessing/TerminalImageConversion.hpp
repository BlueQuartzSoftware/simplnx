#pragma once

#include "simplnx/Common/Types.hpp"

#include <cmath>
#include <limits>
#include <type_traits>

// NOLINTNEXTLINE(readability-identifier-naming) Extend the existing ImageProcessing namespace.
namespace nx::core::ImageProcessing
{
/**
 * @brief Converts a floating image value to its output type, rounding Float32 overflow at the IEEE midpoint.
 * @tparam OutputType Float32 or Float64 output type.
 * @tparam InputType Float32 or Float64 working type.
 * @param value Computed floating-point value.
 * @return The converted value. NaN, signed infinity, and signed zero retain their floating-point behavior.
 */
template <class OutputType, class InputType>
[[nodiscard]] OutputType NarrowFloatingImageValue(InputType value) noexcept
{
  static_assert(std::is_same_v<InputType, float32> || std::is_same_v<InputType, float64>);
  static_assert(std::is_same_v<OutputType, float32> || std::is_same_v<OutputType, float64>);
  if constexpr(std::is_same_v<InputType, float64> && std::is_same_v<OutputType, float32>)
  {
    static_assert(std::numeric_limits<float32>::is_iec559 && std::numeric_limits<float32>::digits == 24 && std::numeric_limits<float32>::max_exponent == 128);
    const float64 magnitude = std::abs(value);
    if(magnitude > static_cast<float64>(std::numeric_limits<float32>::max()))
    {
      // At the midpoint between FLT_MAX and 2^128, ties-to-even overflows to infinity.
      constexpr float64 overflowMidpoint = 0x1.ffffffp127;
      const float32 rounded = magnitude < overflowMidpoint ? std::numeric_limits<float32>::max() : std::numeric_limits<float32>::infinity();
      return std::signbit(value) ? -rounded : rounded;
    }
  }
  return static_cast<OutputType>(value);
}
} // namespace nx::core::ImageProcessing
