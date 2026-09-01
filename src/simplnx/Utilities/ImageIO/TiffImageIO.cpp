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
constexpr int32_t k_ErrorOpenFailed = -20100;
constexpr int32_t k_ErrorReadMetadataFailed = -20101;
constexpr int32_t k_ErrorReadPixelFailed = -20102;
constexpr int32_t k_ErrorWriteFailed = -20103;
constexpr int32_t k_ErrorUnsupportedFormat = -20104;
constexpr int32_t k_ErrorBufferSizeMismatch = -20105;

/**
 * @class TiffFile
 * @brief Owns one TIFF handle and its per-handle diagnostic state.
 *
 * The constructor owns the open-options sequence and frees the options after open.
 * libtiff copies the options into the TIFF handle. libtiff stores the address of
 * m_ErrorMessage as user data. Therefore, this object is not copyable or movable.
 */
class TiffFile
{
public:
  /**
   * @brief Opens one TIFF file with local error and warning handlers.
   * @param pathStr Supplies the narrow native file path.
   * @param mode Supplies the libtiff open mode.
   */
  TiffFile(const std::string& pathStr, const char* mode)
  {
    TIFFOpenOptions* opts = TIFFOpenOptionsAlloc();
    if(opts == nullptr)
    {
      m_ErrorMessage = "TIFFOpenOptionsAlloc failed";
      return;
    }
    TIFFOpenOptionsSetErrorHandlerExtR(opts, errorHandler, &m_ErrorMessage);
    TIFFOpenOptionsSetWarningHandlerExtR(opts, warningHandler, nullptr);
    m_Tiff = TIFFOpenExt(pathStr.c_str(), mode, opts);
    TIFFOpenOptionsFree(opts);
  }

  ~TiffFile() noexcept
  {
    if(m_Tiff != nullptr)
    {
      TIFFClose(m_Tiff);
    }
  }

  TiffFile(const TiffFile&) = delete;
  TiffFile& operator=(const TiffFile&) = delete;
  TiffFile(TiffFile&&) = delete;
  TiffFile& operator=(TiffFile&&) = delete;

  TIFF* get() const
  {
    return m_Tiff;
  }

  bool valid() const
  {
    return m_Tiff != nullptr;
  }

  // Use a stable fallback when libtiff fails without a diagnostic string.
  std::string_view errorMessage() const
  {
    return m_ErrorMessage.empty() ? std::string_view{"unknown error"} : std::string_view{m_ErrorMessage};
  }

private:
  // Store the diagnostic in this handle's user data. Returning one prevents
  // libtiff from forwarding the same error to its global stderr handler.
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

  // Suppress optional libtiff warnings so library consumers receive Result diagnostics only.
  static int warningHandler(TIFF* /*tif*/, void* /*user_data*/, const char* /*module*/, const char* /*formatStr*/, va_list /*args*/)
  {
    return 1;
  }

  TIFF* m_Tiff = nullptr;
  std::string m_ErrorMessage;
};

/**
 * @brief Converts supported TIFF sample tags to a simplnx data type.
 * @param tiff Supplies an open TIFF handle.
 * @return uint8, uint16, or float32, or an unsupported-format error.
 * @pre tiff is valid.
 *
 * The function rejects bilevel, palette, int32, float64, and unknown combinations.
 * It does not silently convert them to uint8.
 */
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

Result<ImageMetadata> TiffImageIO::readMetadata(const std::filesystem::path& filePath) const
{
  std::string pathStr = filePath.string();
  TiffFile tiffFile(pathStr, "r");
  if(!tiffFile.valid())
  {
    return MakeErrorResult<ImageMetadata>(k_ErrorOpenFailed, fmt::format("Failed to open TIFF file '{}': {}", pathStr, tiffFile.errorMessage()));
  }

  TIFF* tiff = tiffFile.get();

  uint32_t width = 0;
  uint32_t height = 0;
  if(TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width) == 0 || TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height) == 0)
  {
    return MakeErrorResult<ImageMetadata>(k_ErrorReadMetadataFailed, fmt::format("Failed to read TIFF dimensions from '{}': {}", pathStr, tiffFile.errorMessage()));
  }

  // TIFF defines a default of one, but this backend requires an explicit tag.
  // This restriction avoids treating malformed multi-channel data as grayscale.
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

  // X and Y position tags supply optional origin metadata.
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

  // X and Y resolution tags supply optional spacing metadata.
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
    // Resolution is pixels per unit. Spacing is its inverse.
    float32 xSpacing = (xRes > 0.0f) ? (1.0f / xRes) : 1.0f;
    float32 ySpacing = (yRes > 0.0f) ? (1.0f / yRes) : 1.0f;
    metadata.spacing = FloatVec3(xSpacing, ySpacing, 1.0f);
  }

  return {std::move(metadata)};
}

Result<> TiffImageIO::readPixelData(const std::filesystem::path& filePath, std::span<uint8> buffer) const
{
  Result<ImageMetadata> metadataResult = readMetadata(filePath);
  if(metadataResult.invalid())
  {
    return ConvertResult(std::move(metadataResult));
  }
  const ImageMetadata& metadata = metadataResult.value();
  const usize bytesPerElement = GetDataTypeSize(metadata.dataType);
  const usize rowBytes = metadata.width * metadata.numComponents * bytesPerElement;
  const usize expectedSize = rowBytes * metadata.height;
  if(buffer.size() != expectedSize)
  {
    return MakeErrorResult(k_ErrorBufferSizeMismatch, fmt::format("Buffer size {} does not match expected size {} for TIFF image '{}'", buffer.size(), expectedSize, filePath.string()));
  }

  return readPixelDataRows(filePath, [&](usize row, usize columnOffset, usize pixelCount, std::span<const uint8> pixels) -> Result<> {
    const usize byteOffset = row * rowBytes + columnOffset * metadata.numComponents * bytesPerElement;
    const usize byteCount = pixelCount * metadata.numComponents * bytesPerElement;
    std::memcpy(buffer.data() + byteOffset, pixels.data(), byteCount);
    return {};
  });
}

Result<> TiffImageIO::readPixelDataRows(const std::filesystem::path& filePath, const ReadRowCallback& callback) const
{
  const std::string pathStr = filePath.string();
  TiffFile tiffFile(pathStr, "r");
  if(!tiffFile.valid())
  {
    return MakeErrorResult(k_ErrorOpenFailed, fmt::format("Failed to open TIFF file '{}': {}", pathStr, tiffFile.errorMessage()));
  }

  TIFF* tiff = tiffFile.get();
  uint32_t width = 0;
  uint32_t height = 0;
  uint16_t samplesPerPixel = 0;
  if(TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width) == 0 || TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height) == 0 || TIFFGetField(tiff, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel) == 0)
  {
    return MakeErrorResult(k_ErrorReadMetadataFailed, fmt::format("Failed to read required TIFF dimensions/components from '{}': {}", pathStr, tiffFile.errorMessage()));
  }

  Result<DataType> dataTypeResult = DetermineTiffDataType(tiff);
  if(dataTypeResult.invalid())
  {
    return ConvertResult(std::move(dataTypeResult));
  }
  const usize bytesPerElement = GetDataTypeSize(dataTypeResult.value());
  const usize pixelBytes = static_cast<usize>(samplesPerPixel) * bytesPerElement;

  uint16_t planarConfig = PLANARCONFIG_CONTIG;
  TIFFGetFieldDefaulted(tiff, TIFFTAG_PLANARCONFIG, &planarConfig);
  if(planarConfig != PLANARCONFIG_CONTIG)
  {
    return MakeErrorResult(k_ErrorUnsupportedFormat, fmt::format("Planar-separate TIFF data is not supported for '{}'.", pathStr));
  }

  if(TIFFIsTiled(tiff) != 0)
  {
    uint32_t tileWidth = 0;
    uint32_t tileHeight = 0;
    if(TIFFGetField(tiff, TIFFTAG_TILEWIDTH, &tileWidth) == 0 || TIFFGetField(tiff, TIFFTAG_TILELENGTH, &tileHeight) == 0 || tileWidth == 0 || tileHeight == 0)
    {
      return MakeErrorResult(k_ErrorReadMetadataFailed, fmt::format("Failed to read valid TIFF tile dimensions from '{}': {}", pathStr, tiffFile.errorMessage()));
    }

    const tmsize_t encodedTileSize = TIFFTileSize(tiff);
    const usize tileRowBytes = static_cast<usize>(tileWidth) * pixelBytes;
    const usize computedTileSize = tileRowBytes * static_cast<usize>(tileHeight);
    if(encodedTileSize <= 0)
    {
      return MakeErrorResult(k_ErrorReadMetadataFailed, fmt::format("TIFFTileSize returned {} for '{}': {}", encodedTileSize, pathStr, tiffFile.errorMessage()));
    }
    // One reusable tile buffer bounds memory for tiled input. The computed size
    // protects row access when libtiff reports a smaller encoded size.
    std::vector<uint8> tileBuffer(std::max(static_cast<usize>(encodedTileSize), computedTileSize));

    for(uint32_t tileY = 0; tileY < height; tileY += tileHeight)
    {
      for(uint32_t tileX = 0; tileX < width; tileX += tileWidth)
      {
        if(TIFFReadTile(tiff, tileBuffer.data(), tileX, tileY, 0, 0) < 0)
        {
          return MakeErrorResult(k_ErrorReadPixelFailed, fmt::format("Failed to read tile at ({}, {}) from TIFF '{}': {}", tileX, tileY, pathStr, tiffFile.errorMessage()));
        }

        const usize validWidth = std::min<usize>(tileWidth, static_cast<usize>(width - tileX));
        const usize validHeight = std::min<usize>(tileHeight, static_cast<usize>(height - tileY));
        for(usize localRow = 0; localRow < validHeight; ++localRow)
        {
          const uint8* rowData = tileBuffer.data() + localRow * tileRowBytes;
          Result<> result = callback(static_cast<usize>(tileY) + localRow, tileX, validWidth, std::span<const uint8>(rowData, validWidth * pixelBytes));
          if(result.invalid())
          {
            return result;
          }
        }
      }
    }
    return {};
  }

  // TIFFScanlineSize has a signed return type. Reject nonpositive values before
  // conversion to usize. Otherwise, -1 can request an extremely large allocation.
  tsize_t scanlineSize = TIFFScanlineSize(tiff);
  if(scanlineSize <= 0)
  {
    return MakeErrorResult(k_ErrorReadMetadataFailed, fmt::format("TIFFScanlineSize returned {} for '{}': {}", scanlineSize, pathStr, tiffFile.errorMessage()));
  }
  usize rowBytes = static_cast<usize>(width) * pixelBytes;

  // libtiff can include row padding. Allocate the larger reported or packed row size.
  usize scanlineBufSize = std::max(static_cast<usize>(scanlineSize), rowBytes);
  std::vector<uint8> scanlineBuf(scanlineBufSize);

  for(uint32_t row = 0; row < height; row++)
  {
    if(TIFFReadScanline(tiff, scanlineBuf.data(), row) < 0)
    {
      return MakeErrorResult(k_ErrorReadPixelFailed, fmt::format("Failed to read scanline {} from TIFF '{}': {}", row, pathStr, tiffFile.errorMessage()));
    }
    Result<> result = callback(row, 0, width, std::span<const uint8>(scanlineBuf.data(), rowBytes));
    if(result.invalid())
    {
      return result;
    }
  }

  return {};
}

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
  TiffFile tiffFile(pathStr, "w");
  if(!tiffFile.valid())
  {
    return MakeErrorResult(k_ErrorOpenFailed, fmt::format("Failed to open TIFF file for writing '{}': {}", pathStr, tiffFile.errorMessage()));
  }

  TIFF* tiff = tiffFile.get();

  uint32_t w = static_cast<uint32_t>(metadata.width);
  uint32_t h = static_cast<uint32_t>(metadata.height);
  uint16_t comp = static_cast<uint16_t>(metadata.numComponents);

  // TIFFSetField returns zero on failure. Most required-tag failures indicate an unwritable handle.
  if(TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, w) == 0 || TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, h) == 0 || TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, comp) == 0 ||
     TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT) == 0 || TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG) == 0 ||
     TIFFSetField(tiff, TIFFTAG_COMPRESSION, COMPRESSION_LZW) == 0 || TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tiff, 0)) == 0)
  {
    return MakeErrorResult(k_ErrorWriteFailed, fmt::format("Failed to set required TIFF tags for '{}': {}", pathStr, tiffFile.errorMessage()));
  }

  // The value type determines the bit width and TIFF sample format.
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
    return MakeErrorResult(k_ErrorWriteFailed, fmt::format("Failed to set TIFF sample format tags for '{}': {}", pathStr, tiffFile.errorMessage()));
  }

  // One component is grayscale. Three and four components use RGB photometric interpretation.
  uint16_t photometric = (comp == 1) ? PHOTOMETRIC_MINISBLACK : PHOTOMETRIC_RGB;
  if(TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, photometric) == 0)
  {
    return MakeErrorResult(k_ErrorWriteFailed, fmt::format("Failed to set TIFF photometric tag for '{}': {}", pathStr, tiffFile.errorMessage()));
  }

  // Declare the fourth RGBA sample as unassociated alpha. Otherwise, readers can
  // interpret it as an undefined extra channel.
  if(comp == 4)
  {
    const uint16_t extraSamples[1] = {EXTRASAMPLE_UNASSALPHA};
    TIFFSetField(tiff, TIFFTAG_EXTRASAMPLES, 1, extraSamples);
  }

  // Position tags are optional. A tag failure does not invalidate pixel output.
  if(metadata.origin.has_value())
  {
    const FloatVec3& origin = metadata.origin.value();
    TIFFSetField(tiff, TIFFTAG_XPOSITION, origin[0]);
    TIFFSetField(tiff, TIFFTAG_YPOSITION, origin[1]);
  }

  // Store positive spacing as its pixels-per-unit resolution.
  if(metadata.spacing.has_value())
  {
    const FloatVec3& spacing = metadata.spacing.value();
    float32 xRes = (spacing[0] > 0.0f) ? (1.0f / spacing[0]) : 1.0f;
    float32 yRes = (spacing[1] > 0.0f) ? (1.0f / spacing[1]) : 1.0f;
    TIFFSetField(tiff, TIFFTAG_XRESOLUTION, xRes);
    TIFFSetField(tiff, TIFFTAG_YRESOLUTION, yRes);
    TIFFSetField(tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);
  }

  usize rowBytes = static_cast<usize>(w) * static_cast<usize>(comp) * bpe;
  for(uint32_t row = 0; row < h; row++)
  {
    const uint8* rowData = buffer.data() + (static_cast<usize>(row) * rowBytes);
    // libtiff does not modify the row, but its C API accepts a non-const pointer.
    if(TIFFWriteScanline(tiff, const_cast<uint8*>(rowData), row) < 0)
    {
      return MakeErrorResult(k_ErrorWriteFailed, fmt::format("Failed to write scanline {} to TIFF '{}': {}", row, pathStr, tiffFile.errorMessage()));
    }
  }

  return {};
}

std::set<DataType> TiffImageIO::supportedWriteDataTypes() const
{
  return {DataType::uint8, DataType::uint16, DataType::float32};
}

std::set<usize> TiffImageIO::supportedWriteComponentCounts() const
{
  // Supported layouts are grayscale, RGB, and RGBA with declared alpha.
  return {1, 3, 4};
}
