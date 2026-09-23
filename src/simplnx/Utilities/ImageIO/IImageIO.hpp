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
   * @brief Reads pixel data for a single page/directory into a caller-owned byte buffer.
   *
   * Reads the page/directory identified by @p pageIndex (0 for single-image formats such as
   * PNG/JPEG/BMP). For multi-page TIFFs, pages are counted the same way readMetadata() reports
   * numPages (reduced-resolution/mask subfiles are skipped), so pageIndex is in the range
   * [0, numPages).
   *
   * The buffer must be pre-sized to THAT page's dimensions: width * height * numComponents *
   * bytesPerPixel. Because individual pages may differ in size, callers should size the buffer
   * from the selected page rather than assuming page 0's dimensions.
   * Data is packed row-major, top-to-bottom.
   *
   * @param filePath Path to the image file
   * @param buffer Pre-allocated output buffer for the selected page's pixel data
   * @param pageIndex Zero-based page/directory index to read (default 0)
   * @return Empty Result on success, or error Result with library-provided message
   */
  virtual Result<> readPixelData(const std::filesystem::path& filePath, std::span<uint8> buffer, usize pageIndex = 0) const = 0;

  /**
   * @brief Decodes pixel data for a single page/directory and delivers bounded row segments to a callback.
   *
   * This avoids requiring the caller to allocate an entire image buffer. TIFF
   * implementations stream scanlines or tiles. Backends whose decoder owns a
   * whole-image allocation may retain that decoder allocation for the duration
   * of this call, but do not create a second full-image staging buffer.
   *
   * @p pageIndex selects the page/directory to stream and follows the same semantics as
   * readPixelData(): 0 for single-image formats (PNG/JPEG/BMP), and for multi-page TIFFs the pages are
   * counted the same way readMetadata() reports numPages (reduced-resolution/mask subfiles are skipped),
   * so pageIndex is in the range [0, numPages). The delivered segments describe THAT page's geometry,
   * which may differ from page 0.
   */
  virtual Result<> readPixelDataRows(const std::filesystem::path& filePath, const ReadRowCallback& callback, usize pageIndex = 0) const = 0;

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
