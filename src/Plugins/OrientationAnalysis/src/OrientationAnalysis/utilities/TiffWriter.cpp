#include "TiffWriter.hpp"

#include "simplnx/Common/Bit.hpp"

#include <fmt/format.h>

#include <array>
#include <fstream>
#include <vector>

namespace
{

const uint16_t k_PhotometricMinisblack = 0x0001;
const uint16_t k_PhotometricRgb = 0x0002;

struct TIFTAG
{
  int16_t TagId = 0;      // The tag identifier
  int16_t DataType = 0;   // The scalar type of the data items
  int32_t DataCount = 0;  // The number of items in the tag data
  int32_t DataOffset = 0; // The byte offset to the data items

  friend std::ostream& operator<<(std::ostream& out, const TIFTAG& tag)
  {
    out.write(reinterpret_cast<const char*>(&tag.TagId), sizeof(tag.TagId));
    out.write(reinterpret_cast<const char*>(&tag.DataType), sizeof(tag.DataType));
    out.write(reinterpret_cast<const char*>(&tag.DataCount), sizeof(tag.DataCount));
    out.write(reinterpret_cast<const char*>(&tag.DataOffset), sizeof(tag.DataOffset));

    return out;
  }
};

} // namespace
std::pair<int32_t, std::string> TiffWriter::WriteImage(const std::string& filepath, int32_t width, int32_t height, uint16_t samplesPerPixel, const uint8_t* data)
{
  const uint16_t photometricInterpretation = (samplesPerPixel == 1 ? k_PhotometricMinisblack : k_PhotometricRgb);

  // Check for Endianess of the system and write the appropriate magic number according to the tiff spec
  std::array<char, 4> magicNumber = {0x49, 0x49, 0x2A, 0x00};

  if(nx::core::checkEndian() == nx::core::endian::big)
  {
    magicNumber = {0x4D, 0x4D, 0x00, 0x2A};
  }

  // open file and write header
  std::ofstream outputFile(filepath, std::ios::binary);
  if(!outputFile.is_open())
  {
    return {-1, fmt::format("TiffWriter::WriteImage Error: Could not open output file '{}' for writing.", filepath)};
  }

  outputFile.write(magicNumber.data(), magicNumber.size());
  // Generate the offset into the Image File Directory (ifd) which we are going to write first
  constexpr uint32_t ifdOffset = 8;
  outputFile.write(reinterpret_cast<const char*>(&ifdOffset), sizeof(ifdOffset));
  const int k_NumTags = 12;
  std::vector<TIFTAG> tags;
  tags.push_back(TIFTAG{0x00FE, 0x0004, 1, 0x00000000});                // NewSubfileType
  tags.push_back(TIFTAG{0x0100, 0x0004, 1, width});                     // ImageWidth
  tags.push_back(TIFTAG{0x0101, 0x0004, 1, height});                    // ImageLength
  tags.push_back(TIFTAG{0x0102, 0x0003, 1, 8 * sizeof(char)});          // BitsPerSample
  tags.push_back(TIFTAG{0x0103, 0x0003, 1, 0x0001});                    // Compression
  tags.push_back(TIFTAG{0x0106, 0x0003, 1, photometricInterpretation}); // PhotometricInterpretation  // For SamplesPerPixel = 3 or 4 (RGB or RGBA)
  // Now compute the offset to the image data so that we can put that into the tag.
  // The math on this ONLY Works if we have 11 total Tags.
  // IF YOU ADD MORE TAGS, YOU NEED TO ADJUST THE NEXT LINE OF CODE
  int32_t imageDataOffset = static_cast<int32_t>(8 + (k_NumTags * 12) + 6); // Header + tags + IDF Tag entry count and Next IFD Offset
  tags.push_back(TIFTAG{0x0111, 0x0004, 1, imageDataOffset});               // StripOffsets

  tags.push_back(TIFTAG{0x0112, 0x0003, 1, 1});                                // Orientation
  tags.push_back(TIFTAG{0x0115, 0x0003, 1, samplesPerPixel});                  // SamplesPerPixel
  tags.push_back(TIFTAG{0x0116, 0x0004, 1, height});                           // RowsPerStrip
  tags.push_back(TIFTAG{0x0117, 0x0004, 1, width * height * samplesPerPixel}); // StripByteCounts

  // TIFTAG XResolution;
  // TIFTAG YResolution;
  // TIFTAG ResolutionUnit;
  tags.push_back(TIFTAG{0x011c, 0x0003, 1, 0x0001}); // PlanarConfiguration // 284

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
  const int32_t imageByteCount = width * height * samplesPerPixel;
  outputFile.write(reinterpret_cast<const char*>(data), imageByteCount);

  // and we are done.
  return {0, "No Error"};
}
