#include "StlUtilities.hpp"

using namespace nx::core;

int32_t StlUtilities::DetermineStlFileType(const fs::path& path)
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
  // close the file
  std::ignore = std::fclose(f);

  size_t solid_pos = header.find("solid", 0);
  // The word 'solid' was not found ANYWHERE in the first 80 bytes.
  if(solid_pos == std::string::npos)
  {
    return 0;
  }
  // 'solid' was found as the first 5 bytes of the header. This is am ambiguous case so let's try to find 'facet'
  if(solid_pos == 0)
  {
    size_t facet_pos = header.find("facet", solid_pos + 6);
    if(facet_pos == std::string::npos)
    {
      // 'facet' was NOT found so this is a binary file.
      return 0;
    }
    return 1;
  }
  return 0;
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
