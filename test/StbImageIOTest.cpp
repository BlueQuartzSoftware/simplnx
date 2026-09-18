#include "simplnx/Utilities/ImageIO/StbImageIO.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
constexpr usize k_Width = 23;
constexpr usize k_Height = 17;
constexpr usize k_NumComponents = 3;

class TemporaryPngFile
{
public:
  explicit TemporaryPngFile(const std::string& name)
  : m_Path(fs::path(unit_test::k_BinaryTestOutputDir.view()) / name)
  {
    std::error_code errorCode;
    fs::remove(m_Path, errorCode);
  }

  ~TemporaryPngFile() noexcept
  {
    std::error_code errorCode;
    fs::remove(m_Path, errorCode);
  }

  TemporaryPngFile(const TemporaryPngFile&) = delete;
  TemporaryPngFile(TemporaryPngFile&&) = delete;
  TemporaryPngFile& operator=(const TemporaryPngFile&) = delete;
  TemporaryPngFile& operator=(TemporaryPngFile&&) = delete;

  const fs::path& path() const
  {
    return m_Path;
  }

private:
  fs::path m_Path;
};

uint8 PixelValue(usize x, usize y, usize component)
{
  const std::array<uint8, k_NumComponents> values = {static_cast<uint8>((x + 3 * y) % 251), static_cast<uint8>((17 + 5 * x + 7 * y) % 241), static_cast<uint8>((29 + 11 * x + 13 * y) % 239)};
  return values[component];
}

std::vector<uint8> WritePng(const fs::path& path)
{
  std::vector<uint8> pixels(k_Width * k_Height * k_NumComponents);
  for(usize y = 0; y < k_Height; ++y)
  {
    for(usize x = 0; x < k_Width; ++x)
    {
      for(usize component = 0; component < k_NumComponents; ++component)
      {
        pixels[(y * k_Width + x) * k_NumComponents + component] = PixelValue(x, y, component);
      }
    }
  }

  ImageMetadata metadata;
  metadata.width = k_Width;
  metadata.height = k_Height;
  metadata.numComponents = k_NumComponents;
  metadata.dataType = DataType::uint8;

  StbImageIO imageIO;
  const auto result = imageIO.writePixelData(path, pixels, metadata);
  SIMPLNX_RESULT_REQUIRE_VALID(result)
  return pixels;
}
} // namespace

TEST_CASE("StbImageIO:: PNG callback returns the first error", "[StbImageIO]")
{
  TemporaryPngFile file("StbImageIO_callback_error.png");
  const std::vector<uint8> expected = WritePng(file.path());

  constexpr int32_t k_CallbackError = -98768;
  constexpr usize k_FailingCallback = 3;
  constexpr usize k_RowBytes = k_Width * k_NumComponents;
  usize callbackCount = 0;
  StbImageIO imageIO;
  const auto result = imageIO.readPixelDataRows(file.path(), [&](usize row, usize columnOffset, usize pixelCount, std::span<const uint8> pixels) -> Result<> {
    ++callbackCount;
    REQUIRE(row == callbackCount - 1);
    REQUIRE(columnOffset == 0);
    REQUIRE(pixelCount == k_Width);
    REQUIRE(pixels.size() == k_RowBytes);
    REQUIRE(std::equal(pixels.begin(), pixels.end(), expected.cbegin() + row * k_RowBytes));
    if(callbackCount == k_FailingCallback)
    {
      return MakeErrorResult(k_CallbackError, "Intentional PNG row callback failure.");
    }
    return {};
  });

  REQUIRE(result.invalid());
  REQUIRE(callbackCount == k_FailingCallback);
  REQUIRE(result.errors().size() == 1);
  REQUIRE(result.errors().front().code == k_CallbackError);
  REQUIRE(result.errors().front().message == "Intentional PNG row callback failure.");
}

TEST_CASE("StbImageIO:: direct read rejects a short buffer without mutation", "[StbImageIO]")
{
  TemporaryPngFile file("StbImageIO_short_direct_buffer.png");
  const std::vector<uint8> expected = WritePng(file.path());
  std::vector<uint8> poison(expected.size() - 1, 0xCD);

  StbImageIO imageIO;
  const auto result = imageIO.readPixelData(file.path(), poison);

  REQUIRE(result.invalid());
  REQUIRE(result.errors().size() == 1);
  REQUIRE(result.errors().front().code == -20005);
  REQUIRE(std::all_of(poison.cbegin(), poison.cend(), [](uint8 value) { return value == 0xCD; }));
}
