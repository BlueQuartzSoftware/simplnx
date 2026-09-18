#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/DoubleThresholdImageFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
// Legacy ITKDoubleThresholdImageFilter, created at runtime by UUID (so this target does not link ITKImageProcessing).
// It is ENABLED in the ITKImageProcessing build, so it is the live-ITK reference for the two-threshold +
// reconstruction-by-dilation façade's bit-exact output-VALUE parity gate.
const Uuid k_LegacyDTUuid = *Uuid::FromString("e268a65f-33f1-493f-a6c7-4635e57df3c4");

using DTFilter = DoubleThresholdImageFilter;

// A single (T1,T2,T3,T4) arrangement to parity-test.
struct ThreshCase
{
  const char* label;
  float64 t1;
  float64 t2;
  float64 t3;
  float64 t4;
};

// Threshold arrangements: two NESTED tight/wide bands where reconstruction actually grows the marker (the narrow
// band seeds only part of a wide-band component, so the marker dilates into the connected remainder while a
// DISCONNECTED wide-band component stays outside -- this is what distinguishes a correct marker-under-mask
// reconstruction from a plain threshold, and what a swapped marker/mask would get wrong), plus a degenerate
// all-equal case (narrow == wide == {60}). See MakeDoubleThresholdField for the gray levels each band selects.
const std::array<ThreshCase, 3> k_ThreshCases = {{
    {"grow-from-narrow-seed", 10.0, 40.0, 80.0, 120.0}, // narrow [40,80] -> {60}; wide [10,120] -> {60,100}
    {"grow-from-mask-only", 10.0, 90.0, 115.0, 120.0},  // narrow [90,115] -> {100}; wide [10,120] -> {60,100}
    {"degenerate-all-equal", 60.0, 60.0, 60.0, 60.0},   // narrow == wide == {60}
}};

// A single (InsideValue, OutsideValue) label pairing.
struct InOut
{
  uint8 inside;
  uint8 outside;
};

// Inside/Outside label pairings: the default {1,0} and a both-nonzero {200,5} (both inside > outside, so
// reconstruction-by-dilation grows the "inside" from the marker seeds). NOTE: inside < outside is deliberately NOT
// tested -- it makes the marker exceed the mask over the wide-band-only region, which live ITK's
// ReconstructionByDilationImageFilter REJECTS with a "marker must be <= mask" precondition throw (no comparable
// output to byte-match), so it is outside the parity contract.
const std::array<InOut, 2> k_InOutCases = {{{1, 0}, {200, 5}}};

// Sets the standard geom/input/output keys + the 7 double-threshold params on a shared Arguments, runs preflight +
// execute, and requires both succeed. Reuses DTFilter::k_*_Key for BOTH the new and the legacy ITK filter --
// correct only because the new filter deliberately reuses the legacy key strings ("input_image_geometry_path",
// "input_image_data_path", "output_array_name", "threshold1".."threshold4", "inside_value", "outside_value",
// "fully_connected").
void RunDT(IFilter& filter, DataStructure& ds, const DataPath& inputPath, const ThreshCase& tc, uint8 insideValue, uint8 outsideValue, bool fullyConnected, const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(DTFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(DTFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(DTFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(DTFilter::k_Threshold1_Key, std::make_any<float64>(tc.t1));
  args.insertOrAssign(DTFilter::k_Threshold2_Key, std::make_any<float64>(tc.t2));
  args.insertOrAssign(DTFilter::k_Threshold3_Key, std::make_any<float64>(tc.t3));
  args.insertOrAssign(DTFilter::k_Threshold4_Key, std::make_any<float64>(tc.t4));
  args.insertOrAssign(DTFilter::k_InsideValue_Key, std::make_any<uint8>(insideValue));
  args.insertOrAssign(DTFilter::k_OutsideValue_Key, std::make_any<uint8>(outsideValue));
  args.insertOrAssign(DTFilter::k_FullyConnected_Key, std::make_any<bool>(fullyConnected));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

// A deterministic MULTI-LEVEL field with several gray plateaus. All gray levels stay in [0, 125] so the pattern is
// representable in EVERY scalar type incl. int8 (max 127) and uint8, and is exact in float32/float64. Regions
// (fractional positions so the same builder serves the 12^3 parity dims and the 200^3 OOC volume):
//   - "seed"     (value 60): a narrow-band plateau -> the reconstruction marker for band [40,80].
//   - "bridge"   (value 100): a wide-band-only plateau FACE-ADJACENT to the seed -> reconstruction grows the marker
//                             into it (they share a column at every scale so they are always connected).
//   - "isolated" (value 100): a wide-band-only plateau FAR from the seed/bridge -> in the wide band but NOT reachable
//                             from any narrow-band seed, so it stays OUTSIDE (the marker/mask-order bug-catch).
//   - "low"      (value 5): below the wide band -> background/outside.
//   - "high"     (value 125): above the wide band -> background/outside.
//   - everything else: 0 (outside).
template <class T>
std::vector<T> MakeDoubleThresholdField(usize dimX, usize dimY, usize dimZ)
{
  std::vector<T> v(dimX * dimY * dimZ, T{0});
  const int64 nX = static_cast<int64>(dimX);
  const int64 nY = static_cast<int64>(dimY);
  const int64 nZ = static_cast<int64>(dimZ);

  auto frac = [](double f, int64 n) { return static_cast<int64>(f * static_cast<double>(n)); };

  // Paint the inclusive, clamped fractional box [fx0,fx1] x [fy0,fy1] (all z) with value.
  auto paintBox = [&](double fx0, double fx1, double fy0, double fy1, T value) {
    const int64 x0 = std::max<int64>(0, frac(fx0, nX));
    const int64 x1 = std::min<int64>(nX - 1, frac(fx1, nX));
    const int64 y0 = std::max<int64>(0, frac(fy0, nY));
    const int64 y1 = std::min<int64>(nY - 1, frac(fy1, nY));
    for(int64 z = 0; z < nZ; ++z)
    {
      for(int64 y = y0; y <= y1; ++y)
      {
        for(int64 x = x0; x <= x1; ++x)
        {
          v[rt::FlatIndex(static_cast<usize>(x), static_cast<usize>(y), static_cast<usize>(z), dimX, dimY)] = value;
        }
      }
    }
  };

  paintBox(0.10, 0.30, 0.10, 0.30, static_cast<T>(60));  // seed (narrow band)
  paintBox(0.30, 0.50, 0.10, 0.30, static_cast<T>(100)); // bridge (wide only, face-adjacent to seed at the shared column)
  paintBox(0.65, 0.85, 0.65, 0.85, static_cast<T>(100)); // isolated (wide only, disconnected from the seed)
  paintBox(0.65, 0.85, 0.10, 0.30, static_cast<T>(5));   // low (below wide band)
  paintBox(0.10, 0.30, 0.65, 0.85, static_cast<T>(125)); // high (above wide band)
  return v;
}

// Run new + legacy on the same field/params and require the uint8 outputs match EXACTLY (byte-identical label
// VALUES -- the live-ITK gate on the double-threshold façade). Also asserts the output is NON-VACUOUS (has both an
// inside-valued and an outside-valued voxel) so an all-inside or all-outside parity pass cannot masquerade as
// success.
template <class T>
void RequireParity(const std::vector<T>& field, usize dx, usize dy, usize dz, const ThreshCase& tc, uint8 insideValue, uint8 outsideValue, bool fullyConnected)
{
  DataStructure newDs;
  const DataPath newInput = rt::BuildImageFromPattern<T>(newDs, dx, dy, dz, field);
  DTFilter newFilter;
  RunDT(newFilter, newDs, newInput, tc, insideValue, outsideValue, fullyConnected);

  IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyDTUuid);
  REQUIRE(legacyFilter != nullptr);
  DataStructure legacyDs;
  const DataPath legacyInput = rt::BuildImageFromPattern<T>(legacyDs, dx, dy, dz, field);
  RunDT(*legacyFilter, legacyDs, legacyInput, tc, insideValue, outsideValue, fullyConnected);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& newOut = newDs.getDataRefAs<IDataArray>(outputPath);
  const auto& legacyOut = legacyDs.getDataRefAs<IDataArray>(outputPath);
  REQUIRE(newOut.getDataType() == DataType::uint8);
  REQUIRE(newOut.getDataType() == legacyOut.getDataType());

  // Non-vacuous guard: the crafted field + thresholds must produce a mixed output (some inside AND some outside).
  const auto& store = newOut.getIDataStoreRefAs<AbstractDataStore<uint8>>();
  usize insideCount = 0;
  usize outsideCount = 0;
  for(usize i = 0; i < store.getSize(); ++i)
  {
    const uint8 value = store.getValue(i);
    insideCount += (value == insideValue) ? 1 : 0;
    outsideCount += (value == outsideValue) ? 1 : 0;
  }
  REQUIRE(insideCount > 0);
  REQUIRE(outsideCount > 0);

  UnitTest::CompareDataArrays<uint8>(newOut, legacyOut); // EXACT -- label values must byte-match live ITK
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Live-ITK EXACT parity, FULL scalar type grid x {threshold arrangement} x {FullyConnected on/off} x
//     {(inside,outside) settings} x {3D, 2D}. Double thresholding is a fixed uint8 output; the new filter must
//     reproduce legacy ITK's label VALUES EXACTLY (no tolerance -- see the Parity model). This is the primary gate
//     on the two-threshold + reconstruction-by-dilation façade (threshold cast-to-T, inclusive band test, marker
//     under mask reconstruction, FullyConnected connectivity).
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::DoubleThresholdImageFilter: Live-ITK parity", "[ImageProcessing][DoubleThresholdImageFilter]", uint8, int8, uint16, int16, uint32, int32, uint64, int64, float32,
                   float64)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyDTUuid) != nullptr);

  const ThreshCase tc = GENERATE(from_range(k_ThreshCases));
  const bool fullyConnected = GENERATE(false, true);
  const InOut io = GENERATE(from_range(k_InOutCases));
  const uint8 insideValue = io.inside;
  const uint8 outsideValue = io.outside;
  CAPTURE(tc.label, fullyConnected, insideValue, outsideValue);

  SECTION("3D")
  {
    constexpr usize DX = 12, DY = 12, DZ = 4;
    RequireParity<T>(MakeDoubleThresholdField<T>(DX, DY, DZ), DX, DY, DZ, tc, insideValue, outsideValue, fullyConnected);
  }
  SECTION("2D (Z==1)")
  {
    constexpr usize DX = 12, DY = 12, DZ = 1;
    RequireParity<T>(MakeDoubleThresholdField<T>(DX, DY, DZ), DX, DY, DZ, tc, insideValue, outsideValue, fullyConnected);
  }
}

// -----------------------------------------------------------------------------
// (1b) OUT-OF-RANGE threshold cast lock: a narrow integer input (int8) with a Float64 threshold that exceeds its
//      range but still narrows to a VALID band (lower <= upper). The façade narrows each Float64 threshold to the
//      input pixel type with a plain static_cast (matching ITK's static_cast<InputPixelType>); that narrowing is
//      implementation-defined when out of range, so this pins it against live ITK -- a saturating cast (which we
//      deliberately do NOT use) would select a different set of voxels and break byte-exact parity. Passes EXACT for
//      the same reason the whole suite does: our façade and legacy ITK perform the IDENTICAL cast on the same
//      compiler. (Thresholds that narrow to an INVERTED band -- e.g. the default 254/255 -> -2/-1 on int8 -- are NOT
//      tested: they make ITK's BinaryThresholdImageFilter throw "lower > upper", so there is no output to byte-match.)
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::DoubleThresholdImageFilter: out-of-range threshold cast lock (int8)", "[ImageProcessing][DoubleThresholdImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyDTUuid) != nullptr);

  constexpr usize DX = 12, DY = 12, DZ = 4;
  const std::vector<int8> field = MakeDoubleThresholdField<int8>(DX, DY, DZ); // gray levels 0/5/60/100/125 all fit int8

  // static_cast<int8>(356) == 100, so the wide band is [0,100] and the value-125 plateau falls OUTSIDE. A saturating
  // cast would clamp 356 to 127, INCLUDING that plateau -> the plateau's label is the exact byte that distinguishes
  // the casts, and it must match ITK. Produces a mixed output (125 plateau outside, rest inside).
  const ThreshCase tc{"int8-wrap-356", 0.0, 40.0, 80.0, 356.0};
  RequireParity<int8>(field, DX, DY, DZ, tc, /*inside=*/1, /*outside=*/0, /*fullyConnected=*/false);
}

// -----------------------------------------------------------------------------
// (2) Preflight guards: a multi-component input is rejected (requireScalar); a valid scalar input (of any scalar
//     type, incl. unsigned and float -- AllNumeric) produces a FIXED uint8 output array (AlwaysUInt8).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::DoubleThresholdImageFilter: preflight guards", "[ImageProcessing][DoubleThresholdImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  SECTION("multi-component rejected")
  {
    DataStructure ds;
    const DataPath scalarPath = rt::BuildImageFromPattern<int32>(ds, 6, 6, 1, std::vector<int32>(36, 0));
    const DataPath vecPath({"Image Geometry", "CellData", "Vec"});
    auto vecStore = DataStoreUtilities::CreateDataStore<int32>(ds, vecPath, {1, 6, 6}, {3});
    const auto& cellAM = ds.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
    DataArray<int32>::Create(ds, "Vec", vecStore, cellAM.getId());
    const DataPath geomPath = vecPath.getParent().getParent();
    DTFilter filter;
    Arguments args;
    args.insertOrAssign(DTFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(DTFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(vecPath));
    args.insertOrAssign(DTFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(DTFilter::k_Threshold1_Key, std::make_any<float64>(0.0));
    args.insertOrAssign(DTFilter::k_Threshold2_Key, std::make_any<float64>(1.0));
    args.insertOrAssign(DTFilter::k_Threshold3_Key, std::make_any<float64>(254.0));
    args.insertOrAssign(DTFilter::k_Threshold4_Key, std::make_any<float64>(255.0));
    args.insertOrAssign(DTFilter::k_InsideValue_Key, std::make_any<uint8>(uint8{1}));
    args.insertOrAssign(DTFilter::k_OutsideValue_Key, std::make_any<uint8>(uint8{0}));
    args.insertOrAssign(DTFilter::k_FullyConnected_Key, std::make_any<bool>(false));
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
  }
  SECTION("scalar float input creates a uint8 output")
  {
    DataStructure ds;
    const std::vector<float32> field = MakeDoubleThresholdField<float32>(12, 12, 1);
    const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, 12, 12, 1, field);
    DTFilter filter;
    RunDT(filter, ds, inputPath, k_ThreshCases[0], /*inside=*/1, /*outside=*/0, /*fullyConnected=*/false);
    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    REQUIRE(ds.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::uint8);
  }
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden -- duplicates ITKDoubleThresholdImageTest.cpp on OUR ITK-free filter. Output is a
// FIXED uint8 image (AlwaysUInt8); (A) DURABLE golden = md5-validity-first (plan Sec.4), (B) LIVE-ITK parity =
// BIT-EXACT (uint8, CompareImages@0.0). Helper pins ForceInCore. (These ITK real-image cases were NOT covered by the
// earlier Task-4 threshold batch -- this file had no prior ItkGolden coverage.)
//   DoubleThreshold1: RA-Short.nrrd, filter defaults (T1..T4 = 0/1/254/255, Inside 1, Outside 0, FullyConnected false).
//   DoubleThreshold2: RA-Slice-Short.png, thresholds 0/0/3000/2700 (passed verbatim -- non-monotone by design).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::DoubleThresholdImageFilter: ITK real-image golden (DoubleThreshold1)", "[ImageProcessing][ItkGolden][DoubleThresholdImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<DoubleThresholdImageFilter>("RA-Short.nrrd", "dbd0ea7d6f16bb93e9c688cb0f1bfd85");
}

TEST_CASE("ImageProcessing::DoubleThresholdImageFilter: ITK real-image golden (DoubleThreshold2)", "[ImageProcessing][ItkGolden][DoubleThresholdImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<DoubleThresholdImageFilter>("RA-Slice-Short.png", "2c8fc2345ccfa980ef42aef5910efaa3", [](Arguments& args) {
    args.insertOrAssign(DoubleThresholdImageFilter::k_Threshold1_Key, std::make_any<float64>(0.0));
    args.insertOrAssign(DoubleThresholdImageFilter::k_Threshold2_Key, std::make_any<float64>(0.0));
    args.insertOrAssign(DoubleThresholdImageFilter::k_Threshold3_Key, std::make_any<float64>(3000.0));
    args.insertOrAssign(DoubleThresholdImageFilter::k_Threshold4_Key, std::make_any<float64>(2700.0));
  });
}
