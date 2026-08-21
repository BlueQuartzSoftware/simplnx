#pragma once

#include <concepts>
#include <limits>
#include <type_traits>

namespace nx::core
{
namespace detail
{
/**
 * @brief Computes 2^exponent as a floating point value at compile time.
 *
 * Every power of two within the exponent range of FloatT is exactly representable, so repeated
 * doubling introduces no rounding error.
 * @tparam FloatT The floating point type to compute the value in
 * @param exponent The power of two to compute. Must be non-negative and within the exponent range of FloatT.
 * @return 2^exponent
 */
template <std::floating_point FloatT>
constexpr FloatT PowerOfTwo(int exponent)
{
  FloatT result = 1;
  for(int i = 0; i < exponent; i++)
  {
    result *= 2;
  }
  return result;
}
} // namespace detail

/**
 * @brief Returns true if `value` is larger than the largest value that T can represent.
 *
 * Comparing a floating point value directly against std::numeric_limits<T>::max() is incorrect for
 * the 64 bit integer types. Their maximum is not exactly representable as a float64, so the
 * implicit conversion rounds the bound *up* to 2^63 / 2^64. That both widens the accepted range by
 * one ulp and trips clang's -Wimplicit-const-int-float-conversion. Compare against 2^digits
 * instead: it is exactly representable, and because it is one past the largest integer T can hold,
 * an inclusive comparison against it is exact. Any float64 strictly below 2^digits truncates to a
 * value that T can hold.
 *
 * @tparam T The destination type whose maximum is the bound
 * @tparam U The type of the value being checked
 * @param value The value to test against the maximum of T
 * @return true if converting `value` to T would overflow
 */
template <class T, class U>
constexpr bool ExceedsMaxOf(U value)
{
  if constexpr(std::is_integral_v<T> && std::is_floating_point_v<U>)
  {
    return value >= detail::PowerOfTwo<U>(std::numeric_limits<T>::digits);
  }
  else
  {
    return value > std::numeric_limits<T>::max();
  }
}

/**
 * @brief Returns true if `value` is smaller than the smallest value that T can represent.
 *
 * See ExceedsMaxOf() for why the floating point comparison against an integer bound is written as a
 * power of two. The lowest value of a signed integer type is -2^digits, which is exactly
 * representable, so that comparison needs no adjustment beyond making the conversion explicit.
 *
 * @tparam T The destination type whose lowest value is the bound
 * @tparam U The type of the value being checked
 * @param value The value to test against the lowest value of T
 * @return true if converting `value` to T would underflow
 */
template <class T, class U>
constexpr bool ExceedsLowestOf(U value)
{
  if constexpr(std::is_integral_v<T> && std::is_floating_point_v<U>)
  {
    if constexpr(std::is_signed_v<T>)
    {
      return value < -detail::PowerOfTwo<U>(std::numeric_limits<T>::digits);
    }
    else
    {
      return value < U{0};
    }
  }
  else
  {
    return value < std::numeric_limits<T>::lowest();
  }
}
} // namespace nx::core
