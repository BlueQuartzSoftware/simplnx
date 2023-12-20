#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace nx::core
{
namespace StlConstants
{
inline constexpr size_t k_STL_HEADER_LENGTH = 80;

inline constexpr int32_t k_InputFileNotSet = -1100;
inline constexpr int32_t k_InputFileDoesNotExist = -1101;
inline constexpr int32_t k_UnsupportedFileType = -1102;
inline constexpr int32_t k_ErrorOpeningFile = -1103;
inline constexpr int32_t k_StlHeaderParseError = -1104;
inline constexpr int32_t k_TriangleCountParseError = -1105;
inline constexpr int32_t k_TriangleParseError = -1106;
inline constexpr int32_t k_AttributeParseError = -1107;
} // namespace StlConstants

namespace StlUtilities
{
// -----------------------------------------------------------------------------
// Returns 0 for Binary, 1 for ASCII, anything else is an error.
int32_t DetermineStlFileType(const fs::path& path);

/**
 * @brief Returns the number of triangles in the file according to the header. This
 * may OR may NOT be correct.
 * @param path The absolute file path to the STL File
 * @return Number of triangle faces
 */
int32_t NumFacesFromHeader(const fs::path& path);
} // namespace StlUtilities
} // namespace nx::core
