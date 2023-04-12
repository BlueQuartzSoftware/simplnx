#pragma once

#include <cstdint>
#include <string>
#include <utility>

namespace TiffWriter
{
/**
 * @brief WriteColorImage Writes an RGB or RGBA image to a tiff file.
 * @param filepath Output file path
 * @param width Width of Image
 * @param height Height of Image
 * @param samplesPerPixel RGB=3, RGBA=4
 * @param data The image data to be written
 * @return
 */
std::pair<int32_t, std::string> WriteColorImage(const std::string& filepath, int32_t width, int32_t height, uint16_t samplesPerPixel, const uint8_t* data);

/**
 * @brief WriteGrayScaleImage
 * @param filepath
 * @param width
 * @param height
 * @param data
 * @return
 */
std::pair<int32_t, std::string> WriteGrayScaleImage(const std::string& filepath, int32_t width, int32_t height, const uint8_t* data);

} // namespace TiffWriter
