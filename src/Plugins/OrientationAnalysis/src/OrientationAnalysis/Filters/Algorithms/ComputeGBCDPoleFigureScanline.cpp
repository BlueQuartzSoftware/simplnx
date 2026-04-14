#include "ComputeGBCDPoleFigureScanline.hpp"

#include "ComputeGBCDPoleFigureDirect.hpp" // for ComputeGBCDPoleFigureInputValues

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ParallelData2DAlgorithm.hpp"

#include <EbsdLib/Core/Orientation.hpp>
#include <EbsdLib/LaueOps/LaueOps.h>
#include <EbsdLib/Orientation/OrientationFwd.hpp>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
/**
 * @class ComputeGBCDPoleFigureImpl
 * @brief Threaded worker for generating a GBCD stereographic pole figure (Scanline variant).
 *
 * This is the same pixel-computation logic as the Direct variant's worker. The only
 * difference is in how the GBCD data is provided:
 * - In the Direct variant, m_Gbcd points to the full multi-phase GBCD array, and
 *   m_PhaseOfInterest is used to offset into the correct phase slice.
 * - In the Scanline variant, m_Gbcd points to a pre-extracted single-phase slice
 *   (already offset), and m_PhaseOfInterest is set to 0 so no additional offset
 *   is applied.
 *
 * This shared-worker design means the parallel pixel computation is identical regardless
 * of whether the GBCD was loaded in full (Direct) or as a single-phase slice (Scanline).
 *
 * @see ComputeGBCDPoleFigureScanline::operator()() for the OOC data-loading strategy.
 */
class ComputeGBCDPoleFigureImpl
{
private:
  float64* m_PoleFigure;                                ///< Output pole figure pixel intensities (xPoints * yPoints).
  std::array<int32, 2> m_Dimensions;                    ///< [xPoints, yPoints] of the output image.
  ebsdlib::LaueOps::Pointer m_OrientOps;                ///< LaueOps for the crystal structure of the phase of interest.
  const std::vector<float32>& m_GbcdDeltas;             ///< Bin width in each of the 5 GBCD dimensions.
  const std::vector<float32>& m_GbcdLimits;             ///< Lower [0-4] and upper [5-9] bounds for the 5 GBCD dimensions.
  const std::vector<int32>& m_GbcdSizes;                ///< Number of bins in each of the 5 GBCD dimensions.
  const float64* m_Gbcd;                                ///< Pointer to the GBCD data (may be phase-offset or single-phase slice).
  int32 m_PhaseOfInterest = 0;                          ///< Phase index offset (0 when using a pre-extracted phase slice).
  const std::vector<float32>& m_MisorientationRotation; ///< User-specified misorientation [angle_deg, axis_x, axis_y, axis_z].

public:
  ComputeGBCDPoleFigureImpl(float64* poleFigurePtr, const std::array<int32, 2>& dimensions, const ebsdlib::LaueOps::Pointer& orientOps, const std::vector<float32>& gbcdDeltasArray,
                            const std::vector<float32>& gbcdLimitsArray, const std::vector<int32>& gbcdSizesArray, const float64* gbcdPtr, int32 phaseOfInterest,
                            const std::vector<float32>& misorientationRotation)
  : m_PoleFigure(poleFigurePtr)
  , m_Dimensions(dimensions)
  , m_OrientOps(orientOps)
  , m_GbcdDeltas(gbcdDeltasArray)
  , m_GbcdLimits(gbcdLimitsArray)
  , m_GbcdSizes(gbcdSizesArray)
  , m_Gbcd(gbcdPtr)
  , m_PhaseOfInterest(phaseOfInterest)
  , m_MisorientationRotation(misorientationRotation)
  {
  }
  ~ComputeGBCDPoleFigureImpl() = default;

  void generate(usize xStart, usize xEnd, usize yStart, usize yEnd) const
  {
    ebsdlib::Matrix3X1<float32> vec = {0.0f, 0.0f, 0.0f};
    ebsdlib::Matrix3X1<float32> vec2 = {0.0f, 0.0f, 0.0f};
    ebsdlib::Matrix3X1<float32> rotNormal = {0.0f, 0.0f, 0.0f};
    ebsdlib::Matrix3X1<float32> rotNormal2 = {0.0f, 0.0f, 0.0f};
    std::array<float32, 2> sqCoord = {0.0f, 0.0f};
    ebsdlib::Matrix3X3<float32> dg1;
    ebsdlib::Matrix3X3<float32> dg2;
    ebsdlib::Matrix3X3<float32> sym1;
    ebsdlib::Matrix3X3<float32> sym2;
    ebsdlib::Matrix3X3<float32> sym2t;

    float32 misAngle = m_MisorientationRotation[0] * nx::core::Constants::k_PiOver180F;
    nx::core::FloatVec3 normAxis = {m_MisorientationRotation[1], m_MisorientationRotation[2], m_MisorientationRotation[3]};
    normAxis = normAxis.normalize();
    ebsdlib::Matrix3X3<float32> dg = ebsdlib::AxisAngleFType(normAxis[0], normAxis[1], normAxis[2], misAngle).toOrientationMatrix().toGMatrix();
    ebsdlib::Matrix3X3<float32> dgt = dg.transpose();

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

    for(int32 k = yStart; k < yEnd; k++)
    {
      for(int32 l = xStart; l < xEnd; l++)
      {
        float32 x = static_cast<float32>(l - xPointsHalf) * xRes + (xRes / 2.0F);
        float32 y = static_cast<float32>(k - yPointsHalf) * yRes + (yRes / 2.0F);

        if((x * x + y * y) <= 1.0)
        {
          float64 sum = 0.0;
          int32 count = 0;
          vec[2] = -((x * x + y * y) - 1) / ((x * x + y * y) + 1);
          vec[0] = x * (1 + vec[2]);
          vec[1] = y * (1 + vec[2]);
          vec2 = dgt * vec;

          for(int32 i = 0; i < nSym; i++)
          {
            sym1 = m_OrientOps->getMatSymOpF(i);
            for(int32 j = 0; j < nSym; j++)
            {
              sym2 = m_OrientOps->getMatSymOpF(j);
              sym2t = sym2.transpose();
              dg1 = dg * sym2t;
              dg2 = sym1 * dg1;

              ebsdlib::EulerFType misEuler1 = ebsdlib::OrientationMatrixFType(dg2).toEuler();
              if(misEuler1[0] < nx::core::Constants::k_PiOver2F && misEuler1[1] < nx::core::Constants::k_PiOver2F && misEuler1[2] < nx::core::Constants::k_PiOver2F)
              {
                misEuler1[1] = cosf(misEuler1[1]);
                auto location1 = static_cast<int32>((misEuler1[0] - m_GbcdLimits[0]) / m_GbcdDeltas[0]);
                auto location2 = static_cast<int32>((misEuler1[1] - m_GbcdLimits[1]) / m_GbcdDeltas[1]);
                auto location3 = static_cast<int32>((misEuler1[2] - m_GbcdLimits[2]) / m_GbcdDeltas[2]);
                rotNormal = sym1 * vec;
                nhCheck = getSquareCoord(rotNormal.data(), sqCoord.data());
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
                  // m_Gbcd points to the phase-of-interest slice, so index directly without phase offset
                  sum += m_Gbcd[2 * ((location5 * shift4) + (location4 * shift3) + (location3 * shift2) + (location2 * shift1) + location1) + hemisphere];
                  count++;
                }
              }

              // again in second crystal reference frame
              dg1 = dgt * sym2;
              dg2 = sym1 * dg1;
              misEuler1 = ebsdlib::OrientationMatrixFType(dg2).toEuler();
              if(misEuler1[0] < nx::core::Constants::k_PiOver2D && misEuler1[1] < nx::core::Constants::k_PiOver2F && misEuler1[2] < nx::core::Constants::k_PiOver2F)
              {
                misEuler1[1] = cosf(misEuler1[1]);
                auto location1 = static_cast<int32>((misEuler1[0] - m_GbcdLimits[0]) / m_GbcdDeltas[0]);
                auto location2 = static_cast<int32>((misEuler1[1] - m_GbcdLimits[1]) / m_GbcdDeltas[1]);
                auto location3 = static_cast<int32>((misEuler1[2] - m_GbcdLimits[2]) / m_GbcdDeltas[2]);
                rotNormal2 = sym1 * vec2;
                nhCheck = getSquareCoord(rotNormal2.data(), sqCoord.data());
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
                  sum += m_Gbcd[2 * ((location5 * shift4) + (location4 * shift3) + (location3 * shift2) + (location2 * shift1) + location1) + hemisphere];
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
      sqCoord[0] = (crystalNormal[0] / fabsf(crystalNormal[0])) * sqrtf(2.0f * 1.0f * (1.0f + (crystalNormal[2] * adjust))) * (nx::core::Constants::k_SqrtPiF / 2.0f);
      sqCoord[1] = (crystalNormal[0] / fabsf(crystalNormal[0])) * sqrtf(2.0f * 1.0f * (1.0f + (crystalNormal[2] * adjust))) *
                   ((2.0f / nx::core::Constants::k_SqrtPiF) * atanf(crystalNormal[1] / crystalNormal[0]));
    }
    else
    {
      sqCoord[0] = (crystalNormal[1] / fabsf(crystalNormal[1])) * sqrtf(2.0f * 1.0f * (1.0f + (crystalNormal[2] * adjust))) *
                   ((2.0f / nx::core::Constants::k_SqrtPiF) * atanf(crystalNormal[0] / crystalNormal[1]));
      sqCoord[1] = (crystalNormal[1] / fabsf(crystalNormal[1])) * sqrtf(2.0f * 1.0f * (1.0f + (crystalNormal[2] * adjust))) * (nx::core::Constants::k_SqrtPiF / 2.0f);
    }
    return nhCheck;
  }
};

} // namespace

// -----------------------------------------------------------------------------
ComputeGBCDPoleFigureScanline::ComputeGBCDPoleFigureScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                             ComputeGBCDPoleFigureInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeGBCDPoleFigureScanline::~ComputeGBCDPoleFigureScanline() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeGBCDPoleFigureScanline::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
/**
 * @brief OOC-optimized GBCD pole figure generation.
 *
 * The key difference from the Direct variant is the GBCD caching strategy:
 * instead of caching the entire multi-phase GBCD array, this variant extracts
 * only the single-phase slice needed for the requested PhaseOfInterest via a
 * single copyIntoBuffer() call. This dramatically reduces memory consumption
 * when the GBCD has many phases.
 *
 * Once the phase slice is cached locally, the computation is parallelized
 * identically to the Direct path using ParallelData2DAlgorithm on the cached
 * raw pointers. The m_PhaseOfInterest parameter is set to 0 when constructing
 * the worker because the cached buffer already starts at the phase-of-interest
 * offset.
 */
Result<> ComputeGBCDPoleFigureScanline::operator()()
{
  auto& gbcd = m_DataStructure.getDataRefAs<Float64Array>(m_InputValues->GBCDArrayPath);
  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  DataPath cellIntensityArrayPath = m_InputValues->ImageGeometryPath.createChildPath(m_InputValues->CellAttributeMatrixName).createChildPath(m_InputValues->CellIntensityArrayName);
  auto& poleFigure = m_DataStructure.getDataRefAs<Float64Array>(cellIntensityArrayPath);

  // Cache ensemble-level crystal structures (typically < 10 elements).
  const usize numCrystalStructures = crystalStructures.getSize();
  auto crystalStructuresCache = std::make_unique<uint32[]>(numCrystalStructures);
  crystalStructures.getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint32>(crystalStructuresCache.get(), numCrystalStructures));

  // Allocate a local buffer for the output pole figure. Initialize to zero;
  // pixels outside the stereographic unit circle will remain at zero.
  const usize poleFigureSize = poleFigure.getSize();
  auto poleFigureCache = std::make_unique<float64[]>(poleFigureSize);
  std::fill(poleFigureCache.get(), poleFigureCache.get() + poleFigureSize, 0.0);

  // ----- GBCD bin configuration (same as Direct variant) -----
  std::vector<float32> gbcdDeltas(5, 0);
  std::vector<float32> gbcdLimits(10, 0);
  std::vector<int32> gbcdSizes(5, 0);

  // Greg Rohrer's ranges for the 5D GBCD parameter space.
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

  // Override the 3rd and 4th dimension bounds to use the Lambert equal-area
  // square-grid projection.
  gbcdLimits[3] = -sqrtf(Constants::k_PiOver2D);
  gbcdLimits[4] = -sqrtf(Constants::k_PiOver2D);
  gbcdLimits[8] = sqrtf(Constants::k_PiOver2D);
  gbcdLimits[9] = sqrtf(Constants::k_PiOver2D);

  // Extract the 5D component shape from the GBCD DataArray.
  ShapeType cDims = gbcd.getComponentShape();

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

  // Total number of GBCD bins per phase (both hemispheres).
  int64 totalGbcdBins = gbcdSizes[0] * gbcdSizes[1] * gbcdSizes[2] * gbcdSizes[3] * gbcdSizes[4] * 2;

  // ---- OOC optimization: cache only the single phase slice ----
  // The full GBCD array has (numPhases * totalGbcdBins) elements. For an OOC store,
  // reading the entire array would load all phases' bins from disk. Instead, we
  // compute the element offset for the phase-of-interest and read only that
  // contiguous slice. This is the critical optimization: one phase's GBCD is
  // typically 100K-500K float64 elements vs. millions for all phases combined.
  const usize phaseOffset = static_cast<usize>(m_InputValues->PhaseOfInterest) * static_cast<usize>(totalGbcdBins);
  auto gbcdPhaseCache = std::make_unique<float64[]>(static_cast<usize>(totalGbcdBins));
  gbcd.getDataStoreRef().copyIntoBuffer(phaseOffset, nonstd::span<float64>(gbcdPhaseCache.get(), static_cast<usize>(totalGbcdBins)));

  // Select the LaueOps instance for the phase of interest.
  ebsdlib::LaueOps::Pointer orientOps = ebsdlib::LaueOps::GetAllOrientationOps()[crystalStructuresCache[m_InputValues->PhaseOfInterest]];

  int32 xPoints = m_InputValues->OutputImageDimension;
  int32 yPoints = m_InputValues->OutputImageDimension;

  m_MessageHandler({IFilter::Message::Type::Info, fmt::format("Generating Intensity Plot for phase {} (OOC)", m_InputValues->PhaseOfInterest)});

  // Parallel execution is safe because all data is accessed through locally-cached
  // raw pointers -- no DataStore access occurs in the hot loop.
  ParallelData2DAlgorithm dataAlg;
  dataAlg.setRange(0, xPoints, 0, yPoints);

  // Pass phaseOfInterest=0 to the worker because gbcdPhaseCache already points to
  // the start of the phase-of-interest slice. The worker's GBCD indexing formula
  // uses (phaseOfInterest * totalGbcdBins) as an offset, so passing 0 means no
  // additional offset is applied to our already-offset buffer.
  dataAlg.execute(ComputeGBCDPoleFigureImpl(poleFigureCache.get(), {xPoints, yPoints}, orientOps, gbcdDeltas, gbcdLimits, gbcdSizes, gbcdPhaseCache.get(), 0, m_InputValues->MisorientationRotation));

  // Write the computed pole figure intensities back to the DataStore.
  poleFigure.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float64>(poleFigureCache.get(), poleFigureSize));

  return {};
}
