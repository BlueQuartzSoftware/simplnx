#include "GenerateVectorColors.hpp"

#include "complex/Common/Constants.hpp"
#include "complex/Common/RgbColor.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

#include <Eigen/Dense>

using namespace complex;

namespace
{
// typedef Eigen::Array<float, 3, 1> ArrayType;
typedef Eigen::Map<Eigen::Vector3f> VectorMapType;
} // namespace

// -----------------------------------------------------------------------------
GenerateVectorColors::GenerateVectorColors(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, GenerateVectorColorsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
GenerateVectorColors::~GenerateVectorColors() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& GenerateVectorColors::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> GenerateVectorColors::operator()()
{
  std::unique_ptr<MaskCompare> maskCompare = nullptr;
  try
  {
    maskCompare = InstantiateMaskCompare(m_DataStructure, m_InputValues->GoodVoxelsArrayPath);
  } catch(const std::out_of_range& exception)
  {
    // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
    // somewhere else that is NOT going through the normal complex::IFilter API of Preflight and Execute
    return MakeErrorResult(-54700, fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->GoodVoxelsArrayPath.toString()));
  }

  auto& vectors = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->VectorsArrayPath);
  auto& cellVectorColors = m_DataStructure.getDataRefAs<UInt8Array>(m_InputValues->CellVectorColorsArrayPath);

  usize totalPoints = vectors.getNumberOfTuples();

  usize index = 0;

  // Write the Vector Coloring Cell Data
  for(usize i = 0; i < totalPoints; i++)
  {
    index = i * 3;
    cellVectorColors[index] = 0;
    cellVectorColors[index + 1] = 0;
    cellVectorColors[index + 2] = 0;

    float dir[3] = {0.0f, 0.0f, 0.0f};
    float r = 0, g = 0, b = 0;
    Rgb argb;
    if(maskCompare->isTrue(i))
    {
      dir[0] = vectors[index + 0];
      dir[1] = vectors[index + 1];
      dir[2] = vectors[index + 2];

      VectorMapType array(const_cast<float*>(dir));
      array.normalize();

      if(dir[2] < 0)
      {
        array = array * -1.0f;
      }
      float trend = atan2f(dir[1], dir[0]) * (Constants::k_RadToDegF);
      float plunge = acosf(dir[2]) * (Constants::k_RadToDegF);
      if(trend < 0.0)
      {
        trend += 360.0;
      }
      if(trend <= 120.0)
      {
        r = 255.0 * ((120.0 - trend) / 120.0);
        g = 255.0 * (trend / 120.0);
        b = 0.0;
      }
      if(trend > 120.0 && trend <= 240.0)
      {
        trend -= 120.0;
        r = 0.0;
        g = 255.0 * ((120.0 - trend) / 120.0);
        b = 255.0 * (trend / 120.0);
      }
      if(trend > 240.0 && trend < 360.0)
      {
        trend -= 240.0;
        r = 255.0 * (trend / 120.0);
        g = 0.0;
        b = 255.0 * ((120.0 - trend) / 120.0);
      }
      float deltaR = 255.0 - r;
      float deltaG = 255.0 - g;
      float deltaB = 255.0 - b;
      r += (deltaR * ((90.0 - plunge) / 90.0));
      g += (deltaG * ((90.0 - plunge) / 90.0));
      b += (deltaB * ((90.0 - plunge) / 90.0));
      if(r > 255.0)
      {
        r = 255.0;
      }
      if(g > 255.0)
      {
        g = 255.0;
      }
      if(b > 255.0)
      {
        b = 255.0;
      }
      argb = RgbColor::dRgb(r, g, b, 255);
      cellVectorColors[index] = RgbColor::dRed(argb);
      cellVectorColors[index + 1] = RgbColor::dGreen(argb);
      cellVectorColors[index + 2] = RgbColor::dBlue(argb);
    }
  }

  return {};
}
