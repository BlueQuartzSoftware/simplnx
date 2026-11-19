#include "simplnx/Utilities/ImageIO/TiffImageIO.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/ImageIO/StbImageIO.hpp"

#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>
#include <tiffio.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
constexpr uint32_t k_Width = 35;
constexpr uint32_t k_Height = 19;
constexpr uint32_t k_TileWidth = 16;
constexpr uint32_t k_TileHeight = 16;

class TemporaryTiffFile
{
public:
  explicit TemporaryTiffFile(const std::string& name)
  : m_Path(fs::path(unit_test::k_BinaryTestOutputDir.view()) / name)
  {
    std::error_code errorCode;
    fs::remove(m_Path, errorCode);
  }

  ~TemporaryTiffFile() noexcept
  {
    std::error_code errorCode;
    fs::remove(m_Path, errorCode);
  }

  TemporaryTiffFile(const TemporaryTiffFile&) = delete;
  TemporaryTiffFile(TemporaryTiffFile&&) = delete;
  TemporaryTiffFile& operator=(const TemporaryTiffFile&) = delete;
  TemporaryTiffFile& operator=(TemporaryTiffFile&&) = delete;

  const fs::path& path() const
  {
    return m_Path;
  }

private:
  fs::path m_Path;
};

using TiffHandle = std::unique_ptr<TIFF, decltype(&TIFFClose)>;

TiffHandle CreateTiledTiff(const fs::path& path, uint16_t samplesPerPixel, uint16_t bitsPerSample, uint16_t sampleFormat, uint16_t photometric, uint16_t orientation, uint16_t planarConfig)
{
  TiffHandle tiff(TIFFOpen(path.string().c_str(), "w"), TIFFClose);
  REQUIRE(tiff != nullptr);
  REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_IMAGEWIDTH, k_Width) == 1);
  REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_IMAGELENGTH, k_Height) == 1);
  REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_BITSPERSAMPLE, bitsPerSample) == 1);
  REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_SAMPLEFORMAT, sampleFormat) == 1);
  REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_SAMPLESPERPIXEL, samplesPerPixel) == 1);
  REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_PHOTOMETRIC, photometric) == 1);
  REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_ORIENTATION, orientation) == 1);
  REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_TILEWIDTH, k_TileWidth) == 1);
  REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_TILELENGTH, k_TileHeight) == 1);
  REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_PLANARCONFIG, planarConfig) == 1);
  REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_COMPRESSION, COMPRESSION_NONE) == 1);
  return tiff;
}

template <class GeneratorT>
void WriteTiledUInt8(const fs::path& path, uint16_t samplesPerPixel, uint16_t photometric, uint16_t orientation, uint16_t planarConfig, GeneratorT&& generator)
{
  TiffHandle tiff = CreateTiledTiff(path, samplesPerPixel, 8, SAMPLEFORMAT_UINT, photometric, orientation, planarConfig);
  if(samplesPerPixel == 4)
  {
    const uint16_t extraSample = EXTRASAMPLE_ASSOCALPHA;
    REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_EXTRASAMPLES, 1, &extraSample) == 1);
  }

  const usize tilePixelCount = static_cast<usize>(k_TileWidth) * k_TileHeight;
  const usize bufferSize = planarConfig == PLANARCONFIG_CONTIG ? tilePixelCount * samplesPerPixel : tilePixelCount;
  std::vector<uint8_t> tile(bufferSize, 0);

  for(uint32_t tileY = 0; tileY < k_Height; tileY += k_TileHeight)
  {
    for(uint32_t tileX = 0; tileX < k_Width; tileX += k_TileWidth)
    {
      const uint16_t planeCount = planarConfig == PLANARCONFIG_CONTIG ? 1 : samplesPerPixel;
      for(uint16_t plane = 0; plane < planeCount; ++plane)
      {
        std::fill(tile.begin(), tile.end(), uint8_t{0});
        for(uint32_t localY = 0; localY < k_TileHeight; ++localY)
        {
          for(uint32_t localX = 0; localX < k_TileWidth; ++localX)
          {
            const uint32_t sourceX = tileX + localX;
            const uint32_t sourceY = tileY + localY;
            if(sourceX >= k_Width || sourceY >= k_Height)
            {
              continue;
            }

            if(planarConfig == PLANARCONFIG_CONTIG)
            {
              const usize pixelOffset = (static_cast<usize>(localY) * k_TileWidth + localX) * samplesPerPixel;
              for(uint16_t component = 0; component < samplesPerPixel; ++component)
              {
                tile[pixelOffset + component] = generator(sourceX, sourceY, component);
              }
            }
            else
            {
              tile[static_cast<usize>(localY) * k_TileWidth + localX] = generator(sourceX, sourceY, plane);
            }
          }
        }

        const uint16_t sample = planarConfig == PLANARCONFIG_CONTIG ? 0 : plane;
        const ttile_t tileIndex = TIFFComputeTile(tiff.get(), tileX, tileY, 0, sample);
        const auto byteCount = static_cast<tmsize_t>(tile.size());
        REQUIRE(TIFFWriteEncodedTile(tiff.get(), tileIndex, tile.data(), byteCount) == byteCount);
      }
    }
  }
}

template <class T, class GeneratorT>
void WriteTiledScalar(const fs::path& path, uint16_t sampleFormat, GeneratorT&& generator)
{
  static_assert(std::is_trivially_copyable_v<T>);
  TiffHandle tiff = CreateTiledTiff(path, 1, static_cast<uint16_t>(sizeof(T) * 8), sampleFormat, PHOTOMETRIC_MINISBLACK, ORIENTATION_TOPLEFT, PLANARCONFIG_CONTIG);
  std::vector<T> tile(static_cast<usize>(k_TileWidth) * k_TileHeight, T{});

  for(uint32_t tileY = 0; tileY < k_Height; tileY += k_TileHeight)
  {
    for(uint32_t tileX = 0; tileX < k_Width; tileX += k_TileWidth)
    {
      std::fill(tile.begin(), tile.end(), T{});
      for(uint32_t localY = 0; localY < k_TileHeight; ++localY)
      {
        for(uint32_t localX = 0; localX < k_TileWidth; ++localX)
        {
          const uint32_t sourceX = tileX + localX;
          const uint32_t sourceY = tileY + localY;
          if(sourceX < k_Width && sourceY < k_Height)
          {
            tile[static_cast<usize>(localY) * k_TileWidth + localX] = generator(sourceX, sourceY);
          }
        }
      }

      const ttile_t tileIndex = TIFFComputeTile(tiff.get(), tileX, tileY, 0, 0);
      const auto byteCount = static_cast<tmsize_t>(tile.size() * sizeof(T));
      REQUIRE(TIFFWriteEncodedTile(tiff.get(), tileIndex, tile.data(), byteCount) == byteCount);
    }
  }
}

void WriteStrippedUInt8(const fs::path& path)
{
  TiffHandle tiff(TIFFOpen(path.string().c_str(), "w"), TIFFClose);
  REQUIRE(tiff != nullptr);
  REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_IMAGEWIDTH, k_Width) == 1);
  REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_IMAGELENGTH, k_Height) == 1);
  REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_BITSPERSAMPLE, 8) == 1);
  REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT) == 1);
  REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_SAMPLESPERPIXEL, 1) == 1);
  REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK) == 1);
  REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG) == 1);
  REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_ROWSPERSTRIP, 1) == 1);
  REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_COMPRESSION, COMPRESSION_NONE) == 1);

  std::array<uint8_t, k_Width> row{};
  for(uint32_t y = 0; y < k_Height; ++y)
  {
    for(uint32_t x = 0; x < k_Width; ++x)
    {
      row[x] = static_cast<uint8_t>((x + 3 * y) % 251);
    }
    REQUIRE(TIFFWriteScanline(tiff.get(), row.data(), y) == 1);
  }
}

std::vector<uint8_t> ReadDirect(const fs::path& path, usize byteCount)
{
  TiffImageIO imageIO;
  std::vector<uint8_t> pixels(byteCount, 0xCD);
  auto result = imageIO.readPixelData(path, pixels);
  SIMPLNX_RESULT_REQUIRE_VALID(result)
  return pixels;
}

std::vector<uint8_t> ReadRows(const fs::path& path, usize samplesPerPixel, usize bytesPerSample, std::vector<uint8_t>& coverage)
{
  const usize pixelCount = static_cast<usize>(k_Width) * k_Height;
  const usize bytesPerPixel = samplesPerPixel * bytesPerSample;
  std::vector<uint8_t> pixels(pixelCount * bytesPerPixel, 0xCD);
  coverage.assign(pixelCount, 0);

  TiffImageIO imageIO;
  auto result = imageIO.readPixelDataRows(path, [&](usize row, usize columnOffset, usize segmentPixelCount, std::span<const uint8_t> segment) -> Result<> {
    REQUIRE(row < k_Height);
    REQUIRE(columnOffset + segmentPixelCount <= k_Width);
    REQUIRE(segment.size() == segmentPixelCount * bytesPerPixel);
    std::memcpy(pixels.data() + (row * k_Width + columnOffset) * bytesPerPixel, segment.data(), segment.size());
    for(usize column = columnOffset; column < columnOffset + segmentPixelCount; ++column)
    {
      ++coverage[row * k_Width + column];
    }
    return {};
  });
  SIMPLNX_RESULT_REQUIRE_VALID(result)
  return pixels;
}

std::array<uint8_t, 4> ColorValue(uint32_t x, uint32_t y)
{
  return {static_cast<uint8_t>((x + 3 * y) % 101), static_cast<uint8_t>((17 + 5 * x + 7 * y) % 131), static_cast<uint8_t>((29 + 11 * x + 13 * y) % 151), static_cast<uint8_t>(160 + (x + y) % 80)};
}
} // namespace

TEST_CASE("TiffImageIO:: tiled uint8_t grayscale preserves photometric and orientation", "[TiffImageIO]")
{
  TemporaryTiffFile file("TiffImageIO_tiled_grayscale.tif");
  const std::array<uint16_t, 2> photometrics = {PHOTOMETRIC_MINISBLACK, PHOTOMETRIC_MINISWHITE};

  for(const uint16_t photometric : photometrics)
  {
    for(uint16_t orientation = ORIENTATION_TOPLEFT; orientation <= ORIENTATION_LEFTBOT; ++orientation)
    {
      CAPTURE(photometric, orientation);
      WriteTiledUInt8(file.path(), 1, photometric, orientation, PLANARCONFIG_CONTIG, [](uint32_t x, uint32_t y, uint16_t) -> uint8_t { return static_cast<uint8_t>((x + 3 * y) % 251); });

      const usize pixelCount = static_cast<usize>(k_Width) * k_Height;
      const std::vector<uint8_t> direct = ReadDirect(file.path(), pixelCount);
      std::vector<uint8_t> coverage;
      const std::vector<uint8_t> rows = ReadRows(file.path(), 1, 1, coverage);
      REQUIRE(coverage == std::vector<uint8_t>(pixelCount, 1));
      REQUIRE(rows == direct);

      const bool flipX = orientation == ORIENTATION_TOPRIGHT || orientation == ORIENTATION_BOTRIGHT || orientation == ORIENTATION_RIGHTTOP || orientation == ORIENTATION_RIGHTBOT;
      const bool flipY = orientation == ORIENTATION_BOTRIGHT || orientation == ORIENTATION_BOTLEFT || orientation == ORIENTATION_RIGHTBOT || orientation == ORIENTATION_LEFTBOT;
      for(usize outputY = 0; outputY < k_Height; ++outputY)
      {
        for(usize outputX = 0; outputX < k_Width; ++outputX)
        {
          const usize sourceX = flipX ? k_Width - 1 - outputX : outputX;
          const usize sourceY = flipY ? k_Height - 1 - outputY : outputY;
          const uint8_t raw = static_cast<uint8_t>((sourceX + 3 * sourceY) % 251);
          const uint8_t expected = photometric == PHOTOMETRIC_MINISWHITE ? static_cast<uint8_t>(255 - raw) : raw;
          REQUIRE(direct[outputY * k_Width + outputX] == expected);
        }
      }
    }
  }
}

TEST_CASE("TiffImageIO:: tiled uint8_t preserves RGB and RGBA components", "[TiffImageIO]")
{
  const std::array<std::pair<uint16_t, uint16_t>, 3> layouts = {{{3, PLANARCONFIG_CONTIG}, {3, PLANARCONFIG_SEPARATE}, {4, PLANARCONFIG_CONTIG}}};
  for(const auto [samplesPerPixel, planarConfig] : layouts)
  {
    CAPTURE(samplesPerPixel, planarConfig);
    TemporaryTiffFile file("TiffImageIO_tiled_color.tif");
    WriteTiledUInt8(file.path(), samplesPerPixel, PHOTOMETRIC_RGB, ORIENTATION_TOPLEFT, planarConfig,
                    [](uint32_t x, uint32_t y, uint16_t component) -> uint8_t { return ColorValue(x, y)[component]; });

    const usize byteCount = static_cast<usize>(k_Width) * k_Height * samplesPerPixel;
    const std::vector<uint8_t> direct = ReadDirect(file.path(), byteCount);
    std::vector<uint8_t> coverage;
    const std::vector<uint8_t> rows = ReadRows(file.path(), samplesPerPixel, 1, coverage);
    REQUIRE(coverage == std::vector<uint8_t>(static_cast<usize>(k_Width) * k_Height, 1));
    REQUIRE(rows == direct);

    for(usize y = 0; y < k_Height; ++y)
    {
      for(usize x = 0; x < k_Width; ++x)
      {
        const auto expected = ColorValue(static_cast<uint32_t>(x), static_cast<uint32_t>(y));
        for(usize component = 0; component < samplesPerPixel; ++component)
        {
          REQUIRE(direct[(y * k_Width + x) * samplesPerPixel + component] == expected[component]);
        }
      }
    }
  }
}

TEST_CASE("TiffImageIO:: tiled uint16_t and float32 remain lossless", "[TiffImageIO]")
{
  SECTION("uint16_t")
  {
    TemporaryTiffFile file("TiffImageIO_tiled_uint16.tif");
    WriteTiledScalar<uint16_t>(file.path(), SAMPLEFORMAT_UINT, [](uint32_t x, uint32_t y) -> uint16_t { return static_cast<uint16_t>(1000 + x * 17 + y * 31); });
    const std::vector<uint8_t> bytes = ReadDirect(file.path(), static_cast<usize>(k_Width) * k_Height * sizeof(uint16_t));
    std::vector<uint8_t> coverage;
    const std::vector<uint8_t> rowBytes = ReadRows(file.path(), 1, sizeof(uint16_t), coverage);
    REQUIRE(coverage == std::vector<uint8_t>(static_cast<usize>(k_Width) * k_Height, 1));
    REQUIRE(rowBytes == bytes);
    std::vector<uint16_t> actual(static_cast<usize>(k_Width) * k_Height);
    std::memcpy(actual.data(), rowBytes.data(), rowBytes.size());
    for(usize y = 0; y < k_Height; ++y)
    {
      for(usize x = 0; x < k_Width; ++x)
      {
        REQUIRE(actual[y * k_Width + x] == static_cast<uint16_t>(1000 + x * 17 + y * 31));
      }
    }
  }

  SECTION("float32")
  {
    TemporaryTiffFile file("TiffImageIO_tiled_float32.tif");
    WriteTiledScalar<float32>(file.path(), SAMPLEFORMAT_IEEEFP, [](uint32_t x, uint32_t y) -> float32 { return static_cast<float32>(x) * 0.25F - static_cast<float32>(y) * 1.5F; });
    const std::vector<uint8_t> bytes = ReadDirect(file.path(), static_cast<usize>(k_Width) * k_Height * sizeof(float32));
    std::vector<uint8_t> coverage;
    const std::vector<uint8_t> rowBytes = ReadRows(file.path(), 1, sizeof(float32), coverage);
    REQUIRE(coverage == std::vector<uint8_t>(static_cast<usize>(k_Width) * k_Height, 1));
    REQUIRE(rowBytes == bytes);
    std::vector<float32> actual(static_cast<usize>(k_Width) * k_Height);
    std::memcpy(actual.data(), rowBytes.data(), rowBytes.size());
    for(usize y = 0; y < k_Height; ++y)
    {
      for(usize x = 0; x < k_Width; ++x)
      {
        REQUIRE(actual[y * k_Width + x] == static_cast<float32>(x) * 0.25F - static_cast<float32>(y) * 1.5F);
      }
    }
  }
}

TEST_CASE("TiffImageIO:: tiled uint8_t row callback returns the first error", "[TiffImageIO]")
{
  TemporaryTiffFile file("TiffImageIO_callback_error.tif");
  WriteTiledUInt8(file.path(), 1, PHOTOMETRIC_MINISBLACK, ORIENTATION_TOPLEFT, PLANARCONFIG_CONTIG,
                  [](uint32_t x, uint32_t y, uint16_t) -> uint8_t { return static_cast<uint8_t>((x + 3 * y) % 251); });

  constexpr int32_t k_CallbackError = -98765;
  constexpr usize k_FailingCallback = 3;
  usize callbackCount = 0;
  TiffImageIO imageIO;
  auto result = imageIO.readPixelDataRows(file.path(), [&callbackCount](usize, usize, usize, std::span<const uint8_t>) -> Result<> {
    ++callbackCount;
    if(callbackCount == k_FailingCallback)
    {
      return MakeErrorResult(k_CallbackError, "Intentional tiled uint8_t TIFF row callback failure.");
    }
    return {};
  });

  REQUIRE(result.invalid());
  REQUIRE(callbackCount == k_FailingCallback);
  REQUIRE(result.errors().size() == 1);
  REQUIRE(result.errors().front().code == k_CallbackError);
  REQUIRE(result.errors().front().message == "Intentional tiled uint8_t TIFF row callback failure.");
}

TEST_CASE("TiffImageIO:: raw tiled callback returns the first error", "[TiffImageIO]")
{
  TemporaryTiffFile file("TiffImageIO_raw_tiled_callback_error.tif");
  WriteTiledScalar<uint16_t>(file.path(), SAMPLEFORMAT_UINT, [](uint32_t x, uint32_t y) -> uint16_t { return static_cast<uint16_t>(1000 + x * 17 + y * 31); });

  constexpr int32_t k_CallbackError = -98766;
  constexpr usize k_FailingCallback = 3;
  usize callbackCount = 0;
  TiffImageIO imageIO;
  auto result = imageIO.readPixelDataRows(file.path(), [&callbackCount](usize, usize, usize, std::span<const uint8_t>) -> Result<> {
    ++callbackCount;
    if(callbackCount == k_FailingCallback)
    {
      return MakeErrorResult(k_CallbackError, "Intentional raw tiled TIFF row callback failure.");
    }
    return {};
  });

  REQUIRE(result.invalid());
  REQUIRE(callbackCount == k_FailingCallback);
  REQUIRE(result.errors().size() == 1);
  REQUIRE(result.errors().front().code == k_CallbackError);
  REQUIRE(result.errors().front().message == "Intentional raw tiled TIFF row callback failure.");
}

TEST_CASE("TiffImageIO:: stripped callback returns the first error", "[TiffImageIO]")
{
  TemporaryTiffFile file("TiffImageIO_stripped_callback_error.tif");
  WriteStrippedUInt8(file.path());

  constexpr int32_t k_CallbackError = -98767;
  constexpr usize k_FailingCallback = 3;
  usize callbackCount = 0;
  TiffImageIO imageIO;
  auto result = imageIO.readPixelDataRows(file.path(), [&callbackCount](usize, usize, usize, std::span<const uint8_t>) -> Result<> {
    ++callbackCount;
    if(callbackCount == k_FailingCallback)
    {
      return MakeErrorResult(k_CallbackError, "Intentional stripped TIFF row callback failure.");
    }
    return {};
  });

  REQUIRE(result.invalid());
  REQUIRE(callbackCount == k_FailingCallback);
  REQUIRE(result.errors().size() == 1);
  REQUIRE(result.errors().front().code == k_CallbackError);
  REQUIRE(result.errors().front().message == "Intentional stripped TIFF row callback failure.");
}

TEST_CASE("TiffImageIO:: direct read rejects a short buffer without mutation", "[TiffImageIO]")
{
  TemporaryTiffFile file("TiffImageIO_short_direct_buffer.tif");
  WriteTiledScalar<float32>(file.path(), SAMPLEFORMAT_IEEEFP, [](uint32_t x, uint32_t y) -> float32 { return static_cast<float32>(x) * 0.25F - static_cast<float32>(y) * 1.5F; });

  std::vector<uint8_t> poison(static_cast<usize>(k_Width) * k_Height * sizeof(float32) - 1, 0xCD);
  TiffImageIO imageIO;
  const auto result = imageIO.readPixelData(file.path(), poison);

  REQUIRE(result.invalid());
  REQUIRE(result.errors().size() == 1);
  REQUIRE(result.errors().front().code == -20105);
  REQUIRE(std::all_of(poison.cbegin(), poison.cend(), [](uint8_t value) { return value == 0xCD; }));
}

namespace
{
// Appends one stripped page whose samples are firstValue, firstValue + 1, ... in row-major order. An empty
// sampleFormat leaves the tag out (TIFF then defaults to unsigned); an empty subfileType leaves that tag out.
template <class T>
void WriteLayoutDirectory(TIFF* tiff, uint32_t width, uint32_t height, uint16_t components, std::optional<uint16_t> sampleFormat, std::optional<uint32_t> subfileType, T firstValue)
{
  REQUIRE(TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, width) == 1);
  REQUIRE(TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, height) == 1);
  REQUIRE(TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, components) == 1);
  REQUIRE(TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, static_cast<uint16_t>(sizeof(T) * 8)) == 1);
  if(sampleFormat.has_value())
  {
    REQUIRE(TIFFSetField(tiff, TIFFTAG_SAMPLEFORMAT, *sampleFormat) == 1);
  }
  if(subfileType.has_value())
  {
    REQUIRE(TIFFSetField(tiff, TIFFTAG_SUBFILETYPE, *subfileType) == 1);
  }
  REQUIRE(TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG) == 1);
  REQUIRE(TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, components == 3 ? PHOTOMETRIC_RGB : PHOTOMETRIC_MINISBLACK) == 1);
  REQUIRE(TIFFSetField(tiff, TIFFTAG_COMPRESSION, COMPRESSION_NONE) == 1);
  std::vector<T> row(static_cast<usize>(width) * components);
  for(uint32_t y = 0; y < height; ++y)
  {
    for(usize i = 0; i < row.size(); ++i)
    {
      row[i] = static_cast<T>(firstValue + static_cast<T>(y * row.size() + i));
    }
    REQUIRE(TIFFWriteScanline(tiff, row.data(), y, 0) == 1);
  }
  REQUIRE(TIFFWriteDirectory(tiff) == 1);
}

void RequireLayout(const IImageIO::PageMetadata& layout, usize width, usize height, usize components, usize bits, IImageIO::SampleFormat format)
{
  CHECK(layout.width == width);
  CHECK(layout.height == height);
  CHECK(layout.numComponents == components);
  CHECK(layout.bitsPerSample == bits);
  CHECK(layout.sampleFormat == format);
}
} // namespace

TEST_CASE("TiffImageIO:: readPageLayouts returns each page in order", "[simplnx][TiffImageIO]")
{
  TemporaryTiffFile file("TiffImageIO_page_layouts.tif");
  {
    TiffHandle tiff(TIFFOpen(file.path().string().c_str(), "w"), TIFFClose);
    REQUIRE(tiff != nullptr);
    WriteLayoutDirectory<uint8_t>(tiff.get(), 2, 3, 1, std::nullopt, std::nullopt, 1);        // no SampleFormat tag
    WriteLayoutDirectory<uint16_t>(tiff.get(), 4, 1, 1, SAMPLEFORMAT_UINT, std::nullopt, 10); // explicit unsigned
    WriteLayoutDirectory<int16_t>(tiff.get(), 1, 2, 1, SAMPLEFORMAT_INT, std::nullopt, -5);
    WriteLayoutDirectory<float32>(tiff.get(), 3, 2, 1, SAMPLEFORMAT_IEEEFP, std::nullopt, 0.5F);
    WriteLayoutDirectory<uint8_t>(tiff.get(), 2, 2, 3, SAMPLEFORMAT_VOID, std::nullopt, 7);
  }

  TiffImageIO imageIO;
  const auto result = imageIO.readPageLayouts(file.path());
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const auto& layouts = result.value();
  REQUIRE(layouts.size() == 5);
  RequireLayout(layouts[0], 2, 3, 1, 8, IImageIO::SampleFormat::Unsigned); // TIFF default sample format
  RequireLayout(layouts[1], 4, 1, 1, 16, IImageIO::SampleFormat::Unsigned);
  RequireLayout(layouts[2], 1, 2, 1, 16, IImageIO::SampleFormat::Signed);
  RequireLayout(layouts[3], 3, 2, 1, 32, IImageIO::SampleFormat::Float);
  RequireLayout(layouts[4], 2, 2, 3, 8, IImageIO::SampleFormat::Unknown);
}

TEST_CASE("TiffImageIO:: readPageLayouts skips the same subfiles as readPixelDataRows", "[simplnx][TiffImageIO]")
{
  // Directories: 0 page, 1 reduced-resolution image, 2 transparency mask, 3 page. Only directories 0 and 3 are pages.
  TemporaryTiffFile file("TiffImageIO_page_layouts_subfiles.tif");
  {
    TiffHandle tiff(TIFFOpen(file.path().string().c_str(), "w"), TIFFClose);
    REQUIRE(tiff != nullptr);
    WriteLayoutDirectory<uint16_t>(tiff.get(), 2, 2, 1, SAMPLEFORMAT_UINT, 0, 100);
    WriteLayoutDirectory<uint8_t>(tiff.get(), 1, 1, 3, SAMPLEFORMAT_UINT, FILETYPE_REDUCEDIMAGE, 1);
    WriteLayoutDirectory<uint8_t>(tiff.get(), 2, 2, 1, SAMPLEFORMAT_UINT, FILETYPE_MASK, 2);
    WriteLayoutDirectory<uint16_t>(tiff.get(), 2, 2, 1, SAMPLEFORMAT_UINT, 0, 200);
  }

  TiffImageIO imageIO;
  const auto metadata = imageIO.readMetadata(file.path());
  SIMPLNX_RESULT_REQUIRE_VALID(metadata);
  REQUIRE(metadata.value().numPages == 2);
  const auto result = imageIO.readPageLayouts(file.path());
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE(result.value().size() == metadata.value().numPages);
  RequireLayout(result.value()[0], 2, 2, 1, 16, IImageIO::SampleFormat::Unsigned);
  RequireLayout(result.value()[1], 2, 2, 1, 16, IImageIO::SampleFormat::Unsigned);

  // readPixelDataRows maps page 1 to directory 3: its first sample is 200, not a value of the skipped subfiles.
  std::vector<uint16_t> page1;
  const auto readResult = imageIO.readPixelDataRows(
      file.path(),
      [&page1](usize, usize, usize, std::span<const uint8_t> segment) -> Result<> {
        const usize start = page1.size();
        page1.resize(start + segment.size() / sizeof(uint16_t));
        std::memcpy(page1.data() + start, segment.data(), segment.size());
        return {};
      },
      1);
  SIMPLNX_RESULT_REQUIRE_VALID(readResult);
  REQUIRE(page1 == std::vector<uint16_t>{200, 201, 202, 203});
}

TEST_CASE("TiffImageIO:: readPageLayouts reports a file that cannot be read", "[simplnx][TiffImageIO]")
{
  TiffImageIO imageIO;
  SECTION("Missing file")
  {
    const fs::path path = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "TiffImageIO_page_layouts_missing.tif";
    std::error_code errorCode;
    fs::remove(path, errorCode);
    const auto result = imageIO.readPageLayouts(path);
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == -20100);
    REQUIRE(result.errors().front().message.find(path.string()) != std::string::npos);
  }
  SECTION("File that is not a TIFF")
  {
    TemporaryTiffFile file("TiffImageIO_page_layouts_not_a_tiff.tif");
    {
      std::ofstream stream(file.path(), std::ios::binary);
      stream << "This is not a TIFF file.";
    }
    const auto result = imageIO.readPageLayouts(file.path());
    REQUIRE(result.invalid());
  }
}

TEST_CASE("TiffImageIO:: IImageIO default readPageLayouts returns the single page", "[simplnx][TiffImageIO]")
{
  // StbImageIO has no override, so it uses the IImageIO default built from readMetadata().
  const fs::path path = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "IImageIO_default_page_layout.png";
  std::error_code errorCode;
  fs::remove(path, errorCode);
  StbImageIO imageIO;
  ImageMetadata metadata;
  metadata.width = 3;
  metadata.height = 2;
  metadata.numComponents = 3;
  metadata.dataType = DataType::uint8;
  const std::vector<uint8_t> pixels(18, 42);
  const auto writeResult = imageIO.writePixelData(path, pixels, metadata);
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  const auto result = imageIO.readPageLayouts(path);
  fs::remove(path, errorCode);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE(result.value().size() == 1);
  RequireLayout(result.value()[0], 3, 2, 3, 8, IImageIO::SampleFormat::Unsigned);
}
