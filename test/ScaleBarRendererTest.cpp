#include "simplnx/Utilities/ScaleBarRenderer.hpp"

#include <catch2/catch.hpp>

#include <string>

using namespace nx::core;
using LengthUnit = IGeometry::LengthUnit;

TEST_CASE("Simplnx::ScaleBarRenderer: ComputeNiceBarLength", "[Simplnx][ScaleBarRenderer]")
{
  // target = 0.25 * width * unitsPerPixel; result is the largest 1/2/5 x 10^n <= target
  REQUIRE(ScaleBarRenderer::ComputeNiceBarLength(512, 1.0) == Approx(100.0));    // target 128
  REQUIRE(ScaleBarRenderer::ComputeNiceBarLength(512, 0.25) == Approx(20.0));    // target 32
  REQUIRE(ScaleBarRenderer::ComputeNiceBarLength(100, 1.0) == Approx(20.0));     // target 25
  REQUIRE(ScaleBarRenderer::ComputeNiceBarLength(1000, 0.5) == Approx(100.0));   // target 125
  REQUIRE(ScaleBarRenderer::ComputeNiceBarLength(200, 0.35) == Approx(10.0));    // target 17.5
  REQUIRE(ScaleBarRenderer::ComputeNiceBarLength(512, 2.0) == Approx(200.0));    // target 256
  REQUIRE(ScaleBarRenderer::ComputeNiceBarLength(400, 0.5) == Approx(50.0));     // target 50 (exact boundary)
  REQUIRE(ScaleBarRenderer::ComputeNiceBarLength(10, 1.0e-6) == Approx(2.0e-6)); // target 2.5e-6
  // Degenerate inputs return 0.0 (callers validate spacing during preflight)
  REQUIRE(ScaleBarRenderer::ComputeNiceBarLength(0, 1.0) == Approx(0.0));
  REQUIRE(ScaleBarRenderer::ComputeNiceBarLength(512, 0.0) == Approx(0.0));
  REQUIRE(ScaleBarRenderer::ComputeNiceBarLength(512, -1.0) == Approx(0.0));
}

TEST_CASE("Simplnx::ScaleBarRenderer: FormatLengthLabel", "[Simplnx][ScaleBarRenderer]")
{
  // Metric units rescale to the most readable engineering prefix (mantissa in [1, 1000))
  REQUIRE(ScaleBarRenderer::FormatLengthLabel(100.0, LengthUnit::Micrometer) == "100 µm");
  REQUIRE(ScaleBarRenderer::FormatLengthLabel(0.0001, LengthUnit::Meter) == "100 µm");
  REQUIRE(ScaleBarRenderer::FormatLengthLabel(2.0, LengthUnit::Millimeter) == "2 mm");
  REQUIRE(ScaleBarRenderer::FormatLengthLabel(500.0, LengthUnit::Nanometer) == "500 nm");
  REQUIRE(ScaleBarRenderer::FormatLengthLabel(0.05, LengthUnit::Micrometer) == "50 nm");
  REQUIRE(ScaleBarRenderer::FormatLengthLabel(1000.0, LengthUnit::Micrometer) == "1 mm");
  REQUIRE(ScaleBarRenderer::FormatLengthLabel(1.0, LengthUnit::Meter) == "1 m");
  REQUIRE(ScaleBarRenderer::FormatLengthLabel(5.0, LengthUnit::Kilometer) == "5 km");
  // Angstrom converts like a metric unit (1 A = 1e-10 m)
  REQUIRE(ScaleBarRenderer::FormatLengthLabel(100.0, LengthUnit::Angstrom) == "10 nm");
  // Non-metric units are labeled as-is with the unit name, no rescaling
  REQUIRE(ScaleBarRenderer::FormatLengthLabel(2.0, LengthUnit::Inch) == "2 " + IGeometry::LengthUnitToString(LengthUnit::Inch));
  // Unspecified / Unknown units omit the suffix entirely
  REQUIRE(ScaleBarRenderer::FormatLengthLabel(100.0, LengthUnit::Unspecified) == "100");
  REQUIRE(ScaleBarRenderer::FormatLengthLabel(100.0, LengthUnit::Unknown) == "100");
}

TEST_CASE("Simplnx::ScaleBarRenderer: ComputeBandHeight", "[Simplnx][ScaleBarRenderer]")
{
  REQUIRE(ScaleBarRenderer::ComputeBandHeight(100) == 24);  // 8 -> clamped to minimum 24
  REQUIRE(ScaleBarRenderer::ComputeBandHeight(300) == 24);  // 24 exactly
  REQUIRE(ScaleBarRenderer::ComputeBandHeight(500) == 40);  // 40
  REQUIRE(ScaleBarRenderer::ComputeBandHeight(1000) == 80); // 80
  REQUIRE(ScaleBarRenderer::ComputeBandHeight(0) == 24);    // degenerate -> minimum
}
