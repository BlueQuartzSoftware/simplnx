#include "TiffWriter.hpp"

#include <array>
#include <cstddef>
#include <fstream>
#include <vector>

namespace
{

const uint16_t PHOTOMETRIC_MINISBLACK = 0x0001;
const uint16_t PHOTOMETRIC_RGB = 0x0002;

struct TIFTAG
{
  int16_t tagId = 0;      // The tag identifier
  int16_t dataType = 0;   // The scalar type of the data items
  int32_t dataCount = 0;  // The number of items in the tag data
  int32_t dataOffset = 0; // The byte offset to the data items

  friend std::ostream& operator<<(std::ostream& out, const TIFTAG& tag)
  {
    out.write(reinterpret_cast<const char*>(&tag.tagId), sizeof(tag.tagId));
    out.write(reinterpret_cast<const char*>(&tag.dataType), sizeof(tag.dataType));
    out.write(reinterpret_cast<const char*>(&tag.dataCount), sizeof(tag.dataCount));
    out.write(reinterpret_cast<const char*>(&tag.dataOffset), sizeof(tag.dataOffset));

    return out;
  }
};

enum class Endianess
{
  Little = 0,
  Big
};

Endianess checkEndianess()
{
  constexpr uint32_t i = 0x01020304;
  const std::byte* u8 = reinterpret_cast<const std::byte*>(&i);

  return u8[0] == std::byte{0x01} ? Endianess::Big : Endianess::Little;
}

} // namespace

// -----------------------------------------------------------------------------
std::pair<int32_t, std::string> TiffWriter::WriteColorImage(const std::string& filepath, int32_t width, int32_t height, uint16_t samplesPerPixel, const uint8_t* data)
{
  // Check for Endianess of the system and write the appropriate magic number according to the tiff spec
  std::array<char, 4> magicNumber = {0x49, 0x49, 0x2A, 0x00};

  if(checkEndianess() == Endianess::Big)
  {
    magicNumber = {0x4D, 0x4D, 0x00, 0x2A};
  }

  // open file and write header
  std::ofstream outputFile(filepath, std::ios::binary);
  if(!outputFile.is_open())
  {
    return {-1, "Could not open output file for writing"};
  }

  outputFile.write(magicNumber.data(), magicNumber.size());
  // Generate the offset into the Image File Directory (ifd) which we are going to write first
  constexpr uint32_t ifd_Offset = 8;
  outputFile.write(reinterpret_cast<const char*>(&ifd_Offset), sizeof(ifd_Offset));

  std::vector<TIFTAG> tags;
  tags.push_back(TIFTAG{0x00FE, 0x0004, 1, 0x00000000});                       // NewSubfileType
  tags.push_back(TIFTAG{0x0100, 0x0004, 1, width});                            // ImageWidth
  tags.push_back(TIFTAG{0x0101, 0x0004, 1, height});                           // ImageLength
  tags.push_back(TIFTAG{0x0102, 0x0003, 1, 8 * sizeof(char)});                 // BitsPerSample
  tags.push_back(TIFTAG{0x0103, 0x0003, 1, 0x0001});                           // Compression
  tags.push_back(TIFTAG{0x0106, 0x0003, 1, PHOTOMETRIC_RGB});                  // PhotometricInterpretation  // For SamplesPerPixel = 3 or 4 (RGB or RGBA)
  tags.push_back(TIFTAG{0x0112, 0x0003, 1, 1});                                // Orientation
  tags.push_back(TIFTAG{0x0115, 0x0003, 1, samplesPerPixel});                  // SamplesPerPixel
  tags.push_back(TIFTAG{0x0116, 0x0004, 1, height});                           // RowsPerStrip
  tags.push_back(TIFTAG{0x0117, 0x0004, 1, width * height * samplesPerPixel}); // StripByteCounts
  // TIFTAG XResolution;
  // TIFTAG YResolution;
  // TIFTAG ResolutionUnit;
  tags.push_back(TIFTAG{0x011c, 0x0003, 1, 0x0001}); // PlanarConfiguration

  // Now compute the offset to the image data so that we can put that into the tag.
  // THESE NEXT 2 LINES MUST BE THE LAST TAG TO BE PUSHED BACK INTO THE VECTOR OR THE MATH WILL BE WRONG
  int32_t imageDataOffset = static_cast<int32_t>(8 + ((tags.size() + 1) * 12) + 6); // Header + tags + IDF Tag entry count and Next IFD Offset
  tags.push_back(TIFTAG{0x0111, 0x0004, 1, imageDataOffset});                       // StripOffsets

  // Write the number of tags to the IFD section
  uint16_t numEntries = static_cast<uint16_t>(tags.size());
  outputFile.write(reinterpret_cast<const char*>(&numEntries), sizeof(numEntries));
  // write the tags to the file.
  for(const auto& tag : tags)
  {
    outputFile << tag;
  }
  // Write the "Next Tag Offset"
  constexpr uint32_t nextOffset = 0;
  outputFile.write(reinterpret_cast<const char*>(&nextOffset), sizeof(nextOffset));

  // Now write the actual image data
  int32_t imageByteCount = width * height * samplesPerPixel;
  outputFile.write(reinterpret_cast<const char*>(data), imageByteCount);

  // and we are done.
  return {0, "No Error"};
}

// -----------------------------------------------------------------------------
std::pair<int32_t, std::string> TiffWriter::WriteGrayScaleImage(const std::string& filepath, int32_t width, int32_t height, const uint8_t* data)
{
  int32_t samplesPerPixel = 1;
  // Check for Endianess of the system and write the appropriate magic number according to the tiff spec
  std::array<char, 4> magicNumber = {0x49, 0x49, 0x2A, 0x00};

  if(checkEndianess() == Endianess::Big)
  {
    magicNumber = {0x4D, 0x4D, 0x00, 0x2A};
  }

  // open file and write header
  std::ofstream outputFile(filepath, std::ios::binary);
  if(!outputFile.is_open())
  {
    return {-1, "Could not open output file for writing"};
  }

  outputFile.write(magicNumber.data(), magicNumber.size());
  // Generate the offset into the Image File Directory (ifd) which we are going to write first
  constexpr uint32_t ifd_Offset = 8;
  outputFile.write(reinterpret_cast<const char*>(&ifd_Offset), sizeof(ifd_Offset));

  std::vector<TIFTAG> tags;
  tags.push_back(TIFTAG{0x00FE, 0x0004, 1, 0x00000000});                       // NewSubfileType
  tags.push_back(TIFTAG{0x0100, 0x0004, 1, width});                            // ImageWidth
  tags.push_back(TIFTAG{0x0101, 0x0004, 1, height});                           // ImageLength
  tags.push_back(TIFTAG{0x0102, 0x0003, 1, 8 * sizeof(char)});                 // BitsPerSample
  tags.push_back(TIFTAG{0x0103, 0x0003, 1, 0x0001});                           // Compression
  tags.push_back(TIFTAG{0x0106, 0x0003, 1, PHOTOMETRIC_MINISBLACK});           // PhotometricInterpretation  // For SamplesPerPixel = 3 or 4 (RGB or RGBA)
  tags.push_back(TIFTAG{0x0112, 0x0003, 1, 1});                                // Orientation
  tags.push_back(TIFTAG{0x0115, 0x0003, 1, 1});                                // SamplesPerPixel
  tags.push_back(TIFTAG{0x0116, 0x0004, 1, height});                           // RowsPerStrip
  tags.push_back(TIFTAG{0x0117, 0x0004, 1, width * height * samplesPerPixel}); // StripByteCounts
  // TIFTAG XResolution;
  // TIFTAG YResolution;
  // TIFTAG ResolutionUnit;
  tags.push_back(TIFTAG{0x011c, 0x0003, 1, 0x0001}); // PlanarConfiguration

  // Now compute the offset to the image data so that we can put that into the tag.
  // THESE NEXT 2 LINES MUST BE THE LAST TAG TO BE PUSHED BACK INTO THE VECTOR OR THE MATH WILL BE WRONG
  int32_t imageDataOffset = static_cast<int32_t>(8 + ((tags.size() + 1) * 12) + 6); // Header + tags + IDF Tag entry count and Next IFD Offset
  tags.push_back(TIFTAG{0x0111, 0x0004, 1, imageDataOffset});                       // StripOffsets

  // Write the number of tags to the IFD section
  uint16_t numEntries = static_cast<uint16_t>(tags.size());
  outputFile.write(reinterpret_cast<const char*>(&numEntries), sizeof(numEntries));
  // write the tags to the file.
  for(const auto& tag : tags)
  {
    outputFile << tag;
  }
  // Write the "Next Tag Offset"
  constexpr uint32_t nextOffset = 0;
  outputFile.write(reinterpret_cast<const char*>(&nextOffset), sizeof(nextOffset));

  // Now write the actual image data
  int32_t imageByteCount = width * height * samplesPerPixel;
  outputFile.write(reinterpret_cast<const char*>(data), imageByteCount);

  // and we are done.
  return {0, "No Error"};
}
