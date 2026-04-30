#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Utilities/ImageIO/IImageIO.hpp"

namespace nx::core
{

/**
 * @class TiffImageIO
 * @brief IImageIO backend using libtiff for TIFF format support.
 *
 * Supports uint8, uint16, and float32 pixel types.
 * Reads/writes scanline-by-scanline.
 * Captures libtiff error messages via a thread-local error handler.
 */
class SIMPLNX_EXPORT TiffImageIO : public IImageIO
{
public:
  TiffImageIO() = default;
  ~TiffImageIO() noexcept override = default;

  Result<ImageMetadata> readMetadata(const std::filesystem::path& filePath) const override;
  Result<> readPixelData(const std::filesystem::path& filePath, std::span<uint8> buffer) const override;
  Result<> writePixelData(const std::filesystem::path& filePath, std::span<const uint8> buffer, const ImageMetadata& metadata) const override;
};

} // namespace nx::core
