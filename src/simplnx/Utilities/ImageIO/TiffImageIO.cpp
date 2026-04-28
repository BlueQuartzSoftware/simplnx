#include "TiffImageIO.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/TypesUtility.hpp"

#include <tiffio.h>

#include <fmt/format.h>

#include <cstdarg>
#include <cstring>

using namespace nx::core;

namespace
{
constexpr int32 k_ErrorOpenFailed = -20100;
constexpr int32 k_ErrorReadMetadataFailed = -20101;
constexpr int32 k_ErrorReadPixelFailed = -20102;
constexpr int32 k_ErrorWriteFailed = -20103;
constexpr int32 k_ErrorUnsupportedFormat = -20104;
constexpr int32 k_ErrorBufferSizeMismatch = -20105;

/**
 * @brief Owns a TIFFOpenOptions* and the std::string buffer that the per-handle
 * libtiff error handler writes into. The handlers are registered at construction
 * and kept private — every call site that opens a TIFF needs both the options
 * and a place to capture errors, so they are bundled into a single RAII type.
 *
 * Non-copyable and non-movable so that the address of m_ErrorMessage stays
 * stable (libtiff stores it as opaque user_data).
 */
class TiffOpenOptions
{
public:
  TiffOpenOptions()
  : m_Opts(TIFFOpenOptionsAlloc())
  {
    if(m_Opts != nullptr)
    {
      TIFFOpenOptionsSetErrorHandlerExtR(m_Opts, errorHandler, &m_ErrorMessage);
      TIFFOpenOptionsSetWarningHandlerExtR(m_Opts, warningHandler, nullptr);
    }
  }

  ~TiffOpenOptions() noexcept
  {
    if(m_Opts != nullptr)
    {
      TIFFOpenOptionsFree(m_Opts);
    }
  }

  TiffOpenOptions(const TiffOpenOptions&) = delete;
  TiffOpenOptions& operator=(const TiffOpenOptions&) = delete;
  TiffOpenOptions(TiffOpenOptions&&) = delete;
  TiffOpenOptions& operator=(TiffOpenOptions&&) = delete;

  TIFFOpenOptions* get() const
  {
    return m_Opts;
  }

  // Returns the most recent libtiff error captured by the handler, or
  // "unknown error" if libtiff failed without producing a string.
  std::string_view errorMessage() const
  {
    return m_ErrorMessage.empty() ? std::string_view{"unknown error"} : std::string_view{m_ErrorMessage};
  }

private:
  // Reports each libtiff error into the std::string pointed to by user_data.
  // Returning 1 tells libtiff the error was handled so it will not call the
  // global error handler (avoids stderr spam in library consumers).
  static int errorHandler(TIFF* /*tif*/, void* user_data, const char* /*module*/, const char* formatStr, va_list args)
  {
    if(user_data == nullptr || formatStr == nullptr)
    {
      return 1;
    }
    char buf[1024];
    vsnprintf(buf, sizeof(buf), formatStr, args);
    *static_cast<std::string*>(user_data) = buf;
    return 1;
  }

  // Intentionally suppress warnings.
  static int warningHandler(TIFF* /*tif*/, void* /*user_data*/, const char* /*module*/, const char* /*formatStr*/, va_list /*args*/)
  {
    return 1;
  }

  TIFFOpenOptions* m_Opts = nullptr;
  std::string m_ErrorMessage;
};

/**
 * @brief RAII wrapper for TIFF* that calls TIFFClose on destruction.
 */
class TiffHandleGuard
{
public:
  explicit TiffHandleGuard(TIFF* tiff)
  : m_Tiff(tiff)
  {
  }

  ~TiffHandleGuard() noexcept
  {
    if(m_Tiff != nullptr)
    {
      TIFFClose(m_Tiff);
    }
  }

  TiffHandleGuard(const TiffHandleGuard&) = delete;
  TiffHandleGuard& operator=(const TiffHandleGuard&) = delete;
  TiffHandleGuard(TiffHandleGuard&&) = delete;
  TiffHandleGuard& operator=(TiffHandleGuard&&) = delete;

  TIFF* get() const
  {
    return m_Tiff;
  }

private:
  TIFF* m_Tiff = nullptr;
};

// Maps the TIFF (bits-per-sample, sample-format) pair to the DataTypes the
// image-IO backends support. Other combinations (e.g. 1-bit bilevel, 4-bit
// palette, int32, double) are explicitly rejected rather than silently falling
// through to uint8.
Result<DataType> DetermineTiffDataType(TIFF* tiff)
{
  uint16_t bitsPerSample = 8;
  uint16_t sampleFormat = SAMPLEFORMAT_UINT;

  TIFFGetFieldDefaulted(tiff, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
  TIFFGetFieldDefaulted(tiff, TIFFTAG_SAMPLEFORMAT, &sampleFormat);

  if(sampleFormat == SAMPLEFORMAT_UINT)
  {
    if(bitsPerSample == 8)
    {
      return {DataType::uint8};
    }
    if(bitsPerSample == 16)
    {
      return {DataType::uint16};
    }
  }
  else if(sampleFormat == SAMPLEFORMAT_IEEEFP && bitsPerSample == 32)
  {
    return {DataType::float32};
  }

  return MakeErrorResult<DataType>(k_ErrorUnsupportedFormat,
                                   fmt::format("Unsupported TIFF pixel format: bits-per-sample={}, sample-format={}. Supported combinations are (8, UINT), (16, UINT), (32, IEEEFP).", bitsPerSample,
                                               static_cast<int>(sampleFormat)));
}
} // namespace

// -----------------------------------------------------------------------------
Result<ImageMetadata> TiffImageIO::readMetadata(const std::filesystem::path& filePath) const
{
  std::string pathStr = filePath.string();
  TiffOpenOptions opts;
  TiffHandleGuard tiffGuard(opts.get() != nullptr ? TIFFOpenExt(pathStr.c_str(), "r", opts.get()) : nullptr);
  if(tiffGuard.get() == nullptr)
  {
    return MakeErrorResult<ImageMetadata>(k_ErrorOpenFailed, fmt::format("Failed to open TIFF file '{}': {}", pathStr, opts.errorMessage()));
  }

  TIFF* tiff = tiffGuard.get();

  uint32_t width = 0;
  uint32_t height = 0;
  if(TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width) == 0 || TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height) == 0)
  {
    return MakeErrorResult<ImageMetadata>(k_ErrorReadMetadataFailed, fmt::format("Failed to read TIFF dimensions from '{}': {}", pathStr, opts.errorMessage()));
  }

  // SamplesPerPixel is required by the TIFF 6.0 spec (Section 7). Treat absence as an error
  // rather than silently defaulting to 1, which would mis-read multi-channel images that lack
  // the tag.
  uint16_t samplesPerPixel = 0;
  if(TIFFGetField(tiff, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel) == 0)
  {
    return MakeErrorResult<ImageMetadata>(k_ErrorReadMetadataFailed, fmt::format("Required TIFF tag SamplesPerPixel is missing from '{}'", pathStr));
  }

  Result<DataType> dataTypeResult = DetermineTiffDataType(tiff);
  if(dataTypeResult.invalid())
  {
    return ConvertResultTo<ImageMetadata>(ConvertResult(std::move(dataTypeResult)), {});
  }

  ImageMetadata metadata;
  metadata.width = static_cast<usize>(width);
  metadata.height = static_cast<usize>(height);
  metadata.numComponents = static_cast<usize>(samplesPerPixel);
  metadata.dataType = dataTypeResult.value();
  metadata.numPages = static_cast<usize>(TIFFNumberOfDirectories(tiff));

  // Read optional origin (TIFF X/Y position tags)
  float xPosition = 0.0f;
  float yPosition = 0.0f;
  bool hasOrigin = false;
  if(TIFFGetField(tiff, TIFFTAG_XPOSITION, &xPosition) != 0)
  {
    hasOrigin = true;
  }
  if(TIFFGetField(tiff, TIFFTAG_YPOSITION, &yPosition) != 0)
  {
    hasOrigin = true;
  }
  if(hasOrigin)
  {
    metadata.origin = FloatVec3(xPosition, yPosition, 0.0f);
  }

  // Read optional spacing (TIFF resolution tags)
  float xRes = 0.0f;
  float yRes = 0.0f;
  bool hasSpacing = false;
  if(TIFFGetField(tiff, TIFFTAG_XRESOLUTION, &xRes) != 0 && xRes > 0.0f)
  {
    hasSpacing = true;
  }
  if(TIFFGetField(tiff, TIFFTAG_YRESOLUTION, &yRes) != 0 && yRes > 0.0f)
  {
    hasSpacing = true;
  }
  if(hasSpacing)
  {
    // Resolution is in pixels-per-unit; spacing is the inverse
    float32 xSpacing = (xRes > 0.0f) ? (1.0f / xRes) : 1.0f;
    float32 ySpacing = (yRes > 0.0f) ? (1.0f / yRes) : 1.0f;
    metadata.spacing = FloatVec3(xSpacing, ySpacing, 1.0f);
  }

  return {std::move(metadata)};
}

// -----------------------------------------------------------------------------
Result<> TiffImageIO::readPixelData(const std::filesystem::path& filePath, std::span<uint8> buffer) const
{
  std::string pathStr = filePath.string();
  TiffOpenOptions opts;
  TiffHandleGuard tiffGuard(opts.get() != nullptr ? TIFFOpenExt(pathStr.c_str(), "r", opts.get()) : nullptr);
  if(tiffGuard.get() == nullptr)
  {
    return MakeErrorResult(k_ErrorOpenFailed, fmt::format("Failed to open TIFF file '{}': {}", pathStr, opts.errorMessage()));
  }

  TIFF* tiff = tiffGuard.get();

  uint32_t width = 0;
  uint32_t height = 0;
  if(TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width) == 0 || TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height) == 0)
  {
    return MakeErrorResult(k_ErrorReadMetadataFailed, fmt::format("Failed to read TIFF dimensions from '{}': {}", pathStr, opts.errorMessage()));
  }

  // SamplesPerPixel is required by the TIFF 6.0 spec (Section 7). See readMetadata().
  uint16_t samplesPerPixel = 0;
  if(TIFFGetField(tiff, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel) == 0)
  {
    return MakeErrorResult(k_ErrorReadMetadataFailed, fmt::format("Required TIFF tag SamplesPerPixel is missing from '{}'", pathStr));
  }

  Result<DataType> dataTypeResult = DetermineTiffDataType(tiff);
  if(dataTypeResult.invalid())
  {
    return ConvertResult(std::move(dataTypeResult));
  }
  DataType dataType = dataTypeResult.value();
  usize bpe = GetDataTypeSize(dataType);

  usize expectedSize = static_cast<usize>(width) * static_cast<usize>(height) * static_cast<usize>(samplesPerPixel) * bpe;
  if(buffer.size() != expectedSize)
  {
    return MakeErrorResult(k_ErrorBufferSizeMismatch, fmt::format("Buffer size {} does not match expected size {} for TIFF image '{}'", buffer.size(), expectedSize, pathStr));
  }

  // Check if the image is tiled
  if(TIFFIsTiled(tiff) != 0)
  {
    // For tiled TIFFs, use TIFFReadRGBAImageOriented as a fallback.
    // This converts to uint8 RGBA, so it only works well for uint8 images.
    if(dataType != DataType::uint8)
    {
      return MakeErrorResult(k_ErrorUnsupportedFormat, fmt::format("Tiled TIFF with non-uint8 data is not supported for '{}'. Convert to stripped TIFF first.", pathStr));
    }

    std::vector<uint32_t> raster(static_cast<usize>(width) * static_cast<usize>(height));
    if(TIFFReadRGBAImageOriented(tiff, width, height, raster.data(), ORIENTATION_TOPLEFT, 0) == 0)
    {
      return MakeErrorResult(k_ErrorReadPixelFailed, fmt::format("Failed to read tiled TIFF pixel data from '{}': {}", pathStr, opts.errorMessage()));
    }

    // TIFFReadRGBAImageOriented produces ABGR uint32 packed pixels.
    // Extract the requested number of components.
    usize pixelCount = static_cast<usize>(width) * static_cast<usize>(height);
    for(usize i = 0; i < pixelCount; i++)
    {
      uint32_t pixel = raster[i];
      uint8 r = TIFFGetR(pixel);
      uint8 g = TIFFGetG(pixel);
      uint8 b = TIFFGetB(pixel);
      uint8 a = TIFFGetA(pixel);

      usize offset = i * samplesPerPixel;
      if(samplesPerPixel >= 1)
      {
        buffer[offset] = r;
      }
      if(samplesPerPixel >= 2)
      {
        buffer[offset + 1] = g;
      }
      if(samplesPerPixel >= 3)
      {
        buffer[offset + 2] = b;
      }
      if(samplesPerPixel >= 4)
      {
        buffer[offset + 3] = a;
      }
    }

    return {};
  }

  // Scanline-based reading. TIFFScanlineSize returns tsize_t (signed); libtiff returns 0 or -1
  // on failure. Without an explicit guard, the cast to usize below would treat -1 as ~16 EiB
  // and std::vector::vector would throw std::bad_alloc.
  tsize_t scanlineSize = TIFFScanlineSize(tiff);
  if(scanlineSize <= 0)
  {
    return MakeErrorResult(k_ErrorReadMetadataFailed, fmt::format("TIFFScanlineSize returned {} for '{}': {}", scanlineSize, pathStr, opts.errorMessage()));
  }
  usize rowBytes = static_cast<usize>(width) * static_cast<usize>(samplesPerPixel) * bpe;

  // Use the larger of TIFFScanlineSize and our computed row size
  usize scanlineBufSize = std::max(static_cast<usize>(scanlineSize), rowBytes);
  std::vector<uint8> scanlineBuf(scanlineBufSize);

  for(uint32_t row = 0; row < height; row++)
  {
    if(TIFFReadScanline(tiff, scanlineBuf.data(), row) < 0)
    {
      return MakeErrorResult(k_ErrorReadPixelFailed, fmt::format("Failed to read scanline {} from TIFF '{}': {}", row, pathStr, opts.errorMessage()));
    }
    std::memcpy(buffer.data() + (static_cast<usize>(row) * rowBytes), scanlineBuf.data(), rowBytes);
  }

  return {};
}

// -----------------------------------------------------------------------------
Result<> TiffImageIO::writePixelData(const std::filesystem::path& filePath, std::span<const uint8> buffer, const ImageMetadata& metadata) const
{
  if(metadata.dataType != DataType::uint8 && metadata.dataType != DataType::uint16 && metadata.dataType != DataType::float32)
  {
    return MakeErrorResult(k_ErrorUnsupportedFormat, fmt::format("Unsupported data type for TIFF writing to '{}'. Supported: uint8, uint16, float32.", filePath.string()));
  }
  usize bpe = GetDataTypeSize(metadata.dataType);

  usize expectedSize = metadata.width * metadata.height * metadata.numComponents * bpe;
  if(buffer.size() != expectedSize)
  {
    return MakeErrorResult(k_ErrorBufferSizeMismatch, fmt::format("Buffer size {} does not match expected size {} for TIFF write to '{}'", buffer.size(), expectedSize, filePath.string()));
  }

  std::string pathStr = filePath.string();
  TiffOpenOptions opts;
  TiffHandleGuard tiffGuard(opts.get() != nullptr ? TIFFOpenExt(pathStr.c_str(), "w", opts.get()) : nullptr);
  if(tiffGuard.get() == nullptr)
  {
    return MakeErrorResult(k_ErrorOpenFailed, fmt::format("Failed to open TIFF file for writing '{}': {}", pathStr, opts.errorMessage()));
  }

  TIFF* tiff = tiffGuard.get();

  uint32_t w = static_cast<uint32_t>(metadata.width);
  uint32_t h = static_cast<uint32_t>(metadata.height);
  uint16_t comp = static_cast<uint16_t>(metadata.numComponents);

  // TIFFSetField returns 0 on failure; most failures here indicate an unwritable handle.
  if(TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, w) == 0 || TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, h) == 0 || TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, comp) == 0 ||
     TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT) == 0 || TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG) == 0 ||
     TIFFSetField(tiff, TIFFTAG_COMPRESSION, COMPRESSION_LZW) == 0 || TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tiff, 0)) == 0)
  {
    return MakeErrorResult(k_ErrorWriteFailed, fmt::format("Failed to set required TIFF tags for '{}': {}", pathStr, opts.errorMessage()));
  }

  // Set bits per sample and sample format based on data type
  bool fieldsSet = false;
  switch(metadata.dataType)
  {
  case DataType::uint8:
    fieldsSet = TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 8) != 0 && TIFFSetField(tiff, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT) != 0;
    break;
  case DataType::uint16:
    fieldsSet = TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 16) != 0 && TIFFSetField(tiff, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT) != 0;
    break;
  case DataType::float32:
    fieldsSet = TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 32) != 0 && TIFFSetField(tiff, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP) != 0;
    break;
  default:
    // Unreachable: filtered out above.
    return MakeErrorResult(k_ErrorUnsupportedFormat, fmt::format("Unsupported data type for TIFF writing to '{}'. Supported: uint8, uint16, float32.", pathStr));
  }
  if(!fieldsSet)
  {
    return MakeErrorResult(k_ErrorWriteFailed, fmt::format("Failed to set TIFF sample format tags for '{}': {}", pathStr, opts.errorMessage()));
  }

  // Set photometric interpretation
  uint16_t photometric = (comp == 1) ? PHOTOMETRIC_MINISBLACK : PHOTOMETRIC_RGB;
  if(TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, photometric) == 0)
  {
    return MakeErrorResult(k_ErrorWriteFailed, fmt::format("Failed to set TIFF photometric tag for '{}': {}", pathStr, opts.errorMessage()));
  }

  // Write optional origin
  if(metadata.origin.has_value())
  {
    const FloatVec3& origin = metadata.origin.value();
    // Position tags are optional metadata; ignore failure so writing still succeeds on uncommon variants.
    TIFFSetField(tiff, TIFFTAG_XPOSITION, origin[0]);
    TIFFSetField(tiff, TIFFTAG_YPOSITION, origin[1]);
  }

  // Write optional spacing as resolution
  if(metadata.spacing.has_value())
  {
    const FloatVec3& spacing = metadata.spacing.value();
    float32 xRes = (spacing[0] > 0.0f) ? (1.0f / spacing[0]) : 1.0f;
    float32 yRes = (spacing[1] > 0.0f) ? (1.0f / spacing[1]) : 1.0f;
    TIFFSetField(tiff, TIFFTAG_XRESOLUTION, xRes);
    TIFFSetField(tiff, TIFFTAG_YRESOLUTION, yRes);
    TIFFSetField(tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);
  }

  // Write scanlines
  usize rowBytes = static_cast<usize>(w) * static_cast<usize>(comp) * bpe;
  for(uint32_t row = 0; row < h; row++)
  {
    const uint8* rowData = buffer.data() + (static_cast<usize>(row) * rowBytes);
    // TIFFWriteScanline takes a non-const void* but does not modify the data
    if(TIFFWriteScanline(tiff, const_cast<uint8*>(rowData), row) < 0)
    {
      return MakeErrorResult(k_ErrorWriteFailed, fmt::format("Failed to write scanline {} to TIFF '{}': {}", row, pathStr, opts.errorMessage()));
    }
  }

  return {};
}
