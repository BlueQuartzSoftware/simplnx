#pragma once

#include <iostream>
#include <tuple>

#include "complex/complex_export.hpp"

namespace complex
{
using Rgba = uint32_t;

namespace RgbColor
{
using Component = uint8_t;

/**
 * @brief Returns the integer representation of the red component.
 * @param rgb
 * @return uint8_t
 */
Component COMPLEX_EXPORT dRed(complex::Rgba rgb);

/**
 * @brief Returns the integer representation of the green component.
 * @param rgb
 * @return uint8_t
 */
Component COMPLEX_EXPORT dGreen(complex::Rgba rgb);

/**
 * @brief Returns the integrer representation of the blue component.
 * @param rgb
 * @return int23_t
 */
Component COMPLEX_EXPORT dBlue(complex::Rgba rgb);

/**
 * @brief Returns the integer representation of the alpha component.
 * @param rgb
 * @return uint8_t
 */
Component COMPLEX_EXPORT dAlpha(complex::Rgba rgb);

/**
 * @brief Returns the integer representation of the grayscale value.
 * @param rgb
 * @return uint8_t
 */
Component COMPLEX_EXPORT dGray(complex::Rgba rgb);

/**
 * @brief Returns the Rgba value from the individual component values.
 * @param r
 * @param g
 * @param b
 * @param a
 * @return complex::Rgba
 */
complex::Rgba COMPLEX_EXPORT dRgb(Component r, Component g, Component b, Component a);

/**
 * @brief Prints the Rgba value to stream using the specified separator.
 * @param out
 * @param sep
 * @param rgb
 */
void COMPLEX_EXPORT print(std::ostream& out, const char* sep, complex::Rgba rgb);

/**
 * @brief Checks equality between two Rgba values.
 * @param lhs
 * @param rhs
 * @return bool
 */
bool COMPLEX_EXPORT isEqual(complex::Rgba lhs, complex::Rgba rhs);

/**
 * @brief Converts an Rgba value to a tuple of floats
 * @param rgb
 * @return tuple<float, float, float>
 */
std::tuple<float, float, float> COMPLEX_EXPORT fRgb(complex::Rgba rgb);
} // namespace RgbColor
} // namespace complex
