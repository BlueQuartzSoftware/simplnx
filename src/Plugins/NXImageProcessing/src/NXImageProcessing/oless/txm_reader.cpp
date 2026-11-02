/**
This is just a test program that can be useful to debug a .txm file or dump
the contents of a .txm file.
*/

#include "oless/oless.h"
#include "oless/oless_common.hpp"
#include "oless/pole.h"

#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#define SIMPLNX_BYTE_ORDER little
#elif defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define SIMPLNX_BYTE_ORDER little
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define SIMPLNX_BYTE_ORDER big
#endif
#endif

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

namespace nx::core
{
enum class endian : uint8_t
{
  little = 0,
  big = 1,
  native = SIMPLNX_BYTE_ORDER
};

inline endian checkEndian()
{
  constexpr uint32_t i = 0x01020304;
  const auto* u8 = reinterpret_cast<const std::byte*>(&i);

  return u8[0] == std::byte{0x01} ? endian::big : endian::little;
}
} // namespace nx::core

namespace TiffWriter
{
std::pair<int32_t, std::string> WriteImage(const std::string& filepath, int32_t width, int32_t height, uint16_t bytesPerSample, uint16_t samplesPerPixel, const uint8_t* data)
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
    return {-1, "TiffWriter::WriteImage Error: Could not open output file '{}' for writing."};
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
  tags.push_back(TIFTAG{0x0102, 0x0003, 1, 8 * bytesPerSample});        // BitsPerSample
  tags.push_back(TIFTAG{0x0103, 0x0003, 1, 0x0001});                    // Compression
  tags.push_back(TIFTAG{0x0106, 0x0003, 1, photometricInterpretation}); // PhotometricInterpretation  // For SamplesPerPixel = 3 or 4 (RGB or RGBA)
  // Now compute the offset to the image data so that we can put that into the tag.
  // The math on this ONLY Works if we have 11 total Tags.
  // IF YOU ADD MORE TAGS, YOU NEED TO ADJUST THE NEXT LINE OF CODE
  int32_t imageDataOffset = static_cast<int32_t>(8 + (k_NumTags * 12) + 6); // Header + tags + IDF Tag entry count and Next IFD Offset
  tags.push_back(TIFTAG{0x0111, 0x0004, 1, imageDataOffset});               // StripOffsets

  tags.push_back(TIFTAG{0x0112, 0x0003, 1, 1});                                                 // Orientation
  tags.push_back(TIFTAG{0x0115, 0x0003, 1, samplesPerPixel});                                   // SamplesPerPixel
  tags.push_back(TIFTAG{0x0116, 0x0004, 1, height});                                            // RowsPerStrip
  tags.push_back(TIFTAG{0x0117, 0x0004, 1, width * height * samplesPerPixel * bytesPerSample}); // StripByteCounts

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
  const int32_t imageByteCount = width * height * samplesPerPixel * bytesPerSample;
  outputFile.write(reinterpret_cast<const char*>(data), imageByteCount);

  // and we are done.
  return {0, "No Error"};
}

} // namespace TiffWriter

using StoragePtrType = std::shared_ptr<POLE::Storage>;
using StreamPtrType = std::shared_ptr<POLE::Stream>;

void printStreamInfo(POLE::Storage* storage, const std::string& name, const std::string& fullname)
{
  POLE::Stream* stream = new POLE::Stream(storage, fullname.c_str());

  if(!storage->isDirectory(fullname))
  {
    if(!stream->fail())
    {
      //      OleSummary* summary = new OleSummary();
      //      summary->FullName = stream->fullName();
      //      summary->Size = stream->size();
      //      m_results.push_back((IExportable*)summary);
      std::cout << stream->fullName() << "\t" << stream->size() << "\n";
    }
  }
  else
  {
    std::cout << fullname << "\t DIRECTORY" << "\n";
  }
}

// -- Constants --
static int FLOAT_TYPE = 10;
static int INT16_TYPE = 5;
static int UCHAR_TYPE = 3;

template <typename T>
T ReadValue(StoragePtrType storage, const std::string& path)
{
  T value = 0;
  StreamPtrType stream = std::make_shared<POLE::Stream>(storage.get(), path);
  if(!stream->fail())
  {
    stream->read(reinterpret_cast<unsigned char*>(&value), sizeof(T));
  }
  return value;
}

void ReadImages(StoragePtrType storage)
{

  std::string path = "/ImageInfo/ImageWidth";
  int32_t ImageWidth = ReadValue<int32_t>(storage, path);
  std::cout << path << "\t" << ImageWidth << " pixels\n";

  path = "/ImageInfo/ImageHeight";
  int32_t ImageHeight = ReadValue<int32_t>(storage, path);
  std::cout << path << "\t" << ImageHeight << " pixels\n";

  path = "/ImageInfo/PixelSize";
  float PixelSize = ReadValue<float>(storage, path);
  std::cout << path << "\t" << PixelSize << " microns\n";

  path = "/ImageInfo/DataType";
  int32_t DataType = ReadValue<int32_t>(storage, path);
  std::cout << path << "\t" << DataType << "\n";

  path = "/ImageInfo/NoOfImages";
  int32_t NoOfImages = ReadValue<int32_t>(storage, path);
  std::cout << path << "\t" << NoOfImages << "\n";

  int numImageGroups = std::ceil(static_cast<float>(NoOfImages) / 100.0f);
  std::cout << "numImageGroups:" << "\t" << numImageGroups << "\n";

  size_t bitsPerPixel = 0;
  if(DataType == FLOAT_TYPE)
  {
    bitsPerPixel = 4;
  }
  if(DataType == INT16_TYPE)
  {
    bitsPerPixel = 2;
  }
  if(DataType == UCHAR_TYPE)
  {
    bitsPerPixel = 1;
  }

  size_t totalPixels = ImageWidth * ImageHeight;
  size_t totalBytes = totalPixels * bitsPerPixel;
  size_t readBytes = 0;
  std::vector<unsigned char> buffer(totalBytes);

  int imageGroupIndex = 1;
  for(int32_t i = 1; i < NoOfImages + 1; i++)
  {
    std::stringstream pathStrm;
    pathStrm << "/ImageData" << imageGroupIndex << "/Image" << i;
    std::cout << "Reading Image: " << pathStrm.str() << "\n";
    StreamPtrType stream = std::make_shared<POLE::Stream>(storage.get(), pathStrm.str());
    if(!stream->fail())
    {
      readBytes = stream->read(buffer.data(), totalBytes);
      if(readBytes != totalBytes)
      {
        std::cout << "Not enough bytes were read: " << readBytes << " vs " << totalBytes << "\n";
        return;
      }

      // std::stringstream filepathstream;
      // filepathstream << "/tmp/txm/Image_" << i << ".tif";
      // TiffWriter::WriteImage(filepathstream.str(), ImageWidth, ImageHeight, bitsPerPixel, 1, buffer.data());
    }
    if(i % 100 == 0)
    {
      imageGroupIndex++;
    }
  }
}

int main(int argc, char* argv[])
{
  std::cout << "txm_reader Starting... \n";
  if(argc != 3)
  {
    std::cout << "3 Args are required\n1-Command\n2-Input File\n";
    return 1;
  }

  std::string command = argv[1];
  std::string file = argv[2];

  StoragePtrType storage = std::make_shared<POLE::Storage>(file.c_str());
  storage->open();

  if(storage->result() != POLE::Storage::Ok)
  {
    std::cout << "Unable to open OLESS file" << std::endl;
    return 2;
  }

  ReadImages(storage);

  storage->close();

  return 0;
}
