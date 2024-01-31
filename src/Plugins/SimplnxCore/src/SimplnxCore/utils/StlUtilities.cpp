#include "StlUtilities.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace nx::core;

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
