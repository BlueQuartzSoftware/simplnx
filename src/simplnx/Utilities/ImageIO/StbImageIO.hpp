#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Utilities/ImageIO/IImageIO.hpp"

namespace nx::core
{

/**
 * @class StbImageIO
 * @brief Reads and writes PNG, JPEG, and BMP images with stb.
 */
class SIMPLNX_EXPORT StbImageIO : public IImageIO
{
public:
  StbImageIO() = default;
  ~StbImageIO() noexcept override = default;

  Result<ImageMetadata> readMetadata(const std::filesystem::path& filePath) const override;
  /**
   * @brief Decodes the requested page with stb's required whole-image allocation.
   */
  Result<> readPixelData(const std::filesystem::path& filePath, std::span<uint8> buffer, usize pageIndex = 0) const override;

  /**
   * @brief Decodes with stb and supplies each row without a second full-image buffer.
   * @param filePath Identifies the image file.
   * @param callback Receives each complete decoded row.
   * @return Valid result on success, or a decoder or callback error.
   *
   * stb owns one whole-image allocation until all synchronous callbacks finish.
   */
  Result<> readPixelDataRows(const std::filesystem::path& filePath, const ReadRowCallback& callback, usize pageIndex = 0) const override;
  Result<> writePixelData(const std::filesystem::path& filePath, std::span<const uint8> buffer, const ImageMetadata& metadata) const override;
  std::set<DataType> supportedWriteDataTypes() const override;
  std::set<usize> supportedWriteComponentCounts() const override;
};

} // namespace nx::core
