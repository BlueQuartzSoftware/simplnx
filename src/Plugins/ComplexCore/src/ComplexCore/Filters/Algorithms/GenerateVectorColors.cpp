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
  std::unique_ptr<MaskCompare> maskCompare;
  try
  {
    maskCompare = InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
  } catch(const std::out_of_range& exception)
  {
    // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
    // somewhere else that is NOT going through the normal complex::IFilter API of Preflight and Execute
    return MakeErrorResult(-54700, fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString()));
  }

  auto& vectors = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->VectorsArrayPath);
  auto& cellVectorColors = m_DataStructure.getDataRefAs<UInt8Array>(m_InputValues->CellVectorColorsArrayPath);

  usize totalPoints = vectors.getNumberOfTuples();

  usize index;
  // Write the Vector Coloring Cell Data
  for(usize i = 0; i < totalPoints; i++)
  {
    index = i * 3;
    cellVectorColors[index] = 0;
    cellVectorColors[index + 1] = 0;
    cellVectorColors[index + 2] = 0;

    if(maskCompare->isTrue(i))
    {
      float32 dir[3] = {0.0f, 0.0f, 0.0f};
      dir[0] = vectors[index + 0];
      dir[1] = vectors[index + 1];
      dir[2] = vectors[index + 2];
      VectorMapType array(dir);
      array.normalize();

      if(dir[2] < 0)
      {
        // *= is not a valid operator in this case
        array = array * -1.0f;
      }

      float32 trend = std::atan2(array[1], array[0]) * (Constants::k_RadToDegF);
      float32 plunge = std::acos(array[2]) * (Constants::k_RadToDegF);
      if(trend < 0.0f)
      {
        trend += 360.0f;
      }

      float32 r = 0, g = 0, b = 0;
      if(trend <= 120.0f)
      {
        r = 255.0f * ((120.0f - trend) / 120.0f);
        g = 255.0f * (trend / 120.0f);
        b = 0.0f;
      }
      if(trend > 120.0f && trend <= 240.0f)
      {
        trend -= 120.0f;
        r = 0.0f;
        g = 255.0f * ((120.0f - trend) / 120.0f);
        b = 255.0f * (trend / 120.0f);
      }
      if(trend > 240.0f && trend < 360.0f)
      {
        trend -= 240.0f;
        r = 255.0f * (trend / 120.0f);
        g = 0.0f;
        b = 255.0f * ((120.0f - trend) / 120.0f);
      }
      float32 deltaR = 255.0f - r;
      float32 deltaG = 255.0f - g;
      float32 deltaB = 255.0f - b;
      r += (deltaR * ((90.0f - plunge) / 90.0f));
      g += (deltaG * ((90.0f - plunge) / 90.0f));
      b += (deltaB * ((90.0f - plunge) / 90.0f));
      if(r > 255.0f)
      {
        r = 255.0f;
      }
      if(g > 255.0f)
      {
        g = 255.0f;
      }
      if(b > 255.0f)
      {
        b = 255.0f;
      }

      Rgb argb = RgbColor::dRgb(static_cast<uint8>(r), static_cast<uint8>(g), static_cast<uint8>(b), 255);
      cellVectorColors[index] = RgbColor::dRed(argb);
      cellVectorColors[index + 1] = RgbColor::dGreen(argb);
      cellVectorColors[index + 2] = RgbColor::dBlue(argb);
    }
  }

  return {};
}
