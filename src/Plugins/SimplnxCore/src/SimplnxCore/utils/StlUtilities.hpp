#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>
#include <tuple>

namespace fs = std::filesystem;

namespace nx::core
{
namespace StlConstants
{
inline constexpr size_t k_STL_HEADER_LENGTH = 80;

/**
 * @brief The number of float32 values that make up a single binary STL triangle record:
 * 3 for the facet normal plus 3 each for the 3 vertices.
 */
inline constexpr size_t k_StlElementCount = 12;

/**
 * @brief The size, in bytes, of the fixed portion at the front of a binary STL file: the
 * 80 byte header followed by the uint32 triangle count. (80 + 4 = 84)
 */
inline constexpr uintmax_t k_StlFixedHeaderBytes = k_STL_HEADER_LENGTH + sizeof(uint32_t);

/**
 * @brief The size, in bytes, of a single binary STL triangle record: 12 float32 values
 * followed by the uint16 "attribute byte count". (48 + 2 = 50)
 */
inline constexpr uintmax_t k_StlTriangleBytes = (k_StlElementCount * sizeof(float)) + sizeof(uint16_t);

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

/**
 * @brief The result of StlUtilities::SanityCheckFile().
 *
 * If @c error is non-zero the file could not be inspected and every other member is left
 * at its default value; @c errorMessage describes the failure. If @c error is zero then
 * the remaining members describe the file.
 */
struct SIMPLNXCORE_EXPORT StlFileCheck
{
  /** @brief Zero on success, otherwise one of the StlConstants error codes. */
  int32_t error = 0;
  /** @brief Human readable description of the failure. Empty on success. */
  std::string errorMessage;
  /** @brief The raw 80 byte file header. */
  std::string header;
  /** @brief The triangle count declared by the file header. Always >= 0 on success. */
  int32_t numTriangles = 0;
  /** @brief The size of the file on disk, in bytes. */
  uintmax_t fileSize = 0;
  /**
   * @brief True when the file actually carries per-triangle attribute payload bytes and the
   * reader should therefore honor each triangle's "attribute byte count" field. When false
   * the reader must ignore that field entirely.
   */
  bool attributePayloadPresent = false;
};
} // namespace StlConstants

namespace StlUtilities
{
/**
 * @brief RAII guard that closes a C @c FILE* when it leaves scope.
 */
class SIMPLNXCORE_EXPORT StlFileSentinel
{
public:
  explicit StlFileSentinel(FILE* file)
  : m_File(file)
  {
  }
  ~StlFileSentinel() noexcept
  {
    if(m_File != nullptr)
    {
      std::ignore = std::fclose(m_File);
    }
  }
  StlFileSentinel(const StlFileSentinel&) = delete;            // Copy Constructor Not Implemented
  StlFileSentinel(StlFileSentinel&&) = delete;                 // Move Constructor Not Implemented
  StlFileSentinel& operator=(const StlFileSentinel&) = delete; // Copy Assignment Not Implemented
  StlFileSentinel& operator=(StlFileSentinel&&) = delete;      // Move Assignment Not Implemented

private:
  FILE* m_File = nullptr;
};

// -----------------------------------------------------------------------------
/**
 * @brief This function will determine if the given STL file is ASCII or BINARY.
 *
 * This could give a false positive for BINARY for _any_ file that doesn't have
 * the first few lines of a valid ASCII STL file.
 * @param path The path to the file to check
 * @return Enumeration that represents either the type of file or a possible parsing error
 */
SIMPLNXCORE_EXPORT StlConstants::StlFileType DetermineStlFileType(const fs::path& path);

/**
 * @brief Returns the number of triangles in the file according to the header. This
 * may OR may NOT be correct.
 * @param path The absolute file path to the STL File
 * @return Number of triangle faces
 */
SIMPLNXCORE_EXPORT int32_t NumFacesFromHeader(const fs::path& path);

/**
 * @brief This function will return whether or not the 2 byte Triangle attribute byte count
 * value should be honored or not.
 *
 * The binary STL specification places a uint16 "attribute byte count" after each triangle that
 * states how many vendor specific bytes follow it. Many writers instead abuse that field to
 * store something else entirely: Magics Materialise packs a per-facet RGB color into it and
 * VxElements leaves garbage in it, while neither writes any payload bytes at all. Seeking
 * forward by those values would desynchronize the read stream and produce garbage geometry, so
 * the reader has to determine up front whether payload bytes are really present.
 *
 * That decision is made from the file size, which cannot be fooled by the contents of any
 * individual triangle:
 * - A file of exactly @c k_StlFixedHeaderBytes + numTriangles * @c k_StlTriangleBytes has no
 *   room for payload, so the attribute fields are ignored no matter what they contain.
 * - A file smaller than that is truncated and is reported as @c k_StlFileLengthError.
 * - A larger file does carry extra bytes, so the attribute fields are honored, unless the
 *   header identifies a vendor known to misuse them.
 *
 * @param path The path to the input STL file
 * @return StlFileCheck struct. Check @c error before using any other member.
 */
SIMPLNXCORE_EXPORT StlConstants::StlFileCheck SanityCheckFile(const fs::path& path);

/**
 * @brief A very basic function to convert a well behaved ASCII STL File into a binary STL file
 * @param inputPath The input ASCII STL File
 * @param outputPath The output Binary STL file
 */
SIMPLNXCORE_EXPORT void ConvertAsciiToBinaryStl(const std::filesystem::path& inputPath, const std::filesystem::path& outputPath);

} // namespace StlUtilities
} // namespace nx::core
