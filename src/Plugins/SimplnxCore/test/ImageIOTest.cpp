#include <catch2/catch.hpp>

#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/Utilities/ImageIO/ImageMetadata.hpp"
#include "simplnx/Utilities/ImageIO/StbImageIO.hpp"
#include "simplnx/Utilities/ImageIO/TiffImageIO.hpp"

#include <tiffio.h>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
fs::path ImageIOOutputDir()
{
  fs::path dir = fs::path(std::string(nx::core::unit_test::k_BinaryTestOutputDir.view())) / "ImageIO";
  fs::create_directories(dir);
  return dir;
}

// Describes one TIFF directory (page) to synthesize.
struct TiffPage
{
  uint32_t width = 0;
  uint32_t height = 0;
  uint16_t spp = 1;                    // samples-per-pixel
  std::optional<uint32_t> subfileType; // nullopt => do not emit the SubfileType tag
  std::vector<uint8_t> pixels;         // row-major, top-to-bottom, size == width*height*spp
};

// Builds a page filled with a single constant value across all pixels/components.
TiffPage MakeConstantPage(uint32_t width, uint32_t height, uint16_t spp, uint8_t value, std::optional<uint32_t> subfileType = std::nullopt)
{
  TiffPage page;
  page.width = width;
  page.height = height;
  page.spp = spp;
  page.subfileType = subfileType;
  page.pixels.assign(static_cast<size_t>(width) * height * spp, value);
  return page;
}

// Builds a single-component page whose pixel (row, col) holds value (row*width + col) truncated to
// uint8, so the decoded buffer must equal {0, 1, 2, ...} when read row-major top-to-bottom.
TiffPage MakeRampPage(uint32_t width, uint32_t height, std::optional<uint32_t> subfileType = std::nullopt)
{
  TiffPage page;
  page.width = width;
  page.height = height;
  page.spp = 1;
  page.subfileType = subfileType;
  page.pixels.resize(static_cast<size_t>(width) * height);
  for(size_t i = 0; i < page.pixels.size(); ++i)
  {
    page.pixels[i] = static_cast<uint8_t>(i);
  }
  return page;
}

// Synthesizes a (possibly multi-directory) uint8 TIFF at @p path using libtiff directly.
void WriteTiff(const fs::path& path, const std::vector<TiffPage>& pages)
{
  TIFF* tif = TIFFOpen(path.string().c_str(), "w");
  REQUIRE(tif != nullptr);
  for(const auto& page : pages)
  {
    REQUIRE(page.pixels.size() == static_cast<size_t>(page.width) * page.height * page.spp);
    REQUIRE(TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, page.width) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_IMAGELENGTH, page.height) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, page.spp) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, page.spp == 1 ? PHOTOMETRIC_MINISBLACK : PHOTOMETRIC_RGB) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tif, 0)) != 0);
    if(page.subfileType.has_value())
    {
      REQUIRE(TIFFSetField(tif, TIFFTAG_SUBFILETYPE, page.subfileType.value()) != 0);
    }
    const size_t rowBytes = static_cast<size_t>(page.width) * page.spp;
    for(uint32_t row = 0; row < page.height; ++row)
    {
      // TIFFWriteScanline takes a non-const void* but does not modify the buffer.
      REQUIRE(TIFFWriteScanline(tif, const_cast<uint8_t*>(page.pixels.data() + static_cast<size_t>(row) * rowBytes), row, 0) >= 0);
    }
    REQUIRE(TIFFWriteDirectory(tif) != 0);
  }
  TIFFClose(tif);
}
} // namespace

TEST_CASE("SimplnxCore::TiffImageIO: single-page 2D uint8", "[SimplnxCore][ImageIO]")
{
  constexpr uint32_t width = 4;
  constexpr uint32_t height = 3;
  const fs::path path = ImageIOOutputDir() / "single_page.tif";
  const TiffPage page = MakeRampPage(width, height);
  WriteTiff(path, {page});

  const TiffImageIO io;
  const auto metaResult = io.readMetadata(path);
  REQUIRE(metaResult.valid());
  const ImageMetadata& meta = metaResult.value();
  REQUIRE(meta.numPages == 1);
  REQUIRE(meta.width == width);
  REQUIRE(meta.height == height);
  REQUIRE(meta.numComponents == 1);
  REQUIRE(meta.dataType == DataType::uint8);

  std::vector<uint8> buffer(static_cast<size_t>(width) * height);
  REQUIRE(io.readPixelData(path, buffer, 0).valid());
  REQUIRE(buffer == page.pixels);

  // Default page argument reads page 0.
  std::vector<uint8> defaultBuffer(static_cast<size_t>(width) * height);
  REQUIRE(io.readPixelData(path, defaultBuffer).valid());
  REQUIRE(defaultBuffer == page.pixels);
}

TEST_CASE("SimplnxCore::TiffImageIO: multi-page uint8 Z-stack", "[SimplnxCore][ImageIO]")
{
  constexpr uint32_t width = 3;
  constexpr uint32_t height = 2;
  const std::vector<uint8_t> planeValues = {10, 20, 30};

  std::vector<TiffPage> pages;
  for(uint8_t value : planeValues)
  {
    // No SubfileType tag: the common ImageJ-style multi-page stack layout.
    pages.push_back(MakeConstantPage(width, height, 1, value));
  }
  const fs::path path = ImageIOOutputDir() / "three_page_stack.tif";
  WriteTiff(path, pages);

  const TiffImageIO io;
  const auto metaResult = io.readMetadata(path);
  REQUIRE(metaResult.valid());
  REQUIRE(metaResult.value().numPages == 3);

  for(usize p = 0; p < planeValues.size(); ++p)
  {
    std::vector<uint8> buffer(static_cast<size_t>(width) * height);
    REQUIRE(io.readPixelData(path, buffer, p).valid());
    const std::vector<uint8> expected(static_cast<size_t>(width) * height, planeValues[p]);
    REQUIRE(buffer == expected);
  }

  // Requesting a page past the end fails cleanly rather than reading garbage.
  std::vector<uint8> overflowBuffer(static_cast<size_t>(width) * height);
  REQUIRE(io.readPixelData(path, overflowBuffer, 3).invalid());
}

TEST_CASE("SimplnxCore::TiffImageIO: reduced-resolution subfile excluded from page count", "[SimplnxCore][ImageIO]")
{
  constexpr uint32_t fullWidth = 4;
  constexpr uint32_t fullHeight = 3;

  // Directory 0: full-resolution page (SubfileType == 0). Directory 1: a reduced-resolution
  // pyramid level (SubfileType == FILETYPE_REDUCEDIMAGE) that must be ignored.
  const TiffPage fullPage = MakeRampPage(fullWidth, fullHeight, static_cast<uint32_t>(0));
  const TiffPage reducedPage = MakeConstantPage(2, 2, 1, 99, static_cast<uint32_t>(FILETYPE_REDUCEDIMAGE));
  const fs::path path = ImageIOOutputDir() / "pyramid.tif";
  WriteTiff(path, {fullPage, reducedPage});

  const TiffImageIO io;
  const auto metaResult = io.readMetadata(path);
  REQUIRE(metaResult.valid());
  REQUIRE(metaResult.value().numPages == 1);
  REQUIRE(metaResult.value().width == fullWidth);
  REQUIRE(metaResult.value().height == fullHeight);

  std::vector<uint8> buffer(static_cast<size_t>(fullWidth) * fullHeight);
  REQUIRE(io.readPixelData(path, buffer, 0).valid());
  REQUIRE(buffer == fullPage.pixels);

  // Only the full-resolution page is addressable; the pyramid level is not page 1.
  std::vector<uint8> reducedBuffer(2 * 2);
  REQUIRE(io.readPixelData(path, reducedBuffer, 1).invalid());
}

TEST_CASE("SimplnxCore::StbImageIO: single-image formats reject non-zero page index", "[SimplnxCore][ImageIO]")
{
  constexpr int width = 4;
  constexpr int height = 3;
  std::vector<uint8> pixels(static_cast<size_t>(width) * height);
  for(size_t i = 0; i < pixels.size(); ++i)
  {
    pixels[i] = static_cast<uint8>(i);
  }

  ImageMetadata writeMeta;
  writeMeta.width = width;
  writeMeta.height = height;
  writeMeta.numComponents = 1;
  writeMeta.dataType = DataType::uint8;

  const fs::path path = ImageIOOutputDir() / "single_image.png";
  const StbImageIO io;
  REQUIRE(io.writePixelData(path, pixels, writeMeta).valid());

  const auto metaResult = io.readMetadata(path);
  REQUIRE(metaResult.valid());
  REQUIRE(metaResult.value().numPages == 1);

  // Page 0 round-trips the pixel data.
  std::vector<uint8> buffer(static_cast<size_t>(width) * height);
  REQUIRE(io.readPixelData(path, buffer, 0).valid());
  REQUIRE(buffer == pixels);

  // Any page beyond 0 is an error for a single-image format.
  std::vector<uint8> pageOneBuffer(static_cast<size_t>(width) * height);
  REQUIRE(io.readPixelData(path, pageOneBuffer, 1).invalid());
}
