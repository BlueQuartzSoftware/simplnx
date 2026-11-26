#include "ReconstructionFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/ValuedRegionalMaximaImageFilter.hpp"

#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <array>
#include <limits>
#include <memory>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

// -----------------------------------------------------------------------------
// (2) Hand-computed behavior on a small 2D image: a central peak plateau (a regional maximum) is kept at its value;
//     the surrounding lower field is not a maximum (it has a higher neighbor via the connected flat zone) and is
//     set to the type minimum.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::ValuedRegionalMaximaImageFilter: hand-computed central peak kept", "[NXImageProcessing][ValuedRegionalMaximaImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_Dx = 5;
  constexpr usize k_Dy = 5;
  constexpr usize k_Dz = 1;
  constexpr int32 k_Field = 10;
  constexpr int32 k_Peak = 50;

  std::vector<int32> pattern(k_Dx * k_Dy * k_Dz, k_Field);
  pattern[rt::FlatIndex(2, 2, 0, k_Dx, k_Dy)] = k_Peak;
  pattern[rt::FlatIndex(3, 2, 0, k_Dx, k_Dy)] = k_Peak;
  pattern[rt::FlatIndex(2, 3, 0, k_Dx, k_Dy)] = k_Peak;
  pattern[rt::FlatIndex(3, 3, 0, k_Dx, k_Dy)] = k_Peak;

  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_Dx, k_Dy, k_Dz, pattern);

  ValuedRegionalMaximaImageFilter filter;
  rt::RunReconstructionFilter<ValuedRegionalMaximaImageFilter>(filter, dataStructure, inputPath, rt::FullyConnectedParamSetter<ValuedRegionalMaximaImageFilter>(), /*fullyConnected=*/false,
                                                               /*height=*/0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = dataStructure.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();
  const int32 markerMin = std::numeric_limits<int32>::lowest();

  REQUIRE(outStore.getValue(rt::FlatIndex(2, 2, 0, k_Dx, k_Dy)) == k_Peak);    // peak plateau kept at value
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, k_Dx, k_Dy)) == k_Peak);    // peak plateau kept at value
  REQUIRE(outStore.getValue(rt::FlatIndex(0, 0, 0, k_Dx, k_Dy)) == markerMin); // field is not a regional maximum
}

// -----------------------------------------------------------------------------
// (2b) Both-algorithm-paths coverage: the SAME hand-computed central-peak oracle as test (2), run under BOTH the
//      in-core and out-of-core ALGORITHM paths on in-memory stores (selected by SIMPLNX_TEST_ALGORITHM_PATH) via
//      UnitTest::AlgorithmTestScope. The valued regional-extrema engine routes through DispatchAlgorithm, so the
//      scope proves both implementations keep the peak plateau and mark the field as the type minimum. The
//      hand-computed expected values are the non-circular oracle (not the filter's own in-core run).
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::ValuedRegionalMaximaImageFilter: hand-computed central peak kept (both algorithm paths)", "[NXImageProcessing][ValuedRegionalMaximaImageFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  constexpr usize k_Dx = 5;
  constexpr usize k_Dy = 5;
  constexpr usize k_Dz = 1;
  constexpr int32 k_Field = 10;
  constexpr int32 k_Peak = 50;

  std::vector<int32> pattern(k_Dx * k_Dy * k_Dz, k_Field);
  pattern[rt::FlatIndex(2, 2, 0, k_Dx, k_Dy)] = k_Peak;
  pattern[rt::FlatIndex(3, 2, 0, k_Dx, k_Dy)] = k_Peak;
  pattern[rt::FlatIndex(2, 3, 0, k_Dx, k_Dy)] = k_Peak;
  pattern[rt::FlatIndex(3, 3, 0, k_Dx, k_Dy)] = k_Peak;

  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_Dx, k_Dy, k_Dz, pattern);
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));

  ValuedRegionalMaximaImageFilter filter;
  Arguments args;
  args.insertOrAssign(ValuedRegionalMaximaImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(ValuedRegionalMaximaImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(ValuedRegionalMaximaImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  rt::FullyConnectedParamSetter<ValuedRegionalMaximaImageFilter>()(args, /*fullyConnected=*/false, /*height=*/0.0);

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = dataStructure.getDataRefAs<DataArray<int32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  const int32 markerMin = std::numeric_limits<int32>::lowest();
  REQUIRE(outStore.getValue(rt::FlatIndex(2, 2, 0, k_Dx, k_Dy)) == k_Peak);    // peak plateau kept at value
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 3, 0, k_Dx, k_Dy)) == k_Peak);    // peak plateau kept at value
  REQUIRE(outStore.getValue(rt::FlatIndex(0, 0, 0, k_Dx, k_Dy)) == markerMin); // field is not a regional maximum
}

// -----------------------------------------------------------------------------
// (3) A FLAT image has no strictly-more-extreme neighbor anywhere, so the valued regional maxima result is the
//     input unchanged.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::ValuedRegionalMaximaImageFilter: flat image unchanged", "[NXImageProcessing][ValuedRegionalMaximaImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_Dx = 6;
  constexpr usize k_Dy = 5;
  constexpr usize k_Dz = 3;
  const std::vector<int32> pattern(k_Dx * k_Dy * k_Dz, 77);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_Dx, k_Dy, k_Dz, pattern);

  ValuedRegionalMaximaImageFilter filter;
  rt::RunReconstructionFilter<ValuedRegionalMaximaImageFilter>(filter, dataStructure, inputPath, rt::FullyConnectedParamSetter<ValuedRegionalMaximaImageFilter>(), /*fullyConnected=*/false,
                                                               /*height=*/0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = dataStructure.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    REQUIRE(outStore.getValue(i) == 77);
  }
}

TEST_CASE("NXImageProcessing::ValuedRegionalMaximaImageFilter: FromSIMPLJson", "[NXImageProcessing][ValuedRegionalMaximaImageFilter]")
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
TEST_CASE("NXImageProcessing::ValuedRegionalMaximaImageFilter: ITK real-image golden (defaults)", "[NXImageProcessing][ItkGolden][ValuedRegionalMaximaImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<ValuedRegionalMaximaImageFilter>("cthead1.png", "c94b3702844c508818e4718a75102472");
}
