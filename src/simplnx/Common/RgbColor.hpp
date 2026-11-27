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
using RgbaType = uint32;
using RgbType = uint32;

/**
 * @brief The RgbColor namespace stores calculations on or for ARGB values.
 * Calculations range from extracting color components to creating new ARGB
 * values to printing values to an output stream.
 */
namespace rgb_color
{
/**
 * @brief RgbColor::Components are integer color values for a single color
 * component. Values range from 0 to 255.
 */
using ComponentType = uint8;

/**
 * @brief Returns the integer representation of the red component.
 * @param rgb
 * @return uint8
 */
ComponentType SIMPLNX_EXPORT DRed(nx::core::RgbaType rgb);

/**
 * @brief Returns the integer representation of the green component.
 * @param rgb
 * @return uint8
 */
ComponentType SIMPLNX_EXPORT DGreen(nx::core::RgbaType rgb);

/**
 * @brief Returns the integrer representation of the blue component.
 * @param rgb
 * @return uint8
 */
ComponentType SIMPLNX_EXPORT DBlue(nx::core::RgbaType rgb);

/**
 * @brief Returns the integer representation of the alpha component.
 * @param rgb
 * @return uint8
 */
ComponentType SIMPLNX_EXPORT DAlpha(nx::core::RgbaType rgb);

/**
 * @brief Returns the integer representation of the grayscale value.
 * @param rgb
 * @return uint8
 */
ComponentType SIMPLNX_EXPORT DGray(nx::core::RgbaType rgb);

/**
 * @brief Returns the Rgba value from the individual component values.
 * @param r
 * @param g
 * @param b
 * @param a
 * @return nx::core::Rgba
 */
nx::core::RgbaType SIMPLNX_EXPORT DRgb(ComponentType r, ComponentType g, ComponentType b, ComponentType a);

/**
 * @brief Prints the Rgba value to stream using the specified separator.
 * @param out
 * @param sep
 * @param rgb
 */
void SIMPLNX_EXPORT Print(std::ostream& out, const char* sep, nx::core::RgbaType rgb);

/**
 * @brief Checks equality between two Rgba values.
 * @param lhs
 * @param rhs
 * @return bool
 */
bool SIMPLNX_EXPORT IsEqual(nx::core::RgbaType lhs, nx::core::RgbaType rhs);

/**
 * @brief Converts an Rgba value to a tuple of floats
 * @param rgb
 * @return tuple<float32, float32, float32>
 */
std::tuple<float32, float32, float32> SIMPLNX_EXPORT FRgb(nx::core::RgbaType rgb);

/**
 * @brief Generate a color table.
 * @param numColors The number of colors to generate
 * @param colorsOut The vector to store the colors into.
 */
void SIMPLNX_EXPORT GetColorTable(int numColors, std::vector<float32>& colorsOut);

} // namespace RgbColor
} // namespace nx::core
