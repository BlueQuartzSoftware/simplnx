#include "simplnx/Common/Types.hpp"
#include "simplnx/Utilities/ImageProcessing/TerminalImageConversion.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <cmath>
#include <limits>

using namespace nx::core;

TEST_CASE("Image terminal conversion: Float32 overflow rounds at the half-ULP boundary", "[simplnx][ImageNumericContracts]")
{
  const float64 sign = GENERATE(-1.0, 1.0);
  const float32 maximum = std::numeric_limits<float32>::max();
  const float64 finiteLimit = maximum;
  const float64 lastSpacing = finiteLimit - std::nextafter(maximum, 0.0F);
  const float64 midpoint = finiteLimit + (lastSpacing / 2);
  const float64 infinity = std::numeric_limits<float64>::infinity();

  for(const float64 magnitude : std::array{std::nextafter(finiteLimit, 0.0), finiteLimit, std::nextafter(finiteLimit, infinity), std::nextafter(midpoint, 0.0)})
  {
    REQUIRE(ImageProcessing::NarrowFloatingImageValue<float32>(sign * magnitude) == static_cast<float32>(sign) * maximum);
  }
  for(const float64 magnitude : std::array{midpoint, std::nextafter(midpoint, infinity), infinity})
  {
    const float32 result = ImageProcessing::NarrowFloatingImageValue<float32>(sign * magnitude);
    REQUIRE(std::isinf(result));
    REQUIRE(std::signbit(result) == (sign < 0));
  }
  REQUIRE(ImageProcessing::NarrowFloatingImageValue<float64>(sign * midpoint) == sign * midpoint);
}

TEST_CASE("Image terminal conversion: floating outputs preserve special values", "[simplnx][ImageNumericContracts]")
{
  REQUIRE(std::isnan(ImageProcessing::NarrowFloatingImageValue<float32>(std::numeric_limits<float64>::quiet_NaN())));
  REQUIRE_FALSE(std::signbit(ImageProcessing::NarrowFloatingImageValue<float32>(0.0)));
  REQUIRE(std::signbit(ImageProcessing::NarrowFloatingImageValue<float32>(-0.0)));
  REQUIRE(ImageProcessing::NarrowFloatingImageValue<float32>(static_cast<float64>(std::numeric_limits<float32>::denorm_min())) == std::numeric_limits<float32>::denorm_min());

  REQUIRE(ImageProcessing::NarrowFloatingImageValue<float64>(std::numeric_limits<float64>::denorm_min()) == std::numeric_limits<float64>::denorm_min());
  REQUIRE(std::signbit(ImageProcessing::NarrowFloatingImageValue<float64>(-0.0)));
}
