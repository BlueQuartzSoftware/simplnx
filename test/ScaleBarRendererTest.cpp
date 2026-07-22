#include "simplnx/Utilities/ScaleBarRenderer.hpp"

#include <catch2/catch.hpp>

#include <array>
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

TEST_CASE("Simplnx::ScaleBarRenderer: RenderScaleBarBandRgb", "[Simplnx][ScaleBarRenderer]")
{
  // 100 px wide, 50 px tall image, 1 µm/pixel:
  //   band      = max(24, llround(0.08*50)=4)            = 24
  //   margin    = max(2, llround(24*0.12)=3)             = 3
  //   thickness = max(2, llround(24*0.08)=2)             = 2
  //   nice      = ComputeNiceBarLength(100, 1.0)         = 20 µm -> 20 px bar
  //   barTopRow = (24 - 2) / 2                           = 11  (bar covers rows 11-12, vertically centered)
  //   barStart  = margin                                 = 3   (left-justified; bar covers cols 3-22)
  //   label     = same line, starting at col 3+20+3      = 26
  const usize width = 100;
  const std::vector<uint8> band = ScaleBarRenderer::RenderScaleBarBandRgb(width, 50, 1.0, IGeometry::LengthUnit::Micrometer);
  REQUIRE(band.size() == width * 24 * 3);

  auto pixel = [&](usize row, usize col) -> std::array<uint8, 3> {
    const usize i = (row * width + col) * 3;
    return {band[i], band[i + 1], band[i + 2]};
  };
  const std::array<uint8, 3> white = {255, 255, 255};
  const std::array<uint8, 3> black = {0, 0, 0};

  // Background corners are white
  REQUIRE(pixel(0, 0) == white);
  REQUIRE(pixel(23, 99) == white);

  // The bar is a crisp black run on both bar rows, white immediately outside it.
  // Column 2 is inside the left margin; column 23 sits in the gap between bar end (col 22)
  // and the label start (col 26).
  for(usize row : {usize(11), usize(12)})
  {
    REQUIRE(pixel(row, 2) == white);
    for(usize col = 3; col < 23; col++)
    {
      REQUIRE(pixel(row, col) == black);
    }
    REQUIRE(pixel(row, 23) == white);
  }

  // The label ("20 µm") renders on the same line to the right of the bar: at least one
  // visibly dark pixel in the text region (cols 26+). Anti-aliased glyphs are not pixel-compared.
  bool foundTextPixel = false;
  for(usize row = 0; row < 24 && !foundTextPixel; row++)
  {
    for(usize col = 26; col < width; col++)
    {
      if(pixel(row, col)[0] < 128)
      {
        foundTextPixel = true;
        break;
      }
    }
  }
  REQUIRE(foundTextPixel);
}
