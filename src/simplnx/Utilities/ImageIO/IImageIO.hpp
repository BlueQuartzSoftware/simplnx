#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Utilities/ImageIO/ImageMetadata.hpp"

#include <filesystem>
#include <span>

namespace nx::core
{

/**
 * @class IImageIO
 * @brief Abstract interface for reading and writing 2D image files.
 *
 * Implementations handle specific format backends (stb for PNG/JPEG/BMP,
 * libtiff for TIFF). The interface operates on raw byte buffers and
 * ImageMetadata structs -- it knows nothing about DataStore, ImageGeom,
 * or any other simplnx data structures.
 *
 * Pixel data buffers are packed row-major, top-to-bottom:
 *   buffer[y * width * numComponents * bytesPerPixel + x * numComponents * bytesPerPixel + c * bytesPerPixel]
 */
class SIMPLNX_EXPORT IImageIO
{
public:
  virtual ~IImageIO() noexcept = default;

  IImageIO() = default;
  IImageIO(const IImageIO&) = delete;
  IImageIO(IImageIO&&) noexcept = delete;
  IImageIO& operator=(const IImageIO&) = delete;
  IImageIO& operator=(IImageIO&&) noexcept = delete;

  /**
   * @brief Reads image metadata (dimensions, type, components, origin, spacing, page count)
   * without loading pixel data into memory.
   * @param filePath Path to the image file
   * @return ImageMetadata on success, or error Result with library-provided message
   */
  virtual Result<ImageMetadata> readMetadata(const std::filesystem::path& filePath) const = 0;

  /**
   * @brief Reads pixel data into a caller-owned byte buffer.
   *
   * The buffer must be pre-sized to: width * height * numComponents * bytesPerPixel.
   * Caller should obtain dimensions from readMetadata() first.
   * Data is packed row-major, top-to-bottom.
   *
   * @param filePath Path to the image file
   * @param buffer Pre-allocated output buffer for pixel data
   * @return Empty Result on success, or error Result with library-provided message
   */
  virtual Result<> readPixelData(const std::filesystem::path& filePath, std::span<uint8> buffer) const = 0;

  /**
   * @brief Writes a 2D image from a raw byte buffer.
   *
   * Buffer layout: row-major, width * height * numComponents * bytesPerPixel.
   * The metadata struct provides dimensions, component count, and data type
   * so the backend knows how to interpret the buffer.
   *
   * @param filePath Path to the output image file
   * @param buffer Pixel data to write
   * @param metadata Image dimensions, type, and component info
   * @return Empty Result on success, or error Result with library-provided message
   */
  virtual Result<> writePixelData(const std::filesystem::path& filePath, std::span<const uint8> buffer, const ImageMetadata& metadata) const = 0;
};

} // namespace nx::core
