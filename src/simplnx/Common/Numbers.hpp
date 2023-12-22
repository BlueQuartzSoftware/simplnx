#pragma once

#include <type_traits>

namespace nx::core
{
// Based on <numbers> from C++20
namespace numbers
{
template <class T>
inline constexpr T e_v = static_cast<std::enable_if_t<std::is_floating_point_v<T>, T>>(2.718281828459045);

template <class T>
inline constexpr T log2e_v = static_cast<std::enable_if_t<std::is_floating_point_v<T>, T>>(1.4426950408889634);

template <class T>
inline constexpr T log10e_v = static_cast<std::enable_if_t<std::is_floating_point_v<T>, T>>(0.4342944819032518);

template <class T>
inline constexpr T pi_v = static_cast<std::enable_if_t<std::is_floating_point_v<T>, T>>(3.141592653589793);

template <class T>
inline constexpr T inv_pi_v = static_cast<std::enable_if_t<std::is_floating_point_v<T>, T>>(0.3183098861837907);

template <class T>
inline constexpr T inv_sqrtpi_v = static_cast<std::enable_if_t<std::is_floating_point_v<T>, T>>(0.5641895835477563);

template <class T>
inline constexpr T ln2_v = static_cast<std::enable_if_t<std::is_floating_point_v<T>, T>>(0.6931471805599453);

template <class T>
inline constexpr T ln10_v = static_cast<std::enable_if_t<std::is_floating_point_v<T>, T>>(2.302585092994046);

template <class T>
inline constexpr T sqrt2_v = static_cast<std::enable_if_t<std::is_floating_point_v<T>, T>>(1.4142135623730951);

template <class T>
inline constexpr T sqrt3_v = static_cast<std::enable_if_t<std::is_floating_point_v<T>, T>>(1.7320508075688772);

template <class T>
inline constexpr T inv_sqrt3_v = static_cast<std::enable_if_t<std::is_floating_point_v<T>, T>>(0.5773502691896257);

template <class T>
inline constexpr T egamma_v = static_cast<std::enable_if_t<std::is_floating_point_v<T>, T>>(0.5772156649015329);

template <class T>
inline constexpr T phi_v = static_cast<std::enable_if_t<std::is_floating_point_v<T>, T>>(1.618033988749895);

inline constexpr double e = e_v<double>;
inline constexpr double log2e = log2e_v<double>;
inline constexpr double log10e = log10e_v<double>;
inline constexpr double pi = pi_v<double>;
inline constexpr double inv_pi = inv_pi_v<double>;
inline constexpr double inv_sqrtpi = inv_sqrtpi_v<double>;
inline constexpr double ln2 = ln2_v<double>;
inline constexpr double ln10 = ln10_v<double>;
inline constexpr double sqrt2 = sqrt2_v<double>;
inline constexpr double sqrt3 = sqrt3_v<double>;
inline constexpr double inv_sqrt3 = inv_sqrt3_v<double>;
inline constexpr double egamma = egamma_v<double>;
inline constexpr double phi = phi_v<double>;

} // namespace numbers
} // namespace nx::core
