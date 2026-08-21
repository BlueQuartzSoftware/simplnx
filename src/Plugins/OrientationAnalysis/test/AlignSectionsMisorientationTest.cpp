#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/AlignSectionsMisorientationFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;

/**
 * Read H5Ebsd File
 * MultiThreshold Objects
 * Convert Orientation Representation (Euler->Quats)
 * Align Sections Misorientation
 *
 * Compare all the data arrays from the "Exemplar Data / CellData"
 */

// -----------------------------------------------------------------------------
// Class 1 (analytical) oracle scaffolding.
//
// The full hand derivation that every expected value below comes from lives in
// docs form beside this test suite; the load-bearing steps are reproduced as
// comments at each assertion site. The short version:
//
// SIGN CONVENTION (re-derived from Algorithms/AlignSectionsMisorientation.cpp:135-136).
//   refposition = ((slice + 1) * dimX * dimY) + (l * dimX) + n
//   curposition = ( slice      * dimX * dimY) + ((l + sy) * dimX) + (n + sx)
// The reference is the UPPER slice (slice + 1), read UNSHIFTED; the candidate shift
// (sx, sy) is applied to the LOWER (moving) slice's read window. If slice z carries an
// analytic pattern P displaced by D_z -- slice_z(x, y) = P(x - Dx_z, y - Dy_z) -- then the
// candidate that makes every sampled pair agree is exactly s = D_lower - D_upper.
//   => the reported relative shift is +d, NOT negated.
// The shared base then fills the aligned voxel (x, y) from the ORIGINAL voxel
// (x + Sx, y + Sy) (Utilities/AlignSections.cpp:79-85), so applying the accumulated shift
// S_z = D_z - D_top registers every slice onto the top slice, and voxels whose source
// falls off the slice are zero-filled (AlignSections.cpp:86-89).
//
// SAMPLING. The mismatch fraction is evaluated on a stride-4 lattice only
// (AlignSectionsMisorientation.cpp:128-130), so on a 32x32 slice the sampled plane
// coordinates are {0, 4, ..., 28} -- 8 x 8 = 64 positions at full overlap. `count` is
// incremented for EVERY in-bounds sampled pair (:134), including mask-both-false pairs.
//
// PATTERN. Each fixture slice carries this pattern, in pattern coordinates
// (u, v) = (x - Dx, y - Dy):
//   * v not a multiple of 4  -> phase 0. Phase 0 fails the `cellPhases > 0` test at :140,
//     so the angle stays FLT_MAX and the pair is an UNCONDITIONAL mismatch at any
//     tolerance. These rows pin the y component of the shift.
//   * otherwise              -> phase 1, orientation A if u < T(v) else B.
// T(v) is a zigzag staircase that steps one voxel right every 4 rows and reverses at the
// midpoint. Over the 8 sampled rows of a 32-row slice it takes the values
// 16,17,18,19,20,19,18,17, whose residues mod 4 are 0,1,2,3,0,3,2,1 -- each residue
// exactly twice. Two consequences, both load-bearing:
//   (a) a boundary whose x position varies by one voxel per 4 rows is the only way a
//       stride-4 sampling can resolve a single-voxel shift (a plain vertical boundary
//       aliases and produces 4-way ties);
//   (b) because the 8 values cover each residue mod 4 exactly twice, the number of sampled
//       columns falling between the reference boundary and the probed boundary is EXACTLY
//       2 * |ex| where ex is the candidate's x error. Hence, for zero y error,
//              mismatch(ex, 0) = 2 * |ex|,     zero only at ex == 0.
//       The zigzag (rather than a monotone staircase) is required: a monotone staircase
//       satisfies T(v - 4) == T(v) - 1 exactly, which makes the candidate e = (-1, -4)
//       score identically to e = (0, 0) -- an exact tie with the true answer.
// -----------------------------------------------------------------------------
namespace AnalyticalFixtures
{
constexpr int64 k_Dim = 32;     // X and Y extent of every analytical fixture
constexpr int64 k_HalfDim = 16; // == int64(32 * 0.5f), the algorithm's halfDim0/halfDim1 (:78-79)
constexpr int64 k_Stride = 4;   // the algorithm's sampling stride (:128-130)

const std::string k_ImageGeomName = "Image Geometry";
const DataPath k_ImagePath({k_ImageGeomName});
const std::string k_CellDataName = "Cell Data";
const DataPath k_CellDataPath = k_ImagePath.createChildPath(k_CellDataName);
const DataPath k_QuatsPath = k_CellDataPath.createChildPath("Quats");
const DataPath k_PhasesPath = k_CellDataPath.createChildPath("Phases");
const DataPath k_MaskPath = k_CellDataPath.createChildPath("Mask");
const std::string k_EnsembleDataName = "Cell Ensemble Data";
const DataPath k_EnsembleDataPath = k_ImagePath.createChildPath(k_EnsembleDataName);
const DataPath k_CrystalStructuresPath = k_EnsembleDataPath.createChildPath("CrystalStructures");

const std::string k_AlignmentAMName = "Alignment Shifts Data";
const std::string k_SlicesName = "Slice Indices";
const std::string k_RelativeShiftsName = "Relative Shifts";
const std::string k_CumulativeShiftsName = "Cumulative Shifts";
const DataPath k_AlignmentAMPath = k_ImagePath.createChildPath(k_AlignmentAMName);
const DataPath k_SlicesPath = k_AlignmentAMPath.createChildPath(k_SlicesName);
const DataPath k_RelativeShiftsPath = k_AlignmentAMPath.createChildPath(k_RelativeShiftsName);
const DataPath k_CumulativeShiftsPath = k_AlignmentAMPath.createChildPath(k_CumulativeShiftsName);

// EbsdLib crystal-structure enum values, verified against
// EbsdLib/Core/EbsdLibConstants.h (Hexagonal_High == 0, Cubic_High == 1,
// UnknownCrystalStructure == 999). GetAllOrientationOps() pushes the ops in the same order
// as the enum, so enum value N indexes ops entry N.
constexpr uint32 k_HexagonalHigh = 0;
constexpr uint32 k_CubicHigh = 1;
constexpr uint32 k_UnknownStructure = 999;

// C++ '/' and '%' truncate toward zero, which corrupts the pattern definition at negative
// pattern coordinates (a slice with a positive offset has negative u/v near the origin).
int64 FloorDiv(int64 numerator, int64 denominator)
{
  const int64 quotient = numerator / denominator;
  return ((numerator % denominator != 0) && ((numerator < 0) != (denominator < 0))) ? quotient - 1 : quotient;
}

int64 FloorMod(int64 numerator, int64 denominator)
{
  return numerator - (FloorDiv(numerator, denominator) * denominator);
}

// Zigzag staircase boundary T(v). See the header comment, point (a)/(b).
int64 StaircaseBoundary(int64 v)
{
  static constexpr std::array<int64, 8> k_Zigzag = {0, 1, 2, 3, 4, 3, 2, 1};
  return k_HalfDim + k_Zigzag[static_cast<usize>(FloorMod(FloorDiv(v, k_Stride), 8))];
}

// Unit quaternion for a rotation of `degrees` about [001], in the (x, y, z, w) storage
// order that both the on-disk Quats layout and ebsdlib::QuatD's constructor use
// (EbsdLib/Orientation/Quaternion.hpp:45).
std::array<float32, 4> QuatAboutZ(float64 degrees)
{
  const float64 halfAngle = (degrees * nx::core::numbers::pi) / 360.0;
  return {0.0F, 0.0F, static_cast<float32>(std::sin(halfAngle)), static_cast<float32>(std::cos(halfAngle))};
}

// What carries the fixture's spatial structure.
enum class Carrier
{
  Orientation, // phase-0 rows pin y; the A/B staircase carries the x structure
  Mask,        // identity quaternion + phase 1 everywhere; the mask carries both
  MaskAllFalse // identity quaternion + phase 1 everywhere; the mask is entirely false
};

struct FixtureSpec
{
  std::vector<std::array<int64, 2>> offsets;                                 // offsets[z] = {Dx, Dy} of slice z
  Carrier carrier = Carrier::Orientation;                                    //
  float64 patternDegrees = 30.0;                                             // orientation B, as a rotation about [001]
  std::vector<uint32> crystalStructures = {k_UnknownStructure, k_CubicHigh}; // ensemble tuple 0 is conventionally unused
  int32 stripePhase = 0;                                                     // if > 0, pattern columns u in [4, 8) take this phase
};

// True when the pattern row v is one of the phase-0 "y pin" rows.
bool IsPinRow(int64 v)
{
  return FloorMod(v, k_Stride) != 0;
}

// Orientation index (0 == A == identity, 1 == B) of the pattern at pattern coordinates.
int32 PatternOrientation(int64 u, int64 v)
{
  return (u < StaircaseBoundary(v)) ? 0 : 1;
}

// Builds a k_Dim x k_Dim x zDim ImageGeom carrying the analytical pattern.
DataStructure BuildFixture(const FixtureSpec& spec)
{
  const auto zDim = static_cast<int64>(spec.offsets.size());

  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeomName);
  imageGeom->setDimensions({static_cast<usize>(k_Dim), static_cast<usize>(k_Dim), static_cast<usize>(zDim)});

  // AttributeMatrix tuple shape for a grid geometry is (z, y, x)
  const ShapeType cellTupleShape = {static_cast<usize>(zDim), static_cast<usize>(k_Dim), static_cast<usize>(k_Dim)};
  auto* cellAM = AttributeMatrix::Create(dataStructure, k_CellDataName, cellTupleShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  auto* quats = UnitTest::CreateTestDataArray<float32>(dataStructure, "Quats", cellTupleShape, {4}, cellAM->getId());
  auto* phases = UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", cellTupleShape, {1}, cellAM->getId());
  auto* mask = UnitTest::CreateTestDataArray<bool>(dataStructure, "Mask", cellTupleShape, {1}, cellAM->getId());

  auto* ensembleAM = AttributeMatrix::Create(dataStructure, k_EnsembleDataName, ShapeType{spec.crystalStructures.size()}, imageGeom->getId());
  auto* crystalStructures = UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", ShapeType{spec.crystalStructures.size()}, {1}, ensembleAM->getId());
  for(usize i = 0; i < spec.crystalStructures.size(); i++)
  {
    (*crystalStructures)[i] = spec.crystalStructures[i];
  }

  const std::array<float32, 4> orientationA = QuatAboutZ(0.0);
  const std::array<float32, 4> orientationB = QuatAboutZ(spec.patternDegrees);

  for(int64 z = 0; z < zDim; z++)
  {
    const int64 offsetX = spec.offsets[static_cast<usize>(z)][0];
    const int64 offsetY = spec.offsets[static_cast<usize>(z)][1];
    for(int64 y = 0; y < k_Dim; y++)
    {
      for(int64 x = 0; x < k_Dim; x++)
      {
        const int64 u = x - offsetX;
        const int64 v = y - offsetY;
        const auto position = static_cast<usize>((z * k_Dim * k_Dim) + (y * k_Dim) + x);

        const bool pinRow = IsPinRow(v);
        const int32 orientation = PatternOrientation(u, v);

        if(spec.carrier == Carrier::Orientation)
        {
          (*phases)[position] = pinRow ? 0 : 1;
          if(!pinRow && spec.stripePhase > 0 && u >= 4 && u < 8)
          {
            (*phases)[position] = spec.stripePhase;
          }
          const std::array<float32, 4>& quat = (!pinRow && orientation == 1) ? orientationB : orientationA;
          for(usize c = 0; c < 4; c++)
          {
            (*quats)[(position * 4) + c] = quat[c];
          }
          (*mask)[position] = true;
        }
        else
        {
          // Mask carriers: every cell is phase 1 with the identity quaternion, so every
          // both-true pair has a disorientation of exactly 0 degrees and can never
          // contribute to the mismatch numerator at any tolerance. All structure is in
          // the mask.
          (*phases)[position] = 1;
          for(usize c = 0; c < 4; c++)
          {
            (*quats)[(position * 4) + c] = orientationA[c];
          }
          (*mask)[position] = (spec.carrier == Carrier::MaskAllFalse) ? false : (!pinRow && orientation == 0);
        }
      }
    }
  }

  return dataStructure;
}

// Fills `args` for an analytical fixture run.
Arguments MakeArguments(float32 toleranceDegrees, bool useMask, bool storeShifts)
{
  Arguments args;
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_MisorientationTolerance_Key, std::make_any<float32>(toleranceDegrees));
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_UseMask_Key, std::make_any<bool>(useMask));
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_MaskPath));
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ImagePath));
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsPath));
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresPath));
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_StoreAlignmentShifts_Key, std::make_any<bool>(storeShifts));
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_AlignmentAMName_Key, std::make_any<std::string>(k_AlignmentAMName));
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_SlicesArrayName_Key, std::make_any<std::string>(k_SlicesName));
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_RelativeShiftsArrayName_Key, std::make_any<std::string>(k_RelativeShiftsName));
  args.insertOrAssign(AlignSectionsMisorientationFilter::k_CumulativeShiftsArrayName_Key, std::make_any<std::string>(k_CumulativeShiftsName));
  return args;
}

// Asserts the three shift arrays against a hand-derived table. `expected[iter]` is
// {slice, relativeX, relativeY, cumulativeX, cumulativeY}; the `Slices` array's second
// component is always slice + 1 (the reference slice).
void CheckShiftArrays(const DataStructure& dataStructure, const std::vector<std::array<int64, 5>>& expected)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>(k_SlicesPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int64Array>(k_RelativeShiftsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int64Array>(k_CumulativeShiftsPath));
  const auto& slices = dataStructure.getDataRefAs<UInt32Array>(k_SlicesPath);
  const auto& relative = dataStructure.getDataRefAs<Int64Array>(k_RelativeShiftsPath);
  const auto& cumulative = dataStructure.getDataRefAs<Int64Array>(k_CumulativeShiftsPath);

  REQUIRE(slices.getNumberOfTuples() == expected.size());
  REQUIRE(relative.getNumberOfTuples() == expected.size());
  REQUIRE(cumulative.getNumberOfTuples() == expected.size());

  for(usize iter = 0; iter < expected.size(); iter++)
  {
    CAPTURE(iter);
    const std::array<int64, 5>& row = expected[iter];
    // Tuple 0 is never written by findShifts (the loop starts at iter == 1); the
    // CreateArrayActions pass a fill value of "0" so it is deterministically {0, 0}.
    const uint32 expectedSliceLow = (iter == 0) ? 0U : static_cast<uint32>(row[0]);
    const uint32 expectedSliceHigh = (iter == 0) ? 0U : static_cast<uint32>(row[0] + 1);
    REQUIRE(slices[(iter * 2)] == expectedSliceLow);
    REQUIRE(slices[(iter * 2) + 1] == expectedSliceHigh);
    REQUIRE(relative[(iter * 2)] == row[1]);
    REQUIRE(relative[(iter * 2) + 1] == row[2]);
    REQUIRE(cumulative[(iter * 2)] == row[3]);
    REQUIRE(cumulative[(iter * 2) + 1] == row[4]);
  }
}

// Asserts the aligned volume. `cumulative[z]` is the hand-derived accumulated shift applied
// to slice z (all zero for the top slice, which the base never touches). Per
// AlignSections.cpp:79-89 the aligned voxel (x, y) is filled from the ORIGINAL voxel
// (x + Sx, y + Sy), or with the zero of the array's type when that source is off-slice.
// Because S_z == D_z - D_top for every slice, every aligned slice must carry the pattern at
// the TOP slice's offset wherever it was not zero-filled.
void CheckAlignedVolume(const DataStructure& dataStructure, const FixtureSpec& spec, const std::vector<std::array<int64, 2>>& cumulative)
{
  const auto zDim = static_cast<int64>(spec.offsets.size());
  const int64 topOffsetX = spec.offsets[static_cast<usize>(zDim - 1)][0];
  const int64 topOffsetY = spec.offsets[static_cast<usize>(zDim - 1)][1];

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_PhasesPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_QuatsPath));
  const auto& phases = dataStructure.getDataRefAs<Int32Array>(k_PhasesPath);
  const auto& quats = dataStructure.getDataRefAs<Float32Array>(k_QuatsPath);

  const std::array<float32, 4> orientationA = QuatAboutZ(0.0);
  const std::array<float32, 4> orientationB = QuatAboutZ(spec.patternDegrees);

  for(int64 z = 0; z < zDim; z++)
  {
    const int64 shiftX = cumulative[static_cast<usize>(z)][0];
    const int64 shiftY = cumulative[static_cast<usize>(z)][1];
    for(int64 y = 0; y < k_Dim; y++)
    {
      for(int64 x = 0; x < k_Dim; x++)
      {
        CAPTURE(z, y, x);
        const auto position = static_cast<usize>((z * k_Dim * k_Dim) + (y * k_Dim) + x);
        const int64 sourceX = x + shiftX;
        const int64 sourceY = y + shiftY;
        const bool inBounds = (sourceX >= 0) && (sourceX < k_Dim) && (sourceY >= 0) && (sourceY < k_Dim);

        if(!inBounds)
        {
          // Zero fill (AlignSections.cpp:86-89)
          REQUIRE(phases[position] == 0);
          for(usize c = 0; c < 4; c++)
          {
            REQUIRE(quats[(position * 4) + c] == 0.0F);
          }
          continue;
        }

        // Registered onto the top slice: pattern coordinates of the aligned voxel
        const int64 u = x - topOffsetX;
        const int64 v = y - topOffsetY;
        const bool pinRow = IsPinRow(v);
        int32 expectedPhase = pinRow ? 0 : 1;
        if(!pinRow && spec.stripePhase > 0 && u >= 4 && u < 8)
        {
          expectedPhase = spec.stripePhase;
        }
        REQUIRE(phases[position] == expectedPhase);

        const std::array<float32, 4>& expectedQuat = (!pinRow && PatternOrientation(u, v) == 1) ? orientationB : orientationA;
        for(usize c = 0; c < 4; c++)
        {
          REQUIRE(quats[(position * 4) + c] == expectedQuat[c]);
        }
      }
    }
  }
}
} // namespace AnalyticalFixtures

TEST_CASE("OrientationAnalysis::AlignSectionsMisorientation Small IN100 Pipeline", "[OrientationAnalysis][AlignSectionsMisorientation]")
{
  UnitTest::LoadPlugins();
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "align_sections_misorientation.tar.gz", "align_sections_misorientation");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");

  auto* filterList = Application::Instance()->getFilterList();

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/align_sections_misorientation/6_6_align_sections_misorientation.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // MultiThreshold Objects Filter (From SimplnxCore Plugins)
  SmallIn100::ExecuteMultiThresholdObjects(dataStructure, *filterList);

  // Convert Orientations Filter (From OrientationAnalysis Plugin)
  SmallIn100::ExecuteConvertOrientations(dataStructure, *filterList);

  // Align Sections Misorientation Filter (From OrientationAnalysis Plugin)
  {
    Arguments args;
    AlignSectionsMisorientationFilter filter;
    // Create default Parameters for the filter.

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0F));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(Constants::k_MaskArrayPath));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(Constants::k_QuatsArrayPath));
    args.insertOrAssign(AlignSectionsMisorientationFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(Constants::k_PhasesArrayPath));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(Constants::k_CrystalStructuresArrayPath));

    args.insertOrAssign(AlignSectionsMisorientationFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  UnitTest::CompareExemplarToGeneratedData(dataStructure, exemplarDataStructure, Constants::k_CellAttributeMatrix, Constants::k_ExemplarDataContainer);

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/align_sections_misorientation.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("OrientationAnalysis::AlignSectionsMisorientationFilter: Class 1 Oracle Shift Accumulation And Shift Arrays", "[OrientationAnalysis][AlignSectionsMisorientationFilter]")
{
  UnitTest::LoadPlugins();

  // 32 x 32 x 3. Pattern offsets chosen so that every slice that serves as a REFERENCE has
  // Dy congruent to 0 (mod 4) -- reference rows must land on phase-1 staircase rows.
  //   D_2 = (0, 0)   top slice, the registration anchor
  //   D_1 = (2, 0)
  //   D_0 = (1, 2)
  AnalyticalFixtures::FixtureSpec spec;
  spec.offsets = {{1, 2}, {2, 0}, {0, 0}};
  DataStructure dataStructure = AnalyticalFixtures::BuildFixture(spec);

  AlignSectionsMisorientationFilter filter;
  Arguments args = AnalyticalFixtures::MakeArguments(5.0F, false, true);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // DERIVATION.
  // iter == 1 pairs slice 1 (moving) with slice 2 (reference); d = D_1 - D_2 = (2, 0).
  //   The pass-1 candidate window is s in [-3, 3]^2, so e = s - (2, 0) spans
  //   ex in [-5, 1], ey in [-3, 3].
  //     * ey != 0 -> the probed row is a phase-0 pin row -> every sampled pair mismatches
  //       -> score 1.0 (the maximum).
  //     * ey == 0 -> score = 2 * |ex| / count, which is 0 only at ex == 0, i.e. s = (2, 0).
  //   s = (2, 0) is inside the pass-1 window and is the UNIQUE zero, so it is accepted with
  //   min == 0; pass 2 re-centres on it, finds no other zero, and the while loop exits.
  //   => relative shift (2, 0); xShifts[1] = 0 + 2 = 2, yShifts[1] = 0 + 0 = 0.
  //
  // iter == 2 pairs slice 0 (moving) with slice 1 (reference); d = D_0 - D_1 = (-1, 2).
  //   e = s - (-1, 2) spans ex in [-2, 4], ey in [-5, 1].
  //     * ey not a multiple of 4 -> phase-0 pin row -> score 1.0.
  //     * ey == 0 -> 2 * |ex| / count, zero only at s = (-1, 2).
  //     * ey == -4 (s_y == -2) is the case the zigzag exists to kill. The probed row is
  //       4 rows up, and g(r-1) - g(r) is -1 for r = 1..4 but +1 for r = 5,6,7. With
  //       ex == -1 the upper rows cancel exactly (interval length |-1 + 1| = 0) but the
  //       lower rows give interval length |+1 + 1| = 2. For r = 5,6,7 the boundary T is
  //       19, 18, 17; a half-open interval [T, T+2) contains a sampled u (== 2 mod 4 here)
  //       iff T == 1 or 2 (mod 4), which holds for r = 6 (T = 18) and r = 7 (T = 17).
  //       => 2 mismatches, strictly greater than zero. Not a tie.
  //   s = (-1, 2) is the UNIQUE zero.
  //   => relative shift (-1, 2); xShifts[2] = 2 + (-1) = 1, yShifts[2] = 0 + 2 = 2.
  //
  // Cross-check: Cumulative[iter] must equal D_slice - D_top.
  //   iter 1 -> D_1 - D_2 = (2, 0)   OK
  //   iter 2 -> D_0 - D_2 = (1, 2)   OK
  //
  //                                  {slice, relX, relY, cumX, cumY}
  const std::vector<std::array<int64, 5>> expectedShifts = {
      {0, 0, 0, 0, 0}, // tuple 0: never written by findShifts; fill value "0"
      {1, 2, 0, 2, 0}, // iter 1: slices {1, 2}
      {0, -1, 2, 1, 2} // iter 2: slices {0, 1}
  };
  AnalyticalFixtures::CheckShiftArrays(dataStructure, expectedShifts);

  // The accumulated shift applied to each slice, indexed by z. The top slice is never
  // touched by the base transfer loop (it starts at i == 1), so its shift is (0, 0).
  const std::vector<std::array<int64, 2>> appliedShifts = {{1, 2}, {2, 0}, {0, 0}};
  AnalyticalFixtures::CheckAlignedVolume(dataStructure, spec, appliedShifts);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::AlignSectionsMisorientationFilter: Class 1 Oracle Shift Application Without Shift Arrays", "[OrientationAnalysis][AlignSectionsMisorientationFilter]")
{
  UnitTest::LoadPlugins();

  // findShifts duplicates its entire body between the store-shifts and no-store-shifts
  // branches, so a test that only ever sets Store Alignment Shifts exercises just one of the
  // two copies. This is the same fixture and the same hand-derived answer as the shift
  // accumulation test, run with the shift arrays turned OFF so the second copy of the search
  // is covered too. The shift arrays cannot be inspected here, so the aligned volume is the
  // observable: it is only correct if the accumulated shifts came out as (2, 0) and (1, 2).
  AnalyticalFixtures::FixtureSpec spec;
  spec.offsets = {{1, 2}, {2, 0}, {0, 0}};
  DataStructure dataStructure = AnalyticalFixtures::BuildFixture(spec);

  AlignSectionsMisorientationFilter filter;
  Arguments args = AnalyticalFixtures::MakeArguments(5.0F, false, false);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // No Alignment Shifts Data attribute matrix should have been created.
  REQUIRE(dataStructure.getDataAs<AttributeMatrix>(AnalyticalFixtures::k_AlignmentAMPath) == nullptr);

  const std::vector<std::array<int64, 2>> appliedShifts = {{1, 2}, {2, 0}, {0, 0}};
  AnalyticalFixtures::CheckAlignedVolume(dataStructure, spec, appliedShifts);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::AlignSectionsMisorientationFilter: Class 1 Oracle Multi Hop Convergence", "[OrientationAnalysis][AlignSectionsMisorientationFilter]")
{
  UnitTest::LoadPlugins();

  // 32 x 32 x 2 with d = (4, 0). |d| == 4 > 3, so the answer lies OUTSIDE the first 7x7
  // candidate window and the re-centring while loop at AlignSectionsMisorientation.cpp:113
  // must run more than once. dy == 0 keeps ey == sy in [-3, 3] throughout, so the phase-0
  // pin rows dominate every candidate with a nonzero y component.
  AnalyticalFixtures::FixtureSpec spec;
  spec.offsets = {{4, 0}, {0, 0}};
  DataStructure dataStructure = AnalyticalFixtures::BuildFixture(spec);

  AlignSectionsMisorientationFilter filter;
  Arguments args = AnalyticalFixtures::MakeArguments(5.0F, false, true);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // DERIVATION. Scores are 2 * |sx - 4| / count for sy == 0, and 1.0 for sy != 0.
  // count == 8 rows * #{n in {0,4,...,28} : 0 <= n + sx < 32}, i.e. 64 for sx in [0, 3] and
  // 56 otherwise. (Dropping the n == 0 or n == 28 column never removes a mismatching
  // column: the mismatch interval lives in u in [9, 23].)
  //
  // Pass 1, centred (0, 0), s in [-3, 3]^2:
  //   (-3,0) 14/56 = 0.2500   (-2,0) 12/56 = 0.2143   (-1,0) 10/56 = 0.1786
  //   ( 0,0)  8/64 = 0.1250   ( 1,0)  6/64 = 0.0938   ( 2,0)  4/64 = 0.0625
  //   ( 3,0)  2/64 = 0.03125  <-- unique minimum       any sy != 0 -> 1.0
  //   => newshift = (3, 0). newshift != oldshift, so the loop iterates.
  //
  // Pass 2, centred (3, 0). Candidates with sx in [0, 3] were memoised in pass 1, so the
  // new ones are sx in {4, 5, 6}:
  //   (4,0) 0/56 = 0  <-- unique zero   (5,0) 2/56 = 0.0357   (6,0) 4/56 = 0.0714
  //   => newshift = (4, 0), min = 0.
  //
  // Pass 3, centred (4, 0). The only new candidates are sx == 7:
  //   (7,0) 6/56 = 0.107 > 0; sy != 0 -> 1.0. Nothing beats or ties zero, so newshift is
  //   unchanged and the loop exits after 3 passes.
  //
  // => relative shift (4, 0). A single-pass search would report (3, 0) instead.
  const std::vector<std::array<int64, 5>> expectedShifts = {
      {0, 0, 0, 0, 0}, // tuple 0
      {0, 4, 0, 4, 0}  // iter 1: slices {0, 1}
  };
  AnalyticalFixtures::CheckShiftArrays(dataStructure, expectedShifts);

  const std::vector<std::array<int64, 2>> appliedShifts = {{4, 0}, {0, 0}};
  AnalyticalFixtures::CheckAlignedVolume(dataStructure, spec, appliedShifts);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::AlignSectionsMisorientationFilter: Class 1 Oracle Misorientation Tolerance Bracket", "[OrientationAnalysis][AlignSectionsMisorientationFilter]")
{
  UnitTest::LoadPlugins();

  // The A/B pair is identity vs 30 degrees about [001]. For Cubic_High the disorientation is
  // exactly 30 degrees: the smallest non-identity cubic symmetry rotation is 90 degrees, and
  // the rotation angle is bi-invariant, so no symmetry operator can bring a 30-degree
  // rotation below 90 - 30 = 60 degrees. The tolerance is a parameter in DEGREES, converted
  // to radians per slice pair at AlignSectionsMisorientation.cpp:111.
  //
  // 30 degrees is deliberately NOT tested: the comparison at :152 is a strict `>` and the
  // exact-boundary behaviour is a documented precision finding, not a specified contract.
  // Both brackets sit a full degree away from the fixture's disorientation, which is three
  // orders of magnitude more than the float/double spread of the two code paths.
  struct Bracket
  {
    std::string label;
    float32 tolerance;
    std::array<int64, 2> expectedRelativeShift;
  };

  const std::vector<Bracket> brackets = {// 30 > 29 -> A/B pairs are mismatches -> the staircase is visible -> the unique zero
                                         // is at s = d = (2, 0).
                                         {"tolerance 29 degrees - pattern visible", 29.0F, {2, 0}},
                                         // 30 > 31 is false -> A/B pairs MATCH -> the staircase becomes invisible and the only
                                         // remaining mismatch source is the phase-0 pin rows, which fire iff ey != 0. So every
                                         // sy == 0 candidate scores exactly 0 and every sy != 0 candidate scores 1.0.
                                         //
                                         // The sy == 0 row is a 7-way tie at zero, resolved by the asymmetric OR tie-break at
                                         // :176. Trace (scan order is j = -3..3 outer, k = -3..3 inner):
                                         //   j = -3 (score 1.0): (-3,-3) is accepted because min is still FLT_MAX; then
                                         //       k = -2,-1,0 each satisfy |k| < |newx| so newshift walks to (0,-3);
                                         //       k = 1,2,3 fail both clauses.
                                         //   j = -2: k = -3 fails |k| < |newx| == 0 but satisfies |j| == 2 < |newy| == 3, so it
                                         //       is accepted as (-3,-2) and then walks to (0,-2). Same shape for j = -1.
                                         //   j =  0 (score 0.0 < 1.0): k = -3 is accepted immediately, min becomes 0 and
                                         //       newshift becomes (-3,0); k = -2,-1,0 walk it to (0,0); k = 1,2,3 fail both.
                                         //   j =  1,2,3: score 1.0 is neither < nor == min == 0, so all are rejected.
                                         // The pass ends with newshift == (0,0) == oldshift, so the loop exits.
                                         {"tolerance 31 degrees - pattern invisible", 31.0F, {0, 0}}};

  for(const Bracket& bracket : brackets)
  {
    DYNAMIC_SECTION(bracket.label)
    {
      AnalyticalFixtures::FixtureSpec spec;
      spec.offsets = {{2, 0}, {0, 0}};
      DataStructure dataStructure = AnalyticalFixtures::BuildFixture(spec);

      AlignSectionsMisorientationFilter filter;
      Arguments args = AnalyticalFixtures::MakeArguments(bracket.tolerance, false, true);

      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
      auto executeResult = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

      const std::vector<std::array<int64, 5>> expectedShifts = {
          {0, 0, 0, 0, 0}, {0, bracket.expectedRelativeShift[0], bracket.expectedRelativeShift[1], bracket.expectedRelativeShift[0], bracket.expectedRelativeShift[1]}};
      AnalyticalFixtures::CheckShiftArrays(dataStructure, expectedShifts);

      UnitTest::CheckArraysInheritTupleDims(dataStructure);
    }
  }
}

TEST_CASE("OrientationAnalysis::AlignSectionsMisorientationFilter: Class 1 Oracle Mask Semantics", "[OrientationAnalysis][AlignSectionsMisorientationFilter]")
{
  UnitTest::LoadPlugins();

  SECTION("structured mask drives the shift")
  {
    // Every cell is phase 1 with the identity quaternion, so a both-true pair always has a
    // disorientation of 0 degrees and can never add to the numerator. The mask carries the
    // structure: mask(u, v) = (v == 0 mod 4) && (u < T(v)).
    AnalyticalFixtures::FixtureSpec spec;
    spec.offsets = {{2, 0}, {0, 0}};
    spec.carrier = AnalyticalFixtures::Carrier::Mask;
    DataStructure dataStructure = AnalyticalFixtures::BuildFixture(spec);

    AlignSectionsMisorientationFilter filter;
    Arguments args = AnalyticalFixtures::MakeArguments(5.0F, true, true);

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    // DERIVATION. Reference rows have v == 0 (mod 4), so mask_ref = [u < T].
    //   * ey not a multiple of 4: the probed row is a pin row, so mask_cur is false
    //     everywhere. Pairs whose reference is masked-in are exclusive-or pairs and add 1
    //     each (:159-162); pairs whose reference is masked-out are both-false and add
    //     NOTHING to the numerator while still being counted in the denominator (:134).
    //     With T in [16, 20] that is 4 or 5 sampled columns per row, i.e. a score near
    //     34/64 -- far above the true answer.
    //   * ey == 0: the exclusive-or fires iff [u < T] != [u + ex < T], which is the same
    //     interval count as the orientation carrier -> 2 * |ex|. Both-true pairs compare
    //     identity against identity -> 0 degrees -> not > tolerance -> contribute nothing.
    //     So the score is 2 * |ex| / count, zero only at ex == 0.
    // => the unique zero is s = d = (2, 0).
    const std::vector<std::array<int64, 5>> expectedShifts = {{0, 0, 0, 0, 0}, {0, 2, 0, 2, 0}};
    AnalyticalFixtures::CheckShiftArrays(dataStructure, expectedShifts);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("all false mask yields a zero shift and no NaN")
  {
    // Every pair is both-false, so the numerator stays 0 while `count` still increments for
    // every in-bounds sampled pair (:134 precedes all mask logic). The quotient is therefore
    // 0 / count == 0, NOT 0 / 0 == NaN. All 49 candidates tie at 0 and the asymmetric OR
    // tie-break walks to (0, 0) exactly as traced in the tolerance-bracket test.
    AnalyticalFixtures::FixtureSpec spec;
    spec.offsets = {{2, 0}, {0, 0}};
    spec.carrier = AnalyticalFixtures::Carrier::MaskAllFalse;
    DataStructure dataStructure = AnalyticalFixtures::BuildFixture(spec);

    AlignSectionsMisorientationFilter filter;
    Arguments args = AnalyticalFixtures::MakeArguments(5.0F, true, true);

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    const std::vector<std::array<int64, 5>> expectedShifts = {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}};
    AnalyticalFixtures::CheckShiftArrays(dataStructure, expectedShifts);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("OrientationAnalysis::AlignSectionsMisorientationFilter: Class 1 Oracle Multi Phase And Cross Laue Class", "[OrientationAnalysis][AlignSectionsMisorientationFilter]")
{
  UnitTest::LoadPlugins();

  // Ensemble: tuple 0 unused (999), phase 1 = Cubic_High, phase 2 = Hexagonal_High,
  // phase 3 = Cubic_High again. A co-moving stripe (pattern columns u in [4, 8)) carries the
  // decorated phase.
  //
  // The dispatch guard at :146 is `laueClass1 == laueClass2 && laueClass1 < ops.size()`,
  // and the classes compared are CRYSTAL STRUCTURE values, not phase indices.
  SECTION("cross Laue class stripe does not perturb the argmin")
  {
    AnalyticalFixtures::FixtureSpec spec;
    spec.offsets = {{2, 0}, {0, 0}};
    spec.crystalStructures = {AnalyticalFixtures::k_UnknownStructure, AnalyticalFixtures::k_CubicHigh, AnalyticalFixtures::k_HexagonalHigh};
    spec.stripePhase = 2;
    DataStructure dataStructure = AnalyticalFixtures::BuildFixture(spec);

    AlignSectionsMisorientationFilter filter;
    Arguments args = AnalyticalFixtures::MakeArguments(5.0F, false, true);

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    // DERIVATION. Because the phase-2 stripe CO-MOVES with the pattern, at e == 0 every
    // sampled pair is phase-matched (Cubic vs Cubic or Hex vs Hex) and orientation-matched,
    // so the score is still exactly 0. Any ex != 0 misaligns the stripe and creates
    // cross-Laue pairs, which fail the :146 guard, keep angle == FLT_MAX and count as
    // mismatches on top of the staircase's 2 * |ex|. So the unique zero is unchanged.
    const std::vector<std::array<int64, 5>> expectedShifts = {{0, 0, 0, 0, 0}, {0, 2, 0, 2, 0}};
    AnalyticalFixtures::CheckShiftArrays(dataStructure, expectedShifts);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("same Laue class under a different phase index still compares")
  {
    AnalyticalFixtures::FixtureSpec spec;
    spec.offsets = {{2, 0}, {0, 0}};
    spec.crystalStructures = {AnalyticalFixtures::k_UnknownStructure, AnalyticalFixtures::k_CubicHigh, AnalyticalFixtures::k_HexagonalHigh, AnalyticalFixtures::k_CubicHigh};
    spec.stripePhase = 3;
    DataStructure dataStructure = AnalyticalFixtures::BuildFixture(spec);

    AlignSectionsMisorientationFilter filter;
    Arguments args = AnalyticalFixtures::MakeArguments(5.0F, false, true);

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    // Phase 1 and phase 3 both map to Cubic_High, so :146 lets the comparison through and
    // the identical quaternions match. Dispatch is by crystal structure, not phase index:
    // if it keyed on the phase index the stripe would read as a permanent mismatch and the
    // score at e == 0 would no longer be zero.
    const std::vector<std::array<int64, 5>> expectedShifts = {{0, 0, 0, 0, 0}, {0, 2, 0, 2, 0}};
    AnalyticalFixtures::CheckShiftArrays(dataStructure, expectedShifts);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("OrientationAnalysis::AlignSectionsMisorientationFilter: Class 1 Oracle Hexagonal Laue Class Path", "[OrientationAnalysis][AlignSectionsMisorientationFilter]")
{
  UnitTest::LoadPlugins();

  // Hexagonal_High routes through the GENERIC LaueOps::calculateMisorientationInternal,
  // which still finishes with 2 * acos(w) -- the code path the CubicOps atan2 precision fix
  // does NOT touch. B is 20 degrees about [0001]; the smallest non-identity hexagonal
  // symmetry rotation is 60 degrees, so by bi-invariance no operator can reduce a
  // 20-degree rotation below 60 - 20 = 40 degrees, and the disorientation is exactly
  // 20 degrees -- 15 degrees clear of the 5-degree tolerance.
  AnalyticalFixtures::FixtureSpec spec;
  spec.offsets = {{2, 0}, {0, 0}};
  spec.patternDegrees = 20.0;
  spec.crystalStructures = {AnalyticalFixtures::k_UnknownStructure, AnalyticalFixtures::k_HexagonalHigh};
  DataStructure dataStructure = AnalyticalFixtures::BuildFixture(spec);

  AlignSectionsMisorientationFilter filter;
  Arguments args = AnalyticalFixtures::MakeArguments(5.0F, false, true);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const std::vector<std::array<int64, 5>> expectedShifts = {{0, 0, 0, 0, 0}, {0, 2, 0, 2, 0}};
  AnalyticalFixtures::CheckShiftArrays(dataStructure, expectedShifts);

  const std::vector<std::array<int64, 2>> appliedShifts = {{2, 0}, {0, 0}};
  AnalyticalFixtures::CheckAlignedVolume(dataStructure, spec, appliedShifts);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::AlignSectionsMisorientationFilter: Preflight Guards", "[OrientationAnalysis][AlignSectionsMisorientationFilter][preflight]")
{
  UnitTest::LoadPlugins();

  SECTION("single slice geometry is rejected")
  {
    // dims[2] == 1 leaves the findShifts loop body unentered, so the filter would be a
    // silent no-op. DREAM3D 6.5.171 rejected non-3D geometries in
    // AlignSections::dataCheck with -3010; the port had no equivalent guard.
    AnalyticalFixtures::FixtureSpec spec;
    spec.offsets = {{0, 0}};
    DataStructure dataStructure = AnalyticalFixtures::BuildFixture(spec);

    AlignSectionsMisorientationFilter filter;
    Arguments args = AnalyticalFixtures::MakeArguments(5.0F, false, false);

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
    REQUIRE(preflightResult.outputActions.errors().size() == 1);
    REQUIRE(preflightResult.outputActions.errors()[0].code == -68005);
  }

  SECTION("degenerate X dimension is rejected")
  {
    // dims[0] == 1 makes halfDim0 == int64(1 * 0.5f) == 0, so xIdx = k + oldxshift + 0 is
    // negative for every candidate with k < 0 and `misorients[idx]` at :126 is a
    // negative-index read that happens BEFORE the bounds test short-circuits.
    DataStructure dataStructure;
    auto* imageGeom = ImageGeom::Create(dataStructure, AnalyticalFixtures::k_ImageGeomName);
    imageGeom->setDimensions({1, 32, 3});
    const ShapeType cellTupleShape = {3, 32, 1};
    auto* cellAM = AttributeMatrix::Create(dataStructure, AnalyticalFixtures::k_CellDataName, cellTupleShape, imageGeom->getId());
    imageGeom->setCellData(*cellAM);
    UnitTest::CreateTestDataArray<float32>(dataStructure, "Quats", cellTupleShape, {4}, cellAM->getId());
    UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", cellTupleShape, {1}, cellAM->getId());
    UnitTest::CreateTestDataArray<bool>(dataStructure, "Mask", cellTupleShape, {1}, cellAM->getId());
    auto* ensembleAM = AttributeMatrix::Create(dataStructure, AnalyticalFixtures::k_EnsembleDataName, ShapeType{2}, imageGeom->getId());
    UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", ShapeType{2}, {1}, ensembleAM->getId());

    AlignSectionsMisorientationFilter filter;
    Arguments args = AnalyticalFixtures::MakeArguments(5.0F, false, false);

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
    REQUIRE(preflightResult.outputActions.errors().size() == 1);
    REQUIRE(preflightResult.outputActions.errors()[0].code == -68005);
  }

  SECTION("cell array tuple count that disagrees with the geometry is rejected")
  {
    // The three cell arrays are validated against EACH OTHER only. Selecting a consistent
    // set that belongs to a different, smaller geometry passed preflight and then indexed
    // out of bounds inside findShifts, because every position there is derived from the
    // SELECTED geometry's dimensions (:135-136).
    DataStructure dataStructure;
    auto* imageGeom = ImageGeom::Create(dataStructure, AnalyticalFixtures::k_ImageGeomName);
    imageGeom->setDimensions({32, 32, 3});
    // Cell AM deliberately sized for a 2-slice volume while the geometry claims 3 slices.
    const ShapeType shortTupleShape = {2, 32, 32};
    auto* cellAM = AttributeMatrix::Create(dataStructure, AnalyticalFixtures::k_CellDataName, shortTupleShape, imageGeom->getId());
    imageGeom->setCellData(*cellAM);
    UnitTest::CreateTestDataArray<float32>(dataStructure, "Quats", shortTupleShape, {4}, cellAM->getId());
    UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", shortTupleShape, {1}, cellAM->getId());
    UnitTest::CreateTestDataArray<bool>(dataStructure, "Mask", shortTupleShape, {1}, cellAM->getId());
    auto* ensembleAM = AttributeMatrix::Create(dataStructure, AnalyticalFixtures::k_EnsembleDataName, ShapeType{2}, imageGeom->getId());
    UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", ShapeType{2}, {1}, ensembleAM->getId());

    AlignSectionsMisorientationFilter filter;
    Arguments args = AnalyticalFixtures::MakeArguments(5.0F, false, false);

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
    REQUIRE(preflightResult.outputActions.errors().size() == 1);
    REQUIRE(preflightResult.outputActions.errors()[0].code == -68006);
  }

  SECTION("negative misorientation tolerance is rejected")
  {
    // A negative tolerance makes `angle > tolerance` true for every pair, including a pair
    // of identical orientations (0 > negative), so every candidate scores 1.0 and the
    // result is silent garbage.
    AnalyticalFixtures::FixtureSpec spec;
    spec.offsets = {{2, 0}, {0, 0}};
    DataStructure dataStructure = AnalyticalFixtures::BuildFixture(spec);

    AlignSectionsMisorientationFilter filter;
    Arguments args = AnalyticalFixtures::MakeArguments(-1.0F, false, false);

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
    REQUIRE(preflightResult.outputActions.errors().size() == 1);
    REQUIRE(preflightResult.outputActions.errors()[0].code == -68007);
  }
}

TEST_CASE("OrientationAnalysis::AlignSectionsMisorientationFilter: Execute Guards", "[OrientationAnalysis][AlignSectionsMisorientationFilter]")
{
  UnitTest::LoadPlugins();

  SECTION("phase value beyond the ensemble array is rejected")
  {
    // `crystalStructures[cellPhases[pos]]` (:143, :145) is indexed with only a `> 0` check
    // on the phase, so a phase value at or above the ensemble tuple count reads out of
    // bounds. Not preventable at preflight -- the phase values are data.
    AnalyticalFixtures::FixtureSpec spec;
    spec.offsets = {{2, 0}, {0, 0}};
    spec.crystalStructures = {AnalyticalFixtures::k_UnknownStructure, AnalyticalFixtures::k_CubicHigh};
    spec.stripePhase = 5; // 5 >= 2 ensemble tuples
    DataStructure dataStructure = AnalyticalFixtures::BuildFixture(spec);

    AlignSectionsMisorientationFilter filter;
    Arguments args = AnalyticalFixtures::MakeArguments(5.0F, false, false);

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
    REQUIRE(executeResult.result.errors().size() == 1);
    REQUIRE(executeResult.result.errors()[0].code == -68008);
  }

  SECTION("indexed phase with an unknown crystal structure warns")
  {
    // A crystal-structure value of 999 (UnknownCrystalStructure) fails
    // `laueClass1 < ops.size()` at :146, so those voxels are silently treated as permanent
    // mismatches. The run must succeed and say so.
    AnalyticalFixtures::FixtureSpec spec;
    spec.offsets = {{2, 0}, {0, 0}};
    spec.crystalStructures = {AnalyticalFixtures::k_UnknownStructure, AnalyticalFixtures::k_CubicHigh, AnalyticalFixtures::k_UnknownStructure};
    spec.stripePhase = 2; // phase 2 is an INDEXED phase whose structure is unknown
    DataStructure dataStructure = AnalyticalFixtures::BuildFixture(spec);

    AlignSectionsMisorientationFilter filter;
    Arguments args = AnalyticalFixtures::MakeArguments(5.0F, false, false);

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    REQUIRE(executeResult.result.warnings().size() == 1);
    REQUIRE(executeResult.result.warnings()[0].code == -68009);
  }
}

TEST_CASE("OrientationAnalysis::AlignSectionsMisorientationFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][AlignSectionsMisorientationFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "AlignSectionsMisorientationFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "AlignSectionsMisorientationFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<AlignSectionsMisorientationFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<bool>(AlignSectionsMisorientationFilter::k_StoreAlignmentShifts_Key) == true);
      CHECK(args.value<float32>(AlignSectionsMisorientationFilter::k_MisorientationTolerance_Key) == 2.5f);
      CHECK(args.value<bool>(AlignSectionsMisorientationFilter::k_UseMask_Key) == true);
      CHECK(args.value<DataPath>(AlignSectionsMisorientationFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(AlignSectionsMisorientationFilter::k_QuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(AlignSectionsMisorientationFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(AlignSectionsMisorientationFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(AlignSectionsMisorientationFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}
