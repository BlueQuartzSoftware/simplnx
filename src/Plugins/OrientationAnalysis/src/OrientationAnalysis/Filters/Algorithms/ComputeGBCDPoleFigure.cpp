#include "ComputeGBCDPoleFigure.hpp"

#include "simplnx/Common/Bit.hpp"
#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/Math/MatrixMath.hpp"
#include "simplnx/Utilities/ParallelData2DAlgorithm.hpp"

#include "EbsdLib/Core/Orientation.hpp"
#include "EbsdLib/Core/OrientationTransformation.hpp"
#include "EbsdLib/LaueOps/LaueOps.h"

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
class ComputeGBCDPoleFigureImpl
{
private:
  Float64Array& m_PoleFigure;
  std::array<int32, 2> m_Dimensions;
  LaueOps::Pointer m_OrientOps;
  const std::vector<float32>& m_GbcdDeltas;
  const std::vector<float32>& m_GbcdLimits;
  const std::vector<int32>& m_GbcdSizes;
  const Float64Array& m_Gbcd;
  int32 m_PhaseOfInterest = 0;
  const std::vector<float32>& m_MisorientationRotation;

public:
  ComputeGBCDPoleFigureImpl(Float64Array& poleFigureArray, const std::array<int32, 2>& dimensions, const LaueOps::Pointer& orientOps, const std::vector<float32>& gbcdDeltasArray,
                            const std::vector<float32>& gbcdLimitsArray, const std::vector<int32>& gbcdSizesArray, const Float64Array& gbcd, int32 phaseOfInterest,
                            const std::vector<float32>& misorientationRotation)
  : m_PoleFigure(poleFigureArray)
  , m_Dimensions(dimensions)
  , m_OrientOps(orientOps)
  , m_GbcdDeltas(gbcdDeltasArray)
  , m_GbcdLimits(gbcdLimitsArray)
  , m_GbcdSizes(gbcdSizesArray)
  , m_Gbcd(gbcd)
  , m_PhaseOfInterest(phaseOfInterest)
  , m_MisorientationRotation(misorientationRotation)
  {
  }
  ~ComputeGBCDPoleFigureImpl() = default;

  void generate(usize xStart, usize xEnd, usize yStart, usize yEnd) const
  {
    float32 vec[3] = {0.0f, 0.0f, 0.0f};
    float32 vec2[3] = {0.0f, 0.0f, 0.0f};
    float32 rotNormal[3] = {0.0f, 0.0f, 0.0f};
    float32 rotNormal2[3] = {0.0f, 0.0f, 0.0f};
    float32 sqCoord[2] = {0.0f, 0.0f};
    float32 dg[3][3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    float32 dgt[3][3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    float32 dg1[3][3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    float32 dg2[3][3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    float32 sym1[3][3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    float32 sym2[3][3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    float32 sym2t[3][3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    float32 misEuler1[3] = {0.0f, 0.0f, 0.0f};

    float32 misAngle = m_MisorientationRotation[0] * Constants::k_PiOver180F;
    float32 normAxis[3] = {m_MisorientationRotation[1], m_MisorientationRotation[2], m_MisorientationRotation[3]};
    MatrixMath::Normalize3x1(normAxis);
    // convert axis angle to matrix representation of misorientation
    OrientationTransformation::ax2om<OrientationF, OrientationF>(OrientationF(normAxis[0], normAxis[1], normAxis[2], misAngle)).toGMatrix(dg);

    // take inverse of misorientation variable to use for switching symmetry
    MatrixMath::Transpose3x3(dg, dgt);

    // get number of symmetry operators
    int32 nSym = m_OrientOps->getNumSymOps();

    int32 xPoints = m_Dimensions[0];
    int32 yPoints = m_Dimensions[1];
    int32 xPointsHalf = xPoints / 2;
    int32 yPointsHalf = yPoints / 2;
    float32 xRes = 2.0f / float32(xPoints);
    float32 yRes = 2.0f / float32(yPoints);
    bool nhCheck = false;
    int32 hemisphere = 0;

    int32 shift1 = m_GbcdSizes[0];
    int32 shift2 = m_GbcdSizes[0] * m_GbcdSizes[1];
    int32 shift3 = m_GbcdSizes[0] * m_GbcdSizes[1] * m_GbcdSizes[2];
    int32 shift4 = m_GbcdSizes[0] * m_GbcdSizes[1] * m_GbcdSizes[2] * m_GbcdSizes[3];

    int64 totalGbcdBins = m_GbcdSizes[0] * m_GbcdSizes[1] * m_GbcdSizes[2] * m_GbcdSizes[3] * m_GbcdSizes[4] * 2;

    std::vector<usize> dims = {1ULL};

    for(int32 k = yStart; k < yEnd; k++)
    {
      for(int32 l = xStart; l < xEnd; l++)
      {
        // get (x,y) for stereographic projection pixel
        float32 x = static_cast<float32>(l - xPointsHalf) * xRes + (xRes / 2.0F);
        float32 y = static_cast<float32>(k - yPointsHalf) * yRes + (yRes / 2.0F);

        if((x * x + y * y) <= 1.0)
        {
          double sum = 0.0;
          int32 count = 0;
          vec[2] = -((x * x + y * y) - 1) / ((x * x + y * y) + 1);
          vec[0] = x * (1 + vec[2]);
          vec[1] = y * (1 + vec[2]);
          MatrixMath::Multiply3x3with3x1(dgt, vec, vec2);

          // Loop over all the symmetry operators in the given crystal symmetry
          for(int32 i = 0; i < nSym; i++)
          {
            // get symmetry operator1
            m_OrientOps->getMatSymOp(i, sym1);
            for(int32 j = 0; j < nSym; j++)
            {
              // get symmetry operator2
              m_OrientOps->getMatSymOp(j, sym2);
              MatrixMath::Transpose3x3(sym2, sym2t);
              // calculate symmetric misorientation
              MatrixMath::Multiply3x3with3x3(dg, sym2t, dg1);
              MatrixMath::Multiply3x3with3x3(sym1, dg1, dg2);
              OrientationF om(dg2);
              // convert to euler angle
              OrientationF eu(misEuler1, 3);
              eu = OrientationTransformation::om2eu<OrientationF, OrientationF>(om);
              if(misEuler1[0] < Constants::k_PiOver2F && misEuler1[1] < Constants::k_PiOver2F && misEuler1[2] < Constants::k_PiOver2F)
              {
                misEuler1[1] = cosf(misEuler1[1]);
                // find bins in GBCD
                auto location1 = static_cast<int32>((misEuler1[0] - m_GbcdLimits[0]) / m_GbcdDeltas[0]);
                auto location2 = static_cast<int32>((misEuler1[1] - m_GbcdLimits[1]) / m_GbcdDeltas[1]);
                auto location3 = static_cast<int32>((misEuler1[2] - m_GbcdLimits[2]) / m_GbcdDeltas[2]);
                // find symmetric poles using the first symmetry operator
                MatrixMath::Multiply3x3with3x1(sym1, vec, rotNormal);
                // get coordinates in square projection of crystal normal parallel to boundary normal
                nhCheck = getSquareCoord(rotNormal, sqCoord);
                // Note the switch to have theta in the 4 slot and cos(Phi) int he 3 slot
                auto location4 = static_cast<int32>((sqCoord[0] - m_GbcdLimits[3]) / m_GbcdDeltas[3]);
                auto location5 = static_cast<int32>((sqCoord[1] - m_GbcdLimits[4]) / m_GbcdDeltas[4]);
                if(location1 >= 0 && location2 >= 0 && location3 >= 0 && location4 >= 0 && location5 >= 0 && location1 < m_GbcdSizes[0] && location2 < m_GbcdSizes[1] && location3 < m_GbcdSizes[2] &&
                   location4 < m_GbcdSizes[3] && location5 < m_GbcdSizes[4])
                {
                  hemisphere = 0;
                  if(!nhCheck)
                  {
                    hemisphere = 1;
                  }
                  sum += m_Gbcd[(m_PhaseOfInterest * totalGbcdBins) + 2 * ((location5 * shift4) + (location4 * shift3) + (location3 * shift2) + (location2 * shift1) + location1) + hemisphere];
                  count++;
                }
              }

              // again in second crystal reference frame
              // calculate symmetric misorientation
              MatrixMath::Multiply3x3with3x3(dgt, sym2, dg1);
              MatrixMath::Multiply3x3with3x3(sym1, dg1, dg2);
              om = OrientationF(dg2);
              // convert to euler angle
              eu = OrientationTransformation::om2eu<OrientationF, OrientationF>(om);
              if(misEuler1[0] < Constants::k_PiOver2D && misEuler1[1] < Constants::k_PiOver2F && misEuler1[2] < Constants::k_PiOver2F)
              {
                misEuler1[1] = cosf(misEuler1[1]);
                // find bins in GBCD
                auto location1 = static_cast<int32>((misEuler1[0] - m_GbcdLimits[0]) / m_GbcdDeltas[0]);
                auto location2 = static_cast<int32>((misEuler1[1] - m_GbcdLimits[1]) / m_GbcdDeltas[1]);
                auto location3 = static_cast<int32>((misEuler1[2] - m_GbcdLimits[2]) / m_GbcdDeltas[2]);
                // find symmetric poles using the first symmetry operator
                MatrixMath::Multiply3x3with3x1(sym1, vec2, rotNormal2);
                // get coordinates in square projection of crystal normal parallel to boundary normal
                nhCheck = getSquareCoord(rotNormal2, sqCoord);
                // Note the switch to have theta in the 4 slot and cos(Phi) int he 3 slot
                auto location4 = static_cast<int32>((sqCoord[0] - m_GbcdLimits[3]) / m_GbcdDeltas[3]);
                auto location5 = static_cast<int32>((sqCoord[1] - m_GbcdLimits[4]) / m_GbcdDeltas[4]);
                if(location1 >= 0 && location2 >= 0 && location3 >= 0 && location4 >= 0 && location5 >= 0 && location1 < m_GbcdSizes[0] && location2 < m_GbcdSizes[1] && location3 < m_GbcdSizes[2] &&
                   location4 < m_GbcdSizes[3] && location5 < m_GbcdSizes[4])
                {
                  hemisphere = 0;
                  if(!nhCheck)
                  {
                    hemisphere = 1;
                  }
                  sum += m_Gbcd[(m_PhaseOfInterest * totalGbcdBins) + 2 * ((location5 * shift4) + (location4 * shift3) + (location3 * shift2) + (location2 * shift1) + location1) + hemisphere];
                  count++;
                }
              }
            }
          }
          if(count > 0)
          {
            m_PoleFigure[(k * xPoints) + l] = sum / float32(count);
          }
        }
      }
    }
  }

  void operator()(const Range2D& r) const
  {
    generate(r.minCol(), r.maxCol(), r.minRow(), r.maxRow());
  }

private:
  /**
   * @brief getSquareCoord Computes the square based coordinate based on the incoming normal
   * @param crystalNormal Incoming normal
   * @param sqCoord Computed square coordinate
   * @return Boolean value for whether coordinate lies in the norther hemisphere
   */
  static bool getSquareCoord(float32* crystalNormal, float32* sqCoord)
  {
    bool nhCheck = false;
    float32 adjust = 1.0;
    if(crystalNormal[2] >= 0.0)
    {
      adjust = -1.0;
      nhCheck = true;
    }
    if(fabsf(crystalNormal[0]) >= fabsf(crystalNormal[1]))
    {
      sqCoord[0] = (crystalNormal[0] / fabsf(crystalNormal[0])) * sqrtf(2.0f * 1.0f * (1.0f + (crystalNormal[2] * adjust))) * (Constants::k_SqrtPiF / 2.0f);
      sqCoord[1] =
          (crystalNormal[0] / fabsf(crystalNormal[0])) * sqrtf(2.0f * 1.0f * (1.0f + (crystalNormal[2] * adjust))) * ((2.0f / Constants::k_SqrtPiF) * atanf(crystalNormal[1] / crystalNormal[0]));
    }
    else
    {
      sqCoord[0] =
          (crystalNormal[1] / fabsf(crystalNormal[1])) * sqrtf(2.0f * 1.0f * (1.0f + (crystalNormal[2] * adjust))) * ((2.0f / Constants::k_SqrtPiF) * atanf(crystalNormal[0] / crystalNormal[1]));
      sqCoord[1] = (crystalNormal[1] / fabsf(crystalNormal[1])) * sqrtf(2.0f * 1.0f * (1.0f + (crystalNormal[2] * adjust))) * (Constants::k_SqrtPiF / 2.0f);
    }
    return nhCheck;
  }
};

} // namespace

// -----------------------------------------------------------------------------
ComputeGBCDPoleFigure::ComputeGBCDPoleFigure(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             ComputeGBCDPoleFigureInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeGBCDPoleFigure::~ComputeGBCDPoleFigure() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeGBCDPoleFigure::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeGBCDPoleFigure::operator()()
{
  auto& gbcd = m_DataStructure.getDataRefAs<Float64Array>(m_InputValues->GBCDArrayPath);
  auto crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  DataPath cellIntensityArrayPath = m_InputValues->ImageGeometryPath.createChildPath(m_InputValues->CellAttributeMatrixName).createChildPath(m_InputValues->CellIntensityArrayName);
  auto poleFigure = m_DataStructure.getDataRefAs<Float64Array>(cellIntensityArrayPath);

  std::vector<float32> gbcdDeltas(5, 0);
  std::vector<float32> gbcdLimits(10, 0);
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
  gbcdLimits[3] = 0.0f;
  gbcdLimits[4] = 0.0f;
  gbcdLimits[5] = Constants::k_PiOver2D;
  gbcdLimits[6] = 1.0f;
  gbcdLimits[7] = Constants::k_PiOver2D;
  gbcdLimits[8] = 1.0f;
  gbcdLimits[9] = Constants::k_2PiD;

  // reset the 3rd and 4th dimensions using the square grid approach
  gbcdLimits[3] = -sqrtf(Constants::k_PiOver2D);
  gbcdLimits[4] = -sqrtf(Constants::k_PiOver2D);
  gbcdLimits[8] = sqrtf(Constants::k_PiOver2D);
  gbcdLimits[9] = sqrtf(Constants::k_PiOver2D);

  // get num components of GBCD
  std::vector<usize> cDims = gbcd.getComponentShape();

  gbcdSizes[0] = static_cast<int32>(cDims[0]);
  gbcdSizes[1] = static_cast<int32>(cDims[1]);
  gbcdSizes[2] = static_cast<int32>(cDims[2]);
  gbcdSizes[3] = static_cast<int32>(cDims[3]);
  gbcdSizes[4] = static_cast<int32>(cDims[4]);

  gbcdDeltas[0] = (gbcdLimits[5] - gbcdLimits[0]) / static_cast<float32>(gbcdSizes[0]);
  gbcdDeltas[1] = (gbcdLimits[6] - gbcdLimits[1]) / static_cast<float32>(gbcdSizes[1]);
  gbcdDeltas[2] = (gbcdLimits[7] - gbcdLimits[2]) / static_cast<float32>(gbcdSizes[2]);
  gbcdDeltas[3] = (gbcdLimits[8] - gbcdLimits[3]) / static_cast<float32>(gbcdSizes[3]);
  gbcdDeltas[4] = (gbcdLimits[9] - gbcdLimits[4]) / static_cast<float32>(gbcdSizes[4]);

  // Get our LaueOps pointer for the selected crystal structure
  LaueOps::Pointer orientOps = LaueOps::GetAllOrientationOps()[crystalStructures[m_InputValues->PhaseOfInterest]];

  int32 xPoints = m_InputValues->OutputImageDimension;
  int32 yPoints = m_InputValues->OutputImageDimension;
  int32 zPoints = 1;
  float32 xRes = 2.0f / static_cast<float32>(xPoints);
  float32 yRes = 2.0f / static_cast<float32>(yPoints);
  float32 zRes = (xRes + yRes) / 2.0F;

  m_MessageHandler({IFilter::Message::Type::Info, fmt::format("Generating Intensity Plot for phase {}", m_InputValues->PhaseOfInterest)});

  typename IParallelAlgorithm::AlgorithmArrays algArrays;
  algArrays.push_back(&poleFigure);
  algArrays.push_back(&gbcd);

  ParallelData2DAlgorithm dataAlg;
  dataAlg.setRange(0, xPoints, 0, yPoints);
  dataAlg.requireArraysInMemory(algArrays);

  dataAlg.execute(ComputeGBCDPoleFigureImpl(poleFigure, {xPoints, yPoints}, orientOps, gbcdDeltas, gbcdLimits, gbcdSizes, gbcd, m_InputValues->PhaseOfInterest, m_InputValues->MisorientationRotation));

  return {};
}
