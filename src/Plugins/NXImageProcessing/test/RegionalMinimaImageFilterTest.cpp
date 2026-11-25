#include "ReconstructionFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/RegionalMinimaImageFilter.hpp"

#include "UnaryFilterTestUtils.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <array>
#include <limits>
#include <memory>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
constexpr int32 k_ValueOutOfRange = -8577; // kept in sync with RegionalMinimaImageFilter.cpp

// Build a param-setter that fixes fg/bg + FlatIsMinima and takes FullyConnected from the grid config.
rt::ReconParamSetterType MinimaSetter(float64 fg, float64 bg, bool flatIsMinima)
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
// (2) Flat-image special case: a completely flat image has no regional structure, so RegionalMinima is all
//     foreground when FlatIsMinima is On and all background when Off. The test checks both FlatIsMinima values
//     directly.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::RegionalMinimaImageFilter: flat-image special case", "[NXImageProcessing][RegionalMinimaImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  const bool flatIsMinima = GENERATE(false, true);
  CAPTURE(flatIsMinima);

  constexpr usize k_Dx = 8;
  constexpr usize k_Dy = 8;
  constexpr usize k_Dz = 4;
  const std::vector<uint8> flatImg(k_Dx * k_Dy * k_Dz, uint8{100});

  DataStructure newDs;
  const DataPath newInput = rt::BuildImageFromPattern<uint8>(newDs, k_Dx, k_Dy, k_Dz, flatImg);
  RegionalMinimaImageFilter newFilter;
  rt::RunReconstructionFilter<RegionalMinimaImageFilter>(newFilter, newDs, newInput, MinimaSetter(1.0, 0.0, flatIsMinima), /*fullyConnected=*/false, /*height=*/0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = newDs.getDataRefAs<DataArray<uint32>>(outputPath).getDataStoreRef();
  const uint32 expected = flatIsMinima ? uint32{1} : uint32{0};
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    REQUIRE(outStore.getValue(i) == expected);
  }
}

// -----------------------------------------------------------------------------
// (3) Hand-computed: on a small image with a central pit plateau, the pit becomes foreground and the surrounding
//     higher field becomes background.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::RegionalMinimaImageFilter: hand-computed pit marked", "[NXImageProcessing][RegionalMinimaImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_Dx = 5;
  constexpr usize k_Dy = 5;
  constexpr usize k_Dz = 1;
  std::vector<int32> pattern(k_Dx * k_Dy * k_Dz, 50);
  pattern[rt::FlatIndex(2, 2, 0, k_Dx, k_Dy)] = 10;
  pattern[rt::FlatIndex(3, 2, 0, k_Dx, k_Dy)] = 10;
  pattern[rt::FlatIndex(2, 3, 0, k_Dx, k_Dy)] = 10;
  pattern[rt::FlatIndex(3, 3, 0, k_Dx, k_Dy)] = 10;

  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_Dx, k_Dy, k_Dz, pattern);
  RegionalMinimaImageFilter filter;
  rt::RunReconstructionFilter<RegionalMinimaImageFilter>(filter, dataStructure, inputPath, MinimaSetter(1.0, 0.0, true), /*fullyConnected=*/false, /*height=*/0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = dataStructure.getDataRefAs<DataArray<uint32>>(outputPath).getDataStoreRef();
  REQUIRE(outStore.getValue(rt::FlatIndex(2, 2, 0, k_Dx, k_Dy)) == 1); // pit -> foreground
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, k_Dx, k_Dy)) == 1); // pit -> foreground
  REQUIRE(outStore.getValue(rt::FlatIndex(0, 0, 0, k_Dx, k_Dy)) == 0); // field -> background
}

// -----------------------------------------------------------------------------
// (3b) Both-algorithm-paths coverage: the SAME hand-computed central-pit oracle as test (3), run under BOTH the
//      in-core and out-of-core ALGORITHM paths on in-memory stores (selected by SIMPLNX_TEST_ALGORITHM_PATH) via
//      UnitTest::AlgorithmTestScope. The regional-extrema engine routes through DispatchAlgorithm (a non-flat input
//      skips the flat short-circuit and reaches the valued-extrema flood/sweep), so the scope proves both
//      implementations mark the pit. The hand-computed expected labels are the non-circular oracle.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::RegionalMinimaImageFilter: hand-computed pit marked (both algorithm paths)", "[NXImageProcessing][RegionalMinimaImageFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  constexpr usize k_Dx = 5;
  constexpr usize k_Dy = 5;
  constexpr usize k_Dz = 1;
  std::vector<int32> pattern(k_Dx * k_Dy * k_Dz, 50);
  pattern[rt::FlatIndex(2, 2, 0, k_Dx, k_Dy)] = 10;
  pattern[rt::FlatIndex(3, 2, 0, k_Dx, k_Dy)] = 10;
  pattern[rt::FlatIndex(2, 3, 0, k_Dx, k_Dy)] = 10;
  pattern[rt::FlatIndex(3, 3, 0, k_Dx, k_Dy)] = 10;

  unit_test::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_Dx, k_Dy, k_Dz, pattern);
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));

  RegionalMinimaImageFilter filter;
  Arguments args;
  args.insertOrAssign(RegionalMinimaImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(RegionalMinimaImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(RegionalMinimaImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  MinimaSetter(1.0, 0.0, /*flatIsMinima=*/true)(args, /*fullyConnected=*/false, /*height=*/0.0);

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = dataStructure.getDataRefAs<DataArray<uint32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  REQUIRE(outStore.getValue(rt::FlatIndex(2, 2, 0, k_Dx, k_Dy)) == 1); // pit -> foreground
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, k_Dx, k_Dy)) == 1); // pit -> foreground
  REQUIRE(outStore.getValue(rt::FlatIndex(0, 0, 0, k_Dx, k_Dy)) == 0); // field -> background
}

// -----------------------------------------------------------------------------
// (4) Preflight fg/bg range guard: for an integer input, an out-of-range Foreground Value is rejected; a valid one
//     preflights cleanly.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::RegionalMinimaImageFilter: preflight fg/bg range guard", "[NXImageProcessing][RegionalMinimaImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<uint8>(dataStructure, 8, 8, 8, rt::MakePlateauPattern<uint8>(8, 8, 8));

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
    auto result = filter.preflight(dataStructure, makeArgs(1.0));
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  }

  SECTION("out-of-range fg (negative, invalid for the uint32 output) is rejected")
  {
    auto result = filter.preflight(dataStructure, makeArgs(-1.0));
    REQUIRE(result.output_actions.invalid());
    REQUIRE(result.output_actions.errors().front().code == k_ValueOutOfRange);
  }
}

TEST_CASE("NXImageProcessing::RegionalMinimaImageFilter: FromSIMPLJson", "[NXImageProcessing][RegionalMinimaImageFilter]")
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
TEST_CASE("NXImageProcessing::RegionalMinimaImageFilter: ITK real-image golden (defaults)", "[NXImageProcessing][ItkGolden][RegionalMinimaImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<RegionalMinimaImageFilter>("cthead1.png", "3be99cf6b3116f16f6663cd2c4edb5b4");
}

TEST_CASE("NXImageProcessing::RegionalMinimaImageFilter: preflight rejects a non-scalar input", "[NXImageProcessing][RegionalMinimaImageFilter]")
{
  ip_test::RequirePreflightRejectsNonScalarInput<RegionalMinimaImageFilter, float32>();
}
