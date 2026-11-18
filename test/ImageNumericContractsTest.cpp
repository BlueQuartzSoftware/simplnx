#include "simplnx/Common/Types.hpp"
#include "simplnx/Utilities/ImageProcessing/TerminalImageConversion.hpp"

#include <catch2/catch.hpp>

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
