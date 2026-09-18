#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/RegionalMaximaImageFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <array>
#include <limits>
#include <memory>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
// Legacy ITKRegionalMaximaImageFilter, created at runtime by UUID.
const Uuid k_LegacyRegionalMaximaUuid = *Uuid::FromString("a2b8a295-5730-477a-b97b-8a0b0400397c");

constexpr int32 k_ValueOutOfRange = -8574; // kept in sync with RegionalMaximaImageFilter.cpp

const std::vector<rt::ReconCase> k_ParityConfigs = {
    {"3D 12x12x12 fc=false", false, 0.0, 12, 12, 12}, {"3D 12x12x12 fc=true", true, 0.0, 12, 12, 12}, {"2D 20x16x1 fc=false", false, 0.0, 20, 16, 1}, {"2D 20x16x1 fc=true", true, 0.0, 20, 16, 1}};

// Build a param-setter that fixes fg/bg + FlatIsMaxima and takes FullyConnected from the grid config.
rt::ReconParamSetter MaximaSetter(float64 fg, float64 bg, bool flatIsMaxima)
{
  return [fg, bg, flatIsMaxima](Arguments& args, bool fullyConnected, float64 /*height*/) {
    args.insertOrAssign(RegionalMaximaImageFilter::k_ForegroundValue_Key, std::make_any<float64>(fg));
    args.insertOrAssign(RegionalMaximaImageFilter::k_BackgroundValue_Key, std::make_any<float64>(bg));
    args.insertOrAssign(RegionalMaximaImageFilter::k_FullyConnected_Key, std::make_any<bool>(fullyConnected));
    args.insertOrAssign(RegionalMaximaImageFilter::k_FlatIsMaxima_Key, std::make_any<bool>(flatIsMaxima));
  };
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Live-ITK EXACT parity grid across uint8 / int16 / float32 x FullyConnected {off, on} x 3D + 2D on a
//     multi-plateau image (fg=1, bg=0). RegionalMaxima marks the regional maxima as foreground and everything else
//     as background -- a valued-extrema flood followed by a threshold -- so the new filter must reproduce the
//     legacy ITK output EXACTLY (no tolerance).
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::RegionalMaximaImageFilter: Live-ITK exact parity grid", "[ImageProcessing][RegionalMaximaImageFilter]", uint8, int16, float32)
{
  using T = TestType;
  const auto build = [](DataStructure& ds, usize dx, usize dy, usize dz) { return rt::BuildImageFromPattern<T>(ds, dx, dy, dz, rt::MakePlateauPattern<T>(dx, dy, dz)); };
  // The output is a fixed uint32 label image regardless of the input type T, so the exact comparison uses uint32.
  rt::RunReconstructionParityGrid<RegionalMaximaImageFilter, T, uint32>(k_LegacyRegionalMaximaUuid, build, k_ParityConfigs, MaximaSetter(1.0, 0.0, /*flatIsMaxima=*/true));
}

// -----------------------------------------------------------------------------
// (2) Flat-image special case, vs live ITK: a completely flat image has no regional structure, so RegionalMaxima
//     is all foreground when FlatIsMaxima is On and all background when Off. Checked directly AND against the
//     legacy ITK filter for both FlatIsMaxima values.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::RegionalMaximaImageFilter: flat-image special case", "[ImageProcessing][RegionalMaximaImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  const bool flatIsMaxima = GENERATE(false, true);
  CAPTURE(flatIsMaxima);

  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyRegionalMaximaUuid) != nullptr);

  constexpr usize DX = 8, DY = 8, DZ = 4;
  const std::vector<uint8> flatImg(DX * DY * DZ, uint8{100});

  DataStructure newDs;
  const DataPath newInput = rt::BuildImageFromPattern<uint8>(newDs, DX, DY, DZ, flatImg);
  RegionalMaximaImageFilter newFilter;
  rt::RunReconstructionFilter<RegionalMaximaImageFilter>(newFilter, newDs, newInput, MaximaSetter(1.0, 0.0, flatIsMaxima), /*fullyConnected=*/false, /*height=*/0.0);

  IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyRegionalMaximaUuid);
  REQUIRE(legacyFilter != nullptr);
  DataStructure legacyDs;
  const DataPath legacyInput = rt::BuildImageFromPattern<uint8>(legacyDs, DX, DY, DZ, flatImg);
  rt::RunReconstructionFilter<RegionalMaximaImageFilter>(*legacyFilter, legacyDs, legacyInput, MaximaSetter(1.0, 0.0, flatIsMaxima), /*fullyConnected=*/false, /*height=*/0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = newDs.getDataRefAs<DataArray<uint32>>(outputPath).getDataStoreRef();
  const uint32 expected = flatIsMaxima ? uint32{1} : uint32{0};
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    REQUIRE(outStore.getValue(i) == expected);
  }
  UnitTest::CompareDataArrays<uint32>(newDs.getDataRefAs<IDataArray>(outputPath), legacyDs.getDataRefAs<IDataArray>(outputPath));
}

// -----------------------------------------------------------------------------
// (3) Hand-computed: on a small image with a central peak plateau, the peak becomes foreground and the surrounding
//     field becomes background.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::RegionalMaximaImageFilter: hand-computed peak marked", "[ImageProcessing][RegionalMaximaImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 5, DY = 5, DZ = 1;
  std::vector<int32> pattern(DX * DY * DZ, 10);
  pattern[rt::FlatIndex(2, 2, 0, DX, DY)] = 50;
  pattern[rt::FlatIndex(3, 2, 0, DX, DY)] = 50;
  pattern[rt::FlatIndex(2, 3, 0, DX, DY)] = 50;
  pattern[rt::FlatIndex(3, 3, 0, DX, DY)] = 50;

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);
  RegionalMaximaImageFilter filter;
  rt::RunReconstructionFilter<RegionalMaximaImageFilter>(filter, ds, inputPath, MaximaSetter(1.0, 0.0, true), /*fullyConnected=*/false, /*height=*/0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<uint32>>(outputPath).getDataStoreRef();
  REQUIRE(outStore.getValue(rt::FlatIndex(2, 2, 0, DX, DY)) == 1); // peak -> foreground
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, DX, DY)) == 1); // peak -> foreground
  REQUIRE(outStore.getValue(rt::FlatIndex(0, 0, 0, DX, DY)) == 0); // field -> background
}

// -----------------------------------------------------------------------------
// (3b) Both-algorithm-paths coverage: the SAME hand-computed central-peak oracle as test (3), run under BOTH the
//      in-core and out-of-core ALGORITHM paths on in-memory stores (selected by SIMPLNX_TEST_ALGORITHM_PATH) via
//      UnitTest::AlgorithmTestScope. The regional-extrema engine routes through DispatchAlgorithm (a non-flat input
//      skips the flat short-circuit and reaches the valued-extrema flood/sweep), so the scope proves both
//      implementations mark the peak. The hand-computed expected labels are the non-circular oracle.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::RegionalMaximaImageFilter: hand-computed peak marked (both algorithm paths)", "[ImageProcessing][RegionalMaximaImageFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  constexpr usize DX = 5, DY = 5, DZ = 1;
  std::vector<int32> pattern(DX * DY * DZ, 10);
  pattern[rt::FlatIndex(2, 2, 0, DX, DY)] = 50;
  pattern[rt::FlatIndex(3, 2, 0, DX, DY)] = 50;
  pattern[rt::FlatIndex(2, 3, 0, DX, DY)] = 50;
  pattern[rt::FlatIndex(3, 3, 0, DX, DY)] = 50;

  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));

  RegionalMaximaImageFilter filter;
  Arguments args;
  args.insertOrAssign(RegionalMaximaImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(RegionalMaximaImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(RegionalMaximaImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  MaximaSetter(1.0, 0.0, /*flatIsMaxima=*/true)(args, /*fullyConnected=*/false, /*height=*/0.0);

  auto executeResult = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = ds.getDataRefAs<DataArray<uint32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  REQUIRE(outStore.getValue(rt::FlatIndex(2, 2, 0, DX, DY)) == 1); // peak -> foreground
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, DX, DY)) == 1); // peak -> foreground
  REQUIRE(outStore.getValue(rt::FlatIndex(0, 0, 0, DX, DY)) == 0); // field -> background
}

// -----------------------------------------------------------------------------
// (4) Preflight fg/bg range guard: for an integer input, an out-of-range Foreground Value is rejected; a valid one
//     preflights cleanly.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::RegionalMaximaImageFilter: preflight fg/bg range guard", "[ImageProcessing][RegionalMaximaImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<uint8>(ds, 8, 8, 8, rt::MakePlateauPattern<uint8>(8, 8, 8));

  auto makeArgs = [&](float64 fg) {
    Arguments args;
    args.insertOrAssign(RegionalMaximaImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(RegionalMaximaImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(RegionalMaximaImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(RegionalMaximaImageFilter::k_ForegroundValue_Key, std::make_any<float64>(fg));
    args.insertOrAssign(RegionalMaximaImageFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));
    args.insertOrAssign(RegionalMaximaImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
    args.insertOrAssign(RegionalMaximaImageFilter::k_FlatIsMaxima_Key, std::make_any<bool>(true));
    return args;
  };

  RegionalMaximaImageFilter filter;

  SECTION("valid fg preflights cleanly")
  {
    auto result = filter.preflight(ds, makeArgs(1.0));
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }

  SECTION("out-of-range fg (negative, invalid for the uint32 output) is rejected")
  {
    auto result = filter.preflight(ds, makeArgs(-1.0));
    REQUIRE(result.outputActions.invalid());
    REQUIRE(result.outputActions.errors().front().code == k_ValueOutOfRange);
  }
}

TEST_CASE("ImageProcessing::RegionalMaximaImageFilter: FromSIMPLJson", "[ImageProcessing][RegionalMaximaImageFilter]")
{
  const nlohmann::json json = {{"ForegroundValue", 5.0},
                               {"BackgroundValue", 2.0},
                               {"FullyConnected", true},
                               {"FlatIsMaxima", false},
                               {"SelectedCellArrayPath", {{"Data Container Name", "Image Geometry"}, {"Attribute Matrix Name", "CellData"}, {"Data Array Name", "Input"}}},
                               {"NewCellArrayName", "RMax"}};

  Result<Arguments> result = RegionalMaximaImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments& args = result.value();
  REQUIRE(args.value<float64>(RegionalMaximaImageFilter::k_ForegroundValue_Key) == 5.0);
  REQUIRE(args.value<float64>(RegionalMaximaImageFilter::k_BackgroundValue_Key) == 2.0);
  REQUIRE(args.value<bool>(RegionalMaximaImageFilter::k_FullyConnected_Key) == true);
  REQUIRE(args.value<bool>(RegionalMaximaImageFilter::k_FlatIsMaxima_Key) == false);
  REQUIRE(args.value<DataPath>(RegionalMaximaImageFilter::k_InputImageGeomPath_Key) == DataPath({"Image Geometry"}));
  REQUIRE(args.value<DataPath>(RegionalMaximaImageFilter::k_InputImageDataPath_Key) == DataPath({"Image Geometry", "CellData", "Input"}));
  REQUIRE(args.value<std::string>(RegionalMaximaImageFilter::k_OutputImageArrayName_Key) == "RMax");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image md5 golden -- duplicates ITKRegionalMaximaImageTest.cpp(defaults): cthead1.png (uint8),
// ITK-default params (fg=1, bg=0, fullyConnected off, flatIsMaxima on -- our defaults match the legacy wrapper's).
// The output is a fixed uint32 label image, so the committed hash is over uint32. (B) live-ITK bit-exact + (A)
// md5-validity-first.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::RegionalMaximaImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][RegionalMaximaImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<RegionalMaximaImageFilter>("cthead1.png", "6839f3d9dc5b95e9513d723a3b7430f2");
}
