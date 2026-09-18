#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/HConvexImageFilter.hpp"

#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <array>
#include <limits>
#include <memory>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
// Legacy ITKHConvexImageFilter, created at runtime by UUID (so this target does not link ITKImageProcessing).
const Uuid k_LegacyHConvexUuid = *Uuid::FromString("620240a1-0b04-4bc7-a4c3-531917de4bc0");

// Filter-local error codes for the Height guards (kept in sync with HConvexImageFilter.cpp).
constexpr int32 k_NonFiniteHeight = -8553;
constexpr int32 k_NegativeHeight = -8556;

// Live-parity configs: 3D + genuinely-2D (Z=1) gradient/noise images, each with a height + FullyConnected combo
// (INTEGER-valued heights so integer-type parity is exact).
const std::vector<rt::ReconCase> k_ParityConfigs = {{"3D 12x12x12 h=3 fc=false", false, 3.0, 12, 12, 12},
                                                    {"3D 12x12x12 h=20 fc=true", true, 20.0, 12, 12, 12},
                                                    {"2D 20x16x1 h=3 fc=false", false, 3.0, 20, 16, 1},
                                                    {"2D 20x16x1 h=20 fc=true", true, 20.0, 20, 16, 1}};
} // namespace

// -----------------------------------------------------------------------------
// (1) Live-ITK EXACT parity grid across uint8 / int16 / float32 at two integer heights x FullyConnected {off, on},
//     on a gradient/noise image. HConvex = input - HMaxima(input, h): a reconstruction-by-dilation followed by a
//     subtraction, both of which only copy/clamp/subtract existing values (the reconstruction never exceeds the
//     input, so the difference never underflows), so the new filter must reproduce the legacy ITK H Convex output
//     EXACTLY -- for integer AND float types (no tolerance). Integer heights keep the marker exact for integers.
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::HConvexImageFilter: Live-ITK exact parity grid", "[ImageProcessing][HConvexImageFilter]", uint8, int16, float32)
{
  using T = TestType;
  const auto build = [](DataStructure& ds, usize dx, usize dy, usize dz) { return rt::BuildImageFromPattern<T>(ds, dx, dy, dz, rt::MakeGradientNoisePattern<T>(dx, dy, dz)); };
  rt::RunReconstructionParityGrid<HConvexImageFilter, T>(k_LegacyHConvexUuid, build, k_ParityConfigs, rt::HeightAndFullyConnectedParamSetter<HConvexImageFilter>());
}

// -----------------------------------------------------------------------------
// (2) Hand-computed behavior on a small 2D image: a flat background maps to 0; a bright bump whose contrast is
//     >= Height maps to exactly Height (its dome is capped); a bright bump whose contrast is < Height maps to its
//     own contrast. HConvex(f, h) = f - HMaxima(f, h) = min(contrast, h) at each regional maximum, 0 elsewhere.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::HConvexImageFilter: hand-computed dome heights", "[ImageProcessing][HConvexImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 9;
  constexpr usize DY = 9;
  constexpr usize DZ = 1;
  constexpr int32 background = 50;
  constexpr int32 tallBump = 70;    // contrast 20 (>= Height) -> dome capped to Height (10)
  constexpr int32 shallowBump = 55; // contrast 5  (<  Height) -> dome equals its own contrast (5)
  constexpr float64 height = 10.0;

  std::vector<int32> pattern(DX * DY * DZ, background);
  pattern[rt::FlatIndex(2, 2, 0, DX, DY)] = tallBump;    // isolated regional maximum (all face-neighbors are background)
  pattern[rt::FlatIndex(6, 6, 0, DX, DY)] = shallowBump; // isolated regional maximum, separated from the tall bump by background

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);

  HConvexImageFilter filter;
  rt::RunReconstructionFilter<HConvexImageFilter>(filter, ds, inputPath, rt::HeightAndFullyConnectedParamSetter<HConvexImageFilter>(), /*fullyConnected=*/false, height);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();

  REQUIRE(outStore.getValue(rt::FlatIndex(2, 2, 0, DX, DY)) == static_cast<int32>(height)); // tall dome capped to Height (10)
  REQUIRE(outStore.getValue(rt::FlatIndex(6, 6, 0, DX, DY)) == (shallowBump - background)); // shallow dome == contrast (5)
  REQUIRE(outStore.getValue(rt::FlatIndex(4, 4, 0, DX, DY)) == 0);                          // background -> 0
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 2, 0, DX, DY)) == 0);                          // neighbor of the tall bump -> 0
}

// -----------------------------------------------------------------------------
// (2b) Both-algorithm-paths coverage: the SAME hand-computed dome-height oracle as test (2), run under BOTH the
//      in-core and out-of-core reconstruction ALGORITHM paths on in-memory stores (selected by
//      SIMPLNX_TEST_ALGORITHM_PATH) via UnitTest::AlgorithmTestScope. HConvex reconstructs the (input - Height)
//      marker (which routes through DispatchAlgorithm) and then subtracts it from the input, so the scope proves
//      both reconstruction implementations feed the correct dome; the hand-computed expected values are the
//      non-circular oracle (not the filter's own in-core run).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::HConvexImageFilter: hand-computed dome heights (both algorithm paths)", "[ImageProcessing][HConvexImageFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  constexpr usize DX = 9;
  constexpr usize DY = 9;
  constexpr usize DZ = 1;
  constexpr int32 background = 50;
  constexpr int32 tallBump = 70;    // contrast 20 (>= Height) -> dome capped to Height (10)
  constexpr int32 shallowBump = 55; // contrast 5  (<  Height) -> dome equals its own contrast (5)
  constexpr float64 height = 10.0;

  std::vector<int32> pattern(DX * DY * DZ, background);
  pattern[rt::FlatIndex(2, 2, 0, DX, DY)] = tallBump;    // isolated regional maximum (all face-neighbors are background)
  pattern[rt::FlatIndex(6, 6, 0, DX, DY)] = shallowBump; // isolated regional maximum, separated from the tall bump by background

  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));

  HConvexImageFilter filter;
  Arguments args;
  args.insertOrAssign(HConvexImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(HConvexImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(HConvexImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(HConvexImageFilter::k_Height_Key, std::make_any<float64>(height));
  args.insertOrAssign(HConvexImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));

  auto executeResult = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = ds.getDataRefAs<DataArray<int32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  REQUIRE(outStore.getValue(rt::FlatIndex(2, 2, 0, DX, DY)) == static_cast<int32>(height)); // tall dome capped to Height (10)
  REQUIRE(outStore.getValue(rt::FlatIndex(6, 6, 0, DX, DY)) == (shallowBump - background)); // shallow dome == contrast (5)
  REQUIRE(outStore.getValue(rt::FlatIndex(4, 4, 0, DX, DY)) == 0);                          // background -> 0
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 2, 0, DX, DY)) == 0);                          // neighbor of the tall bump -> 0
}

// -----------------------------------------------------------------------------
// (2c) Height == 0 no-op: HConvex(f, 0) = f - HMaxima(f, 0). The (input - 0) marker equals the input, so its
//      reconstruction-by-dilation converges to the input and the h-dome (input - input) is 0 at every voxel.
//      Preflight accepts Height 0 (only a negative Height is rejected); assert the entire output is zero.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::HConvexImageFilter: Height 0 yields an all-zero output", "[ImageProcessing][HConvexImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 12, DY = 12, DZ = 3;
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, rt::MakeGradientNoisePattern<int32>(DX, DY, DZ));

  HConvexImageFilter filter;
  rt::RunReconstructionFilter<HConvexImageFilter>(filter, ds, inputPath, rt::HeightAndFullyConnectedParamSetter<HConvexImageFilter>(), /*fullyConnected=*/false, /*height=*/0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    INFO("Height-0 h-dome index=" << i);
    REQUIRE(outStore.getValue(i) == 0);
  }
}

// -----------------------------------------------------------------------------
// (2d) Height beyond the dynamic range saturates the marker: on a uint8 image a Height larger than 255 drives the
//      saturating (input - Height) marker to the type minimum (0) at every voxel, whose reconstruction-by-dilation
//      is 0 everywhere, so HConvex = input - 0 = input. Assert the output equals the input exactly (the saturating
//      marker cast, and the input-minus-reconstruction difference, are both well-defined at this extreme).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::HConvexImageFilter: Height beyond dynamic range returns the input (uint8 saturation)", "[ImageProcessing][HConvexImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 12, DY = 12, DZ = 3;
  const std::vector<uint8> pattern = rt::MakeGradientNoisePattern<uint8>(DX, DY, DZ);
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<uint8>(ds, DX, DY, DZ, pattern);

  HConvexImageFilter filter;
  rt::RunReconstructionFilter<HConvexImageFilter>(filter, ds, inputPath, rt::HeightAndFullyConnectedParamSetter<HConvexImageFilter>(), /*fullyConnected=*/false, /*height=*/1000.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<uint8>>(outputPath).getDataStoreRef();
  REQUIRE(outStore.getSize() == pattern.size());
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    INFO("saturated h-dome index=" << i);
    REQUIRE(outStore.getValue(i) == pattern[i]);
  }
}

// -----------------------------------------------------------------------------
// (2e) Preflight scalar guard: a multi-component (non-scalar) input is rejected (requireScalar=true; the grayscale
//      reconstruction operator is defined on scalar images). HConvex uniquely lacked this coverage that its siblings
//      GrayscaleFillhole/GrayscaleGrindPeak have; the input ArraySelectionParameter imposes no component-shape
//      restriction, so the guard lives in preflightImpl and returns k_NonScalarInput (-8550).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::HConvexImageFilter: Preflight scalar guard (multi-component rejected)", "[ImageProcessing][HConvexImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath scalarPath = rt::BuildImageFromPattern<uint8>(ds, 8, 8, 8, rt::MakeGradientNoisePattern<uint8>(8, 8, 8)); // creates the geometry + CellData
  const DataPath vecPath({"Image Geometry", "CellData", "Vec"});
  auto vecStore = DataStoreUtilities::CreateDataStore<uint8>(ds, vecPath, {8, 8, 8}, {2});
  const auto& cellAM = ds.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
  DataArray<uint8>::Create(ds, "Vec", vecStore, cellAM.getId());

  HConvexImageFilter filter;
  Arguments args;
  args.insertOrAssign(HConvexImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(HConvexImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(vecPath));
  args.insertOrAssign(HConvexImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(HConvexImageFilter::k_Height_Key, std::make_any<float64>(2.0));
  args.insertOrAssign(HConvexImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
  const auto result = filter.preflight(ds, args);
  REQUIRE(result.outputActions.invalid());
  REQUIRE(result.outputActions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
}

// -----------------------------------------------------------------------------
// (4) Preflight: a non-finite Height (NaN or Inf) is rejected with the filter-local error code, because the
//     saturating (input - Height) marker offset is undefined for a non-finite value.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::HConvexImageFilter: Preflight non-finite Height guard", "[ImageProcessing][HConvexImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  HConvexImageFilter filter;

  auto makeArgs = [](const DataPath& inputPath, float64 height) {
    Arguments args;
    args.insertOrAssign(HConvexImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(HConvexImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(HConvexImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(HConvexImageFilter::k_Height_Key, std::make_any<float64>(height));
    args.insertOrAssign(HConvexImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
    return args;
  };

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<uint8>(ds, 8, 8, 8, rt::MakeGradientNoisePattern<uint8>(8, 8, 8));

  SECTION("finite Height preflights cleanly")
  {
    auto result = filter.preflight(ds, makeArgs(inputPath, 2.0));
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }

  SECTION("NaN Height is rejected")
  {
    auto result = filter.preflight(ds, makeArgs(inputPath, std::numeric_limits<float64>::quiet_NaN()));
    REQUIRE(result.outputActions.invalid());
    REQUIRE(result.outputActions.errors().front().code == k_NonFiniteHeight);
  }

  SECTION("Inf Height is rejected")
  {
    auto result = filter.preflight(ds, makeArgs(inputPath, std::numeric_limits<float64>::infinity()));
    REQUIRE(result.outputActions.invalid());
    REQUIRE(result.outputActions.errors().front().code == k_NonFiniteHeight);
  }

  SECTION("Negative Height is rejected")
  {
    auto result = filter.preflight(ds, makeArgs(inputPath, -1.0));
    REQUIRE(result.outputActions.invalid());
    REQUIRE(result.outputActions.errors().front().code == k_NegativeHeight);
  }
}

TEST_CASE("ImageProcessing::HConvexImageFilter: FromSIMPLJson", "[ImageProcessing][HConvexImageFilter]")
{
  const nlohmann::json json = {{"Height", 3.5},
                               {"FullyConnected", true},
                               {"SelectedCellArrayPath", {{"Data Container Name", "Image Geometry"}, {"Attribute Matrix Name", "CellData"}, {"Data Array Name", "Input"}}},
                               {"NewCellArrayName", "HConv"}};

  Result<Arguments> result = HConvexImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments& args = result.value();
  REQUIRE(args.value<float64>(HConvexImageFilter::k_Height_Key) == 3.5);
  REQUIRE(args.value<bool>(HConvexImageFilter::k_FullyConnected_Key) == true);
  REQUIRE(args.value<DataPath>(HConvexImageFilter::k_InputImageGeomPath_Key) == DataPath({"Image Geometry"}));
  REQUIRE(args.value<DataPath>(HConvexImageFilter::k_InputImageDataPath_Key) == DataPath({"Image Geometry", "CellData", "Input"}));
  REQUIRE(args.value<std::string>(HConvexImageFilter::k_OutputImageArrayName_Key) == "HConv");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image md5 golden -- duplicates ITKHConvexImageTest.cpp(HConvex): RA-Short.nrrd (int16),
// Height 10000. (B) live-ITK bit-exact + (A) md5-validity-first vs the committed hash.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::HConvexImageFilter: ITK real-image golden (HConvex)", "[ImageProcessing][ItkGolden][HConvexImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<HConvexImageFilter>("RA-Short.nrrd", "f3a7b95a51710d51b3b73e0eb77eb1eb",
                                                        [](Arguments& args) { args.insertOrAssign(HConvexImageFilter::k_Height_Key, std::make_any<float64>(10000.0)); });
}
