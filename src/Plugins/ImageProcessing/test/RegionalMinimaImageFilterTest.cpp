#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/RegionalMinimaImageFilter.hpp"

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
// Legacy ITKRegionalMinimaImageFilter, created at runtime by UUID.
const Uuid k_LegacyRegionalMinimaUuid = *Uuid::FromString("7ec0883e-ac48-40e9-8b97-11bdfde721e2");

constexpr int32 k_ValueOutOfRange = -8577; // kept in sync with RegionalMinimaImageFilter.cpp

const std::vector<rt::ReconCase> k_ParityConfigs = {
    {"3D 12x12x12 fc=false", false, 0.0, 12, 12, 12}, {"3D 12x12x12 fc=true", true, 0.0, 12, 12, 12}, {"2D 20x16x1 fc=false", false, 0.0, 20, 16, 1}, {"2D 20x16x1 fc=true", true, 0.0, 20, 16, 1}};

// Build a param-setter that fixes fg/bg + FlatIsMinima and takes FullyConnected from the grid config.
rt::ReconParamSetter MinimaSetter(float64 fg, float64 bg, bool flatIsMinima)
{
  return [fg, bg, flatIsMinima](Arguments& args, bool fullyConnected, float64 /*height*/) {
    args.insertOrAssign(RegionalMinimaImageFilter::k_ForegroundValue_Key, std::make_any<float64>(fg));
    args.insertOrAssign(RegionalMinimaImageFilter::k_BackgroundValue_Key, std::make_any<float64>(bg));
    args.insertOrAssign(RegionalMinimaImageFilter::k_FullyConnected_Key, std::make_any<bool>(fullyConnected));
    args.insertOrAssign(RegionalMinimaImageFilter::k_FlatIsMinima_Key, std::make_any<bool>(flatIsMinima));
  };
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Live-ITK EXACT parity grid across uint8 / int16 / float32 x FullyConnected {off, on} x 3D + 2D on a
//     multi-plateau image (fg=1, bg=0). RegionalMinima marks the regional minima as foreground and everything else
//     as background -- a valued-extrema flood followed by a threshold -- so the new filter must reproduce the
//     legacy ITK output EXACTLY (no tolerance).
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::RegionalMinimaImageFilter: Live-ITK exact parity grid", "[ImageProcessing][RegionalMinimaImageFilter]", uint8, int16, float32)
{
  using T = TestType;
  const auto build = [](DataStructure& ds, usize dx, usize dy, usize dz) { return rt::BuildImageFromPattern<T>(ds, dx, dy, dz, rt::MakePlateauPattern<T>(dx, dy, dz)); };
  // The output is a fixed uint32 label image regardless of the input type T, so the exact comparison uses uint32.
  rt::RunReconstructionParityGrid<RegionalMinimaImageFilter, T, uint32>(k_LegacyRegionalMinimaUuid, build, k_ParityConfigs, MinimaSetter(1.0, 0.0, /*flatIsMinima=*/true));
}

// -----------------------------------------------------------------------------
// (2) Flat-image special case, vs live ITK: a completely flat image has no regional structure, so RegionalMinima
//     is all foreground when FlatIsMinima is On and all background when Off. Checked directly AND against the
//     legacy ITK filter for both FlatIsMinima values.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::RegionalMinimaImageFilter: flat-image special case", "[ImageProcessing][RegionalMinimaImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  const bool flatIsMinima = GENERATE(false, true);
  CAPTURE(flatIsMinima);

  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyRegionalMinimaUuid) != nullptr);

  constexpr usize DX = 8, DY = 8, DZ = 4;
  const std::vector<uint8> flatImg(DX * DY * DZ, uint8{100});

  DataStructure newDs;
  const DataPath newInput = rt::BuildImageFromPattern<uint8>(newDs, DX, DY, DZ, flatImg);
  RegionalMinimaImageFilter newFilter;
  rt::RunReconstructionFilter<RegionalMinimaImageFilter>(newFilter, newDs, newInput, MinimaSetter(1.0, 0.0, flatIsMinima), /*fullyConnected=*/false, /*height=*/0.0);

  IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyRegionalMinimaUuid);
  REQUIRE(legacyFilter != nullptr);
  DataStructure legacyDs;
  const DataPath legacyInput = rt::BuildImageFromPattern<uint8>(legacyDs, DX, DY, DZ, flatImg);
  rt::RunReconstructionFilter<RegionalMinimaImageFilter>(*legacyFilter, legacyDs, legacyInput, MinimaSetter(1.0, 0.0, flatIsMinima), /*fullyConnected=*/false, /*height=*/0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = newDs.getDataRefAs<DataArray<uint32>>(outputPath).getDataStoreRef();
  const uint32 expected = flatIsMinima ? uint32{1} : uint32{0};
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    REQUIRE(outStore.getValue(i) == expected);
  }
  UnitTest::CompareDataArrays<uint32>(newDs.getDataRefAs<IDataArray>(outputPath), legacyDs.getDataRefAs<IDataArray>(outputPath));
}

// -----------------------------------------------------------------------------
// (3) Hand-computed: on a small image with a central pit plateau, the pit becomes foreground and the surrounding
//     higher field becomes background.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::RegionalMinimaImageFilter: hand-computed pit marked", "[ImageProcessing][RegionalMinimaImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 5, DY = 5, DZ = 1;
  std::vector<int32> pattern(DX * DY * DZ, 50);
  pattern[rt::FlatIndex(2, 2, 0, DX, DY)] = 10;
  pattern[rt::FlatIndex(3, 2, 0, DX, DY)] = 10;
  pattern[rt::FlatIndex(2, 3, 0, DX, DY)] = 10;
  pattern[rt::FlatIndex(3, 3, 0, DX, DY)] = 10;

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);
  RegionalMinimaImageFilter filter;
  rt::RunReconstructionFilter<RegionalMinimaImageFilter>(filter, ds, inputPath, MinimaSetter(1.0, 0.0, true), /*fullyConnected=*/false, /*height=*/0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<uint32>>(outputPath).getDataStoreRef();
  REQUIRE(outStore.getValue(rt::FlatIndex(2, 2, 0, DX, DY)) == 1); // pit -> foreground
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, DX, DY)) == 1); // pit -> foreground
  REQUIRE(outStore.getValue(rt::FlatIndex(0, 0, 0, DX, DY)) == 0); // field -> background
}

// -----------------------------------------------------------------------------
// (3b) Both-algorithm-paths coverage: the SAME hand-computed central-pit oracle as test (3), run under BOTH the
//      in-core and out-of-core ALGORITHM paths on in-memory stores (selected by SIMPLNX_TEST_ALGORITHM_PATH) via
//      UnitTest::AlgorithmTestScope. The regional-extrema engine routes through DispatchAlgorithm (a non-flat input
//      skips the flat short-circuit and reaches the valued-extrema flood/sweep), so the scope proves both
//      implementations mark the pit. The hand-computed expected labels are the non-circular oracle.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::RegionalMinimaImageFilter: hand-computed pit marked (both algorithm paths)", "[ImageProcessing][RegionalMinimaImageFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  constexpr usize DX = 5, DY = 5, DZ = 1;
  std::vector<int32> pattern(DX * DY * DZ, 50);
  pattern[rt::FlatIndex(2, 2, 0, DX, DY)] = 10;
  pattern[rt::FlatIndex(3, 2, 0, DX, DY)] = 10;
  pattern[rt::FlatIndex(2, 3, 0, DX, DY)] = 10;
  pattern[rt::FlatIndex(3, 3, 0, DX, DY)] = 10;

  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));

  RegionalMinimaImageFilter filter;
  Arguments args;
  args.insertOrAssign(RegionalMinimaImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(RegionalMinimaImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(RegionalMinimaImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  MinimaSetter(1.0, 0.0, /*flatIsMinima=*/true)(args, /*fullyConnected=*/false, /*height=*/0.0);

  auto executeResult = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = ds.getDataRefAs<DataArray<uint32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  REQUIRE(outStore.getValue(rt::FlatIndex(2, 2, 0, DX, DY)) == 1); // pit -> foreground
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, DX, DY)) == 1); // pit -> foreground
  REQUIRE(outStore.getValue(rt::FlatIndex(0, 0, 0, DX, DY)) == 0); // field -> background
}

// -----------------------------------------------------------------------------
// (4) Preflight fg/bg range guard: for an integer input, an out-of-range Foreground Value is rejected; a valid one
//     preflights cleanly.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::RegionalMinimaImageFilter: preflight fg/bg range guard", "[ImageProcessing][RegionalMinimaImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<uint8>(ds, 8, 8, 8, rt::MakePlateauPattern<uint8>(8, 8, 8));

  auto makeArgs = [&](float64 fg) {
    Arguments args;
    args.insertOrAssign(RegionalMinimaImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(RegionalMinimaImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(RegionalMinimaImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(RegionalMinimaImageFilter::k_ForegroundValue_Key, std::make_any<float64>(fg));
    args.insertOrAssign(RegionalMinimaImageFilter::k_BackgroundValue_Key, std::make_any<float64>(0.0));
    args.insertOrAssign(RegionalMinimaImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
    args.insertOrAssign(RegionalMinimaImageFilter::k_FlatIsMinima_Key, std::make_any<bool>(true));
    return args;
  };

  RegionalMinimaImageFilter filter;

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

TEST_CASE("ImageProcessing::RegionalMinimaImageFilter: FromSIMPLJson", "[ImageProcessing][RegionalMinimaImageFilter]")
{
  const nlohmann::json json = {{"ForegroundValue", 5.0},
                               {"BackgroundValue", 2.0},
                               {"FullyConnected", true},
                               {"FlatIsMinima", false},
                               {"SelectedCellArrayPath", {{"Data Container Name", "Image Geometry"}, {"Attribute Matrix Name", "CellData"}, {"Data Array Name", "Input"}}},
                               {"NewCellArrayName", "RMin"}};

  Result<Arguments> result = RegionalMinimaImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments& args = result.value();
  REQUIRE(args.value<float64>(RegionalMinimaImageFilter::k_ForegroundValue_Key) == 5.0);
  REQUIRE(args.value<float64>(RegionalMinimaImageFilter::k_BackgroundValue_Key) == 2.0);
  REQUIRE(args.value<bool>(RegionalMinimaImageFilter::k_FullyConnected_Key) == true);
  REQUIRE(args.value<bool>(RegionalMinimaImageFilter::k_FlatIsMinima_Key) == false);
  REQUIRE(args.value<DataPath>(RegionalMinimaImageFilter::k_InputImageGeomPath_Key) == DataPath({"Image Geometry"}));
  REQUIRE(args.value<DataPath>(RegionalMinimaImageFilter::k_InputImageDataPath_Key) == DataPath({"Image Geometry", "CellData", "Input"}));
  REQUIRE(args.value<std::string>(RegionalMinimaImageFilter::k_OutputImageArrayName_Key) == "RMin");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image md5 golden -- duplicates ITKRegionalMinimaImageTest.cpp(defaults): cthead1.png (uint8),
// ITK-default params. Output is a fixed uint32 label image. (B) live-ITK bit-exact + (A) md5-validity-first.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::RegionalMinimaImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][RegionalMinimaImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<RegionalMinimaImageFilter>("cthead1.png", "3be99cf6b3116f16f6663cd2c4edb5b4");
}
