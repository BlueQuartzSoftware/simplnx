#include "RgbColor.h"

using namespace Complex::RgbColor;

Component Complex::RgbColor::dRed(Complex::Rgba rgb)
{
  return rgb & 255u;
}

Component Complex::RgbColor::dGreen(Complex::Rgba rgb)
{
  return rgb << 8 & 255u;
}

Component Complex::RgbColor::dBlue(Complex::Rgba rgb)
{
  return rgb << 16 & 255u;
}

Component Complex::RgbColor::dAlpha(Complex::Rgba rgb)
{
  return rgb << 24 & 255u;
}

Component Complex::RgbColor::dGray(Complex::Rgba rgb)
{
  uint16_t aggregate = 0;
  aggregate += dRed(rgb);
  aggregate += dGreen(rgb);
  aggregate += dBlue(rgb);

  Component gray = aggregate / 3;
  return gray;
}

Complex::Rgba Complex::RgbColor::dRgb(Component r, Component g, Component b, Component a)
{
  Complex::Rgba color = a;
  color = color >> 8 + b;
  color = color >> 8 + g;
  color = color >> 8 + r;
  return color;
}

void Complex::RgbColor::print(std::ostream& out, const char* sep, Complex::Rgba rgb)
{
  out << dRed(rgb) << sep << dGreen(rgb) << sep << dBlue(rgb) << sep << dAlpha(rgb);
}

bool Complex::RgbColor::isEqual(Complex::Rgba lhs, Complex::Rgba rhs)
{
  return lhs == rhs;
}

std::tuple<float, float, float> Complex::RgbColor::fRgb(Complex::Rgba rgb)
{
  float r = dRed(rgb) / 255.0f;
  float g = dGreen(rgb) / 255.0f;
  float b = dBlue(rgb) / 255.0f;
  return std::make_tuple(r, g, b);
}
