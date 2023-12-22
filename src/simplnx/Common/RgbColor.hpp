#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

#include <iostream>
#include <tuple>
#include <vector>

namespace nx::core
{
/**
 * @brief The Rgba alias is used to specify the format for storing integer
 * color information. Values are stored as sequential unsigned char components
 * ranging from 0 to 255 describing red, green, blue, and alpha values.
 */
using Rgba = uint32;
using Rgb = uint32;

/**
 * @brief The RgbColor namespace stores calculations on or for ARGB values.
 * Calculations range from extracting color components to creating new ARGB
 * values to printing values to an output stream.
 */
namespace RgbColor
{
/**
 * @brief RgbColor::Components are integer color values for a single color
 * component. Values range from 0 to 255.
 */
using Component = uint8;

/**
 * @brief Returns the integer representation of the red component.
 * @param rgb
 * @return uint8
 */
Component SIMPLNX_EXPORT dRed(nx::core::Rgba rgb);

/**
 * @brief Returns the integer representation of the green component.
 * @param rgb
 * @return uint8
 */
Component SIMPLNX_EXPORT dGreen(nx::core::Rgba rgb);

/**
 * @brief Returns the integrer representation of the blue component.
 * @param rgb
 * @return uint8
 */
Component SIMPLNX_EXPORT dBlue(nx::core::Rgba rgb);

/**
 * @brief Returns the integer representation of the alpha component.
 * @param rgb
 * @return uint8
 */
Component SIMPLNX_EXPORT dAlpha(nx::core::Rgba rgb);

/**
 * @brief Returns the integer representation of the grayscale value.
 * @param rgb
 * @return uint8
 */
Component SIMPLNX_EXPORT dGray(nx::core::Rgba rgb);

/**
 * @brief Returns the Rgba value from the individual component values.
 * @param r
 * @param g
 * @param b
 * @param a
 * @return nx::core::Rgba
 */
nx::core::Rgba SIMPLNX_EXPORT dRgb(Component r, Component g, Component b, Component a);

/**
 * @brief Prints the Rgba value to stream using the specified separator.
 * @param out
 * @param sep
 * @param rgb
 */
void SIMPLNX_EXPORT print(std::ostream& out, const char* sep, nx::core::Rgba rgb);

/**
 * @brief Checks equality between two Rgba values.
 * @param lhs
 * @param rhs
 * @return bool
 */
bool SIMPLNX_EXPORT isEqual(nx::core::Rgba lhs, nx::core::Rgba rhs);

/**
 * @brief Converts an Rgba value to a tuple of floats
 * @param rgb
 * @return tuple<float32, float32, float32>
 */
std::tuple<float32, float32, float32> SIMPLNX_EXPORT fRgb(nx::core::Rgba rgb);

/**
 * @brief Generate a color table.
 * @param numColors The number of colors to generate
 * @param colorsOut The vector to store the colors into.
 */
void SIMPLNX_EXPORT GetColorTable(int numColors, std::vector<float32>& colorsOut);

} // namespace RgbColor
} // namespace nx::core
