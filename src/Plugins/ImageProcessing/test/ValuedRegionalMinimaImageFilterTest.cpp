#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/ValuedRegionalMinimaImageFilter.hpp"

#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <array>
#include <limits>
#include <memory>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
// Legacy ITKValuedRegionalMinimaImageFilter, created at runtime by UUID.
const Uuid k_LegacyValuedRegionalMinimaUuid = *Uuid::FromString("38548e01-6a3c-49fb-b7b6-489f965cc61e");

// Live-parity configs: 3D + genuinely-2D (Z=1) multi-plateau images, each with FullyConnected off and on.
const std::vector<rt::ReconCase> k_ParityConfigs = {
    {"3D 12x12x12 fc=false", false, 0.0, 12, 12, 12}, {"3D 12x12x12 fc=true", true, 0.0, 12, 12, 12}, {"2D 20x16x1 fc=false", false, 0.0, 20, 16, 1}, {"2D 20x16x1 fc=true", true, 0.0, 20, 16, 1}};
} // namespace

// -----------------------------------------------------------------------------
// (1) Live-ITK EXACT parity grid across uint8 / int16 / float32 x FullyConnected {off, on} x 3D + 2D on a
//     multi-plateau image. Valued regional minima keeps regional-minima pixels at their value and sets everything
//     else to the type maximum -- a pure copy/select, so the new filter must reproduce the legacy ITK output
//     EXACTLY for integer AND float types (no tolerance).
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::ValuedRegionalMinimaImageFilter: Live-ITK exact parity grid", "[ImageProcessing][ValuedRegionalMinimaImageFilter]", uint8, int16, float32)
{
  using T = TestType;
  const auto build = [](DataStructure& ds, usize dx, usize dy, usize dz) { return rt::BuildImageFromPattern<T>(ds, dx, dy, dz, rt::MakePlateauPattern<T>(dx, dy, dz)); };
  rt::RunReconstructionParityGrid<ValuedRegionalMinimaImageFilter, T>(k_LegacyValuedRegionalMinimaUuid, build, k_ParityConfigs, rt::FullyConnectedParamSetter<ValuedRegionalMinimaImageFilter>());
}

// -----------------------------------------------------------------------------
// (2) Hand-computed behavior on a small 2D image: a central pit plateau (a regional minimum) is kept at its value;
//     the surrounding higher field is not a minimum (it has a lower neighbor via the connected flat zone) and is
//     set to the type maximum.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ValuedRegionalMinimaImageFilter: hand-computed central pit kept", "[ImageProcessing][ValuedRegionalMinimaImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 5, DY = 5, DZ = 1;
  constexpr int32 field = 50;
  constexpr int32 pit = 10;

  std::vector<int32> pattern(DX * DY * DZ, field);
  pattern[rt::FlatIndex(2, 2, 0, DX, DY)] = pit;
  pattern[rt::FlatIndex(3, 2, 0, DX, DY)] = pit;
  pattern[rt::FlatIndex(2, 3, 0, DX, DY)] = pit;
  pattern[rt::FlatIndex(3, 3, 0, DX, DY)] = pit;

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);

  ValuedRegionalMinimaImageFilter filter;
  rt::RunReconstructionFilter<ValuedRegionalMinimaImageFilter>(filter, ds, inputPath, rt::FullyConnectedParamSetter<ValuedRegionalMinimaImageFilter>(), /*fullyConnected=*/false, /*height=*/0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();
  const int32 markerMax = std::numeric_limits<int32>::max();

  REQUIRE(outStore.getValue(rt::FlatIndex(2, 2, 0, DX, DY)) == pit);       // pit plateau kept at value
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, DX, DY)) == pit);       // pit plateau kept at value
  REQUIRE(outStore.getValue(rt::FlatIndex(0, 0, 0, DX, DY)) == markerMax); // field is not a regional minimum
}

// -----------------------------------------------------------------------------
// (2b) Both-algorithm-paths coverage: the SAME hand-computed central-pit oracle as test (2), run under BOTH the
//      in-core and out-of-core ALGORITHM paths on in-memory stores (selected by SIMPLNX_TEST_ALGORITHM_PATH) via
//      UnitTest::AlgorithmTestScope. The valued regional-extrema engine routes through DispatchAlgorithm, so the
//      scope proves both implementations keep the pit plateau and mark the field as the type maximum. The
//      hand-computed expected values are the non-circular oracle (not the filter's own in-core run).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ValuedRegionalMinimaImageFilter: hand-computed central pit kept (both algorithm paths)", "[ImageProcessing][ValuedRegionalMinimaImageFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  constexpr usize DX = 5, DY = 5, DZ = 1;
  constexpr int32 field = 50;
  constexpr int32 pit = 10;

  std::vector<int32> pattern(DX * DY * DZ, field);
  pattern[rt::FlatIndex(2, 2, 0, DX, DY)] = pit;
  pattern[rt::FlatIndex(3, 2, 0, DX, DY)] = pit;
  pattern[rt::FlatIndex(2, 3, 0, DX, DY)] = pit;
  pattern[rt::FlatIndex(3, 3, 0, DX, DY)] = pit;

  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));

  ValuedRegionalMinimaImageFilter filter;
  Arguments args;
  args.insertOrAssign(ValuedRegionalMinimaImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(ValuedRegionalMinimaImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(ValuedRegionalMinimaImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  rt::FullyConnectedParamSetter<ValuedRegionalMinimaImageFilter>()(args, /*fullyConnected=*/false, /*height=*/0.0);

  auto executeResult = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = ds.getDataRefAs<DataArray<int32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  const int32 markerMax = std::numeric_limits<int32>::max();
  REQUIRE(outStore.getValue(rt::FlatIndex(2, 2, 0, DX, DY)) == pit);       // pit plateau kept at value
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, DX, DY)) == pit);       // pit plateau kept at value
  REQUIRE(outStore.getValue(rt::FlatIndex(0, 0, 0, DX, DY)) == markerMax); // field is not a regional minimum
}

// -----------------------------------------------------------------------------
// (3) A FLAT image has no strictly-more-extreme neighbor anywhere, so the valued regional minima result is the
//     input unchanged.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ValuedRegionalMinimaImageFilter: flat image unchanged", "[ImageProcessing][ValuedRegionalMinimaImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 6, DY = 5, DZ = 3;
  const std::vector<int32> pattern(DX * DY * DZ, 77);
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);

  ValuedRegionalMinimaImageFilter filter;
  rt::RunReconstructionFilter<ValuedRegionalMinimaImageFilter>(filter, ds, inputPath, rt::FullyConnectedParamSetter<ValuedRegionalMinimaImageFilter>(), /*fullyConnected=*/false, /*height=*/0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    REQUIRE(outStore.getValue(i) == 77);
  }
}

TEST_CASE("ImageProcessing::ValuedRegionalMinimaImageFilter: FromSIMPLJson", "[ImageProcessing][ValuedRegionalMinimaImageFilter]")
{
  const nlohmann::json json = {{"FullyConnected", true},
                               {"SelectedCellArrayPath", {{"Data Container Name", "Image Geometry"}, {"Attribute Matrix Name", "CellData"}, {"Data Array Name", "Input"}}},
                               {"NewCellArrayName", "VRMin"}};

  Result<Arguments> result = ValuedRegionalMinimaImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments& args = result.value();
  REQUIRE(args.value<bool>(ValuedRegionalMinimaImageFilter::k_FullyConnected_Key) == true);
  REQUIRE(args.value<DataPath>(ValuedRegionalMinimaImageFilter::k_InputImageGeomPath_Key) == DataPath({"Image Geometry"}));
  REQUIRE(args.value<DataPath>(ValuedRegionalMinimaImageFilter::k_InputImageDataPath_Key) == DataPath({"Image Geometry", "CellData", "Input"}));
  REQUIRE(args.value<std::string>(ValuedRegionalMinimaImageFilter::k_OutputImageArrayName_Key) == "VRMin");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image md5 golden -- duplicates ITKValuedRegionalMinimaImageTest.cpp(defaults): cthead1.png
// (uint8), ITK-default params. Value-preserving output (SameAsInput type uint8). (B) live-ITK bit-exact + (A)
// md5-validity-first.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ValuedRegionalMinimaImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][ValuedRegionalMinimaImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<ValuedRegionalMinimaImageFilter>("cthead1.png", "8297d018757b1477e31293ab8a8f0db1");
}
