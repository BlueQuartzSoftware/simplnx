#include "simplnx/Common/Types.hpp"
#include "simplnx/Utilities/ImageProcessing/TerminalImageConversion.hpp"

#include <catch2/catch.hpp>
#include <nonstd/span.hpp>

#include <array>
#include <atomic>
#include <cmath>
#include <limits>

using namespace nx::core;

TEST_CASE("Image terminal conversion: last finite Float32 rounding interval", "[simplnx][ImageNumericContracts]")
{
  const auto sign = GENERATE(-1.0, 1.0);
  const float32 maximum = std::numeric_limits<float32>::max();
  constexpr float64 midpoint = 0x1.ffffffp127;
  for(const float64 magnitude : {static_cast<float64>(maximum), std::nextafter(static_cast<float64>(maximum), midpoint), std::nextafter(midpoint, 0.0)})
  {
    // NOLINTNEXTLINE(bugprone-chained-comparison) Catch2 comparison decomposition.
    REQUIRE(ImageProcessing::detail::NarrowFloatingImageValue<float32>(sign * magnitude) == static_cast<float32>(sign) * maximum);
  }
  for(const float64 magnitude : {midpoint, std::nextafter(midpoint, std::numeric_limits<float64>::infinity())})
  {
    const float32 result = ImageProcessing::detail::NarrowFloatingImageValue<float32>(sign * magnitude);
    REQUIRE(std::isinf(result));
    // NOLINTNEXTLINE(bugprone-chained-comparison) Catch2 comparison decomposition.
    REQUIRE(std::signbit(result) == (sign < 0));
  }
}

// NOLINTBEGIN(bugprone-chained-comparison) Catch2 comparison decomposition.
TEMPLATE_TEST_CASE("Image terminal conversion: exact integer bounds", "[simplnx][ImageNumericContracts]", int8, uint8, int16, uint16, int32, uint32, int64, uint64)
{
  using ImageProcessing::detail::ConvertTerminalImageValue;
  using ImageProcessing::detail::TerminalImageConversionStatus;
  TestType output = 7;
  REQUIRE(ConvertTerminalImageValue(93.75, output) == TerminalImageConversionStatus::Valid);
  REQUIRE(output == TestType{93});
  REQUIRE(ConvertTerminalImageValue(-0.5, output) == TerminalImageConversionStatus::Valid);
  REQUIRE(output == TestType{0});
  const double upper = std::ldexp(1.0, std::numeric_limits<TestType>::digits);
  REQUIRE(ConvertTerminalImageValue(upper, output) == TerminalImageConversionStatus::Clipped);
  REQUIRE(output == std::numeric_limits<TestType>::max());
  if constexpr(std::numeric_limits<TestType>::digits <= 32)
  {
    REQUIRE(ConvertTerminalImageValue(static_cast<double>(std::numeric_limits<TestType>::max()) + 0.5, output) == TerminalImageConversionStatus::Valid);
    REQUIRE(output == std::numeric_limits<TestType>::max());
  }
  if constexpr(std::is_signed_v<TestType>)
  {
    REQUIRE(ConvertTerminalImageValue(-upper, output) == TerminalImageConversionStatus::Valid);
    REQUIRE(output == std::numeric_limits<TestType>::lowest());
    REQUIRE(ConvertTerminalImageValue(std::nextafter(-upper - 1.0, -std::numeric_limits<double>::infinity()), output) == TerminalImageConversionStatus::Clipped);
    REQUIRE(output == std::numeric_limits<TestType>::lowest());
    REQUIRE(ConvertTerminalImageValue(-1.5, output) == TerminalImageConversionStatus::Valid);
    REQUIRE(output == TestType{-1});
  }
  else
  {
    REQUIRE(ConvertTerminalImageValue(-1.5, output) == TerminalImageConversionStatus::Clipped);
    REQUIRE(output == TestType{0});
  }
  for(const double value : {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()})
  {
    output = 7;
    REQUIRE(ConvertTerminalImageValue(value, output) == TerminalImageConversionStatus::NonFinite);
    REQUIRE(output == TestType{7});
  }
}
// NOLINTEND(bugprone-chained-comparison)

TEST_CASE("Image terminal conversion: IEEE floating results", "[simplnx][ImageNumericContracts]")
{
  using ImageProcessing::detail::NarrowFloatingImageValue;
  const auto positive = NarrowFloatingImageValue<float32>(std::numeric_limits<float64>::max());
  const auto negative = NarrowFloatingImageValue<float32>(-std::numeric_limits<float64>::max());
  REQUIRE(std::isinf(positive));
  REQUIRE_FALSE(std::signbit(positive));
  REQUIRE(std::isinf(negative));
  REQUIRE(std::signbit(negative));
  REQUIRE(std::isnan(NarrowFloatingImageValue<float32>(std::numeric_limits<float64>::quiet_NaN())));
  REQUIRE(std::signbit(NarrowFloatingImageValue<float32>(-0.0)));
  // NOLINTNEXTLINE(bugprone-chained-comparison) Catch2 comparison decomposition.
  REQUIRE(NarrowFloatingImageValue<float64>(std::numeric_limits<float32>::max()) == static_cast<float64>(std::numeric_limits<float32>::max()));
}

TEST_CASE("Image terminal conversion: a single rejection reports its image index", "[simplnx][ImageNumericContracts]")
{
  const bool tile = GENERATE(false, true);
  const std::array<float64, 6> input{1, 2, 256, std::numeric_limits<float64>::quiet_NaN(), 5, 6};
  std::array<uint8, 6> output{};
  const std::atomic_bool cancelled = false;
  ImageProcessing::detail::TerminalConversionState state(input.size());
  REQUIRE_FALSE((ImageProcessing::detail::ConvertTerminalImageBuffer<uint8, float64>(input, output, 13, state, cancelled, tile ? 2 : 0, tile ? 8 : 0)));
  // NOLINTBEGIN(bugprone-chained-comparison) Catch2 comparison decomposition.
  REQUIRE(state.nonFiniteCount() == 1);
  REQUIRE(state.nonFiniteIndex() == (tile ? 22 : 16));
  REQUIRE(state.clippedCount() == 1);
  // NOLINTEND(bugprone-chained-comparison)
  REQUIRE_FALSE(state.partial());
}

TEST_CASE("Image terminal conversion: a rejected resident block preserves every destination value", "[simplnx][ImageNumericContracts]")
{
  const std::array<float32, 3> rejected{1, std::numeric_limits<float32>::infinity(), 3};
  const std::array<float32, 2> good{4, 5};
  std::array<int32, 5> output{71, 72, 73, 74, 75};
  const std::atomic_bool cancelled = false;
  ImageProcessing::detail::TerminalConversionState state(output.size());
  auto destination = nonstd::span<int32>(output);
  ImageProcessing::detail::ConvertResidentTerminalImageBuffer<int32, float32>(rejected, destination.first(3), 0, state, cancelled);
  ImageProcessing::detail::ConvertResidentTerminalImageBuffer<int32, float32>(good, destination.subspan(3), 3, state, cancelled);
  // NOLINTBEGIN(bugprone-chained-comparison) Catch2 comparison decomposition.
  REQUIRE(output == std::array<int32, 5>{71, 72, 73, 4, 5});
  REQUIRE(state.nonFiniteCount() == 1);
  REQUIRE(state.nonFiniteIndex() == 1);
  // NOLINTEND(bugprone-chained-comparison)
  REQUIRE_FALSE(state.partial());
}
