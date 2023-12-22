#include "RgbColor.hpp"

namespace nx::core
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

// -----------------------------------------------------------------------------
void GetColorTable(int numColors, std::vector<float>& colorsOut)
{
  static const int numColorNodes = 8;
  float color[numColorNodes][3] = {
      {0.0f, 0.0f / 255.0f, 255.0f / 255.0f},            // blue
      {105.0f / 255.0f, 145.0f / 255.0f, 2.0f / 255.0f}, // yellow
      {0.0f / 255.0f, 255.0f / 255.0f, 29.0f / 255.0f},  // Green
      {180.0f / 255.0f, 255.0f / 255.0f, 0.0f / 255.0f},
      {255.0f / 255.0f, 215.0f / 255.0f, 6.0f / 255.0f},
      {255.0f / 255.0f, 143.0f / 255.0f, 1.0f / 255.0f},
      {255.0f / 255.0f, 69.0f / 255.0f, 0.0f / 255.0f},
      {255.0f / 255.0f, 0.0f / 255.0f, 0.0f / 255.0f} // red
  };

  static const int maxNodeIndex = numColorNodes - 1;
  const float stepSize = 1.0f / numColors;
  const float nodeStepSize = 1.0f / (maxNodeIndex);
  for(int i = 0; i < numColors; i++)
  {
    const float pos = static_cast<float>(i) * stepSize; // [0, 1] range
    int currColorBin = static_cast<int>(pos / nodeStepSize);
    const float currFraction = (pos / nodeStepSize) - static_cast<float>(currColorBin);

    float r;
    float g;
    float b;
    currColorBin = std::min(currColorBin, maxNodeIndex);
    // currColorBin + 1 causes this to step out of color[] bounds when currColorBin == (numColorNodes - 1)
    if(i < numColors - 1)
    {
      r = color[currColorBin][0] * (1.0f - currFraction) + color[currColorBin + 1][0] * currFraction;
      g = color[currColorBin][1] * (1.0f - currFraction) + color[currColorBin + 1][1] * currFraction;
      b = color[currColorBin][2] * (1.0f - currFraction) + color[currColorBin + 1][2] * currFraction;
    }
    else
    {
      r = color[currColorBin][0];
      g = color[currColorBin][1];
      b = color[currColorBin][2];
    }
    colorsOut[3 * i] = r;
    colorsOut[3 * i + 1] = g;
    colorsOut[3 * i + 2] = b;
  }
}

} // namespace RgbColor
} // namespace nx::core
