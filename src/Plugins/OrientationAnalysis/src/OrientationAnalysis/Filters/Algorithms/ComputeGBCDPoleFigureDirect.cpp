#include "ComputeGBCDPoleFigureDirect.hpp"

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
 * @brief Threaded worker for generating a GBCD stereographic pole figure.
 *
 * Each instance is invoked by ParallelData2DAlgorithm on a disjoint 2D rectangular
 * region of the output pole figure image. For each pixel in the assigned region, the
 * worker:
 *
 *   1. Performs inverse stereographic projection to obtain a unit-sphere direction.
 *   2. Iterates over all pairs of crystal symmetry operators (O(nSym^2) per pixel).
 *   3. For each symmetry pair, computes the symmetrically-equivalent misorientation
 *      in both crystal reference frames.
 *   4. If the equivalent misorientation falls within the fundamental zone (all three
 *      Euler angles < pi/2), the corresponding 5D GBCD bin is looked up and accumulated.
 *   5. The pixel intensity is the average GBCD value across all valid symmetry pairs.
 *
 * The GBCD is stored as a 5D histogram with dimensions:
 *   [misorientation_phi1, cos(misorientation_Phi), misorientation_phi2, boundary_normal_theta, boundary_normal_phi]
 * multiplied by 2 for the two hemispheres (northern and southern).
 *
 * This worker operates on raw float64 pointers to locally-cached data, making it
 * safe for multi-threaded execution. No OOC DataStore access occurs in the hot loop.
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
  const float64* m_Gbcd;                                ///< Pointer to the (possibly phase-offset) GBCD data.
  int32 m_PhaseOfInterest = 0;                          ///< Phase index offset applied when indexing into m_Gbcd.
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

  /**
   * @brief Generates pole figure intensities for the pixel sub-region [xStart,xEnd) x [yStart,yEnd).
   *
   * For each pixel within the unit circle of the stereographic projection, computes the
   * average GBCD intensity across all symmetrically-equivalent misorientations.
   */
  void generate(usize xStart, usize xEnd, usize yStart, usize yEnd) const
  {
    ebsdlib::Matrix3X1<float32> vec = {0.0f, 0.0f, 0.0f};
    ebsdlib::Matrix3X1<float32> vec2 = {0.0f, 0.0f, 0.0f};
    ebsdlib::Matrix3X1<float32> rotNormal = {0.0f, 0.0f, 0.0f};
    ebsdlib::Matrix3X1<float32> rotNormal2 = {0.0f, 0.0f, 0.0f};
    std::array<float32, 2> sqCoord = {0.0f, 0.0f};
    // float32 dg[3][3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    // float32 dgt[3][3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    ebsdlib::Matrix3X3<float32> dg1;   // = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    ebsdlib::Matrix3X3<float32> dg2;   // = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    ebsdlib::Matrix3X3<float32> sym1;  // = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    ebsdlib::Matrix3X3<float32> sym2;  // = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    ebsdlib::Matrix3X3<float32> sym2t; // = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    // Matrix3X1<float32> misEuler1 = {0.0f, 0.0f, 0.0f};

    // Convert the user-specified misorientation from axis-angle (degrees) to a
    // 3x3 rotation matrix (dg). The transpose (dgt) represents the inverse
    // misorientation, used when repeating the lookup in the second crystal frame.
    float32 misAngle = m_MisorientationRotation[0] * nx::core::Constants::k_PiOver180F;
    nx::core::FloatVec3 normAxis = {m_MisorientationRotation[1], m_MisorientationRotation[2], m_MisorientationRotation[3]};
    normAxis = normAxis.normalize();
    ebsdlib::Matrix3X3<float32> dg = ebsdlib::AxisAngleFType(normAxis[0], normAxis[1], normAxis[2], misAngle).toOrientationMatrix().toGMatrix();
    ebsdlib::Matrix3X3<float32> dgt = dg.transpose();

    // Number of crystal symmetry operators for this Laue class (e.g., 24 for cubic).
    int32 nSym = m_OrientOps->getNumSymOps();

    // Output image grid parameters. The pole figure spans [-1, 1] in both x and y,
    // centered at (xPointsHalf, yPointsHalf). Only pixels inside the unit circle
    // (x^2 + y^2 <= 1) are computed.
    int32 xPoints = m_Dimensions[0];
    int32 yPoints = m_Dimensions[1];
    int32 xPointsHalf = xPoints / 2;
    int32 yPointsHalf = yPoints / 2;
    float32 xRes = 2.0f / float32(xPoints);
    float32 yRes = 2.0f / float32(yPoints);
    bool nhCheck = false;
    int32 hemisphere = 0;

    // Precompute stride multipliers for the 5D GBCD array's row-major linearization.
    // The 5D index [loc1, loc2, loc3, loc4, loc5] maps to:
    //   loc1 + loc2*shift1 + loc3*shift2 + loc4*shift3 + loc5*shift4
    // with a final *2 + hemisphere for the north/south hemisphere selection.
    int32 shift1 = m_GbcdSizes[0];
    int32 shift2 = m_GbcdSizes[0] * m_GbcdSizes[1];
    int32 shift3 = m_GbcdSizes[0] * m_GbcdSizes[1] * m_GbcdSizes[2];
    int32 shift4 = m_GbcdSizes[0] * m_GbcdSizes[1] * m_GbcdSizes[2] * m_GbcdSizes[3];

    // Total number of GBCD bins per phase (both hemispheres).
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
          // Inverse stereographic projection: map (x, y) in the unit disk to a
          // unit-sphere direction (vec). This is the boundary-plane normal direction
          // in the sample reference frame.
          vec[2] = -((x * x + y * y) - 1) / ((x * x + y * y) + 1);
          vec[0] = x * (1 + vec[2]);
          vec[1] = y * (1 + vec[2]);
          // Transform the normal into the second crystal reference frame using
          // the inverse misorientation (dgt). This is needed for the bicrystal
          // symmetry computation below.
          vec2 = dgt * vec;

          // Loop over all pairs of symmetry operators (O(nSym^2) per pixel).
          // For each pair (sym1, sym2), we compute the symmetrically-equivalent
          // misorientation and look up the GBCD bin for that misorientation +
          // boundary-plane normal combination.
          for(int32 i = 0; i < nSym; i++)
          {
            sym1 = m_OrientOps->getMatSymOpF(i);
            for(int32 j = 0; j < nSym; j++)
            {
              sym2 = m_OrientOps->getMatSymOpF(j);
              sym2t = sym2.transpose();
              // Compute the symmetrically-equivalent misorientation:
              //   dg2 = sym1 * dg * sym2^T
              // This applies symmetry operator i on the left and j on the right.
              dg1 = dg * sym2t;
              dg2 = sym1 * dg1;

              // convert to euler angle
              ebsdlib::EulerFType misEuler1 = ebsdlib::OrientationMatrixFType(dg2).toEuler();
              if(misEuler1[0] < nx::core::Constants::k_PiOver2F && misEuler1[1] < nx::core::Constants::k_PiOver2F && misEuler1[2] < nx::core::Constants::k_PiOver2F)
              {
                misEuler1[1] = cosf(misEuler1[1]);
                // find bins in GBCD
                auto location1 = static_cast<int32>((misEuler1[0] - m_GbcdLimits[0]) / m_GbcdDeltas[0]);
                auto location2 = static_cast<int32>((misEuler1[1] - m_GbcdLimits[1]) / m_GbcdDeltas[1]);
                auto location3 = static_cast<int32>((misEuler1[2] - m_GbcdLimits[2]) / m_GbcdDeltas[2]);
                // find symmetric poles using the first symmetry operator
                rotNormal = sym1 * vec;
                // get coordinates in square projection of crystal normal parallel to boundary normal
                nhCheck = getSquareCoord(rotNormal.data(), sqCoord.data());
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
              dg1 = dgt * sym2;
              dg2 = sym1 * dg1;
              // convert to euler angle
              misEuler1 = ebsdlib::OrientationMatrixFType(dg2).toEuler();
              if(misEuler1[0] < nx::core::Constants::k_PiOver2D && misEuler1[1] < nx::core::Constants::k_PiOver2F && misEuler1[2] < nx::core::Constants::k_PiOver2F)
              {
                misEuler1[1] = cosf(misEuler1[1]);
                // find bins in GBCD
                auto location1 = static_cast<int32>((misEuler1[0] - m_GbcdLimits[0]) / m_GbcdDeltas[0]);
                auto location2 = static_cast<int32>((misEuler1[1] - m_GbcdLimits[1]) / m_GbcdDeltas[1]);
                auto location3 = static_cast<int32>((misEuler1[2] - m_GbcdLimits[2]) / m_GbcdDeltas[2]);
                // find symmetric poles using the first symmetry operator
                rotNormal2 = sym1 * vec2;
                // get coordinates in square projection of crystal normal parallel to boundary normal
                nhCheck = getSquareCoord(rotNormal2.data(), sqCoord.data());
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
ComputeGBCDPoleFigureDirect::ComputeGBCDPoleFigureDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                         ComputeGBCDPoleFigureInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeGBCDPoleFigureDirect::~ComputeGBCDPoleFigureDirect() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeGBCDPoleFigureDirect::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
/**
 * @brief In-core GBCD pole figure generation.
 *
 * Caches the entire GBCD array (all phases, all bins) into a local heap buffer,
 * then uses ParallelData2DAlgorithm to compute pole figure pixel intensities in
 * parallel. Each pixel's intensity is the symmetry-averaged GBCD value for the
 * user-specified misorientation at the boundary-plane normal corresponding to
 * that pixel's stereographic projection coordinate.
 *
 * The GBCD data, crystal structures, and output pole figure are all copied to/from
 * local buffers via copyIntoBuffer()/copyFromBuffer() so that the parallel worker
 * operates on plain raw pointers with no DataStore access.
 */
Result<> ComputeGBCDPoleFigureDirect::operator()()
{
  auto& gbcd = m_DataStructure.getDataRefAs<Float64Array>(m_InputValues->GBCDArrayPath);
  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  DataPath cellIntensityArrayPath = m_InputValues->ImageGeometryPath.createChildPath(m_InputValues->CellAttributeMatrixName).createChildPath(m_InputValues->CellIntensityArrayName);
  auto& poleFigure = m_DataStructure.getDataRefAs<Float64Array>(cellIntensityArrayPath);

  // Cache the entire GBCD array into a contiguous local buffer. This is the
  // in-core path: we expect the full array to fit in RAM. The buffer is passed
  // as a raw float64* to the parallel worker, avoiding any DataStore access
  // in the hot loop.
  const usize gbcdTotalElements = gbcd.getSize();
  auto gbcdCache = std::make_unique<float64[]>(gbcdTotalElements);
  gbcd.getDataStoreRef().copyIntoBuffer(0, nonstd::span<float64>(gbcdCache.get(), gbcdTotalElements));

  // Cache ensemble-level crystal structures (typically < 10 elements).
  const usize numCrystalStructures = crystalStructures.getSize();
  auto crystalStructuresCache = std::make_unique<uint32[]>(numCrystalStructures);
  crystalStructures.getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint32>(crystalStructuresCache.get(), numCrystalStructures));

  // Allocate a local buffer for the output pole figure (e.g., 300x300 = 90,000
  // float64 elements). Initialize to zero; pixels outside the unit circle will
  // remain at zero intensity.
  const usize poleFigureSize = poleFigure.getSize();
  auto poleFigureCache = std::make_unique<float64[]>(poleFigureSize);
  std::fill(poleFigureCache.get(), poleFigureCache.get() + poleFigureSize, 0.0);

  // ----- GBCD bin configuration -----
  // The GBCD is a 5-dimensional histogram. The 5 dimensions are:
  //   [0] misorientation phi1         (range: [0, pi/2])
  //   [1] cos(misorientation Phi)     (range: [0, 1])
  //   [2] misorientation phi2         (range: [0, pi/2])
  //   [3] boundary normal theta       (range: [-sqrt(pi/2), sqrt(pi/2)], square-grid projection)
  //   [4] boundary normal phi         (range: [-sqrt(pi/2), sqrt(pi/2)], square-grid projection)
  // Each dimension is split into gbcdSizes[i] bins of width gbcdDeltas[i].
  // gbcdLimits[0..4] are the lower bounds and gbcdLimits[5..9] are the upper bounds.
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
  // square-grid projection instead of raw angular coordinates.
  gbcdLimits[3] = -sqrtf(Constants::k_PiOver2D);
  gbcdLimits[4] = -sqrtf(Constants::k_PiOver2D);
  gbcdLimits[8] = sqrtf(Constants::k_PiOver2D);
  gbcdLimits[9] = sqrtf(Constants::k_PiOver2D);

  // Extract the 5D component shape from the GBCD DataArray. The GBCD array has
  // one tuple per phase and a multi-dimensional component shape encoding the 5D bins.
  ShapeType cDims = gbcd.getComponentShape();

  gbcdSizes[0] = static_cast<int32>(cDims[0]);
  gbcdSizes[1] = static_cast<int32>(cDims[1]);
  gbcdSizes[2] = static_cast<int32>(cDims[2]);
  gbcdSizes[3] = static_cast<int32>(cDims[3]);
  gbcdSizes[4] = static_cast<int32>(cDims[4]);

  // Compute bin widths: delta = (upper_bound - lower_bound) / num_bins.
  gbcdDeltas[0] = (gbcdLimits[5] - gbcdLimits[0]) / static_cast<float32>(gbcdSizes[0]);
  gbcdDeltas[1] = (gbcdLimits[6] - gbcdLimits[1]) / static_cast<float32>(gbcdSizes[1]);
  gbcdDeltas[2] = (gbcdLimits[7] - gbcdLimits[2]) / static_cast<float32>(gbcdSizes[2]);
  gbcdDeltas[3] = (gbcdLimits[8] - gbcdLimits[3]) / static_cast<float32>(gbcdSizes[3]);
  gbcdDeltas[4] = (gbcdLimits[9] - gbcdLimits[4]) / static_cast<float32>(gbcdSizes[4]);

  // Select the LaueOps instance for the phase of interest. The crystal structure
  // enum (e.g., Cubic_High, Hexagonal_High) determines the set of symmetry
  // operators used to enumerate equivalent misorientations.
  ebsdlib::LaueOps::Pointer orientOps = ebsdlib::LaueOps::GetAllOrientationOps()[crystalStructuresCache[m_InputValues->PhaseOfInterest]];

  int32 xPoints = m_InputValues->OutputImageDimension;
  int32 yPoints = m_InputValues->OutputImageDimension;
  int32 zPoints = 1;
  float32 xRes = 2.0f / static_cast<float32>(xPoints);
  float32 yRes = 2.0f / static_cast<float32>(yPoints);
  float32 zRes = (xRes + yRes) / 2.0F;

  m_MessageHandler({IFilter::Message::Type::Info, fmt::format("Generating Intensity Plot for phase {}", m_InputValues->PhaseOfInterest)});

  // Launch multi-threaded pole figure computation. All data is accessed through
  // locally-cached raw pointers (gbcdCache, poleFigureCache), so parallel writes
  // to disjoint pixel regions are safe -- no DataStore access occurs in the hot loop.
  ParallelData2DAlgorithm dataAlg;
  dataAlg.setRange(0, xPoints, 0, yPoints);

  dataAlg.execute(ComputeGBCDPoleFigureImpl(poleFigureCache.get(), {xPoints, yPoints}, orientOps, gbcdDeltas, gbcdLimits, gbcdSizes, gbcdCache.get(), m_InputValues->PhaseOfInterest,
                                            m_InputValues->MisorientationRotation));

  // Write the computed pole figure intensities back to the DataStore. This is a
  // single bulk write that works correctly for both in-core and OOC-backed stores.
  poleFigure.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float64>(poleFigureCache.get(), poleFigureSize));

  return {};
}
