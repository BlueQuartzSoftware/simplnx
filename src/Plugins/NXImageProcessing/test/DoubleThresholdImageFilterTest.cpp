#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/DoubleThresholdImageFilter.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <string>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
using DTFilter = DoubleThresholdImageFilter;

// A single (T1,T2,T3,T4) threshold arrangement.
struct ThreshCase
{
  const char* label;
  float64 t1;
  float64 t2;
  float64 t3;
  float64 t4;
};

// Threshold arrangements: two nested tight/wide bands where the reconstruction grows the marker. The narrow band
// seeds only part of a wide-band component, so the marker dilates into the connected remainder. A disconnected
// wide-band component stays outside. The third case is degenerate: the narrow band equals the wide band ({60}).
// See MakeDoubleThresholdField for the gray levels that each band selects.
const std::array<ThreshCase, 3> k_ThreshCases = {{
    {"grow-from-narrow-seed", 10.0, 40.0, 80.0, 120.0}, // narrow [40,80] -> {60}; wide [10,120] -> {60,100}
    {"grow-from-mask-only", 10.0, 90.0, 115.0, 120.0},  // narrow [90,115] -> {100}; wide [10,120] -> {60,100}
    {"degenerate-all-equal", 60.0, 60.0, 60.0, 60.0},   // narrow == wide == {60}
}};

// Sets the geometry, input, and output keys plus the seven double-threshold parameters on the Arguments. The helper
// runs preflight and execute, and requires that both steps succeed.
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
} // namespace

// -----------------------------------------------------------------------------
// (1) Preflight guards: a multi-component input is rejected (requireScalar); a valid scalar input (of any scalar
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
// ITK-sourced real-image golden -- duplicates ITKDoubleThresholdImageTest.cpp on our ITK-free filter. The output is a
// fixed uint8 image (AlwaysUInt8). The durable golden is an md5 pin. The helper pins ForceInCore.
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
