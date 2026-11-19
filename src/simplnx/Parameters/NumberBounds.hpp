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
    T Value;
    bool Inclusive;
  };

  std::optional<Limit> Lower;
  std::optional<Limit> Upper;

  /**
   * @brief Changes the default finite-value requirement for a floating-point parameter.
   */
  constexpr void setAllowNonFinite(bool allow)
  requires std::floating_point<T>
  {
    m_AllowNonFinite = allow;
  }

  /**
   * @brief Reports whether validation can accept NaN or infinity.
   */
  constexpr bool allowsNonFinite() const
  requires std::floating_point<T>
  {
    return m_AllowNonFinite;
  }

  /**
   * @brief Accepts values greater than zero. Zero is excluded.
   */
  static constexpr NumberBounds Positive()
  {
    return GreaterThan(T{0});
  }

  /**
   * @brief Accepts zero and positive values.
   */
  static constexpr NumberBounds NonNegative()
  {
    return AtLeast(T{0});
  }

  /**
   * @brief Sets an exclusive lower endpoint with no upper endpoint.
   */
  static constexpr NumberBounds GreaterThan(T value)
  {
    NumberBounds bounds;
    bounds.Lower = Limit{value, false};
    return bounds;
  }

  /**
   * @brief Sets an inclusive lower endpoint with no upper endpoint.
   */
  static constexpr NumberBounds AtLeast(T value)
  {
    NumberBounds bounds;
    bounds.Lower = Limit{value, true};
    return bounds;
  }

  /**
   * @brief Sets an exclusive upper endpoint with no lower endpoint.
   */
  static constexpr NumberBounds LessThan(T value)
  {
    NumberBounds bounds;
    bounds.Upper = Limit{value, false};
    return bounds;
  }

  /**
   * @brief Sets an inclusive upper endpoint with no lower endpoint.
   */
  static constexpr NumberBounds AtMost(T value)
  {
    NumberBounds bounds;
    bounds.Upper = Limit{value, true};
    return bounds;
  }

  /**
   * @brief Sets a closed interval. Both endpoints are valid.
   */
  static constexpr NumberBounds Between(T lowerValue, T upperValue)
  {
    NumberBounds bounds = AtLeast(lowerValue);
    bounds.Upper = Limit{upperValue, true};
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
    if(const auto& lower = bounds->Lower; lower && (value < lower->Value || (!lower->Inclusive && value == lower->Value)))
    {
      return MakeErrorResult(k_NumberBelowLowerBound, fmt::format("{} must be greater than{} {}. The value is {}.", getName(), lower->Inclusive ? " or equal to" : "", lower->Value, value));
    }
    if(const auto& upper = bounds->Upper; upper && (value > upper->Value || (!upper->Inclusive && value == upper->Value)))
    {
      return MakeErrorResult(k_NumberAboveUpperBound, fmt::format("{} must be less than{} {}. The value is {}.", getName(), upper->Inclusive ? " or equal to" : "", upper->Value, value));
    }
  }
  return {};
}
} // namespace detail
} // namespace nx::core
