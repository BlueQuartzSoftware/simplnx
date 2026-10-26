#pragma once

#include "simplnx/Common/Types.hpp"

#include <limits>
#include <type_traits>

namespace nx::core::ImageProcessing
{
/** @brief Preserve floating input types and store integer-input math results as float64. */
template <class T>
using IntegerToFloat64Type = std::conditional_t<std::is_integral_v<T>, float64, T>;

/**
 * @brief Convert a floating math result, using signed infinity outside the float32 finite range.
 * NaN, infinities, signed zero, and ordinary subnormal or zero underflow retain floating semantics.
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
