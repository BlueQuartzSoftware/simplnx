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
inline constexpr int32_t k_StlFileLengthError = -1108;

enum class StlFileType : int
{
  Binary = 0,
  ASCI = 1,
  FileOpenError = 2,
  HeaderParseError = 3
};
} // namespace StlConstants

namespace StlUtilities
{
// -----------------------------------------------------------------------------
/**
 * @brief This function will determine if the given STL file is ASCII or BINARY.
 *
 * This could give a false positive for BINARY for _any_ file that doesn't have
 * the first few lines of a valid ASCII STL file.
 * @param path The path to the file to check
 * @return Enumeration that represents either the type of file or a possible parsing error
 */
StlConstants::StlFileType DetermineStlFileType(const fs::path& path);

/**
 * @brief Returns the number of triangles in the file according to the header. This
 * may OR may NOT be correct.
 * @param path The absolute file path to the STL File
 * @return Number of triangle faces
 */
int32_t NumFacesFromHeader(const fs::path& path);

/**
 * @brief A very basic function to convert a well behaved ASCII STL File into a binary STL file
 * @param inputPath The input ASCII STL File
 * @param outputPath The output Binary STL file
 */
void ConvertAsciiToBinaryStl(const std::filesystem::path& inputPath, const std::filesystem::path& outputPath);

} // namespace StlUtilities
} // namespace nx::core
