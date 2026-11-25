#include "ReconstructionFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/ValuedRegionalMinimaImageFilter.hpp"

#include "UnaryFilterTestUtils.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <array>
#include <limits>
#include <memory>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

// -----------------------------------------------------------------------------
// (2) Hand-computed behavior on a small 2D image: a central pit plateau (a regional minimum) is kept at its value;
//     the surrounding higher field is not a minimum (it has a lower neighbor via the connected flat zone) and is
//     set to the type maximum.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::ValuedRegionalMinimaImageFilter: hand-computed central pit kept", "[NXImageProcessing][ValuedRegionalMinimaImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_Dx = 5;
  constexpr usize k_Dy = 5;
  constexpr usize k_Dz = 1;
  constexpr int32 k_Field = 50;
  constexpr int32 k_Pit = 10;

  std::vector<int32> pattern(k_Dx * k_Dy * k_Dz, k_Field);
  pattern[rt::FlatIndex(2, 2, 0, k_Dx, k_Dy)] = k_Pit;
  pattern[rt::FlatIndex(3, 2, 0, k_Dx, k_Dy)] = k_Pit;
  pattern[rt::FlatIndex(2, 3, 0, k_Dx, k_Dy)] = k_Pit;
  pattern[rt::FlatIndex(3, 3, 0, k_Dx, k_Dy)] = k_Pit;

  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_Dx, k_Dy, k_Dz, pattern);

  ValuedRegionalMinimaImageFilter filter;
  rt::RunReconstructionFilter<ValuedRegionalMinimaImageFilter>(filter, dataStructure, inputPath, rt::FullyConnectedParamSetter<ValuedRegionalMinimaImageFilter>(), /*fullyConnected=*/false,
                                                               /*height=*/0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = dataStructure.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();
  const int32 markerMax = std::numeric_limits<int32>::max();

  REQUIRE(outStore.getValue(rt::FlatIndex(2, 2, 0, k_Dx, k_Dy)) == k_Pit);     // pit plateau kept at value
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, k_Dx, k_Dy)) == k_Pit);     // pit plateau kept at value
  REQUIRE(outStore.getValue(rt::FlatIndex(0, 0, 0, k_Dx, k_Dy)) == markerMax); // field is not a regional minimum
}

// -----------------------------------------------------------------------------
// (2b) Both-algorithm-paths coverage: the SAME hand-computed central-pit oracle as test (2), run under BOTH the
//      in-core and out-of-core ALGORITHM paths on in-memory stores (selected by SIMPLNX_TEST_ALGORITHM_PATH) via
//      UnitTest::AlgorithmTestScope. The valued regional-extrema engine routes through DispatchAlgorithm, so the
//      scope proves both implementations keep the pit plateau and mark the field as the type maximum. The
//      hand-computed expected values are the non-circular oracle (not the filter's own in-core run).
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::ValuedRegionalMinimaImageFilter: hand-computed central pit kept (both algorithm paths)", "[NXImageProcessing][ValuedRegionalMinimaImageFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  constexpr usize k_Dx = 5;
  constexpr usize k_Dy = 5;
  constexpr usize k_Dz = 1;
  constexpr int32 k_Field = 50;
  constexpr int32 k_Pit = 10;

  std::vector<int32> pattern(k_Dx * k_Dy * k_Dz, k_Field);
  pattern[rt::FlatIndex(2, 2, 0, k_Dx, k_Dy)] = k_Pit;
  pattern[rt::FlatIndex(3, 2, 0, k_Dx, k_Dy)] = k_Pit;
  pattern[rt::FlatIndex(2, 3, 0, k_Dx, k_Dy)] = k_Pit;
  pattern[rt::FlatIndex(3, 3, 0, k_Dx, k_Dy)] = k_Pit;

  unit_test::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_Dx, k_Dy, k_Dz, pattern);
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));

  ValuedRegionalMinimaImageFilter filter;
  Arguments args;
  args.insertOrAssign(ValuedRegionalMinimaImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(ValuedRegionalMinimaImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(ValuedRegionalMinimaImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  rt::FullyConnectedParamSetter<ValuedRegionalMinimaImageFilter>()(args, /*fullyConnected=*/false, /*height=*/0.0);

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = dataStructure.getDataRefAs<DataArray<int32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  const int32 markerMax = std::numeric_limits<int32>::max();
  REQUIRE(outStore.getValue(rt::FlatIndex(2, 2, 0, k_Dx, k_Dy)) == k_Pit);     // pit plateau kept at value
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, k_Dx, k_Dy)) == k_Pit);     // pit plateau kept at value
  REQUIRE(outStore.getValue(rt::FlatIndex(0, 0, 0, k_Dx, k_Dy)) == markerMax); // field is not a regional minimum
}

// -----------------------------------------------------------------------------
// (3) A FLAT image has no strictly-more-extreme neighbor anywhere, so the valued regional minima result is the
//     input unchanged.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::ValuedRegionalMinimaImageFilter: flat image unchanged", "[NXImageProcessing][ValuedRegionalMinimaImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_Dx = 6;
  constexpr usize k_Dy = 5;
  constexpr usize k_Dz = 3;
  const std::vector<int32> pattern(k_Dx * k_Dy * k_Dz, 77);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_Dx, k_Dy, k_Dz, pattern);

  ValuedRegionalMinimaImageFilter filter;
  rt::RunReconstructionFilter<ValuedRegionalMinimaImageFilter>(filter, dataStructure, inputPath, rt::FullyConnectedParamSetter<ValuedRegionalMinimaImageFilter>(), /*fullyConnected=*/false,
                                                               /*height=*/0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = dataStructure.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    REQUIRE(outStore.getValue(i) == 77);
  }
}

TEST_CASE("NXImageProcessing::ValuedRegionalMinimaImageFilter: FromSIMPLJson", "[NXImageProcessing][ValuedRegionalMinimaImageFilter]")
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
TEST_CASE("NXImageProcessing::ValuedRegionalMinimaImageFilter: ITK real-image golden (defaults)", "[NXImageProcessing][ItkGolden][ValuedRegionalMinimaImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<ValuedRegionalMinimaImageFilter>("cthead1.png", "8297d018757b1477e31293ab8a8f0db1");
}

TEST_CASE("NXImageProcessing::ValuedRegionalMinimaImageFilter: preflight rejects a non-scalar input", "[NXImageProcessing][ValuedRegionalMinimaImageFilter]")
{
  ip_test::RequirePreflightRejectsNonScalarInput<ValuedRegionalMinimaImageFilter, float32>();
}
