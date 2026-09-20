#include "simplnx/Utilities/ImageIO/TiffImageIO.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>
#include <tiffio.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <memory>
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

TEST_CASE("TiffImageIO:: tiled uint16 and float32 remain lossless", "[TiffImageIO]")
{
  SECTION("uint16")
  {
    TemporaryTiffFile file("TiffImageIO_tiled_uint16.tif");
    WriteTiledScalar<uint16>(file.path(), SAMPLEFORMAT_UINT, [](uint32_t x, uint32_t y) -> uint16 { return static_cast<uint16>(1000 + x * 17 + y * 31); });
    const std::vector<uint8_t> bytes = ReadDirect(file.path(), static_cast<usize>(k_Width) * k_Height * sizeof(uint16));
    std::vector<uint8_t> coverage;
    const std::vector<uint8_t> rowBytes = ReadRows(file.path(), 1, sizeof(uint16), coverage);
    REQUIRE(coverage == std::vector<uint8_t>(static_cast<usize>(k_Width) * k_Height, 1));
    REQUIRE(rowBytes == bytes);
    std::vector<uint16> actual(static_cast<usize>(k_Width) * k_Height);
    std::memcpy(actual.data(), rowBytes.data(), rowBytes.size());
    for(usize y = 0; y < k_Height; ++y)
    {
      for(usize x = 0; x < k_Width; ++x)
      {
        REQUIRE(actual[y * k_Width + x] == static_cast<uint16>(1000 + x * 17 + y * 31));
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
  WriteTiledScalar<uint16>(file.path(), SAMPLEFORMAT_UINT, [](uint32_t x, uint32_t y) -> uint16 { return static_cast<uint16>(1000 + x * 17 + y * 31); });

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
