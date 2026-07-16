#include "ScaleBarRenderer.hpp"

#include "simplnx/Utilities/Fonts/Fonts.hpp"
#include "simplnx/Utilities/Fonts/LatoRegular.hpp"

#include <fmt/format.h>

#define CANVAS_ITY_IMPLEMENTATION
#include <canvas_ity.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>

using namespace nx::core;

namespace
{
constexpr float64 k_TargetBarFraction = 0.25;    // bar targets 25% of the physical image width
constexpr float64 k_BandHeightFraction = 0.08;   // band is 8% of the image height...
constexpr usize k_MinBandHeight = 24;            // ...but never less than 24 pixels
constexpr float64 k_MarginFraction = 0.12;       // band-edge margin
constexpr float64 k_BarThicknessFraction = 0.08; // bar thickness relative to band height
constexpr float64 k_FontFraction = 0.45;         // font size relative to band height

// Power-of-ten exponent that converts one geometry unit to meters, or nullopt
// for non-metric units. Angstrom (1e-10 m) is treated as metric so labels
// rescale to nm/pm.
std::optional<int32> MetricExponent(IGeometry::LengthUnit unit)
{
  using LengthUnit = IGeometry::LengthUnit;
  switch(unit)
  {
  case LengthUnit::Yoctometer:
    return -24;
  case LengthUnit::Zeptometer:
    return -21;
  case LengthUnit::Attometer:
    return -18;
  case LengthUnit::Femtometer:
    return -15;
  case LengthUnit::Picometer:
    return -12;
  case LengthUnit::Nanometer:
    return -9;
  case LengthUnit::Micrometer:
    return -6;
  case LengthUnit::Millimeter:
    return -3;
  case LengthUnit::Centimeter:
    return -2;
  case LengthUnit::Decimeter:
    return -1;
  case LengthUnit::Meter:
    return 0;
  case LengthUnit::Decameter:
    return 1;
  case LengthUnit::Hectometer:
    return 2;
  case LengthUnit::Kilometer:
    return 3;
  case LengthUnit::Megameter:
    return 6;
  case LengthUnit::Gigameter:
    return 9;
  case LengthUnit::Terameter:
    return 12;
  case LengthUnit::Petameter:
    return 15;
  case LengthUnit::Exameter:
    return 18;
  case LengthUnit::Zettameter:
    return 21;
  case LengthUnit::Yottameter:
    return 24;
  case LengthUnit::Angstrom:
    return -10;
  default:
    return std::nullopt;
  }
}

// Engineering prefixes for meters, keyed by power-of-ten exponent (multiples of 3).
constexpr std::array<std::pair<int32, const char*>, 17> k_EngineeringPrefixes = {{{-24, "ym"},
                                                                                  {-21, "zm"},
                                                                                  {-18, "am"},
                                                                                  {-15, "fm"},
                                                                                  {-12, "pm"},
                                                                                  {-9, "nm"},
                                                                                  {-6, "µm"},
                                                                                  {-3, "mm"},
                                                                                  {0, "m"},
                                                                                  {3, "km"},
                                                                                  {6, "Mm"},
                                                                                  {9, "Gm"},
                                                                                  {12, "Tm"},
                                                                                  {15, "Pm"},
                                                                                  {18, "Em"},
                                                                                  {21, "Zm"},
                                                                                  {24, "Ym"}}};
} // namespace

namespace nx::core::ScaleBarRenderer
{
//------------------------------------------------------------------------------
float64 ComputeNiceBarLength(usize imageWidthPixels, float64 unitsPerPixel)
{
  const float64 target = k_TargetBarFraction * static_cast<float64>(imageWidthPixels) * unitsPerPixel;
  if(!std::isfinite(target) || target <= 0.0)
  {
    return 0.0;
  }
  const float64 magnitude = std::pow(10.0, std::floor(std::log10(target)));
  const float64 mantissa = target / magnitude; // in [1, 10)
  float64 nice = 1.0;
  if(mantissa >= 5.0)
  {
    nice = 5.0;
  }
  else if(mantissa >= 2.0)
  {
    nice = 2.0;
  }
  return nice * magnitude;
}

//------------------------------------------------------------------------------
std::string FormatLengthLabel(float64 lengthInUnits, IGeometry::LengthUnit unit)
{
  if(!std::isfinite(lengthInUnits) || lengthInUnits <= 0.0)
  {
    return fmt::format("{:g}", lengthInUnits);
  }

  const std::optional<int32> unitExponent = MetricExponent(unit);
  if(!unitExponent.has_value())
  {
    if(unit == IGeometry::LengthUnit::Unspecified || unit == IGeometry::LengthUnit::Unknown)
    {
      return fmt::format("{:g}", lengthInUnits);
    }
    return fmt::format("{:g} {}", lengthInUnits, IGeometry::LengthUnitToString(unit));
  }

  const float64 meters = lengthInUnits * std::pow(10.0, static_cast<float64>(unitExponent.value()));
  int32 engineeringExponent = static_cast<int32>(std::floor(std::log10(meters) / 3.0)) * 3;
  engineeringExponent = std::clamp(engineeringExponent, -24, 24);
  const float64 mantissa = meters / std::pow(10.0, static_cast<float64>(engineeringExponent));

  const char* symbol = "m";
  for(const auto& [exponent, prefixSymbol] : k_EngineeringPrefixes)
  {
    if(exponent == engineeringExponent)
    {
      symbol = prefixSymbol;
      break;
    }
  }
  return fmt::format("{:g} {}", mantissa, symbol);
}

//------------------------------------------------------------------------------
usize ComputeBandHeight(usize imageHeightPixels)
{
  return std::max<usize>(k_MinBandHeight, static_cast<usize>(std::llround(k_BandHeightFraction * static_cast<float64>(imageHeightPixels))));
}

//------------------------------------------------------------------------------
std::vector<uint8> RenderScaleBarBandRgb(usize imageWidthPixels, usize imageHeightPixels, float64 unitsPerPixel, IGeometry::LengthUnit unit)
{
  const usize bandHeight = ComputeBandHeight(imageHeightPixels);
  const usize margin = std::max<usize>(2, static_cast<usize>(std::llround(static_cast<float64>(bandHeight) * k_MarginFraction)));
  const usize barThickness = std::max<usize>(2, static_cast<usize>(std::llround(static_cast<float64>(bandHeight) * k_BarThicknessFraction)));
  const float32 fontSize = static_cast<float32>(k_FontFraction * static_cast<float64>(bandHeight));

  canvas_ity::canvas context(static_cast<int>(imageWidthPixels), static_cast<int>(bandHeight));
  context.set_color(canvas_ity::fill_style, 1.0f, 1.0f, 1.0f, 1.0f);
  context.fill_rectangle(0.0f, 0.0f, static_cast<float>(imageWidthPixels), static_cast<float>(bandHeight));

  const float64 niceLength = ComputeNiceBarLength(imageWidthPixels, unitsPerPixel);
  if(niceLength > 0.0)
  {
    // Integer bar coordinates keep the rectangle edges crisp (full pixel coverage, no anti-aliasing)
    const usize barPixels = std::min<usize>(imageWidthPixels, std::max<usize>(1, static_cast<usize>(std::llround(niceLength / unitsPerPixel))));
    const usize barStartCol = (imageWidthPixels - barPixels) / 2;
    const usize barTopRow = bandHeight - margin - barThickness;

    context.set_color(canvas_ity::fill_style, 0.0f, 0.0f, 0.0f, 1.0f);
    context.fill_rectangle(static_cast<float>(barStartCol), static_cast<float>(barTopRow), static_cast<float>(barPixels), static_cast<float>(barThickness));

    const std::string label = FormatLengthLabel(niceLength, unit);
    std::vector<unsigned char> fontData;
    fonts::Base64Decode(fonts::k_LatoRegularBase64, fontData);
    if(context.set_font(fontData.data(), static_cast<int>(fontData.size()), fontSize))
    {
      const float textWidth = context.measure_text(label.c_str());
      const float baselineY = static_cast<float>(margin) + fontSize;
      context.fill_text(label.c_str(), (static_cast<float>(imageWidthPixels) - textWidth) / 2.0f, baselineY);
    }
  }

  std::vector<uint8> rgba(imageWidthPixels * bandHeight * 4);
  context.get_image_data(rgba.data(), static_cast<int>(imageWidthPixels), static_cast<int>(bandHeight), static_cast<int>(imageWidthPixels * 4), 0, 0);

  std::vector<uint8> rgb(imageWidthPixels * bandHeight * 3);
  for(usize i = 0; i < imageWidthPixels * bandHeight; i++)
  {
    rgb[i * 3 + 0] = rgba[i * 4 + 0];
    rgb[i * 3 + 1] = rgba[i * 4 + 1];
    rgb[i * 3 + 2] = rgba[i * 4 + 2];
  }
  return rgb;
}
} // namespace nx::core::ScaleBarRenderer
