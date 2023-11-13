#include "WriteGBCDGMTFile.hpp"

#include "OrientationAnalysis/Math/Matrix3X1.hpp"
#include "OrientationAnalysis/Math/Matrix3X3.hpp"

#include "EbsdLib/Core/Orientation.hpp"
#include "EbsdLib/Core/OrientationTransformation.hpp"
#include "EbsdLib/LaueOps/LaueOps.h"

#include "complex/Common/Constants.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Utilities/FilterUtilities.hpp"
#include "complex/Utilities/Math/MatrixMath.hpp"

#include <cmath>
#include <filesystem>

namespace fs = std::filesystem;

using namespace complex;

namespace
{
// -----------------------------------------------------------------------------
template <typename T>
bool GetSquareCoord(std::array<T, 3> crystalNormal, std::array<T, 2>& sqCoord)
{
  bool nhCheck = false;
  T adjust = 1.0;
  if(crystalNormal[2] >= 0.0)
  {
    adjust = -1.0;
    nhCheck = true;
  }
  if(fabs(crystalNormal[0]) >= fabs(crystalNormal[1]))
  {
    sqCoord[0] = (crystalNormal[0] / fabs(crystalNormal[0])) * sqrt(2.0 * 1.0 * (1.0 + (crystalNormal[2] * adjust))) * (Constants::k_SqrtPiD / 2.0);
    sqCoord[1] = (crystalNormal[0] / fabs(crystalNormal[0])) * sqrt(2.0 * 1.0 * (1.0 + (crystalNormal[2] * adjust))) * ((2.0 / Constants::k_SqrtPiD) * atanf(crystalNormal[1] / crystalNormal[0]));
  }
  else
  {
    sqCoord[0] = (crystalNormal[1] / fabs(crystalNormal[1])) * sqrtf(2.0 * 1.0 * (1.0 + (crystalNormal[2] * adjust))) * ((2.0f / Constants::k_SqrtPiD) * atanf(crystalNormal[0] / crystalNormal[1]));
    sqCoord[1] = (crystalNormal[1] / fabs(crystalNormal[1])) * sqrtf(2.0 * 1.0 * (1.0 + (crystalNormal[2] * adjust))) * (Constants::k_SqrtPiD / 2.0);
  }
  return nhCheck;
}
} // namespace

// -----------------------------------------------------------------------------
WriteGBCDGMTFile::WriteGBCDGMTFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteGBCDGMTFileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
WriteGBCDGMTFile::~WriteGBCDGMTFile() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& WriteGBCDGMTFile::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> WriteGBCDGMTFile::operator()()
{
  auto gbcd = m_DataStructure.getDataRefAs<Float64Array>(m_InputValues->GBCDArrayPath);
  auto crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);

  // Make sure any directory path is also available as the user may have just typed
  // in a path without actually creating the full path
  Result<> createDirectoriesResult = complex::CreateOutputDirectories(m_InputValues->OutputFile.parent_path());
  if(createDirectoriesResult.invalid())
  {
    return createDirectoriesResult;
  }

  const std::ofstream outStrm(m_InputValues->OutputFile, std::ios_base::out);
  if(!outStrm.is_open())
  {
    return MakeErrorResult(-11021, fmt::format("Error creating output file {}", m_InputValues->OutputFile.string()));
  }

  std::vector<double> gbcdDeltas(5, 0);
  std::vector<double> gbcdLimits(10, 0);
  std::vector<int32> gbcdSizes(5, 0);

  // Original Ranges from Dave R.
  // gbcdLimits[0] = 0.0f;
  // gbcdLimits[1] = cosf(1.0f*m_pi);
  // gbcdLimits[2] = 0.0f;
  // gbcdLimits[3] = 0.0f;
  // gbcdLimits[4] = cosf(1.0f*m_pi);
  // gbcdLimits[5] = 2.0f*m_pi;
  // gbcdLimits[6] = cosf(0.0f);
  // gbcdLimits[7] = 2.0f*m_pi;
  // gbcdLimits[8] = 2.0f*m_pi;
  // gbcdLimits[9] = cosf(0.0f);

  // Greg R. Ranges
  gbcdLimits[0] = 0.0f;
  gbcdLimits[1] = 0.0f;
  gbcdLimits[2] = 0.0f;
  gbcdLimits[3] = -sqrtf(Constants::k_PiOver2D);
  gbcdLimits[4] = -sqrtf(Constants::k_PiOver2D);
  gbcdLimits[5] = Constants::k_PiD / 2.0f;
  gbcdLimits[6] = 1.0f;
  gbcdLimits[7] = Constants::k_PiD / 2.0f;
  gbcdLimits[8] = sqrtf(Constants::k_PiOver2D);
  gbcdLimits[9] = sqrtf(Constants::k_PiOver2D);

  // get num components of GBCD
  std::vector<usize> cDims = gbcd.getComponentShape();

  gbcdSizes[0] = static_cast<int>(cDims[0]);
  gbcdSizes[1] = static_cast<int>(cDims[1]);
  gbcdSizes[2] = static_cast<int>(cDims[2]);
  gbcdSizes[3] = static_cast<int>(cDims[3]);
  gbcdSizes[4] = static_cast<int>(cDims[4]);

  gbcdDeltas[0] = (gbcdLimits[5] - gbcdLimits[0]) / static_cast<double>(gbcdSizes[0]);
  gbcdDeltas[1] = (gbcdLimits[6] - gbcdLimits[1]) / static_cast<double>(gbcdSizes[1]);
  gbcdDeltas[2] = (gbcdLimits[7] - gbcdLimits[2]) / static_cast<double>(gbcdSizes[2]);
  gbcdDeltas[3] = (gbcdLimits[8] - gbcdLimits[3]) / static_cast<double>(gbcdSizes[3]);
  gbcdDeltas[4] = (gbcdLimits[9] - gbcdLimits[4]) / static_cast<double>(gbcdSizes[4]);

  using Matrix3X3Type = Matrix3X3<float64>;
  using Matrix3X1Type = Matrix3X1<float64>;
  Matrix3X3Type dg;

  {
    const float32 misAngle = m_InputValues->MisorientationRotation[0] * Constants::k_PiOver180F;
    std::array<float32, 3> normAxis = {m_InputValues->MisorientationRotation[1], m_InputValues->MisorientationRotation[2], m_InputValues->MisorientationRotation[3]};
    MatrixMath::Normalize3x1(normAxis.data());
    // convert axis angle to matrix representation of misorientation
    auto out = OrientationTransformation::ax2om<OrientationD, OrientationD>(OrientationF(normAxis[0], normAxis[1], normAxis[2], misAngle));
    dg = Matrix3X3Type(out.data());
  }
  // take inverse of misorientation variable to use for switching symmetry
  Matrix3X3Type dgt = dg.transpose();

  // Get our LaueOps pointer for the selected crystal structure
  const LaueOps::Pointer orientOps = LaueOps::GetAllOrientationOps()[crystalStructures[m_InputValues->PhaseOfInterest]];

  // get number of symmetry operators
  const int32 nSym = orientOps->getNumSymOps();

  const usize thetaPoints = 120;
  const usize phiPoints = 30;
  const float64 thetaRes = 360.0 / static_cast<float64>(thetaPoints);
  const float64 phiRes = 90.0f / static_cast<float64>(phiPoints);
  const float64 degToRad = Constants::k_PiOver180D;

  const int32 shift1 = gbcdSizes[0];
  const int32 shift2 = gbcdSizes[0] * gbcdSizes[1];
  const int32 shift3 = gbcdSizes[0] * gbcdSizes[1] * gbcdSizes[2];
  const int32 shift4 = gbcdSizes[0] * gbcdSizes[1] * gbcdSizes[2] * gbcdSizes[3];

  const auto totalGBCDBins = (gbcdSizes[0] * gbcdSizes[1] * gbcdSizes[2] * gbcdSizes[3] * gbcdSizes[4] * 2);

  std::vector<double> gmtValues;
  gmtValues.reserve((phiPoints + 1) * (thetaPoints + 1)); // Allocate what should be needed.

  for(int32 phiPtIndex = 0; phiPtIndex < phiPoints + 1; phiPtIndex++)
  {
    for(int32 thetaPtIndex = 0; thetaPtIndex < thetaPoints + 1; thetaPtIndex++)
    {
      // get (x,y) for stereographic projection pixel
      const float64 theta = static_cast<float64>(thetaPtIndex) * thetaRes;
      const float64 phi = static_cast<float64>(phiPtIndex) * phiRes;
      const float64 thetaRad = theta * degToRad;
      const float64 phiRad = phi * degToRad;
      float64 sum = 0.0;
      int32 count = 0;

      const Matrix3X1Type vec(std::sin(phiRad) * std::cos(thetaRad), std::sin(phiRad) * std::sin(thetaRad), std::cos(phiRad));
      const Matrix3X1Type vec2 = dgt * vec;

      // Loop over all the symmetry operators in the given crystal symmetry
      for(int32 i = 0; i < nSym; i++)
      {
        // get symmetry operator1
        double tempSymOperator[3][3];
        orientOps->getMatSymOp(i, tempSymOperator);
        const Matrix3X3Type sym1(tempSymOperator);

        for(int32 j = 0; j < nSym; j++)
        {
          // get symmetry operator2
          orientOps->getMatSymOp(j, tempSymOperator);
          const Matrix3X3Type sym2(tempSymOperator);
          //  calculate symmetric misorientation
          Matrix3X3Type dg2 = sym1 * (dg * sym2.transpose());
          // convert to euler angle
          auto misEuler1 = OrientationTransformation::om2eu<OrientationD, OrientationD>(OrientationD(dg2.data(), 9));
          if(misEuler1[0] < Constants::k_PiOver2D && misEuler1[1] < Constants::k_PiOver2D && misEuler1[2] < Constants::k_PiOver2D)
          {
            misEuler1[1] = std::cos(misEuler1[1]);
            // find bins in GBCD
            const auto location1 = static_cast<int32>((misEuler1[0] - gbcdLimits[0]) / gbcdDeltas[0]);
            const auto location2 = static_cast<int32>((misEuler1[1] - gbcdLimits[1]) / gbcdDeltas[1]);
            const auto location3 = static_cast<int32>((misEuler1[2] - gbcdLimits[2]) / gbcdDeltas[2]);
            // find symmetric poles using the first symmetry operator
            Matrix3X1Type rotNormal = sym1 * vec;
            // get coordinates in square projection of crystal normal parallel to boundary normal
            // This section of code is in here so that we can essentially remove the tiny error out in the
            // last few decimal places of the double values. This will essentially guarantee
            // the both x86_64 and ARM64 will end up returning the same square coordinates.
            const std::array<float32, 3> rotNormalF = {static_cast<float32>(rotNormal[0]), static_cast<float32>(rotNormal[1]), static_cast<float32>(rotNormal[2])};
            std::array<float32, 2> sqCoordF = {0.0F, 0.0F};

            const bool nhCheck = GetSquareCoord(rotNormalF, sqCoordF); // Result goes into the sqCoordF variable
            // Now copy the square coordinate back to the double version of the values
            // so that we can keep doing double precision calculations
            std::array<double, 3> sqCoord = {static_cast<float64>(sqCoordF[0]), static_cast<float64>(sqCoordF[1])};

            // Note the switch to have theta in the 4 slot and cos(Phi) in the 3 slot
            auto location4 = static_cast<int32>((sqCoord[0] - gbcdLimits[3]) / gbcdDeltas[3]);
            if(std::isnan(sqCoord[0])) // Arm64 and x86 handle casting NaN to integer differently. Make them the same outcome.
            {
              location4 = std::numeric_limits<int32>::min();
            }
            auto location5 = static_cast<int32>((sqCoord[1] - gbcdLimits[4]) / gbcdDeltas[4]);
            if(std::isnan(sqCoord[1]))
            {
              location5 = std::numeric_limits<int32>::min();
            }
            if(location1 >= 0 && location2 >= 0 && location3 >= 0 && location4 >= 0 && location5 >= 0 && location1 < gbcdSizes[0] && location2 < gbcdSizes[1] && location3 < gbcdSizes[2] &&
               location4 < gbcdSizes[3] && location5 < gbcdSizes[4])
            {
              int32 hemisphere = 0;
              if(!nhCheck)
              {
                hemisphere = 1;
              }
              sum += gbcd[(m_InputValues->PhaseOfInterest * totalGBCDBins) + 2 * ((location5 * shift4) + (location4 * shift3) + (location3 * shift2) + (location2 * shift1) + location1) + hemisphere];
              count++;
            }
          }

          // again in second crystal reference frame
          // calculate symmetric misorientation
          dg2 = sym1 * (dgt * sym2);
          // convert to euler angle
          misEuler1 = OrientationTransformation::om2eu<OrientationD, OrientationD>(OrientationD(dg2.data(), 9));
          if(misEuler1[0] < Constants::k_PiOver2D && misEuler1[1] < Constants::k_PiOver2D && misEuler1[2] < Constants::k_PiOver2D)
          {
            misEuler1[1] = std::cos(misEuler1[1]);
            // find bins in GBCD
            const auto location1 = static_cast<int32>((misEuler1[0] - gbcdLimits[0]) / gbcdDeltas[0]);
            const auto location2 = static_cast<int32>((misEuler1[1] - gbcdLimits[1]) / gbcdDeltas[1]);
            const auto location3 = static_cast<int32>((misEuler1[2] - gbcdLimits[2]) / gbcdDeltas[2]);
            // find symmetric poles using the first symmetry operator
            Matrix3X1Type rotNormal2 = sym1 * vec2;
            // get coordinates in square projection of crystal normal parallel to boundary normal
            const std::array<float32, 3> rotNormalF = {static_cast<float32>(rotNormal2[0]), static_cast<float32>(rotNormal2[1]), static_cast<float32>(rotNormal2[2])};
            std::array<float32, 2> sqCoordF = {0.0F, 0.0F};

            const bool nhCheck = GetSquareCoord(rotNormalF, sqCoordF);
            std::array<double, 3> sqCoord = {static_cast<float64>(sqCoordF[0]), static_cast<float64>(sqCoordF[1])};

            // Note the switch to have theta in the 4 slot and cos(Phi) in the 3 slot
            auto location4 = static_cast<int32>((sqCoord[0] - gbcdLimits[3]) / gbcdDeltas[3]);
            if(std::isnan(sqCoord[0]))
            {
              location4 = std::numeric_limits<int32>::min();
            }
            auto location5 = static_cast<int32>((sqCoord[1] - gbcdLimits[4]) / gbcdDeltas[4]);
            if(std::isnan(sqCoord[1]))
            {
              location5 = std::numeric_limits<int32>::min();
            }

            if(location1 >= 0 && location2 >= 0 && location3 >= 0 && location4 >= 0 && location5 >= 0 && location1 < gbcdSizes[0] && location2 < gbcdSizes[1] && location3 < gbcdSizes[2] &&
               location4 < gbcdSizes[3] && location5 < gbcdSizes[4])
            {
              int32 hemisphere = 0;
              if(!nhCheck)
              {
                hemisphere = 1;
              }
              sum += gbcd[(m_InputValues->PhaseOfInterest * totalGBCDBins) + 2 * ((location5 * shift4) + (location4 * shift3) + (location3 * shift2) + (location2 * shift1) + location1) + hemisphere];
              count++;
            }
          }
        }
      }
      gmtValues.push_back(theta);
      gmtValues.push_back((90.0 - phi));
      gmtValues.push_back(sum / float32(count));
    }
  }

  FILE* gmtFilePtr = fopen(m_InputValues->OutputFile.string().c_str(), "wb");
  if(nullptr == gmtFilePtr)
  {
    return MakeErrorResult(-11022, fmt::format("Error opening output file {}", m_InputValues->OutputFile.string()));
  }

  // Remember to use the original Angle in Degrees!!!!
  fprintf(gmtFilePtr, "%.1f %.1f %.1f %.1f\n", m_InputValues->MisorientationRotation[1], m_InputValues->MisorientationRotation[2], m_InputValues->MisorientationRotation[3],
          m_InputValues->MisorientationRotation[0]);
  const size_t size = gmtValues.size() / 3;

  for(size_t i = 0; i < size; i++)
  {
    fprintf(gmtFilePtr, "%f %f %f\n", gmtValues[3 * i], gmtValues[3 * i + 1], gmtValues[3 * i + 2]);
  }
  fclose(gmtFilePtr);

  return {};
}
