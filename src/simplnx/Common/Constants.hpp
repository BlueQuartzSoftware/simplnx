#pragma once

#include "simplnx/Common/Numbers.hpp"

namespace nx::core::Constants
{

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_SqrtPi = static_cast<T>(1.7724538509055159);

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_2OverSqrtPi = static_cast<T>(1.1283791670955126);

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_HalfOfSqrtPi = static_cast<T>(0.88622692545275794);

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_SqrtHalfPi = static_cast<T>(1.2533141373155001);

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_2Pi = static_cast<T>(6.2831853071795862);

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_1OverPi = static_cast<T>(0.31830988618379069);

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_PiOver180 = static_cast<T>(0.017453292519943295);

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_360OverPi = static_cast<T>(114.59155902616465);

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_180OverPi = static_cast<T>(57.295779513082323);

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_PiOver2 = static_cast<T>(1.5707963267948966);

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_PiOver3 = static_cast<T>(1.0471975511965976);

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_PiOver4 = static_cast<T>(0.78539816339744828);

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_PiOver8 = static_cast<T>(0.39269908169872414);

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_PiOver12 = static_cast<T>(0.26179938779914941);

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_Sqrt2 = static_cast<T>(1.4142135623730951);

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_Sqrt3 = static_cast<T>(1.7320508075688772);

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_HalfSqrt2 = static_cast<T>(0.70710678118654757);

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_1OverRoot2 = static_cast<T>(0.70710678118654746);

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_1OverRoot3 = static_cast<T>(0.57735026918962584);

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_Root3Over2 = static_cast<T>(0.8660254037844386);

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_DegToRad = static_cast<T>(0.017453292519943295);

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_RadToDeg = static_cast<T>(57.295779513082323);

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_1Point3 = static_cast<T>(1.3333333333333333);

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_1Over3 = static_cast<T>(0.33333333333333331);

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_ACosNeg1 = numbers::pi_v<T>;

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_ACos1 = static_cast<T>(0.0);

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_Tan_OneEighthPi = static_cast<T>(0.41421356237309503);

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_Cos_OneEighthPi = static_cast<T>(0.92387953251128674);

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_Cos_ThreeEighthPi = static_cast<T>(0.38268343236508984);

template <class T, class = std::enable_if_t<std::is_floating_point_v<T>>>
inline constexpr T k_Sin_ThreeEighthPi = static_cast<T>(0.92387953251128674);

inline constexpr float k_PiF = numbers::pi_v<float>;
inline constexpr double k_PiD = numbers::pi_v<double>;

inline constexpr float k_SqrtPiF = k_SqrtPi<float>;
inline constexpr double k_SqrtPiD = k_SqrtPi<double>;

inline constexpr float k_2OverSqrtPiF = k_2OverSqrtPi<float>;
inline constexpr double k_2OverSqrtPiD = k_2OverSqrtPi<double>;

inline constexpr float k_HalfOfSqrtPiF = k_HalfOfSqrtPi<float>;
inline constexpr double k_HalfOfSqrtPiD = k_HalfOfSqrtPi<double>;

inline constexpr float k_SqrtHalfPiF = k_SqrtHalfPi<float>;
inline constexpr double k_SqrtHalfPiD = k_SqrtHalfPi<double>;

inline constexpr float k_2PiF = k_2Pi<float>;
inline constexpr double k_2PiD = k_2Pi<double>;

inline constexpr float k_1OverPiF = k_1OverPi<float>;
inline constexpr double k_1OverPiD = k_1OverPi<double>;

inline constexpr float k_PiOver180F = k_PiOver180<float>;
inline constexpr double k_PiOver180D = k_PiOver180<double>;

inline constexpr float k_360OverPiF = k_360OverPi<float>;
inline constexpr double k_360OverPiD = k_360OverPi<double>;

inline constexpr float k_180OverPiF = k_180OverPi<float>;
inline constexpr double k_180OverPiD = k_180OverPi<double>;

inline constexpr float k_PiOver2F = k_PiOver2<float>;
inline constexpr double k_PiOver2D = k_PiOver2<double>;

inline constexpr float k_PiOver3F = k_PiOver3<float>;
inline constexpr double k_PiOver3D = k_PiOver3<double>;

inline constexpr float k_PiOver4F = k_PiOver4<float>;
inline constexpr double k_PiOver4D = k_PiOver4<double>;

inline constexpr float k_PiOver8F = k_PiOver8<float>;
inline constexpr double k_PiOver8D = k_PiOver8<double>;

inline constexpr float k_PiOver12F = k_PiOver12<float>;
inline constexpr double k_PiOver12D = k_PiOver12<double>;

inline constexpr float k_Sqrt2F = k_Sqrt2<float>;
inline constexpr double k_Sqrt2D = k_Sqrt2<double>;

inline constexpr float k_Sqrt3F = k_Sqrt3<float>;
inline constexpr double k_Sqrt3D = k_Sqrt3<double>;

inline constexpr float k_HalfSqrt2F = k_HalfSqrt2<float>;
inline constexpr double k_HalfSqrt2D = k_HalfSqrt2<double>;

inline constexpr float k_1OverRoot2F = k_1OverRoot2<float>;
inline constexpr double k_1OverRoot2D = k_1OverRoot2<double>;

inline constexpr float k_1OverRoot3F = k_1OverRoot3<float>;
inline constexpr double k_1OverRoot3D = k_1OverRoot3<double>;

inline constexpr float k_Root3Over2F = k_Root3Over2<float>;
inline constexpr double k_Root3Over2D = k_Root3Over2<double>;

inline constexpr float k_DegToRadF = k_DegToRad<float>;
inline constexpr double k_DegToRadD = k_DegToRad<double>;

inline constexpr float k_RadToDegF = k_RadToDeg<float>;
inline constexpr double k_RadToDegD = k_RadToDeg<double>;

inline constexpr float k_1Point3F = k_1Point3<float>;
inline constexpr double k_1Point3D = k_1Point3<double>;

inline constexpr float k_1Over3F = k_1Over3<float>;
inline constexpr double k_1Over3D = k_1Over3<double>;

inline constexpr float k_ACosNeg1F = k_ACosNeg1<float>;
inline constexpr double k_ACosNeg1D = k_ACosNeg1<double>;

inline constexpr float k_ACos1F = k_ACos1<float>;
inline constexpr double k_ACos1D = k_ACos1<double>;

inline constexpr float k_Tan_OneEighthPiF = k_Tan_OneEighthPi<float>;
inline constexpr double k_Tan_OneEighthPiD = k_Tan_OneEighthPi<double>;

inline constexpr float k_Cos_OneEighthPiF = k_Cos_OneEighthPi<float>;
inline constexpr double k_Cos_OneEighthPiD = k_Cos_OneEighthPi<double>;

inline constexpr float k_Cos_ThreeEighthPiF = k_Cos_ThreeEighthPi<float>;
inline constexpr double k_Cos_ThreeEighthPiD = k_Cos_ThreeEighthPi<double>;

inline constexpr float k_Sin_ThreeEighthPiF = k_Sin_ThreeEighthPi<float>;
inline constexpr double k_Sin_ThreeEighthPiD = k_Sin_ThreeEighthPi<double>;
} // namespace nx::core::Constants
