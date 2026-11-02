#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/HMaximaImageFilter.hpp"

#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <array>
#include <limits>
#include <memory>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
// Filter-local error codes for the Height guards (kept in sync with HMaximaImageFilter.cpp).
constexpr int32 k_NonFiniteHeight = -8551;
constexpr int32 k_NegativeHeight = -8554;
} // namespace

// -----------------------------------------------------------------------------
// (2) Hand-computed behavior on a small 2D image: a flat background with two isolated bright bumps. The bump
//     whose contrast (>= Height) survives is lowered by exactly Height; the bump whose contrast (< Height) is
//     flattened down to the surrounding background; the background is left unchanged.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::HMaximaImageFilter: hand-computed maxima suppression", "[ImageProcessing][HMaximaImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 9;
  constexpr usize DY = 9;
  constexpr usize DZ = 1;
  constexpr int32 background = 50;
  constexpr int32 tallBump = 70;    // contrast 20 (>= Height) -> survives, lowered by Height to 60
  constexpr int32 shallowBump = 55; // contrast 5  (<  Height) -> flattened to background (50)
  constexpr float64 height = 10.0;

  std::vector<int32> pattern(DX * DY * DZ, background);
  pattern[rt::FlatIndex(2, 2, 0, DX, DY)] = tallBump;    // isolated regional maximum (all face-neighbors are background)
  pattern[rt::FlatIndex(6, 6, 0, DX, DY)] = shallowBump; // isolated regional maximum, separated from the tall bump by background

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);

  HMaximaImageFilter filter;
  rt::RunReconstructionFilter<HMaximaImageFilter>(filter, ds, inputPath, rt::HeightParamSetter<HMaximaImageFilter>(), /*fullyConnected=*/false, height);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();

  REQUIRE(outStore.getValue(rt::FlatIndex(2, 2, 0, DX, DY)) == tallBump - static_cast<int32>(height)); // 70 -> 60 (reduced by Height)
  REQUIRE(outStore.getValue(rt::FlatIndex(6, 6, 0, DX, DY)) == background);                            // 55 -> 50 (shallow maximum removed)
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 2, 0, DX, DY)) == background);                            // neighbor of the tall bump, unchanged
  REQUIRE(outStore.getValue(rt::FlatIndex(4, 4, 0, DX, DY)) == background);                            // untouched background voxel
}

// -----------------------------------------------------------------------------
// (2b) Both-algorithm-paths coverage: the SAME hand-computed maxima-suppression oracle as test (2), run under BOTH
//      the in-core and out-of-core reconstruction ALGORITHM paths on in-memory stores (selected by
//      SIMPLNX_TEST_ALGORITHM_PATH) via UnitTest::AlgorithmTestScope. HMaxima's grayscale reconstruction engine
//      routes through DispatchAlgorithm, so the scope proves both implementations produce the correct suppression;
//      the hand-computed expected values are the non-circular oracle (not the filter's own in-core run).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::HMaximaImageFilter: hand-computed maxima suppression (both algorithm paths)", "[ImageProcessing][HMaximaImageFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  constexpr usize DX = 9;
  constexpr usize DY = 9;
  constexpr usize DZ = 1;
  constexpr int32 background = 50;
  constexpr int32 tallBump = 70;    // contrast 20 (>= Height) -> survives, lowered by Height to 60
  constexpr int32 shallowBump = 55; // contrast 5  (<  Height) -> flattened to background (50)
  constexpr float64 height = 10.0;

  std::vector<int32> pattern(DX * DY * DZ, background);
  pattern[rt::FlatIndex(2, 2, 0, DX, DY)] = tallBump;    // isolated regional maximum (all face-neighbors are background)
  pattern[rt::FlatIndex(6, 6, 0, DX, DY)] = shallowBump; // isolated regional maximum, separated from the tall bump by background

  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));

  HMaximaImageFilter filter;
  Arguments args;
  args.insertOrAssign(HMaximaImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(HMaximaImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(HMaximaImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(HMaximaImageFilter::k_Height_Key, std::make_any<float64>(height));

  auto executeResult = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = ds.getDataRefAs<DataArray<int32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  REQUIRE(outStore.getValue(rt::FlatIndex(2, 2, 0, DX, DY)) == tallBump - static_cast<int32>(height)); // 70 -> 60 (reduced by Height)
  REQUIRE(outStore.getValue(rt::FlatIndex(6, 6, 0, DX, DY)) == background);                            // 55 -> 50 (shallow maximum removed)
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 2, 0, DX, DY)) == background);                            // neighbor of the tall bump, unchanged
  REQUIRE(outStore.getValue(rt::FlatIndex(4, 4, 0, DX, DY)) == background);                            // untouched background voxel
}

// -----------------------------------------------------------------------------
// (2c) Height == 0 identity: HMaxima reconstructs the (input - Height) marker under the input mask. With Height 0 the
//      marker equals the input, so the reconstruction-by-dilation converges to the input and the output equals the
//      input exactly. Preflight accepts Height 0 (only a negative Height is rejected); assert output == input.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::HMaximaImageFilter: Height 0 is the identity", "[ImageProcessing][HMaximaImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 12, DY = 12, DZ = 3;
  const std::vector<int32> pattern = rt::MakeGradientNoisePattern<int32>(DX, DY, DZ);
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);

  HMaximaImageFilter filter;
  rt::RunReconstructionFilter<HMaximaImageFilter>(filter, ds, inputPath, rt::HeightParamSetter<HMaximaImageFilter>(), /*fullyConnected=*/false, /*height=*/0.0);

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
// (3b) Clamp (not wrap) at the fully-underflowed extreme: with a Height larger than the whole int8 dynamic range
//      (300 > 255), (input - Height) underflows for EVERY voxel, so the saturating marker is -128 everywhere and
//      the reconstruction of that uniform floor is -128 everywhere. A wrapping cast would instead produce large
//      positive markers whose reconstruction stays near the input, so the uniform -128 output uniquely proves the
//      cast clamps rather than wraps. The test asserts the uniform output on the filter directly.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::HMaximaImageFilter: int8 full-underflow clamps to type minimum", "[ImageProcessing][HMaximaImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 12;
  constexpr usize DY = 12;
  constexpr usize DZ = 8;
  constexpr float64 height = 300.0; // > int8 dynamic range (255) -> every voxel's marker underflows
  const std::vector<int8> pattern = rt::MakeGradientNoisePattern<int8>(DX, DY, DZ);

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int8>(ds, DX, DY, DZ, pattern);
  HMaximaImageFilter filter;
  rt::RunReconstructionFilter<HMaximaImageFilter>(filter, ds, inputPath, rt::HeightParamSetter<HMaximaImageFilter>(), /*fullyConnected=*/false, height);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<int8>>(outputPath).getDataStoreRef();
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    REQUIRE(outStore.getValue(i) == int8{-128}); // clamped to NonpositiveMin, not wrapped to a positive value
  }
}

// -----------------------------------------------------------------------------
// (5) Preflight: a non-finite Height (NaN or Inf) is rejected with the filter-local error code, because the
//     saturating (input - Height) marker offset is undefined for a non-finite value.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::HMaximaImageFilter: Preflight non-finite Height guard", "[ImageProcessing][HMaximaImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  HMaximaImageFilter filter;

  auto makeArgs = [](const DataPath& inputPath, float64 height) {
    Arguments args;
    args.insertOrAssign(HMaximaImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(HMaximaImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(HMaximaImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(HMaximaImageFilter::k_Height_Key, std::make_any<float64>(height));
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

TEST_CASE("ImageProcessing::HMaximaImageFilter: FromSIMPLJson", "[ImageProcessing][HMaximaImageFilter]")
{
  const nlohmann::json json = {
      {"Height", 3.5}, {"SelectedCellArrayPath", {{"Data Container Name", "Image Geometry"}, {"Attribute Matrix Name", "CellData"}, {"Data Array Name", "Input"}}}, {"NewCellArrayName", "HMax"}};

  Result<Arguments> result = HMaximaImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments& args = result.value();
  REQUIRE(args.value<float64>(HMaximaImageFilter::k_Height_Key) == 3.5);
  REQUIRE(args.value<DataPath>(HMaximaImageFilter::k_InputImageGeomPath_Key) == DataPath({"Image Geometry"}));
  REQUIRE(args.value<DataPath>(HMaximaImageFilter::k_InputImageDataPath_Key) == DataPath({"Image Geometry", "CellData", "Input"}));
  REQUIRE(args.value<std::string>(HMaximaImageFilter::k_OutputImageArrayName_Key) == "HMax");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image md5 golden -- duplicates ITKHMaximaImageTest.cpp(HMaxima): RA-Short.nrrd (int16),
// Height 2000. (B) live-ITK bit-exact + (A) md5-validity-first vs the committed hash.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::HMaximaImageFilter: ITK real-image golden (HMaxima)", "[ImageProcessing][ItkGolden][HMaximaImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<HMaximaImageFilter>("RA-Short.nrrd", "b30d403fb1c5948abfb17fa9c346cecd",
                                                        [](Arguments& args) { args.insertOrAssign(HMaximaImageFilter::k_Height_Key, std::make_any<float64>(2000.0)); });
}
