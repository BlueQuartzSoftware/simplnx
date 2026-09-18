#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/GrayscaleGrindPeakImageFilter.hpp"

#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <array>
#include <limits>
#include <memory>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
// Legacy ITKGrayscaleGrindPeakImageFilter, created at runtime by UUID (so this target does not link ITKImageProcessing).
const Uuid k_LegacyGrayscaleGrindPeakUuid = *Uuid::FromString("6aa5b193-c290-4fe3-a409-7759a62d48ea");

// Dark field / bright peak values used by the peak-bearing builder (exactly representable in uint8/int16/float32).
constexpr float64 k_Field = 40.0;
constexpr float64 k_Peak = 220.0;

// Live-parity configs: 3D + genuinely-2D (Z=1) peak-bearing images, each with FullyConnected off and on.
const std::vector<rt::ReconCase> k_ParityConfigs = {
    {"3D 12x12x12 fc=false", false, 0.0, 12, 12, 12}, {"3D 12x12x12 fc=true", true, 0.0, 12, 12, 12}, {"2D 20x16x1 fc=false", false, 0.0, 20, 16, 1}, {"2D 20x16x1 fc=true", true, 0.0, 20, 16, 1}};
} // namespace

// -----------------------------------------------------------------------------
// (1) Live-ITK EXACT parity grid across uint8 / int16 / float32, FullyConnected {off, on}, on a peak-bearing
//     image (interior bright blobs that get ground down + a border-touching bright region that does not). GrindPeak
//     is a reconstruction-by-dilation that only copies/clamps existing input values, so the new filter must
//     reproduce the legacy ITK Grayscale Grind Peak output EXACTLY -- for integer AND float types (no tolerance).
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::GrayscaleGrindPeakImageFilter: Live-ITK exact parity grid", "[ImageProcessing][GrayscaleGrindPeakImageFilter]", uint8, int16, float32)
{
  using T = TestType;
  const auto build = [](DataStructure& ds, usize dx, usize dy, usize dz) { return rt::BuildPeakImage<T>(ds, dx, dy, dz, static_cast<T>(k_Field), static_cast<T>(k_Peak)); };
  rt::RunReconstructionParityGrid<GrayscaleGrindPeakImageFilter, T>(k_LegacyGrayscaleGrindPeakUuid, build, k_ParityConfigs, rt::FullyConnectedParamSetter<GrayscaleGrindPeakImageFilter>());
}

// -----------------------------------------------------------------------------
// (2) Hand-computed behavior on a small 2D image: an isolated interior bright peak is ground down to the
//     surrounding dark level, while a bright region that touches the image border (including its non-border voxel,
//     which is connected to the border through the bright region) is left unchanged.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GrayscaleGrindPeakImageFilter: hand-computed interior grind vs border region", "[ImageProcessing][GrayscaleGrindPeakImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 7;
  constexpr usize DY = 7;
  constexpr usize DZ = 1;
  constexpr int32 field = 50;
  constexpr int32 peak = 90;

  std::vector<int32> pattern(DX * DY * DZ, field);
  // Isolated interior peak (all four face-neighbors are dark field) -> a regional maximum not connected to the
  // border -> must be GROUND DOWN to the surrounding level (50).
  pattern[rt::FlatIndex(3, 3, 0, DX, DY)] = peak;
  // A bright 2x2 region anchored at the (0,0) corner. Voxels (0,0),(1,0),(0,1) are on the 2D border; (1,1) is a
  // non-border voxel but is connected to the border through the bright region -> the whole region is NOT ground.
  pattern[rt::FlatIndex(0, 0, 0, DX, DY)] = peak;
  pattern[rt::FlatIndex(1, 0, 0, DX, DY)] = peak;
  pattern[rt::FlatIndex(0, 1, 0, DX, DY)] = peak;
  pattern[rt::FlatIndex(1, 1, 0, DX, DY)] = peak;

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);

  GrayscaleGrindPeakImageFilter filter;
  rt::RunReconstructionFilter<GrayscaleGrindPeakImageFilter>(filter, ds, inputPath, rt::FullyConnectedParamSetter<GrayscaleGrindPeakImageFilter>(), /*fullyConnected=*/false, /*height=*/0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();

  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, DX, DY)) == field); // interior peak ground down
  REQUIRE(outStore.getValue(rt::FlatIndex(0, 0, 0, DX, DY)) == peak);  // border voxel of the border region
  REQUIRE(outStore.getValue(rt::FlatIndex(1, 1, 0, DX, DY)) == peak);  // non-border voxel of the border region, still not ground
  REQUIRE(outStore.getValue(rt::FlatIndex(5, 5, 0, DX, DY)) == field); // untouched field voxel
}

// -----------------------------------------------------------------------------
// (2b) Both-algorithm-paths correctness on the hand-computed interior-grind/border-region image. GrindPeak is a
//      reconstruction-by-dilation routed through ApplyMorphologicalReconstruction's DispatchAlgorithm, so
//      AlgorithmTestScope runs the filter under BOTH the in-core and out-of-core algorithm paths on in-memory stores
//      (selected by SIMPLNX_TEST_ALGORITHM_PATH). The hand-derived expectations (isolated interior peak ground down,
//      border-connected bright region unchanged) are asserted directly -- an independent oracle, not the new filter's
//      own in-core run.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GrayscaleGrindPeakImageFilter: hand-computed interior grind vs border region (both algorithm paths)", "[ImageProcessing][GrayscaleGrindPeakImageFilter]")
{
  UnitTest::LoadPlugins();
  // Test both algorithm paths on in-memory stores; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  constexpr usize DX = 7, DY = 7, DZ = 1;
  constexpr int32 field = 50;
  constexpr int32 peak = 90;
  std::vector<int32> pattern(DX * DY * DZ, field);
  pattern[rt::FlatIndex(3, 3, 0, DX, DY)] = peak; // isolated interior peak -> ground down
  // A bright 2x2 region anchored at the (0,0) corner: connected to the border through the bright region -> NOT ground.
  pattern[rt::FlatIndex(0, 0, 0, DX, DY)] = peak;
  pattern[rt::FlatIndex(1, 0, 0, DX, DY)] = peak;
  pattern[rt::FlatIndex(0, 1, 0, DX, DY)] = peak;
  pattern[rt::FlatIndex(1, 1, 0, DX, DY)] = peak;

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));

  GrayscaleGrindPeakImageFilter filter;
  Arguments args;
  args.insertOrAssign(GrayscaleGrindPeakImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
  args.insertOrAssign(GrayscaleGrindPeakImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(GrayscaleGrindPeakImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  rt::FullyConnectedParamSetter<GrayscaleGrindPeakImageFilter>()(args, /*fullyConnected=*/false, /*height=*/0.0);

  auto executeResult = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = ds.getDataRefAs<DataArray<int32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, DX, DY)) == field); // interior peak ground down
  REQUIRE(outStore.getValue(rt::FlatIndex(0, 0, 0, DX, DY)) == peak);  // border voxel of the border region
  REQUIRE(outStore.getValue(rt::FlatIndex(1, 1, 0, DX, DY)) == peak);  // non-border voxel of the border region, still not ground
  REQUIRE(outStore.getValue(rt::FlatIndex(5, 5, 0, DX, DY)) == field); // untouched field voxel
  UnitTest::CheckArraysInheritTupleDims(ds);
}

// -----------------------------------------------------------------------------
// (4) Preflight: a multi-component (non-scalar) input is rejected (the reconstruction operator is defined on
//     scalar images), and a valid scalar input preflights cleanly.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GrayscaleGrindPeakImageFilter: Preflight scalar guard", "[ImageProcessing][GrayscaleGrindPeakImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  GrayscaleGrindPeakImageFilter filter;

  auto makeArgs = [](const DataPath& inputPath) {
    Arguments args;
    args.insertOrAssign(GrayscaleGrindPeakImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(GrayscaleGrindPeakImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(GrayscaleGrindPeakImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(GrayscaleGrindPeakImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
    return args;
  };

  SECTION("scalar input preflights cleanly")
  {
    DataStructure ds;
    const DataPath inputPath = rt::BuildPeakImage<uint8>(ds, 8, 8, 8, uint8{40}, uint8{220});
    auto result = filter.preflight(ds, makeArgs(inputPath));
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }

  SECTION("multi-component input is rejected")
  {
    DataStructure ds;
    const DataPath scalarPath = rt::BuildPeakImage<uint8>(ds, 8, 8, 8, uint8{40}, uint8{220}); // creates the geometry + CellData
    const DataPath vecPath({"Image Geometry", "CellData", "Vec"});
    auto vecStore = DataStoreUtilities::CreateDataStore<uint8>(ds, vecPath, {8, 8, 8}, {2});
    const auto& cellAM = ds.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
    DataArray<uint8>::Create(ds, "Vec", vecStore, cellAM.getId());

    auto result = filter.preflight(ds, makeArgs(vecPath));
    REQUIRE(result.outputActions.invalid());
    REQUIRE(result.outputActions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
  }
}

// -----------------------------------------------------------------------------
// (5) FromSIMPLJson maps the legacy SIMPL keys (FullyConnected + array paths) to the new parameter keys.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GrayscaleGrindPeakImageFilter: FromSIMPLJson", "[ImageProcessing][GrayscaleGrindPeakImageFilter]")
{
  const nlohmann::json json = {{"FullyConnected", true},
                               {"SelectedCellArrayPath", {{"Data Container Name", "Image Geometry"}, {"Attribute Matrix Name", "CellData"}, {"Data Array Name", "Input"}}},
                               {"NewCellArrayName", "Ground"}};

  Result<Arguments> result = GrayscaleGrindPeakImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments& args = result.value();
  REQUIRE(args.value<bool>(GrayscaleGrindPeakImageFilter::k_FullyConnected_Key) == true);
  REQUIRE(args.value<std::string>(GrayscaleGrindPeakImageFilter::k_OutputImageArrayName_Key) == "Ground");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image md5 golden -- duplicates ITKGrayscaleGrindPeakImageTest.cpp exactly (ITK-default params):
// (B) live-ITK bit-exact + (A) md5-validity-first. GrayscaleGrindPeak1 = RA-Short.nrrd, GrayscaleGrindPeak2 =
// RA-Slice-Short.png.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::GrayscaleGrindPeakImageFilter: ITK real-image golden (GrayscaleGrindPeak1)", "[ImageProcessing][ItkGolden][GrayscaleGrindPeakImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<GrayscaleGrindPeakImageFilter>("RA-Short.nrrd", "084cdd1d64664ebfab26c2e0ed382e14");
}

TEST_CASE("ImageProcessing::GrayscaleGrindPeakImageFilter: ITK real-image golden (GrayscaleGrindPeak2)", "[ImageProcessing][ItkGolden][GrayscaleGrindPeakImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<GrayscaleGrindPeakImageFilter>("RA-Slice-Short.png", "b18d75cccb9361c65b40bb5c0d3c6e0d");
}
