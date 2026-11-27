#pragma once

#include <type_traits>

// Based on <numbers> from C++20
namespace nx::core::numbers
{
template <class T>
inline constexpr T k_EV = static_cast<std::enable_if_t<std::is_floating_point_v<T>, T>>(2.718281828459045);

template <class T>
inline constexpr T k_Log2eV = static_cast<std::enable_if_t<std::is_floating_point_v<T>, T>>(1.4426950408889634);

template <class T>
inline constexpr T k_Log10eV = static_cast<std::enable_if_t<std::is_floating_point_v<T>, T>>(0.4342944819032518);

template <class T>
inline constexpr T k_PiV = static_cast<std::enable_if_t<std::is_floating_point_v<T>, T>>(3.141592653589793);

template <class T>
inline constexpr T k_InvPiV = static_cast<std::enable_if_t<std::is_floating_point_v<T>, T>>(0.3183098861837907);

template <class T>
inline constexpr T k_InvSqrtpiV = static_cast<std::enable_if_t<std::is_floating_point_v<T>, T>>(0.5641895835477563);

template <class T>
inline constexpr T k_Ln2V = static_cast<std::enable_if_t<std::is_floating_point_v<T>, T>>(0.6931471805599453);

template <class T>
inline constexpr T k_Ln10V = static_cast<std::enable_if_t<std::is_floating_point_v<T>, T>>(2.302585092994046);

template <class T>
inline constexpr T k_Sqrt2V = static_cast<std::enable_if_t<std::is_floating_point_v<T>, T>>(1.4142135623730951);

template <class T>
inline constexpr T k_Sqrt3V = static_cast<std::enable_if_t<std::is_floating_point_v<T>, T>>(1.7320508075688772);

template <class T>
inline constexpr T k_InvSqrt3V = static_cast<std::enable_if_t<std::is_floating_point_v<T>, T>>(0.5773502691896257);

template <class T>
inline constexpr T k_EgammaV = static_cast<std::enable_if_t<std::is_floating_point_v<T>, T>>(0.5772156649015329);

template <class T>
inline constexpr T k_PhiV = static_cast<std::enable_if_t<std::is_floating_point_v<T>, T>>(1.618033988749895);

inline constexpr double k_E = k_EV<double>;
inline constexpr double k_Log2e = k_Log2eV<double>;
inline constexpr double k_Log10e = k_Log10eV<double>;
inline constexpr double k_Pi = k_PiV<double>;
inline constexpr double k_InvPi = k_InvPiV<double>;
inline constexpr double k_InvSqrtpi = k_InvSqrtpiV<double>;
inline constexpr double k_Ln2 = k_Ln2V<double>;
inline constexpr double k_Ln10 = k_Ln10V<double>;
inline constexpr double k_Sqrt2 = k_Sqrt2V<double>;
inline constexpr double k_Sqrt3 = k_Sqrt3V<double>;
inline constexpr double k_InvSqrt3 = k_InvSqrt3V<double>;
inline constexpr double k_Egamma = k_EgammaV<double>;
inline constexpr double k_Phi = k_PhiV<double>;

} // namespace nx::core::numbers
