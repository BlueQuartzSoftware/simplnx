#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/HMinimaImageFilter.hpp"

#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <array>
#include <limits>
#include <memory>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
// Legacy ITKHMinimaImageFilter, created at runtime by UUID (so this target does not link ITKImageProcessing).
const Uuid k_LegacyHMinimaUuid = *Uuid::FromString("9b2eb24b-90e5-41c0-9230-1044170ee8ea");

// Filter-local error codes for the Height guards (kept in sync with HMinimaImageFilter.cpp).
constexpr int32 k_NonFiniteHeight = -8552;
constexpr int32 k_NegativeHeight = -8555;

// Live-parity configs: 3D + genuinely-2D (Z=1) gradient/noise images, each at two INTEGER-valued heights (exact
// for integer types) crossed with BOTH FullyConnected states (HMinima, unlike HMaxima, exposes the flag).
const std::vector<rt::ReconCase> k_ParityConfigs = {{"3D 12x12x12 h=3 fc=off", false, 3.0, 12, 12, 12},   {"3D 12x12x12 h=3 fc=on", true, 3.0, 12, 12, 12},
                                                    {"3D 12x12x12 h=20 fc=off", false, 20.0, 12, 12, 12}, {"3D 12x12x12 h=20 fc=on", true, 20.0, 12, 12, 12},
                                                    {"2D 20x16x1 h=3 fc=off", false, 3.0, 20, 16, 1},     {"2D 20x16x1 h=3 fc=on", true, 3.0, 20, 16, 1},
                                                    {"2D 20x16x1 h=20 fc=off", false, 20.0, 20, 16, 1},   {"2D 20x16x1 h=20 fc=on", true, 20.0, 20, 16, 1}};
} // namespace

// -----------------------------------------------------------------------------
// (1) Live-ITK EXACT parity grid across uint8 / int16 / float32 at two integer heights and BOTH FullyConnected
//     states, on a gradient/noise image. HMinima is a reconstruction-by-erosion of the saturating (input + Height)
//     marker that only copies/clamps existing values, so the new filter must reproduce the legacy ITK output
//     EXACTLY -- for integer AND float types (no tolerance). Integer-valued heights keep the marker exact for
//     integer types.
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::HMinimaImageFilter: Live-ITK exact parity grid", "[ImageProcessing][HMinimaImageFilter]", uint8, int16, float32)
{
  using T = TestType;
  const auto build = [](DataStructure& ds, usize dx, usize dy, usize dz) { return rt::BuildImageFromPattern<T>(ds, dx, dy, dz, rt::MakeGradientNoisePattern<T>(dx, dy, dz)); };
  rt::RunReconstructionParityGrid<HMinimaImageFilter, T>(k_LegacyHMinimaUuid, build, k_ParityConfigs, rt::HeightAndFullyConnectedParamSetter<HMinimaImageFilter>());
}

// -----------------------------------------------------------------------------
// (2) Hand-computed behavior on a small 2D image: a flat background with two isolated dark dents. The dent
//     whose contrast (>= Height) survives is raised by exactly Height; the dent whose contrast (< Height) is
//     filled up to the surrounding background; the background is left unchanged.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::HMinimaImageFilter: hand-computed minima suppression", "[ImageProcessing][HMinimaImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 9;
  constexpr usize DY = 9;
  constexpr usize DZ = 1;
  constexpr int32 background = 50;
  constexpr int32 deepDent = 30;    // contrast 20 (>= Height) -> survives, raised by Height to 40
  constexpr int32 shallowDent = 45; // contrast 5  (<  Height) -> filled up to background (50)
  constexpr float64 height = 10.0;

  std::vector<int32> pattern(DX * DY * DZ, background);
  pattern[rt::FlatIndex(2, 2, 0, DX, DY)] = deepDent;    // isolated regional minimum (all face-neighbors are background)
  pattern[rt::FlatIndex(6, 6, 0, DX, DY)] = shallowDent; // isolated regional minimum, separated from the deep dent by background

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);

  HMinimaImageFilter filter;
  rt::RunReconstructionFilter<HMinimaImageFilter>(filter, ds, inputPath, rt::HeightAndFullyConnectedParamSetter<HMinimaImageFilter>(), /*fullyConnected=*/false, height);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();

  REQUIRE(outStore.getValue(rt::FlatIndex(2, 2, 0, DX, DY)) == deepDent + static_cast<int32>(height)); // 30 -> 40 (raised by Height)
  REQUIRE(outStore.getValue(rt::FlatIndex(6, 6, 0, DX, DY)) == background);                            // 45 -> 50 (shallow minimum removed)
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 2, 0, DX, DY)) == background);                            // neighbor of the deep dent, unchanged
  REQUIRE(outStore.getValue(rt::FlatIndex(4, 4, 0, DX, DY)) == background);                            // untouched background voxel
}

// -----------------------------------------------------------------------------
// (2b) Both-algorithm-paths coverage: the SAME hand-computed minima-suppression oracle as test (2), run under BOTH
//      the in-core and out-of-core reconstruction ALGORITHM paths on in-memory stores (selected by
//      SIMPLNX_TEST_ALGORITHM_PATH) via UnitTest::AlgorithmTestScope. HMinima's grayscale reconstruction engine
//      routes through DispatchAlgorithm, so the scope proves both implementations produce the correct suppression;
//      the hand-computed expected values are the non-circular oracle (not the filter's own in-core run).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::HMinimaImageFilter: hand-computed minima suppression (both algorithm paths)", "[ImageProcessing][HMinimaImageFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  constexpr usize DX = 9;
  constexpr usize DY = 9;
  constexpr usize DZ = 1;
  constexpr int32 background = 50;
  constexpr int32 deepDent = 30;    // contrast 20 (>= Height) -> survives, raised by Height to 40
  constexpr int32 shallowDent = 45; // contrast 5  (<  Height) -> filled up to background (50)
  constexpr float64 height = 10.0;

  std::vector<int32> pattern(DX * DY * DZ, background);
  pattern[rt::FlatIndex(2, 2, 0, DX, DY)] = deepDent;    // isolated regional minimum (all face-neighbors are background)
  pattern[rt::FlatIndex(6, 6, 0, DX, DY)] = shallowDent; // isolated regional minimum, separated from the deep dent by background

  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));

  HMinimaImageFilter filter;
  Arguments args;
  args.insertOrAssign(HMinimaImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(HMinimaImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(HMinimaImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(HMinimaImageFilter::k_Height_Key, std::make_any<float64>(height));
  args.insertOrAssign(HMinimaImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));

  auto executeResult = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = ds.getDataRefAs<DataArray<int32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  REQUIRE(outStore.getValue(rt::FlatIndex(2, 2, 0, DX, DY)) == deepDent + static_cast<int32>(height)); // 30 -> 40 (raised by Height)
  REQUIRE(outStore.getValue(rt::FlatIndex(6, 6, 0, DX, DY)) == background);                            // 45 -> 50 (shallow minimum removed)
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 2, 0, DX, DY)) == background);                            // neighbor of the deep dent, unchanged
  REQUIRE(outStore.getValue(rt::FlatIndex(4, 4, 0, DX, DY)) == background);                            // untouched background voxel
}

// -----------------------------------------------------------------------------
// (2c) Height == 0 identity: HMinima reconstructs the (input + Height) marker under the input mask by erosion. With
//      Height 0 the marker equals the input, so the reconstruction-by-erosion converges to the input and the output
//      equals the input exactly. Preflight accepts Height 0 (only a negative Height is rejected); assert output ==
//      input.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::HMinimaImageFilter: Height 0 is the identity", "[ImageProcessing][HMinimaImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 12, DY = 12, DZ = 3;
  const std::vector<int32> pattern = rt::MakeGradientNoisePattern<int32>(DX, DY, DZ);
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);

  HMinimaImageFilter filter;
  rt::RunReconstructionFilter<HMinimaImageFilter>(filter, ds, inputPath, rt::HeightAndFullyConnectedParamSetter<HMinimaImageFilter>(), /*fullyConnected=*/false, /*height=*/0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();
  REQUIRE(outStore.getSize() == pattern.size());
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    INFO("Height-0 identity index=" << i);
    REQUIRE(outStore.getValue(i) == pattern[i]);
  }
}

// -----------------------------------------------------------------------------
// (3a) Saturation parity under overflow: on a uint8 image, a Height that drives (input + Height) above the uint8
//      maximum for the high-valued voxels must CLAMP their marker to 255 (a saturating cast), not WRAP it to a
//      small value. A wrapped marker would drop below the mask (violating the erosion precondition) and diverge
//      from the legacy ITK output; the exact match here (integer heights, no tolerance) confirms the new filter's
//      saturating cast reproduces ITK's AddConstant clamp. Height 100 leaves the low-valued voxels in range, so
//      the marker is non-degenerate (see the note in test (3b) for the fully-clamped edge case).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::HMinimaImageFilter: uint8 saturation clamp parity vs ITK", "[ImageProcessing][HMinimaImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyHMinimaUuid) != nullptr);

  // Height 100 drives (input + 100) above the uint8 maximum for every voxel whose value is > 155, so their marker
  // must saturate to 255; the lower-valued voxels keep an in-range marker (so the marker is non-degenerate and
  // stays >= the mask, which the legacy ITK ReconstructionByErosion requires).
  constexpr float64 height = 100.0;

  constexpr usize DX = 12;
  constexpr usize DY = 12;
  constexpr usize DZ = 8;
  // Deterministic spread across [0, 240] with no integer wrap, so the high-valued voxels genuinely overflow the
  // uint8 range and exercise the saturating clamp.
  std::vector<uint8> pattern(DX * DY * DZ);
  for(usize i = 0; i < pattern.size(); ++i)
  {
    pattern[i] = static_cast<uint8>((i * 17) % 241);
  }

  DataStructure newDs;
  const DataPath newInput = rt::BuildImageFromPattern<uint8>(newDs, DX, DY, DZ, pattern);
  HMinimaImageFilter newFilter;
  rt::RunReconstructionFilter<HMinimaImageFilter>(newFilter, newDs, newInput, rt::HeightAndFullyConnectedParamSetter<HMinimaImageFilter>(), /*fullyConnected=*/false, height);

  IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyHMinimaUuid);
  REQUIRE(legacyFilter != nullptr);
  DataStructure legacyDs;
  const DataPath legacyInput = rt::BuildImageFromPattern<uint8>(legacyDs, DX, DY, DZ, pattern);
  rt::RunReconstructionFilter<HMinimaImageFilter>(*legacyFilter, legacyDs, legacyInput, rt::HeightAndFullyConnectedParamSetter<HMinimaImageFilter>(), /*fullyConnected=*/false, height);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& newOut = newDs.getDataRefAs<IDataArray>(outputPath);
  const auto& legacyOut = legacyDs.getDataRefAs<IDataArray>(outputPath);
  REQUIRE(newOut.getDataType() == legacyOut.getDataType());
  UnitTest::CompareDataArrays<uint8>(newOut, legacyOut);
}

// -----------------------------------------------------------------------------
// (3b) Clamp (not wrap) at the fully-overflowed extreme: with a Height larger than the whole uint8 dynamic range
//      (300 > 255), (input + Height) overflows for EVERY voxel, so the saturating marker is 255 everywhere and
//      the reconstruction of that uniform ceiling is 255 everywhere. A wrapping cast would instead produce small
//      markers whose reconstruction stays near the input, so the uniform 255 output uniquely proves the cast
//      clamps rather than wraps. This is asserted on the new filter directly (no ITK comparison): at this
//      fully-clamped extreme the marker is a uniform NonnegativeMax, which the legacy ITK ReconstructionByErosion
//      treats as a seedless image and returns the mask (the input) unchanged -- an ITK sentinel shortcut that
//      happens to coincide with the wrong (wrapped) answer, so it is not a useful oracle here. ITK parity under
//      (non-degenerate) overflow is covered by test (3a).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::HMinimaImageFilter: uint8 full-overflow clamps to type maximum", "[ImageProcessing][HMinimaImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 12;
  constexpr usize DY = 12;
  constexpr usize DZ = 8;
  constexpr float64 height = 300.0; // > uint8 dynamic range (255) -> every voxel's marker overflows
  const std::vector<uint8> pattern = rt::MakeGradientNoisePattern<uint8>(DX, DY, DZ);

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<uint8>(ds, DX, DY, DZ, pattern);
  HMinimaImageFilter filter;
  rt::RunReconstructionFilter<HMinimaImageFilter>(filter, ds, inputPath, rt::HeightAndFullyConnectedParamSetter<HMinimaImageFilter>(), /*fullyConnected=*/false, height);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<uint8>>(outputPath).getDataStoreRef();
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    REQUIRE(outStore.getValue(i) == uint8{255}); // clamped to NonnegativeMax, not wrapped to a small value
  }
}

// -----------------------------------------------------------------------------
// (5) Preflight: a non-finite Height (NaN or Inf) is rejected with the filter-local error code, because the
//     saturating (input + Height) marker offset is undefined for a non-finite value.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::HMinimaImageFilter: Preflight non-finite Height guard", "[ImageProcessing][HMinimaImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  HMinimaImageFilter filter;

  auto makeArgs = [](const DataPath& inputPath, float64 height) {
    Arguments args;
    args.insertOrAssign(HMinimaImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(HMinimaImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(HMinimaImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(HMinimaImageFilter::k_Height_Key, std::make_any<float64>(height));
    args.insertOrAssign(HMinimaImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
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

TEST_CASE("ImageProcessing::HMinimaImageFilter: FromSIMPLJson", "[ImageProcessing][HMinimaImageFilter]")
{
  const nlohmann::json json = {{"Height", 3.5},
                               {"FullyConnected", true},
                               {"SelectedCellArrayPath", {{"Data Container Name", "Image Geometry"}, {"Attribute Matrix Name", "CellData"}, {"Data Array Name", "Input"}}},
                               {"NewCellArrayName", "HMin"}};

  Result<Arguments> result = HMinimaImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments& args = result.value();
  REQUIRE(args.value<float64>(HMinimaImageFilter::k_Height_Key) == 3.5);
  REQUIRE(args.value<bool>(HMinimaImageFilter::k_FullyConnected_Key) == true);
  REQUIRE(args.value<DataPath>(HMinimaImageFilter::k_InputImageGeomPath_Key) == DataPath({"Image Geometry"}));
  REQUIRE(args.value<DataPath>(HMinimaImageFilter::k_InputImageDataPath_Key) == DataPath({"Image Geometry", "CellData", "Input"}));
  REQUIRE(args.value<std::string>(HMinimaImageFilter::k_OutputImageArrayName_Key) == "HMin");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image md5 golden -- duplicates ITKHMinimaImageTest.cpp(HMinima): RA-Short.nrrd (int16),
// Height 2000. (B) live-ITK bit-exact + (A) md5-validity-first vs the committed hash.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::HMinimaImageFilter: ITK real-image golden (HMinima)", "[ImageProcessing][ItkGolden][HMinimaImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<HMinimaImageFilter>("RA-Short.nrrd", "7778067eeb752b6ac396dd9e362e8346",
                                                        [](Arguments& args) { args.insertOrAssign(HMinimaImageFilter::k_Height_Key, std::make_any<float64>(2000.0)); });
}
