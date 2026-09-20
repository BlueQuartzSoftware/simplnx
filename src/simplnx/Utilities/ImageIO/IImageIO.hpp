#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Utilities/ImageIO/ImageMetadata.hpp"

#include <filesystem>
#include <functional>
#include <set>
#include <span>

namespace nx::core
{

/**
 * @class IImageIO
 * @brief Abstract interface for reading and writing 2D image files.
 *
 * Implementations use stb for PNG, JPEG, and BMP or libtiff for TIFF. This
 * interface has no DataStore or geometry dependency. Pixel methods operate on
 * the first image page.
 *
 * Buffers use packed, top-to-bottom row-major order. Each pixel contains
 * numComponents consecutive values. Each value uses the size of dataType.
 */
class SIMPLNX_EXPORT IImageIO
{
public:
  /**
   * @typedef ReadRowCallback
   * @brief Receives one contiguous segment of a decoded image row.
   *
   * The arguments are row, column offset, pixel count, and packed pixel bytes.
   * Backends can emit a full row or non-overlapping tile-row segments. Segment
   * order is backend-dependent. Invocation is synchronous. The span expires when
   * the callback returns. An invalid callback result stops decoding and propagates.
   */
  using ReadRowCallback = std::function<Result<>(usize row, usize columnOffset, usize pixelCount, std::span<const uint8> pixels)>;

  virtual ~IImageIO() noexcept = default;

  IImageIO() = default;
  IImageIO(const IImageIO&) = delete;
  IImageIO(IImageIO&&) noexcept = delete;
  IImageIO& operator=(const IImageIO&) = delete;
  IImageIO& operator=(IImageIO&&) noexcept = delete;

  /**
   * @brief Reads image metadata without decoding pixel data.
   * @param filePath Identifies the image file.
   * @return Dimensions, type, components, optional spatial metadata, and page count.
   */
  virtual Result<ImageMetadata> readMetadata(const std::filesystem::path& filePath) const = 0;

  /**
   * @brief Reads pixel data into a caller-owned byte buffer.
   * @param filePath Identifies the image file.
   * @param buffer Receives packed first-page pixel bytes.
   * @return Valid result on success, or a backend diagnostic.
   * @pre buffer size equals width times height times components times element size.
   * @pre The byte-count product fits usize.
   *
   * Call readMetadata() to determine the required buffer size.
   */
  virtual Result<> readPixelData(const std::filesystem::path& filePath, std::span<uint8> buffer) const = 0;

  /**
   * @brief Decodes pixel data and delivers bounded row segments to a callback.
   * @param filePath Identifies the image file.
   * @param callback Receives each first-page row segment synchronously.
   * @return Valid result on success, or a decoder or callback error.
   * @pre callback contains a callable target and does not throw.
   *
   * This avoids a caller-owned full-image buffer. TIFF streams scanlines or tiles.
   * A backend decoder can retain its required full-image allocation during the call.
   * It does not create a second full-image staging buffer.
   */
  virtual Result<> readPixelDataRows(const std::filesystem::path& filePath, const ReadRowCallback& callback) const = 0;

  /**
   * @brief Writes a 2D image from a raw byte buffer.
   * @param filePath Identifies the output image file.
   * @param buffer Supplies packed row-major pixel bytes.
   * @param metadata Specifies dimensions, value type, components, and optional spatial metadata.
   * @return Valid result on success, or a backend diagnostic.
   * @pre metadata dataType and numComponents are in the backend's supported sets.
   * @pre metadata dimensions and component count fit the selected backend types.
   * @pre buffer contains width times height times components times element size bytes.
   * @pre The byte-count product fits usize.
   */
  virtual Result<> writePixelData(const std::filesystem::path& filePath, std::span<const uint8> buffer, const ImageMetadata& metadata) const = 0;

  /**
   * @brief Gets the value types that this backend can write.
   * @return Supported pixel value types for the handled formats.
   */
  virtual std::set<DataType> supportedWriteDataTypes() const = 0;

  /**
   * @brief Gets the per-pixel component counts that this backend can write.
   * @return Supported component counts for the handled formats.
   *
   * Reject other counts before writePixelData(). Unsupported counts can produce
   * nonconforming files or invalid access in a third-party writer.
   */
  virtual std::set<usize> supportedWriteComponentCounts() const = 0;
};

} // namespace nx::core
