#include "OrientationAnalysis/utilities/EbsdPatternFileUtilities.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <bit>
#include <cctype>
#include <fstream>
#include <limits>

namespace nx::core::EbsdPatternFileUtilities
{
std::string NormalizeExtension(const std::filesystem::path& filePath)
{
  std::string extension = filePath.extension().string();
  std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
  return extension;
}

Result<uint64> CheckedMultiply(uint64 lhs, uint64 rhs, const std::string& description, const std::filesystem::path& filePath)
{
  if(lhs != 0 && rhs > std::numeric_limits<uint64>::max() / lhs)
  {
    return MakeErrorResult<uint64>(k_SizeOverflowError,
                                   fmt::format("Cannot calculate {} for EBSD pattern file '{}': {} multiplied by {} exceeds the 64-bit size limit.", description, filePath.string(), lhs, rhs));
  }
  return {lhs * rhs};
}

Result<uint64> CheckedAdd(uint64 lhs, uint64 rhs, const std::string& description, const std::filesystem::path& filePath)
{
  if(rhs > std::numeric_limits<uint64>::max() - lhs)
  {
    return MakeErrorResult<uint64>(k_SizeOverflowError,
                                   fmt::format("Cannot calculate {} for EBSD pattern file '{}': {} plus {} exceeds the 64-bit size limit.", description, filePath.string(), lhs, rhs));
  }
  return {lhs + rhs};
}

Result<usize> CheckedToSize(uint64 value, const std::string& description, const std::filesystem::path& filePath)
{
  if(value > std::numeric_limits<usize>::max())
  {
    return MakeErrorResult<usize>(k_SizeOverflowError, fmt::format("Cannot represent {} ({}) for EBSD pattern file '{}' as a platform array size. Maximum supported value: {}.", description, value,
                                                                   filePath.string(), std::numeric_limits<usize>::max()));
  }
  return {static_cast<usize>(value)};
}

Result<std::vector<uint8>> ReadExactBytes(const std::filesystem::path& filePath, uint64 byteOffset, uint64 byteCount)
{
  std::ifstream stream(filePath, std::ios::binary);
  if(!stream.is_open())
  {
    return MakeErrorResult<std::vector<uint8>>(k_FileAccessError, fmt::format("Unable to open EBSD pattern file '{}'. Check that the file exists and is readable.", filePath.string()));
  }

  stream.seekg(static_cast<std::streamoff>(byteOffset), std::ios::beg);
  if(!stream.good())
  {
    return MakeErrorResult<std::vector<uint8>>(k_ExactReadError, fmt::format("Unable to seek to byte offset {} in EBSD pattern file '{}'.", byteOffset, filePath.string()));
  }

  std::vector<uint8> bytes(static_cast<usize>(byteCount));
  stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(byteCount));
  const uint64 actualByteCount = static_cast<uint64>(stream.gcount());
  if(actualByteCount != byteCount)
  {
    return MakeErrorResult<std::vector<uint8>>(k_ExactReadError, fmt::format("Could not read the requested bytes from EBSD pattern file '{}'. Byte offset: {}. Requested bytes: {}. Actual bytes: {}.",
                                                                             filePath.string(), byteOffset, byteCount, actualByteCount));
  }
  return {std::move(bytes)};
}

uint32 DecodeUInt32LittleEndian(const uint8* bytes)
{
  return static_cast<uint32>(bytes[0]) | (static_cast<uint32>(bytes[1]) << 8U) | (static_cast<uint32>(bytes[2]) << 16U) | (static_cast<uint32>(bytes[3]) << 24U);
}

uint64 DecodeUInt64LittleEndian(const uint8* bytes)
{
  uint64 value = 0;
  for(uint64 index = 0; index < 8; index++)
  {
    value |= static_cast<uint64>(bytes[index]) << (index * 8U);
  }
  return value;
}

float64 DecodeFloat64LittleEndian(const uint8* bytes)
{
  return std::bit_cast<float64>(DecodeUInt64LittleEndian(bytes));
}
} // namespace nx::core::EbsdPatternFileUtilities
