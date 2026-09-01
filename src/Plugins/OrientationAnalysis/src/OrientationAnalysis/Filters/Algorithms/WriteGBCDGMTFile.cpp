#include "WriteGBCDGMTFile.hpp"

#include <EbsdLib/Core/Orientation.hpp>
#include <EbsdLib/LaueOps/LaueOps.h>
#include <EbsdLib/Orientation/OrientationFwd.hpp>

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <cmath>
#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
/**
 * @brief Maps a crystal normal to Lambert square coordinates.
 * @tparam T Floating-point value type.
 * @param crystalNormal Three-component crystal normal.
 * @param sqCoord Receives the two square coordinates.
 * @return True for the nonnegative-Z hemisphere.
 *
 * A normal on the Z axis can produce NaN coordinates. The caller maps those
 * values to an out-of-range bin consistently across processors.
 */
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
    sqCoord[0] = (crystalNormal[0] / fabs(crystalNormal[0])) * sqrt(2.0 * 1.0 * (1.0 + (crystalNormal[2] * adjust))) * (nx::core::Constants::k_SqrtPiD / 2.0);
    sqCoord[1] =
        (crystalNormal[0] / fabs(crystalNormal[0])) * sqrt(2.0 * 1.0 * (1.0 + (crystalNormal[2] * adjust))) * ((2.0 / nx::core::Constants::k_SqrtPiD) * atanf(crystalNormal[1] / crystalNormal[0]));
  }
  else
  {
    sqCoord[0] =
        (crystalNormal[1] / fabs(crystalNormal[1])) * sqrtf(2.0 * 1.0 * (1.0 + (crystalNormal[2] * adjust))) * ((2.0f / nx::core::Constants::k_SqrtPiD) * atanf(crystalNormal[0] / crystalNormal[1]));
    sqCoord[1] = (crystalNormal[1] / fabs(crystalNormal[1])) * sqrtf(2.0 * 1.0 * (1.0 + (crystalNormal[2] * adjust))) * (nx::core::Constants::k_SqrtPiD / 2.0);
  }
  return nhCheck;
}
} // namespace

WriteGBCDGMTFile::WriteGBCDGMTFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteGBCDGMTFileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

WriteGBCDGMTFile::~WriteGBCDGMTFile() noexcept = default;

const std::atomic_bool& WriteGBCDGMTFile::getCancel()
{
  return m_ShouldCancel;
}

Result<> WriteGBCDGMTFile::operator()()
{
  auto& gbcd = m_DataStructure.getDataRefAs<Float64Array>(m_InputValues->GBCDArrayPath);
  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);

  // Create the destination directory before the file handle truncates the output.
  Result<> createDirectoriesResult = nx::core::CreateOutputDirectories(m_InputValues->OutputFile.parent_path());
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

  // Use the canonical GBCD domain with Lambert square boundary-normal limits.
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

  ShapeType cDims = gbcd.getComponentShape();

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

  using Matrix3X3Type = Eigen::Matrix<float64, 3, 3, Eigen::RowMajor>;
  using Matrix3X1Type = Eigen::Vector3d;

  Matrix3X3Type dg;

  {
    const float32 misAngle = m_InputValues->MisorientationRotation[0] * nx::core::Constants::k_PiOver180F;
    nx::core::FloatVec3 normAxis = {m_InputValues->MisorientationRotation[1], m_InputValues->MisorientationRotation[2], m_InputValues->MisorientationRotation[3]};
    normAxis = normAxis.normalize();
    // Convert the normalized degree-axis input to a misorientation matrix.
    auto out = ebsdlib::AxisAngleDType(normAxis[0], normAxis[1], normAxis[2], misAngle).toOrientationMatrix();
    dg = Matrix3X3Type(out.data());
  }
  // Transpose gives the reciprocal misorientation for the second crystal frame.
  Matrix3X3Type dgt = dg.transpose();

  // Cache ensemble metadata and only the selected GBCD phase slice.
  const usize numCrystalStructures = crystalStructures.getSize();
  auto crystalStructuresCache = std::make_unique<uint32[]>(numCrystalStructures);
  crystalStructures.getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint32>(crystalStructuresCache.get(), numCrystalStructures));

  const auto totalGBCDBins = (gbcdSizes[0] * gbcdSizes[1] * gbcdSizes[2] * gbcdSizes[3] * gbcdSizes[4] * 2);
  const usize phaseOffset = static_cast<usize>(m_InputValues->PhaseOfInterest) * static_cast<usize>(totalGBCDBins);
  auto gbcdPhaseCache = std::make_unique<float64[]>(static_cast<usize>(totalGBCDBins));
  gbcd.getDataStoreRef().copyIntoBuffer(phaseOffset, nonstd::span<float64>(gbcdPhaseCache.get(), static_cast<usize>(totalGBCDBins)));

  const ebsdlib::LaueOps::Pointer orientOps = ebsdlib::LaueOps::GetAllOrientationOps()[crystalStructuresCache[m_InputValues->PhaseOfInterest]];

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

  std::vector<double> gmtValues;
  gmtValues.reserve((phiPoints + 1) * (thetaPoints + 1)); // Allocate what should be needed.

  for(int32 phiPtIndex = 0; phiPtIndex < phiPoints + 1; phiPtIndex++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    for(int32 thetaPtIndex = 0; thetaPtIndex < thetaPoints + 1; thetaPtIndex++)
    {
      // Convert one polar-grid point to a boundary-normal direction.
      const float64 theta = static_cast<float64>(thetaPtIndex) * thetaRes;
      const float64 phi = static_cast<float64>(phiPtIndex) * phiRes;
      const float64 thetaRad = theta * degToRad;
      const float64 phiRad = phi * degToRad;
      float64 sum = 0.0;
      int32 count = 0;

      Matrix3X1Type vec = (Matrix3X1Type() << std::sin(phiRad) * std::cos(thetaRad), std::sin(phiRad) * std::sin(thetaRad), std::cos(phiRad)).finished();
      const Matrix3X1Type vec2 = dgt * vec;

      // Average all valid bins from pairs of crystal symmetry operators.
      for(int32 i = 0; i < nSym; i++)
      {
        ebsdlib::Matrix3X3D tSymOp = orientOps->getMatSymOpD(i);
        const Matrix3X3Type sym1 = (Matrix3X3Type() << tSymOp[0], tSymOp[1], tSymOp[2], tSymOp[3], tSymOp[4], tSymOp[5], tSymOp[6], tSymOp[7], tSymOp[8]).finished();

        for(int32 j = 0; j < nSym; j++)
        {
          tSymOp = orientOps->getMatSymOpD(j);
          const Matrix3X3Type sym2 = (Matrix3X3Type() << tSymOp[0], tSymOp[1], tSymOp[2], tSymOp[3], tSymOp[4], tSymOp[5], tSymOp[6], tSymOp[7], tSymOp[8]).finished();

          // Apply symmetry in the first crystal reference frame.
          Matrix3X3Type dg2 = sym1 * (dg * sym2.transpose());
          auto misEuler1 = ebsdlib::OrientationMatrixDType(dg2.data()).toEuler();
          if(misEuler1[0] < Constants::k_PiOver2D && misEuler1[1] < Constants::k_PiOver2D && misEuler1[2] < Constants::k_PiOver2D)
          {
            misEuler1[1] = std::cos(misEuler1[1]);
            const auto location1 = static_cast<int32>((misEuler1[0] - gbcdLimits[0]) / gbcdDeltas[0]);
            const auto location2 = static_cast<int32>((misEuler1[1] - gbcdLimits[1]) / gbcdDeltas[1]);
            const auto location3 = static_cast<int32>((misEuler1[2] - gbcdLimits[2]) / gbcdDeltas[2]);
            Matrix3X1Type rotNormal = sym1 * vec;
            // Use float square coordinates so x86_64 and arm64 bin boundary
            // decisions match despite small double-precision differences.
            const std::array<float32, 3> rotNormalF = {static_cast<float32>(rotNormal[0]), static_cast<float32>(rotNormal[1]), static_cast<float32>(rotNormal[2])};
            std::array<float32, 2> sqCoordF = {0.0F, 0.0F};

            const bool nhCheck = GetSquareCoord(rotNormalF, sqCoordF);
            // Continue bin arithmetic in double precision after the stable float mapping.
            std::array<double, 3> sqCoord = {static_cast<float64>(sqCoordF[0]), static_cast<float64>(sqCoordF[1])};

            // GBCD dimensions four and five store Lambert square coordinates.
            auto location4 = static_cast<int32>((sqCoord[0] - gbcdLimits[3]) / gbcdDeltas[3]);
            if(std::isnan(sqCoord[0]))
            {
              // Map NaN to one explicit out-of-range bin on all processors.
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
              sum += gbcdPhaseCache[2 * ((location5 * shift4) + (location4 * shift3) + (location3 * shift2) + (location2 * shift1) + location1) + hemisphere];
              count++;
            }
          }

          // Apply reciprocal misorientation in the second crystal reference frame.
          dg2 = sym1 * (dgt * sym2);
          misEuler1 = ebsdlib::OrientationMatrixDType(dg2.data()).toEuler();
          if(misEuler1[0] < Constants::k_PiOver2D && misEuler1[1] < Constants::k_PiOver2D && misEuler1[2] < Constants::k_PiOver2D)
          {
            misEuler1[1] = std::cos(misEuler1[1]);
            const auto location1 = static_cast<int32>((misEuler1[0] - gbcdLimits[0]) / gbcdDeltas[0]);
            const auto location2 = static_cast<int32>((misEuler1[1] - gbcdLimits[1]) / gbcdDeltas[1]);
            const auto location3 = static_cast<int32>((misEuler1[2] - gbcdLimits[2]) / gbcdDeltas[2]);
            Matrix3X1Type rotNormal2 = sym1 * vec2;
            const std::array<float32, 3> rotNormalF = {static_cast<float32>(rotNormal2[0]), static_cast<float32>(rotNormal2[1]), static_cast<float32>(rotNormal2[2])};
            std::array<float32, 2> sqCoordF = {0.0F, 0.0F};

            const bool nhCheck = GetSquareCoord(rotNormalF, sqCoordF);
            std::array<double, 3> sqCoord = {static_cast<float64>(sqCoordF[0]), static_cast<float64>(sqCoordF[1])};

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
              sum += gbcdPhaseCache[2 * ((location5 * shift4) + (location4 * shift3) + (location3 * shift2) + (location2 * shift1) + location1) + hemisphere];
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

  // The creation-check stream remains open while this C stream rewrites the same path.
  FILE* gmtFilePtr = fopen(m_InputValues->OutputFile.string().c_str(), "wb");
  if(nullptr == gmtFilePtr)
  {
    return MakeErrorResult(-11022, fmt::format("Error opening output file {}", m_InputValues->OutputFile.string()));
  }

  // The header stores axis components followed by the original angle in degrees.
  fprintf(gmtFilePtr, "%.1f %.1f %.1f %.1f\n", m_InputValues->MisorientationRotation[1], m_InputValues->MisorientationRotation[2], m_InputValues->MisorientationRotation[3],
          m_InputValues->MisorientationRotation[0]);
  const usize size = gmtValues.size() / 3;

  for(usize i = 0; i < size; i++)
  {
    fprintf(gmtFilePtr, "%f %f %f\n", gmtValues[3 * i], gmtValues[3 * i + 1], gmtValues[3 * i + 2]);
  }
  fclose(gmtFilePtr);

  return {};
}
