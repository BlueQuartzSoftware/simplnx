#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"

#include <fmt/format.h>

#include <cmath>
#include <concepts>
#include <optional>

namespace nx::core
{
inline constexpr int32 k_NumberNotFinite = -86200;
inline constexpr int32 k_NumberBelowLowerBound = -86201;
inline constexpr int32 k_NumberAboveUpperBound = -86202;

/**
 * @brief Optional limits for a scalar or for each element of a vector.
 */
template <class T>
struct NumberBounds
{
  struct Limit
  {
    T value;
    bool inclusive;
  };

  std::optional<Limit> lower;
  std::optional<Limit> upper;

  /**
   * @brief Changes the default finite-value requirement for a floating-point parameter.
   */
  constexpr void setAllowNonFinite(bool allow)
  requires std::floating_point<T>
  {
    m_AllowNonFinite = allow;
  }

  constexpr bool allowsNonFinite() const
  requires std::floating_point<T>
  {
    return m_AllowNonFinite;
  }

  static constexpr NumberBounds Positive()
  {
    return GreaterThan(T{0});
  }

  static constexpr NumberBounds NonNegative()
  {
    return AtLeast(T{0});
  }

  static constexpr NumberBounds GreaterThan(T value)
  {
    NumberBounds bounds;
    bounds.lower = Limit{value, false};
    return bounds;
  }

  static constexpr NumberBounds AtLeast(T value)
  {
    NumberBounds bounds;
    bounds.lower = Limit{value, true};
    return bounds;
  }

  static constexpr NumberBounds LessThan(T value)
  {
    NumberBounds bounds;
    bounds.upper = Limit{value, false};
    return bounds;
  }

  static constexpr NumberBounds AtMost(T value)
  {
    NumberBounds bounds;
    bounds.upper = Limit{value, true};
    return bounds;
  }

  static constexpr NumberBounds Between(T lowerValue, T upperValue)
  {
    NumberBounds bounds = AtLeast(lowerValue);
    bounds.upper = Limit{upperValue, true};
    return bounds;
  }

private:
  bool m_AllowNonFinite = false;
};

namespace detail
{
template <class T, class GetName>
Result<> ValidateNumberBounds(T value, const std::optional<NumberBounds<T>>& bounds, GetName getName)
{
  if constexpr(std::floating_point<T>)
  {
    if(!std::isfinite(value) && (!bounds || !bounds->allowsNonFinite()))
    {
      return MakeErrorResult(k_NumberNotFinite, fmt::format("{} must be a finite number. The value is {}.", getName(), value));
    }
  }
  if(bounds)
  {
    if(const auto& lower = bounds->lower; lower && (value < lower->value || (!lower->inclusive && value == lower->value)))
    {
      return MakeErrorResult(k_NumberBelowLowerBound, fmt::format("{} must be greater than{} {}. The value is {}.", getName(), lower->inclusive ? " or equal to" : "", lower->value, value));
    }
    if(const auto& upper = bounds->upper; upper && (value > upper->value || (!upper->inclusive && value == upper->value)))
    {
      return MakeErrorResult(k_NumberAboveUpperBound, fmt::format("{} must be less than{} {}. The value is {}.", getName(), upper->inclusive ? " or equal to" : "", upper->value, value));
    }
  }
  return {};
}
} // namespace detail
} // namespace nx::core
