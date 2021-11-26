#pragma once

#include <string>
#include <filesystem>
namespace fs = std::filesystem;

namespace complex
{

namespace StlConstants
{
constexpr size_t k_STL_HEADER_LENGTH = 80;


constexpr int32_t k_InputFileNotSet = -1100;
constexpr int32_t k_InputFileDoesNotExist = -1101;
constexpr int32_t k_UnsupportedFileType = -1102;
constexpr int32_t k_ErrorOpeningFile = -1103;
constexpr int32_t k_StlHeaderParseError = -1104;
constexpr int32_t k_TriangleCountParseError = -1105;
constexpr int32_t k_TriangleParseError = -1106;
constexpr int32_t k_AttributeParseError = -1107;
} // namespace ReadStlFileErrors

class StlUtilities
{

public:
  ~StlUtilities();
  StlUtilities(const StlUtilities&) = delete;
  StlUtilities(StlUtilities&&) noexcept = delete;
  StlUtilities& operator=(const StlUtilities&) = delete;
  StlUtilities& operator=(StlUtilities&&) noexcept = delete;


  // -----------------------------------------------------------------------------
  // Returns 0 for Binary, 1 for ASCII, anything else is an error.
  static int32_t DetermineStlFileType(const fs::path& path);

  /**
   * @brief Returns the number of triangles in the file according to the header. This
   * may OR may NOT be correct.
   * @param path The absolute file path to the STL File
   * @return Number of triangle faces
   */
  static int32_t NumFacesFromHeader(const fs::path& path);

protected:
  StlUtilities();
};

}

