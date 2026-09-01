#include "StbImageIO.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <fmt/format.h>

#include <cstring>

using namespace nx::core;

namespace
{
constexpr int32 k_ErrorInfoFailed = -20000;
constexpr int32 k_ErrorLoadFailed = -20001;
constexpr int32 k_ErrorWriteFailed = -20002;
constexpr int32 k_ErrorUnsupportedWriteFormat = -20003;
constexpr int32 k_ErrorUnsupportedDataType = -20004;
constexpr int32 k_ErrorBufferSizeMismatch = -20005;
} // namespace

Result<ImageMetadata> StbImageIO::readMetadata(const std::filesystem::path& filePath) const
{
  std::string pathStr = filePath.string();

  int width = 0;
  int height = 0;
  int comp = 0;
  int result = stbi_info(pathStr.c_str(), &width, &height, &comp);
  if(result == 0)
  {
    const char* reason = stbi_failure_reason();
    return MakeErrorResult<ImageMetadata>(k_ErrorInfoFailed, fmt::format("Failed to read image info from '{}': {}", pathStr, reason != nullptr ? reason : "unknown error"));
  }

  ImageMetadata metadata;
  metadata.width = static_cast<usize>(width);
  metadata.height = static_cast<usize>(height);
  metadata.numComponents = static_cast<usize>(comp);

  if(stbi_is_hdr(pathStr.c_str()) != 0)
  {
    metadata.dataType = DataType::float32;
  }
  else if(stbi_is_16_bit(pathStr.c_str()) != 0)
  {
    metadata.dataType = DataType::uint16;
  }
  else
  {
    metadata.dataType = DataType::uint8;
  }

  metadata.numPages = 1;
  // stb does not expose origin or spacing metadata.
  metadata.origin = std::nullopt;
  metadata.spacing = std::nullopt;

  return {std::move(metadata)};
}

Result<> StbImageIO::readPixelData(const std::filesystem::path& filePath, std::span<uint8> buffer) const
{
  Result<ImageMetadata> metaResult = readMetadata(filePath);
  if(metaResult.invalid())
  {
    return ConvertResult(std::move(metaResult));
  }
  const ImageMetadata& metadata = metaResult.value();

  usize bpe = GetDataTypeSize(metadata.dataType);
  usize expectedSize = metadata.width * metadata.height * metadata.numComponents * bpe;

  if(buffer.size() != expectedSize)
  {
    return MakeErrorResult(k_ErrorBufferSizeMismatch, fmt::format("Buffer size {} does not match expected size {} for image '{}'", buffer.size(), expectedSize, filePath.string()));
  }

  const usize rowBytes = metadata.width * metadata.numComponents * bpe;
  return readPixelDataRows(filePath, [&](usize row, usize columnOffset, usize pixelCount, std::span<const uint8> pixels) -> Result<> {
    const usize byteOffset = row * rowBytes + columnOffset * metadata.numComponents * bpe;
    const usize byteCount = pixelCount * metadata.numComponents * bpe;
    std::memcpy(buffer.data() + byteOffset, pixels.data(), byteCount);
    return {};
  });
}

Result<> StbImageIO::readPixelDataRows(const std::filesystem::path& filePath, const ReadRowCallback& callback) const
{
  Result<ImageMetadata> metaResult = readMetadata(filePath);
  if(metaResult.invalid())
  {
    return ConvertResult(std::move(metaResult));
  }
  const ImageMetadata& metadata = metaResult.value();

  const std::string pathStr = filePath.string();
  int width = 0;
  int height = 0;
  int comp = 0;
  void* decodedData = nullptr;

  if(metadata.dataType == DataType::float32)
  {
    decodedData = stbi_loadf(pathStr.c_str(), &width, &height, &comp, 0);
  }
  else if(metadata.dataType == DataType::uint16)
  {
    decodedData = stbi_load_16(pathStr.c_str(), &width, &height, &comp, 0);
  }
  else
  {
    decodedData = stbi_load(pathStr.c_str(), &width, &height, &comp, 0);
  }

  if(decodedData == nullptr)
  {
    const char* reason = stbi_failure_reason();
    return MakeErrorResult(k_ErrorLoadFailed, fmt::format("Failed to load image '{}': {}", pathStr, reason != nullptr ? reason : "unknown error"));
  }

  const usize bytesPerElement = GetDataTypeSize(metadata.dataType);
  const usize rowBytes = metadata.width * metadata.numComponents * bytesPerElement;
  const auto* bytes = static_cast<const uint8*>(decodedData);
  Result<> result;
  for(usize row = 0; row < metadata.height; ++row)
  {
    result = callback(row, 0, metadata.width, std::span<const uint8>(bytes + row * rowBytes, rowBytes));
    if(result.invalid())
    {
      break;
    }
  }
  stbi_image_free(decodedData);
  return result;
}

Result<> StbImageIO::writePixelData(const std::filesystem::path& filePath, std::span<const uint8> buffer, const ImageMetadata& metadata) const
{
  if(metadata.dataType != DataType::uint8)
  {
    return MakeErrorResult(k_ErrorUnsupportedDataType, fmt::format("stb_image_write only supports uint8 pixel data for writing. Got unsupported data type for '{}'.", filePath.string()));
  }

  std::string pathStr = filePath.string();
  const std::string ext = nx::core::StringUtilities::toLower(filePath.extension().string());

  int w = static_cast<int>(metadata.width);
  int h = static_cast<int>(metadata.height);
  int comp = static_cast<int>(metadata.numComponents);
  const void* data = buffer.data();

  int result = 0;

  if(ext == ".png")
  {
    int strideBytes = w * comp;
    result = stbi_write_png(pathStr.c_str(), w, h, comp, data, strideBytes);
  }
  else if(ext == ".bmp")
  {
    result = stbi_write_bmp(pathStr.c_str(), w, h, comp, data);
  }
  else if(ext == ".jpg" || ext == ".jpeg")
  {
    constexpr int k_JpegQuality = 95;
    result = stbi_write_jpg(pathStr.c_str(), w, h, comp, data, k_JpegQuality);
  }
  else
  {
    return MakeErrorResult(k_ErrorUnsupportedWriteFormat, fmt::format("Unsupported write format '{}' for stb backend. Supported: .png, .bmp, .jpg, .jpeg", ext));
  }

  if(result == 0)
  {
    return MakeErrorResult(k_ErrorWriteFailed, fmt::format("Failed to write image to '{}'", pathStr));
  }

  return {};
}

std::set<DataType> StbImageIO::supportedWriteDataTypes() const
{
  return {DataType::uint8};
}

std::set<usize> StbImageIO::supportedWriteComponentCounts() const
{
  // stbi_write_png_to_mem() indexes a five-entry table with the component count.
  // Counts above four cause an out-of-bounds read. stbi_write_jpg_core() accepts
  // only one through four. stbi_write_bmp_core() produces conforming output for
  // one, three, or four components.
  // BMP and JPEG discard the second channel of two-component input. Thus, the
  // conforming component-count intersection for all three formats is {1, 3, 4}.
  return {1, 3, 4};
}
