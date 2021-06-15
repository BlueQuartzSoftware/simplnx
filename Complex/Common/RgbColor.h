#pragma once

#include <iostream>
#include <tuple>

namespace Complex
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
Component dRed(Complex::Rgba rgb);

/**
 * @brief Returns the integer representation of the green component.
 * @param rgb
 * @return uint8_t
 */
Component dGreen(Complex::Rgba rgb);

/**
 * @brief Returns the integrer representation of the blue component.
 * @param rgb
 * @return int23_t
 */
Component dBlue(Complex::Rgba rgb);

/**
 * @brief Returns the integer representation of the alpha component.
 * @param rgb
 * @return uint8_t
 */
Component dAlpha(Complex::Rgba rgb);

/**
 * @brief Returns the integer representation of the grayscale value.
 * @param rgb
 * @return uint8_t
 */
Component dGray(Complex::Rgba rgb);

/**
 * @brief Returns the Rgba value from the individual component values.
 * @param r
 * @param g
 * @param b
 * @param a
 * @return Complex::Rgba
 */
Complex::Rgba dRgb(Component r, Component g, Component b, Component a);

/**
 * @brief Prints the Rgba value to stream using the specified separator.
 * @param out
 * @param sep
 * @param rgb
 */
void print(std::ostream& out, const char* sep, Complex::Rgba rgb);

/**
 * @brief Checks equality between two Rgba values.
 * @param lhs
 * @param rhs
 * @return bool
 */
bool isEqual(Complex::Rgba lhs, Complex::Rgba rhs);

/**
 * @brief Converts an Rgba value to a tuple of floats
 * @param rgb
 * @return tuple<float, float, float>
 */
std::tuple<float, float, float> fRgb(Complex::Rgba rgb);
} // namespace RgbColor
} // namespace Complex
