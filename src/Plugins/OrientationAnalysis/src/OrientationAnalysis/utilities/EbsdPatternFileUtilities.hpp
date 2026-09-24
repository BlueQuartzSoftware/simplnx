#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace nx::core::EbsdPatternFileUtilities
{
inline constexpr int32 k_FileAccessError = -78000;
inline constexpr int32 k_ExactReadError = -78001;
inline constexpr int32 k_SizeOverflowError = -78002;

ORIENTATIONANALYSIS_EXPORT std::string NormalizeExtension(const std::filesystem::path& filePath);
ORIENTATIONANALYSIS_EXPORT Result<uint64> CheckedMultiply(uint64 lhs, uint64 rhs, const std::string& description, const std::filesystem::path& filePath);
ORIENTATIONANALYSIS_EXPORT Result<uint64> CheckedAdd(uint64 lhs, uint64 rhs, const std::string& description, const std::filesystem::path& filePath);
ORIENTATIONANALYSIS_EXPORT Result<usize> CheckedToSize(uint64 value, const std::string& description, const std::filesystem::path& filePath);
ORIENTATIONANALYSIS_EXPORT Result<std::vector<uint8>> ReadExactBytes(const std::filesystem::path& filePath, uint64 byteOffset, uint64 byteCount);
ORIENTATIONANALYSIS_EXPORT uint32 DecodeUInt32LittleEndian(const uint8* bytes);
ORIENTATIONANALYSIS_EXPORT uint64 DecodeUInt64LittleEndian(const uint8* bytes);
ORIENTATIONANALYSIS_EXPORT float64 DecodeFloat64LittleEndian(const uint8* bytes);
} // namespace nx::core::EbsdPatternFileUtilities
