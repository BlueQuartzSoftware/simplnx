#include "StlUtilities.hpp"

#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/format.h>

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>
#include <tuple>
#include <vector>

using namespace nx::core;

namespace
{
/**
 * @brief Looks for the tell-tale signs that the file was written by Magics Materialise.
 *
 * If the file was written by Magics as a "Color STL" file then the 2 byte int value that
 * follows each triangle will be NON-Zero. That value does NOT indicate a length, it is a
 * color packed into the field. Instead of being normal like everyone else and using the
 * STL spec they went off and did their own thing.
 */
bool IsMagicsFile(const std::string& stlHeaderStr)
{
  static const std::string k_ColorHeader("COLOR=");
  static const std::string k_MaterialHeader("MATERIAL=");
  return stlHeaderStr.find(k_ColorHeader) != std::string::npos && stlHeaderStr.find(k_MaterialHeader) != std::string::npos;
}

/**
 * @brief Looks for the tell-tale signs that the file was written by VxElements Creaform.
 *
 * These files do not honor the last 2 bytes of the 50 byte Triangle struct as specified in
 * the STL Binary File specification, so those 2 bytes are not treated as meaningful.
 */
bool IsVxElementsFile(const std::string& stlHeaderStr)
{
  return nx::core::StringUtilities::contains(stlHeaderStr, "VXelements");
}
} // namespace

StlConstants::StlFileType StlUtilities::DetermineStlFileType(const fs::path& path)
{
  // Open File
  FILE* f = std::fopen(path.string().c_str(), "rb");
  if(nullptr == f)
  {
    return StlConstants::StlFileType::FileOpenError;
  }

  // Read the first 256 bytes of data, that should be enough but I'm sure someone will write
  // an ASCII STL File that contains a really long name which messes this up.
  std::string header(StlConstants::k_STL_HEADER_LENGTH, 0x00);
  if(std::fread(header.data(), 1, StlConstants::k_STL_HEADER_LENGTH, f) != StlConstants::k_STL_HEADER_LENGTH)
  {
    std::ignore = std::fclose(f);
    return StlConstants::StlFileType::HeaderParseError;
  }
  // close the file
  std::ignore = std::fclose(f);

  size_t solidPos = header.find("solid", 0);
  // The word 'solid' was not found ANYWHERE in the first 80 bytes.
  if(solidPos == std::string::npos)
  {
    return StlConstants::StlFileType::Binary;
  }
  // 'solid' was found as the first 5 bytes of the header. This is am ambiguous case so let's try to find 'facet'
  if(solidPos == 0)
  {
    size_t facetPos = header.find("facet", solidPos + 6);
    if(facetPos == std::string::npos)
    {
      // 'facet' was NOT found so this is a binary file.
      return StlConstants::StlFileType::Binary;
    }
    return StlConstants::StlFileType::ASCI;
  }
  return StlConstants::StlFileType::Binary;
}

int32_t StlUtilities::NumFacesFromHeader(const fs::path& path)
{
  // Open File
  FILE* f = std::fopen(path.string().c_str(), "rb");
  if(nullptr == f)
  {
    return StlConstants::k_ErrorOpeningFile;
  }

  // Read the first 256 bytes of data, that should be enough but I'm sure someone will write
  // an ASCII STL File that contains a really long name which messes this up.
  std::string header(StlConstants::k_STL_HEADER_LENGTH, 0x00);
  if(std::fread(header.data(), 1, StlConstants::k_STL_HEADER_LENGTH, f) != StlConstants::k_STL_HEADER_LENGTH)
  {
    std::ignore = std::fclose(f);
    return StlConstants::k_StlHeaderParseError;
  }

  int32_t triCount = 0;
  // Read the number of triangles in the file.
  if(std::fread(&triCount, sizeof(int32_t), 1, f) != 1)
  {
    std::ignore = std::fclose(f);
    return StlConstants::k_StlHeaderParseError;
  }

  std::ignore = std::fclose(f);
  return triCount;
}

StlConstants::StlFileCheck StlUtilities::SanityCheckFile(const fs::path& path)
{
  StlConstants::StlFileCheck stlFileCheck;

  std::error_code errorCode;
  const uintmax_t fileSize = std::filesystem::file_size(path, errorCode);
  if(errorCode)
  {
    stlFileCheck.error = StlConstants::k_ErrorOpeningFile;
    stlFileCheck.errorMessage = fmt::format("Could not determine the size of STL file '{}': {}", path.string(), errorCode.message());
    return stlFileCheck;
  }

  // Open File
  FILE* f = std::fopen(path.string().c_str(), "rb");
  if(nullptr == f)
  {
    stlFileCheck.error = StlConstants::k_ErrorOpeningFile;
    stlFileCheck.errorMessage = fmt::format("Error opening STL file '{}'", path.string());
    return stlFileCheck;
  }
  StlFileSentinel fileSentinel(f); // Ensures the file is closed on every return path below

  // Read the 80 byte header.
  std::string header(StlConstants::k_STL_HEADER_LENGTH, 0x00);
  if(std::fread(header.data(), 1, StlConstants::k_STL_HEADER_LENGTH, f) != StlConstants::k_STL_HEADER_LENGTH)
  {
    stlFileCheck.error = StlConstants::k_StlHeaderParseError;
    stlFileCheck.errorMessage = fmt::format("Error reading the {} byte header from STL file '{}'. The file is only {} bytes long.", StlConstants::k_STL_HEADER_LENGTH, path.string(), fileSize);
    return stlFileCheck;
  }

  int32_t triCount = 0;
  // Read the number of triangles in the file.
  if(std::fread(&triCount, sizeof(int32_t), 1, f) != 1)
  {
    stlFileCheck.error = StlConstants::k_TriangleCountParseError;
    stlFileCheck.errorMessage = fmt::format("Error reading the triangle count from STL file '{}'. The file is only {} bytes long.", path.string(), fileSize);
    return stlFileCheck;
  }

  // The spec stores the count as an unsigned 32 bit value but it is read into a signed type.
  // A value with the high bit set is nonsense and would become an enormous allocation
  // downstream, so reject it here rather than letting it reach resizeFaceList().
  if(triCount < 0)
  {
    stlFileCheck.error = StlConstants::k_TriangleCountParseError;
    stlFileCheck.errorMessage =
        fmt::format("STL file '{}' declares an out of range triangle count of {} in its header. The file is not a valid binary STL file.", path.string(), static_cast<uint32_t>(triCount));
    return stlFileCheck;
  }

  stlFileCheck.header = header;
  stlFileCheck.numTriangles = triCount;
  stlFileCheck.fileSize = fileSize;

  // The size this file would be if none of its triangles carry any attribute payload. The
  // arithmetic is done in uintmax_t because triCount * k_StlTriangleBytes overflows int32
  // for meshes beyond roughly 42.9 million triangles.
  const uintmax_t sizeWithoutPayload = StlConstants::k_StlFixedHeaderBytes + (static_cast<uintmax_t>(triCount) * StlConstants::k_StlTriangleBytes);

  // Only a file with bytes left over beyond the fixed size records has anywhere to put
  // attribute payload. If the size matches exactly, then every attribute byte count in the file
  // is something other than a length and must be ignored, no matter what any individual
  // triangle happens to contain. A file that is *smaller* than this is truncated; report no
  // payload so the reader does not seek, and let the read loop report the truncation precisely.
  // Vendors known to misuse the field are excluded even when there are spare bytes, since those
  // bytes may just be trailing junk.
  stlFileCheck.attributePayloadPresent = (fileSize > sizeWithoutPayload) && !IsMagicsFile(header) && !IsVxElementsFile(header);

  return stlFileCheck;
}

struct Triangle
{
  float normal[3];
  float vertex1[3];
  float vertex2[3];
  float vertex3[3];
};

void StlUtilities::ConvertAsciiToBinaryStl(const std::filesystem::path& inputPath, const std::filesystem::path& outputPath)
{
  std::ifstream asciiFile(inputPath);
  std::ofstream binaryFile(outputPath, std::ios::binary);

  if(!asciiFile.is_open() || !binaryFile.is_open())
  {
    throw std::runtime_error("Could not open files");
  }

  // Write the header
  std::string header = "Converted by ChatGPT generated Algorithm";
  header.resize(80, ' ');
  binaryFile.write(header.c_str(), 80);

  // Read ASCII STL and store triangles
  std::vector<Triangle> triangles;
  std::string line;
  Triangle tri;

  while(std::getline(asciiFile, line))
  {
    std::istringstream iss(line);
    std::string token;
    iss >> token;

    if(token == "facet")
    {
      iss >> token; // Skip "normal" word
      for(int i = 0; i < 3; ++i)
      {
        iss >> tri.normal[i];
      }
    }
    else if(token == "vertex")
    {
      if(iss >> tri.vertex1[0] >> tri.vertex1[1] >> tri.vertex1[2])
      {
        std::getline(asciiFile, line); // Read next vertex line
        std::istringstream iss2(line);
        iss2 >> token; // Skip "vertex" word
        iss2 >> tri.vertex2[0] >> tri.vertex2[1] >> tri.vertex2[2];

        std::getline(asciiFile, line); // Read next vertex line
        std::istringstream iss3(line);
        iss3 >> token; // Skip "vertex" word
        iss3 >> tri.vertex3[0] >> tri.vertex3[1] >> tri.vertex3[2];

        triangles.push_back(tri);
      }
    }
  }

  // Write the number of triangles
  uint32_t numTriangles = static_cast<uint32_t>(triangles.size());
  binaryFile.write(reinterpret_cast<const char*>(&numTriangles), sizeof(numTriangles));

  // Write triangles
  for(const auto& t : triangles)
  {
    binaryFile.write(reinterpret_cast<const char*>(&t), sizeof(Triangle));
    uint16_t attributeByteCount = 0;
    binaryFile.write(reinterpret_cast<const char*>(&attributeByteCount), sizeof(attributeByteCount));
  }
}

// Example usage:
// convertAsciiToBinarySTL("input_ascii.stl", "output_binary.stl");
