#include "MhaImageIO.hpp"

#include "simplnx/Common/TypesUtility.hpp"

#include <zlib.h>

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <string_view>

using namespace nx::core;

namespace
{
constexpr int32 k_ErrorReadUnsupported = -20300;
constexpr int32 k_ErrorUnsupportedDataType = -20301;
constexpr int32 k_ErrorUnsupportedComponentCount = -20302;
constexpr int32 k_ErrorBufferSizeMismatch = -20303;
constexpr int32 k_ErrorOpenFailed = -20304;
constexpr int32 k_ErrorCompressionFailed = -20305;
constexpr int32 k_ErrorWriteFailed = -20306;
constexpr usize k_CompressionBufferSize = 1024 * 1024;
constexpr int k_CompressedSizeFieldWidth = 20;

std::optional<std::string_view> MetaElementType(DataType dataType)
{
  switch(dataType)
  {
  case DataType::int8:
    return "MET_CHAR";
  case DataType::uint8:
    return "MET_UCHAR";
  case DataType::int16:
    return "MET_SHORT";
  case DataType::uint16:
    return "MET_USHORT";
  case DataType::int32:
    return "MET_INT";
  case DataType::uint32:
    return "MET_UINT";
  case DataType::int64:
    return "MET_LONG_LONG";
  case DataType::uint64:
    return "MET_ULONG_LONG";
  case DataType::float32:
    return "MET_FLOAT";
  case DataType::float64:
    return "MET_DOUBLE";
  default:
    return std::nullopt;
  }
}

class DeflateStream
{
public:
  DeflateStream()
  : m_InitResult(deflateInit(&m_Stream, Z_DEFAULT_COMPRESSION))
  {
  }

  ~DeflateStream() noexcept
  {
    if(m_InitResult == Z_OK)
    {
      deflateEnd(&m_Stream);
    }
  }

  DeflateStream(const DeflateStream&) = delete;
  DeflateStream(DeflateStream&&) noexcept = delete;
  DeflateStream& operator=(const DeflateStream&) = delete;
  DeflateStream& operator=(DeflateStream&&) noexcept = delete;

  int initResult() const
  {
    return m_InitResult;
  }

  z_stream& stream()
  {
    return m_Stream;
  }

private:
  z_stream m_Stream{};
  int m_InitResult = Z_STREAM_ERROR;
};

Result<uint64> WriteCompressedPayload(std::ofstream& output, std::span<const uint8> buffer, const std::filesystem::path& filePath)
{
  DeflateStream deflateStream;
  if(deflateStream.initResult() != Z_OK)
  {
    return MakeErrorResult<uint64>(k_ErrorCompressionFailed,
                                   fmt::format("Could not initialize zlib compression for MHA file '{}'. zlib returned error {}.", filePath.string(), deflateStream.initResult()));
  }

  z_stream& stream = deflateStream.stream();
  std::array<uint8, k_CompressionBufferSize> compressedBuffer{};
  usize inputOffset = 0;
  uint64 compressedSize = 0;
  int deflateResult = Z_OK;

  do
  {
    const usize remaining = buffer.size() - inputOffset;
    const usize inputSize = std::min(remaining, k_CompressionBufferSize);
    stream.next_in = inputSize == 0 ? nullptr : const_cast<Bytef*>(buffer.data() + inputOffset);
    stream.avail_in = static_cast<uInt>(inputSize);
    inputOffset += inputSize;
    const int flush = inputOffset == buffer.size() ? Z_FINISH : Z_NO_FLUSH;

    do
    {
      stream.next_out = compressedBuffer.data();
      stream.avail_out = static_cast<uInt>(compressedBuffer.size());
      deflateResult = deflate(&stream, flush);
      if(deflateResult != Z_OK && deflateResult != Z_STREAM_END)
      {
        return MakeErrorResult<uint64>(k_ErrorCompressionFailed, fmt::format("Could not compress pixel data for MHA file '{}'. zlib returned error {}.", filePath.string(), deflateResult));
      }

      const usize outputSize = compressedBuffer.size() - stream.avail_out;
      if(outputSize > 0)
      {
        output.write(reinterpret_cast<const char*>(compressedBuffer.data()), static_cast<std::streamsize>(outputSize));
        if(!output.good())
        {
          return MakeErrorResult<uint64>(k_ErrorWriteFailed, fmt::format("Could not write compressed pixel data to MHA file '{}'. Check the output path and available disk space.", filePath.string()));
        }
        compressedSize += outputSize;
      }
    } while(stream.avail_out == 0 || (flush == Z_FINISH && deflateResult != Z_STREAM_END));
  } while(deflateResult != Z_STREAM_END);

  return {compressedSize};
}
} // namespace

Result<ImageMetadata> MhaImageIO::readMetadata(const std::filesystem::path& filePath) const
{
  return MakeErrorResult<ImageMetadata>(k_ErrorReadUnsupported,
                                        fmt::format("MHA input through the ImageIO backend is not supported for '{}'. Use the 'Read MHA/MetaImage File' filter.", filePath.string()));
}

Result<> MhaImageIO::readPixelData(const std::filesystem::path& filePath, std::span<uint8> buffer, usize pageIndex) const
{
  return MakeErrorResult(k_ErrorReadUnsupported,
                         fmt::format("MHA input through the ImageIO backend is not supported for '{}' at page index {}. Use the 'Read MHA/MetaImage File' filter.", filePath.string(), pageIndex));
}

Result<> MhaImageIO::readPixelDataRows(const std::filesystem::path& filePath, const ReadRowCallback& callback, usize pageIndex) const
{
  return MakeErrorResult(k_ErrorReadUnsupported,
                         fmt::format("MHA input through the ImageIO backend is not supported for '{}' at page index {}. Use the 'Read MHA/MetaImage File' filter.", filePath.string(), pageIndex));
}

Result<> MhaImageIO::writePixelData(const std::filesystem::path& filePath, std::span<const uint8> buffer, const ImageMetadata& metadata) const
{
  const std::optional<std::string_view> elementType = MetaElementType(metadata.dataType);
  if(!elementType.has_value())
  {
    return MakeErrorResult(k_ErrorUnsupportedDataType,
                           fmt::format("The MHA format cannot write '{}' pixel data to '{}'. Supported data types are int8, uint8, int16, uint16, int32, uint32, int64, uint64, float32, and float64.",
                                       DataTypeToString(metadata.dataType).str(), filePath.string()));
  }

  if(!supportedWriteComponentCounts().contains(metadata.numComponents))
  {
    return MakeErrorResult(k_ErrorUnsupportedComponentCount, fmt::format("The MHA format cannot write {} components per pixel to '{}'. Supported component counts are 1, 2, 3, 4, 10, 11, and 36.",
                                                                         metadata.numComponents, filePath.string()));
  }

  const usize expectedSize = metadata.width * metadata.height * metadata.numComponents * GetDataTypeSize(metadata.dataType);
  if(buffer.size() != expectedSize)
  {
    return MakeErrorResult(k_ErrorBufferSizeMismatch,
                           fmt::format("The pixel buffer for MHA file '{}' contains {} bytes, but the {} x {} image with {} components of type '{}' requires {} bytes.", filePath.string(),
                                       buffer.size(), metadata.width, metadata.height, metadata.numComponents, DataTypeToString(metadata.dataType).str(), expectedSize));
  }

  std::ofstream output(filePath, std::ios::binary | std::ios::trunc);
  if(!output.is_open())
  {
    return MakeErrorResult(k_ErrorOpenFailed, fmt::format("Could not open MHA file '{}' for writing. Check the output path and write permissions.", filePath.string()));
  }

  const FloatVec3 origin = metadata.origin.value_or(FloatVec3{0.0f, 0.0f, 0.0f});
  const FloatVec3 spacing = metadata.spacing.value_or(FloatVec3{1.0f, 1.0f, 1.0f});
  output << std::setprecision(std::numeric_limits<float32>::max_digits10);
  output << "ObjectType = Image\n";
  output << "NDims = 2\n";
  output << "BinaryData = True\n";
  output << "BinaryDataByteOrderMSB = " << (std::endian::native == std::endian::big ? "True" : "False") << '\n';
  output << "CompressedData = True\n";
  output << "TransformMatrix = 1 0 0 1\n";
  output << "Offset = " << origin[0] << ' ' << origin[1] << '\n';
  output << "CenterOfRotation = 0 0\n";
  output << "ElementSpacing = " << spacing[0] << ' ' << spacing[1] << '\n';
  output << "DimSize = " << metadata.width << ' ' << metadata.height << '\n';
  output << "ElementNumberOfChannels = " << metadata.numComponents << '\n';
  output << "ElementType = " << elementType.value() << '\n';
  output << "CompressedDataSize = ";
  const std::streampos compressedSizePosition = output.tellp();
  output << std::setw(k_CompressedSizeFieldWidth) << std::setfill('0') << 0 << '\n';
  output << "ElementDataFile = LOCAL\n";

  Result<uint64> compressedResult = WriteCompressedPayload(output, buffer, filePath);
  if(compressedResult.invalid())
  {
    return ConvertResult(std::move(compressedResult));
  }

  output.seekp(compressedSizePosition);
  output << std::setw(k_CompressedSizeFieldWidth) << std::setfill('0') << compressedResult.value();
  output.flush();
  if(!output.good())
  {
    return MakeErrorResult(k_ErrorWriteFailed, fmt::format("Could not finalize MHA file '{}'. Check the output path and available disk space.", filePath.string()));
  }
  return {};
}

std::set<DataType> MhaImageIO::supportedWriteDataTypes() const
{
  return {DataType::int8, DataType::uint8, DataType::int16, DataType::uint16, DataType::int32, DataType::uint32, DataType::int64, DataType::uint64, DataType::float32, DataType::float64};
}

std::set<usize> MhaImageIO::supportedWriteComponentCounts() const
{
  return {1, 2, 3, 4, 10, 11, 36};
}
