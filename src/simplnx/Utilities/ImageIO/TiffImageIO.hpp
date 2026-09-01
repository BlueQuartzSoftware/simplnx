#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Utilities/ImageIO/IImageIO.hpp"

namespace nx::core
{

/**
 * @class TiffImageIO
 * @brief Reads and writes TIFF images with libtiff.
 *
 * Supports uint8, uint16, and float32 pixel types.
 * Reads scanlines or tiles and writes scanlines.
 * Captures libtiff error messages with a per-handle error handler.
 * The reader does not normalize the TIFF Orientation tag. Top-to-bottom output
 * requires input whose stored row order is top-to-bottom.
 */
class SIMPLNX_EXPORT TiffImageIO : public IImageIO
{
public:
  TiffImageIO() = default;
  ~TiffImageIO() noexcept override = default;

  Result<ImageMetadata> readMetadata(const std::filesystem::path& filePath) const override;
  Result<> readPixelData(const std::filesystem::path& filePath, std::span<uint8> buffer) const override;

  /**
   * @brief Supplies TIFF scanlines or tile-row segments to a callback.
   * @param filePath Identifies the TIFF file.
   * @param callback Receives bounded first-page row segments.
   * @return Valid result on success, or a decoder or callback error.
   *
   * The bounded segments let callers crop or convert directly into destination pages.
   */
  Result<> readPixelDataRows(const std::filesystem::path& filePath, const ReadRowCallback& callback) const override;
  Result<> writePixelData(const std::filesystem::path& filePath, std::span<const uint8> buffer, const ImageMetadata& metadata) const override;
  std::set<DataType> supportedWriteDataTypes() const override;
  std::set<usize> supportedWriteComponentCounts() const override;
};

} // namespace nx::core
