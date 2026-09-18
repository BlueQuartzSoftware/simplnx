#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/ValuedRegionalMaximaImageFilter.hpp"

#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <array>
#include <limits>
#include <memory>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
// Legacy ITKValuedRegionalMaximaImageFilter, created at runtime by UUID.
const Uuid k_LegacyValuedRegionalMaximaUuid = *Uuid::FromString("2c0bb4f6-69fe-4c43-a32e-21b4b11efcff");

// Live-parity configs: 3D + genuinely-2D (Z=1) multi-plateau images, each with FullyConnected off and on.
const std::vector<rt::ReconCase> k_ParityConfigs = {
    {"3D 12x12x12 fc=false", false, 0.0, 12, 12, 12}, {"3D 12x12x12 fc=true", true, 0.0, 12, 12, 12}, {"2D 20x16x1 fc=false", false, 0.0, 20, 16, 1}, {"2D 20x16x1 fc=true", true, 0.0, 20, 16, 1}};
} // namespace

// -----------------------------------------------------------------------------
// (1) Live-ITK EXACT parity grid across uint8 / int16 / float32 x FullyConnected {off, on} x 3D + 2D on a
//     multi-plateau image. Valued regional maxima keeps regional-maxima pixels at their value and sets everything
//     else to the type minimum -- a pure copy/select, so the new filter must reproduce the legacy ITK output
//     EXACTLY for integer AND float types (no tolerance).
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::ValuedRegionalMaximaImageFilter: Live-ITK exact parity grid", "[ImageProcessing][ValuedRegionalMaximaImageFilter]", uint8, int16, float32)
{
  using T = TestType;
  const auto build = [](DataStructure& ds, usize dx, usize dy, usize dz) { return rt::BuildImageFromPattern<T>(ds, dx, dy, dz, rt::MakePlateauPattern<T>(dx, dy, dz)); };
  rt::RunReconstructionParityGrid<ValuedRegionalMaximaImageFilter, T>(k_LegacyValuedRegionalMaximaUuid, build, k_ParityConfigs, rt::FullyConnectedParamSetter<ValuedRegionalMaximaImageFilter>());
}

// -----------------------------------------------------------------------------
// (2) Hand-computed behavior on a small 2D image: a central peak plateau (a regional maximum) is kept at its value;
//     the surrounding lower field is not a maximum (it has a higher neighbor via the connected flat zone) and is
//     set to the type minimum.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ValuedRegionalMaximaImageFilter: hand-computed central peak kept", "[ImageProcessing][ValuedRegionalMaximaImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 5, DY = 5, DZ = 1;
  constexpr int32 field = 10;
  constexpr int32 peak = 50;

  std::vector<int32> pattern(DX * DY * DZ, field);
  pattern[rt::FlatIndex(2, 2, 0, DX, DY)] = peak;
  pattern[rt::FlatIndex(3, 2, 0, DX, DY)] = peak;
  pattern[rt::FlatIndex(2, 3, 0, DX, DY)] = peak;
  pattern[rt::FlatIndex(3, 3, 0, DX, DY)] = peak;

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);

  ValuedRegionalMaximaImageFilter filter;
  rt::RunReconstructionFilter<ValuedRegionalMaximaImageFilter>(filter, ds, inputPath, rt::FullyConnectedParamSetter<ValuedRegionalMaximaImageFilter>(), /*fullyConnected=*/false, /*height=*/0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();
  const int32 markerMin = std::numeric_limits<int32>::lowest();

  REQUIRE(outStore.getValue(rt::FlatIndex(2, 2, 0, DX, DY)) == peak);      // peak plateau kept at value
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, DX, DY)) == peak);      // peak plateau kept at value
  REQUIRE(outStore.getValue(rt::FlatIndex(0, 0, 0, DX, DY)) == markerMin); // field is not a regional maximum
}

// -----------------------------------------------------------------------------
// (2b) Both-algorithm-paths coverage: the SAME hand-computed central-peak oracle as test (2), run under BOTH the
//      in-core and out-of-core ALGORITHM paths on in-memory stores (selected by SIMPLNX_TEST_ALGORITHM_PATH) via
//      UnitTest::AlgorithmTestScope. The valued regional-extrema engine routes through DispatchAlgorithm, so the
//      scope proves both implementations keep the peak plateau and mark the field as the type minimum. The
//      hand-computed expected values are the non-circular oracle (not the filter's own in-core run).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ValuedRegionalMaximaImageFilter: hand-computed central peak kept (both algorithm paths)", "[ImageProcessing][ValuedRegionalMaximaImageFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  constexpr usize DX = 5, DY = 5, DZ = 1;
  constexpr int32 field = 10;
  constexpr int32 peak = 50;

  std::vector<int32> pattern(DX * DY * DZ, field);
  pattern[rt::FlatIndex(2, 2, 0, DX, DY)] = peak;
  pattern[rt::FlatIndex(3, 2, 0, DX, DY)] = peak;
  pattern[rt::FlatIndex(2, 3, 0, DX, DY)] = peak;
  pattern[rt::FlatIndex(3, 3, 0, DX, DY)] = peak;

  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));

  ValuedRegionalMaximaImageFilter filter;
  Arguments args;
  args.insertOrAssign(ValuedRegionalMaximaImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(ValuedRegionalMaximaImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(ValuedRegionalMaximaImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  rt::FullyConnectedParamSetter<ValuedRegionalMaximaImageFilter>()(args, /*fullyConnected=*/false, /*height=*/0.0);

  auto executeResult = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = ds.getDataRefAs<DataArray<int32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  const int32 markerMin = std::numeric_limits<int32>::lowest();
  REQUIRE(outStore.getValue(rt::FlatIndex(2, 2, 0, DX, DY)) == peak);      // peak plateau kept at value
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, DX, DY)) == peak);      // peak plateau kept at value
  REQUIRE(outStore.getValue(rt::FlatIndex(0, 0, 0, DX, DY)) == markerMin); // field is not a regional maximum
}

// -----------------------------------------------------------------------------
// (3) A FLAT image has no strictly-more-extreme neighbor anywhere, so the valued regional maxima result is the
//     input unchanged.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ValuedRegionalMaximaImageFilter: flat image unchanged", "[ImageProcessing][ValuedRegionalMaximaImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 6, DY = 5, DZ = 3;
  const std::vector<int32> pattern(DX * DY * DZ, 77);
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);

  ValuedRegionalMaximaImageFilter filter;
  rt::RunReconstructionFilter<ValuedRegionalMaximaImageFilter>(filter, ds, inputPath, rt::FullyConnectedParamSetter<ValuedRegionalMaximaImageFilter>(), /*fullyConnected=*/false, /*height=*/0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    REQUIRE(outStore.getValue(i) == 77);
  }
}

TEST_CASE("ImageProcessing::ValuedRegionalMaximaImageFilter: FromSIMPLJson", "[ImageProcessing][ValuedRegionalMaximaImageFilter]")
{
  const nlohmann::json json = {{"FullyConnected", true},
                               {"SelectedCellArrayPath", {{"Data Container Name", "Image Geometry"}, {"Attribute Matrix Name", "CellData"}, {"Data Array Name", "Input"}}},
                               {"NewCellArrayName", "VRMax"}};

  Result<Arguments> result = ValuedRegionalMaximaImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments& args = result.value();
  REQUIRE(args.value<bool>(ValuedRegionalMaximaImageFilter::k_FullyConnected_Key) == true);
  REQUIRE(args.value<DataPath>(ValuedRegionalMaximaImageFilter::k_InputImageGeomPath_Key) == DataPath({"Image Geometry"}));
  REQUIRE(args.value<DataPath>(ValuedRegionalMaximaImageFilter::k_InputImageDataPath_Key) == DataPath({"Image Geometry", "CellData", "Input"}));
  REQUIRE(args.value<std::string>(ValuedRegionalMaximaImageFilter::k_OutputImageArrayName_Key) == "VRMax");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image md5 golden -- duplicates ITKValuedRegionalMaximaImageTest.cpp(defaults): cthead1.png
// (uint8), ITK-default params. Valued regional maxima is a value-preserving output (SameAsInput type uint8). (B)
// live-ITK bit-exact + (A) md5-validity-first.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ValuedRegionalMaximaImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][ValuedRegionalMaximaImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<ValuedRegionalMaximaImageFilter>("cthead1.png", "c94b3702844c508818e4718a75102472");
}
