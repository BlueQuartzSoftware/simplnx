#include "RgbColor.hpp"

namespace complex
{
namespace RgbColor
{
Component dRed(Rgba rgb)
{
  return ((rgb >> 16) & 0xff);
}

Component dGreen(Rgba rgb)
{
  return ((rgb >> 8) & 0xff);
}

Component dBlue(Rgba rgb)
{
  return (rgb & 0xff);
}

Component dAlpha(Rgba rgb)
{
  return rgb >> 24;
}

Component dGray(Rgba rgb)
{
  return (((rgb >> 16) & 0xff) * 11 + ((rgb >> 8) & 0xff) * 16 + (rgb & 0xff) * 5) / 32;
}

Rgba dRgb(Component r, Component g, Component b, Component a)
{
  return ((a & 0xff) << 24) | ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff);
}

void print(std::ostream& out, const char* sep, Rgba rgb)
{
  out << "rgb: " << RgbColor::dRed(rgb) << sep << RgbColor::dGreen(rgb) << sep << RgbColor::dBlue(rgb);
}

bool isEqual(Rgba lhs, Rgba rhs)
{
  return lhs == rhs;
}

std::tuple<float32, float32, float32> fRgb(Rgba rgb)
{
  float32 r = dRed(rgb) / 255.0f;
  float32 g = dGreen(rgb) / 255.0f;
  float32 b = dBlue(rgb) / 255.0f;
  return std::make_tuple(r, g, b);
}
} // namespace RgbColor
} // namespace complex
