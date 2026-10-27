#pragma once

#include "simplnx/Common/Types.hpp"

#include <limits>
#include <type_traits>

namespace nx::core::ImageProcessing
{
/**
 * @brief Maps integer input to float64 and retains floating input types.
 * @tparam T Numeric input type.
 */
template <class T>
using IntegerToFloat64Type = std::conditional_t<std::is_integral_v<T>, float64, T>;

/**
 * @brief Converts a math result to its selected floating output type.
 * @tparam OutputType float32 or float64 output type.
 * @param value Result calculated in float64.
 * @return The converted value. For float32 output, values outside the finite range become signed infinity.
 *
 * NaN, infinity, signed zero, and subnormal or zero underflow retain floating behavior.
 */
template <class OutputType>
OutputType ConvertMathResult(float64 value) noexcept
{
  static_assert(std::is_same_v<OutputType, float32> || std::is_same_v<OutputType, float64>);
  if constexpr(std::is_same_v<OutputType, float32>)
  {
    constexpr float64 finiteLimit = std::numeric_limits<float32>::max();
    if(value > finiteLimit)
    {
      return std::numeric_limits<float32>::infinity();
    }
    if(value < -finiteLimit)
    {
      return -std::numeric_limits<float32>::infinity();
    }
  }
  return static_cast<OutputType>(value);
}
} // namespace nx::core::ImageProcessing
