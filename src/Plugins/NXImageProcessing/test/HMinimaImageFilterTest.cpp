#include "ImageFilterIntegrationTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/HMinimaImageFilter.hpp"
#include "NXImageProcessing/Filters/MorphologicalWatershedFromMarkersImageFilter.hpp"
#include "NXImageProcessing/Filters/RegionalMinimaImageFilter.hpp"

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/UnitTest/AlgorithmTestScope.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <any>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
// Filter-local error codes for the Height guards (kept in sync with HMinimaImageFilter.cpp).
constexpr int32 k_NonFiniteHeight = -8552;
constexpr int32 k_NegativeHeight = -8555;

Arguments makeHMinimaArguments(const DataPath& inputPath, float64 height, bool fullyConnected = false)
{
  auto args = image_integration::ImageArguments(HMinimaImageFilter{}, inputPath, "Output");
  args.insertOrAssign(HMinimaImageFilter::k_Height_Key, std::make_any<float64>(height));
  args.insertOrAssign(HMinimaImageFilter::k_FullyConnected_Key, std::make_any<bool>(fullyConnected));
  return args;
}
} // namespace

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
TEMPLATE_TEST_CASE("NXImageProcessing::HMinimaImageFilter: integer fractional Height fails preflight", "[NXImageProcessing][Issue1874][HMinimaImageFilter]", int8, uint8, int16, uint16, int32, uint32,
                   int64, uint64)
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  const float64 height = GENERATE(0.5, 2.5);
  CAPTURE(height);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 3, 1, 1, {0, 2, 0});
  const HMinimaImageFilter filter;
  const auto wholeHeightPreflightResult = filter.preflight(dataStructure, makeHMinimaArguments(inputPath, 2.0));
  SIMPLNX_RESULT_REQUIRE_VALID(wholeHeightPreflightResult.output_actions);

  const Arguments args = makeHMinimaArguments(inputPath, height);
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions);
  REQUIRE(preflightResult.output_actions.errors().size() == 1);
  REQUIRE(preflightResult.output_actions.errors().front().code == -29201);
  REQUIRE(preflightResult.output_actions.errors().front().message.find(inputPath.toString()) != std::string::npos);
  REQUIRE(preflightResult.output_actions.errors().front().message.find(height == 0.5 ? "0.5" : "2.5") != std::string::npos);
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors().front().code == preflightResult.output_actions.errors().front().code);
  REQUIRE(dataStructure.getData(inputPath.replaceName("Output")) == nullptr);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
TEMPLATE_TEST_CASE("NXImageProcessing::HMinimaImageFilter: zero Height preserves values and type", "[NXImageProcessing][Issue1874][HMinimaImageFilter]", int8, uint8, int16, uint16, int32, uint32,
                   int64, uint64, float32, float64)
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);
  const bool fullyConnected = GENERATE(false, true);
  CAPTURE(fullyConnected);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 3, 2, 1, {1, 3, 2, 4, 0, 5});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(inputPath));
  algorithmTestScope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));
  const Arguments args = makeHMinimaArguments(inputPath, 0.0, fullyConnected);
  const HMinimaImageFilter filter;
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  REQUIRE(preflightResult.output_actions.value().actions.size() == 1);
  const auto* actionPtr = dynamic_cast<const CreateArrayAction*>(preflightResult.output_actions.value().actions.front().get());
  REQUIRE(actionPtr != nullptr);
  REQUIRE(actionPtr->type() == GetDataType<TestType>());
  REQUIRE(actionPtr->dims() == ShapeType{1, 2, 3});
  REQUIRE(actionPtr->componentDims() == ShapeType{1});

  const auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  // At zero height the marker equals the mask, so reconstruction preserves every input value.
  const auto actual = image_integration::Read<TestType>(dataStructure, inputPath.replaceName("Output"), {1, 2, 3}, algorithmTestScope);
  REQUIRE(actual == std::vector<TestType>{1, 3, 2, 4, 0, 5});
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
TEMPLATE_TEST_CASE("NXImageProcessing::HMinimaImageFilter: floating fractional Height retains fractional minima", "[NXImageProcessing][Issue1874][HMinimaImageFilter]", float32, float64)
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);
  const bool fullyConnected = GENERATE(false, true);
  CAPTURE(fullyConnected);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 3, 3, 1, {2, 2, 2, 2, 0, 2, 2, 2, 2});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(inputPath));
  algorithmTestScope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));
  const Arguments args = makeHMinimaArguments(inputPath, 0.5, fullyConnected);
  const HMinimaImageFilter filter;
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  const auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  // Erosion raises the isolated minimum by 0.5 and leaves the surrounding plateau at 2.
  const auto actual = image_integration::Read<TestType>(dataStructure, inputPath.replaceName("Output"), {1, 3, 3}, algorithmTestScope);
  REQUIRE(actual == std::vector<TestType>{2, 2, 2, 2, 0.5, 2, 2, 2, 2});
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
TEMPLATE_TEST_CASE("NXImageProcessing::HMinimaImageFilter: native 64-bit marker preserves units above double precision", "[NXImageProcessing][Issue1874][HMinimaImageFilter]", int64, uint64)
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);
  constexpr TestType inputValue = TestType{1} << 54;
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 1, 1, 1, {inputValue});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(inputPath));
  algorithmTestScope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));
  const HMinimaImageFilter filter;
  const Arguments args = makeHMinimaArguments(inputPath, 1.0);
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  const auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  // Without neighbors, reconstruction preserves the one-pixel marker exactly.
  const auto actual = image_integration::Read<TestType>(dataStructure, inputPath.replaceName("Output"), {1, 1, 1}, algorithmTestScope);
  REQUIRE(actual == std::vector<TestType>{inputValue + TestType{1}});
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
TEMPLATE_TEST_CASE("NXImageProcessing::HMinimaImageFilter: zero Height preserves native integer bounds", "[NXImageProcessing][Issue1874][HMinimaImageFilter]", int8, uint8, int16, uint16, int32,
                   uint32, int64, uint64)
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool fullyConnected = GENERATE(false, true);
  CAPTURE(scenario, fullyConnected);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);
  constexpr TestType minimum = std::numeric_limits<TestType>::lowest();
  constexpr TestType maximum = std::numeric_limits<TestType>::max();
  const std::vector<TestType> expected = {minimum, minimum + TestType{1}, maximum - TestType{1}, maximum};
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 2, 2, 1, {minimum, minimum + TestType{1}, maximum - TestType{1}, maximum});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(inputPath));
  algorithmTestScope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));
  const HMinimaImageFilter filter;
  const auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, makeHMinimaArguments(inputPath, 0.0, fullyConnected));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const auto actual = image_integration::Read<TestType>(dataStructure, inputPath.replaceName("Output"), {1, 2, 2}, algorithmTestScope);
  REQUIRE(actual == expected);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
TEMPLATE_TEST_CASE("NXImageProcessing::HMinimaImageFilter: whole Height raises or saturates native integer markers", "[NXImageProcessing][Issue1874][HMinimaImageFilter]", int8, uint8, int16, uint16,
                   int32, uint32, int64, uint64)
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool fullyConnected = GENERATE(false, true);
  const bool atMaximum = GENERATE(false, true);
  const float64 height = GENERATE(1.0, std::ldexp(1.0, 64), std::numeric_limits<float64>::max());
  CAPTURE(scenario, fullyConnected, atMaximum, height);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);
  constexpr TestType minimum = std::numeric_limits<TestType>::lowest();
  constexpr TestType maximum = std::numeric_limits<TestType>::max();
  const TestType inputValue = atMaximum ? maximum : minimum;
  // A single pixel retains its marker. One raises MIN by one; a Height of at least 2^64 saturates every integer type.
  const TestType expected = atMaximum || height > 1.0 ? maximum : static_cast<TestType>(minimum + TestType{1});
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 1, 1, 1, {inputValue});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(inputPath));
  algorithmTestScope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));
  const HMinimaImageFilter filter;
  const auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, makeHMinimaArguments(inputPath, height, fullyConnected));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const auto actual = image_integration::Read<TestType>(dataStructure, inputPath.replaceName("Output"), {1, 1, 1}, algorithmTestScope);
  REQUIRE(actual == std::vector<TestType>{expected});
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
TEMPLATE_TEST_CASE("NXImageProcessing::HMinimaImageFilter: large whole Height retains exact native 64-bit markers", "[NXImageProcessing][Issue1874][HMinimaImageFilter]", int64, uint64)
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool fullyConnected = GENERATE(false, true);
  const float64 height = GENERATE(std::ldexp(1.0, 63), 0x1.fffffffffffffp63);
  CAPTURE(scenario, fullyConnected, height);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 1, 1, 1, {std::numeric_limits<TestType>::lowest()});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(inputPath));
  algorithmTestScope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));
  const HMinimaImageFilter filter;
  const auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, makeHMinimaArguments(inputPath, height, fullyConnected));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  // The second Height is exactly 2^64 - 2048, the largest float64 below 2^64.
  TestType expected = 0;
  if constexpr(std::is_signed_v<TestType>)
  {
    expected = height == std::ldexp(1.0, 63) ? int64{0} : int64{9223372036854773760LL};
  }
  else
  {
    expected = height == std::ldexp(1.0, 63) ? uint64{9223372036854775808ULL} : uint64{18446744073709549568ULL};
  }
  const auto actual = image_integration::Read<TestType>(dataStructure, inputPath.replaceName("Output"), {1, 1, 1}, algorithmTestScope);
  REQUIRE(actual == std::vector<TestType>{expected});
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
TEST_CASE("NXImageProcessing::HMinimaImageFilter: whole Height is stable under a signed baseline shift", "[NXImageProcessing][Issue1874][HMinimaImageFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool fullyConnected = GENERATE(false, true);
  const bool positiveBaseline = GENERATE(false, true);
  CAPTURE(scenario, fullyConnected, positiveBaseline);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);
  const std::vector<int32> input = positiveBaseline ? std::vector<int32>{50, 30, 50} : std::vector<int32>{-50, -70, -50};
  const std::vector<int32> expected = positiveBaseline ? std::vector<int32>{50, 40, 50} : std::vector<int32>{-50, -60, -50};
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, 3, 1, 1, input);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(inputPath));
  algorithmTestScope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));
  const HMinimaImageFilter filter;
  const auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, makeHMinimaArguments(inputPath, 10.0, fullyConnected));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const auto actual = image_integration::Read<int32>(dataStructure, inputPath.replaceName("Output"), {1, 1, 3}, algorithmTestScope);
  REQUIRE(actual == expected);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// -----------------------------------------------------------------------------
// (2) Hand-computed behavior on a small 2D image: a flat background with two isolated dark dents. The dent
//     whose contrast (>= Height) survives is raised by exactly Height; the dent whose contrast (< Height) is
//     filled up to the surrounding background; the background is left unchanged.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::HMinimaImageFilter: hand-computed minima suppression", "[NXImageProcessing][HMinimaImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_Dx = 9;
  constexpr usize k_Dy = 9;
  constexpr usize k_Dz = 1;
  constexpr int32 k_Background = 50;
  constexpr int32 k_DeepDent = 30;    // contrast 20 (>= Height) -> survives, raised by Height to 40
  constexpr int32 k_ShallowDent = 45; // contrast 5  (<  Height) -> filled up to background (50)
  constexpr float64 k_Height = 10.0;

  std::vector<int32> pattern(k_Dx * k_Dy * k_Dz, k_Background);
  pattern[rt::FlatIndex(2, 2, 0, k_Dx, k_Dy)] = k_DeepDent;    // isolated regional minimum (all face-neighbors are background)
  pattern[rt::FlatIndex(6, 6, 0, k_Dx, k_Dy)] = k_ShallowDent; // isolated regional minimum, separated from the deep dent by background

  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_Dx, k_Dy, k_Dz, pattern);

  HMinimaImageFilter filter;
  rt::RunReconstructionFilter<HMinimaImageFilter>(filter, dataStructure, inputPath, rt::HeightAndFullyConnectedParamSetter<HMinimaImageFilter>(), /*fullyConnected=*/false, k_Height);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<int32>>(outputPath));
  const auto& outStore = dataStructure.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();

  REQUIRE(outStore.getValue(rt::FlatIndex(2, 2, 0, k_Dx, k_Dy)) == k_DeepDent + static_cast<int32>(k_Height)); // 30 -> 40 (raised by Height)
  REQUIRE(outStore.getValue(rt::FlatIndex(6, 6, 0, k_Dx, k_Dy)) == k_Background);                              // 45 -> 50 (shallow minimum removed)
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 2, 0, k_Dx, k_Dy)) == k_Background);                              // neighbor of the deep dent, unchanged
  REQUIRE(outStore.getValue(rt::FlatIndex(4, 4, 0, k_Dx, k_Dy)) == k_Background);                              // untouched background voxel
}

// -----------------------------------------------------------------------------
// (2b) Both-algorithm-paths coverage: the SAME hand-computed minima-suppression oracle as test (2), run under BOTH
//      the in-core and out-of-core reconstruction ALGORITHM paths on in-memory stores (selected by
//      SIMPLNX_TEST_ALGORITHM_PATH) via UnitTest::AlgorithmTestScope. HMinima's grayscale reconstruction engine
//      routes through DispatchAlgorithm, so the scope proves both implementations produce the correct suppression;
//      the hand-computed expected values are the non-circular oracle (not the filter's own in-core run).
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::HMinimaImageFilter: hand-computed minima suppression (both algorithm paths)", "[NXImageProcessing][HMinimaImageFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  constexpr usize k_Dx = 9;
  constexpr usize k_Dy = 9;
  constexpr usize k_Dz = 1;
  constexpr int32 k_Background = 50;
  constexpr int32 k_DeepDent = 30;    // contrast 20 (>= Height) -> survives, raised by Height to 40
  constexpr int32 k_ShallowDent = 45; // contrast 5  (<  Height) -> filled up to background (50)
  constexpr float64 k_Height = 10.0;

  std::vector<int32> pattern(k_Dx * k_Dy * k_Dz, k_Background);
  pattern[rt::FlatIndex(2, 2, 0, k_Dx, k_Dy)] = k_DeepDent;    // isolated regional minimum (all face-neighbors are background)
  pattern[rt::FlatIndex(6, 6, 0, k_Dx, k_Dy)] = k_ShallowDent; // isolated regional minimum, separated from the deep dent by background

  unit_test::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_Dx, k_Dy, k_Dz, pattern);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(inputPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));

  HMinimaImageFilter filter;
  Arguments args;
  args.insertOrAssign(HMinimaImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(HMinimaImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(HMinimaImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(HMinimaImageFilter::k_Height_Key, std::make_any<float64>(k_Height));
  args.insertOrAssign(HMinimaImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<int32>>(outputPath));
  auto& outArray = dataStructure.getDataRefAs<DataArray<int32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  REQUIRE(outStore.getValue(rt::FlatIndex(2, 2, 0, k_Dx, k_Dy)) == k_DeepDent + static_cast<int32>(k_Height)); // 30 -> 40 (raised by Height)
  REQUIRE(outStore.getValue(rt::FlatIndex(6, 6, 0, k_Dx, k_Dy)) == k_Background);                              // 45 -> 50 (shallow minimum removed)
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 2, 0, k_Dx, k_Dy)) == k_Background);                              // neighbor of the deep dent, unchanged
  REQUIRE(outStore.getValue(rt::FlatIndex(4, 4, 0, k_Dx, k_Dy)) == k_Background);                              // untouched background voxel
}

// -----------------------------------------------------------------------------
// (2c) Height == 0 identity: HMinima reconstructs the (input + Height) marker under the input mask by erosion. With
//      Height 0 the marker equals the input, so the reconstruction-by-erosion converges to the input and the output
//      equals the input exactly. Preflight accepts Height 0 (only a negative Height is rejected); assert output ==
//      input.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::HMinimaImageFilter: Height 0 is the identity", "[NXImageProcessing][HMinimaImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_Dx = 12;
  constexpr usize k_Dy = 12;
  constexpr usize k_Dz = 3;
  const std::vector<int32> pattern = rt::MakeGradientNoisePattern<int32>(k_Dx, k_Dy, k_Dz);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_Dx, k_Dy, k_Dz, pattern);

  HMinimaImageFilter filter;
  rt::RunReconstructionFilter<HMinimaImageFilter>(filter, dataStructure, inputPath, rt::HeightAndFullyConnectedParamSetter<HMinimaImageFilter>(), /*fullyConnected=*/false, /*height=*/0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<int32>>(outputPath));
  const auto& outStore = dataStructure.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();
  REQUIRE(outStore.getSize() == pattern.size());
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    INFO("Height-0 identity index=" << i);
    REQUIRE(outStore.getValue(i) == pattern[i]);
  }
}

// -----------------------------------------------------------------------------
// (3b) Clamp (not wrap) at the fully-overflowed extreme: with a Height larger than the whole uint8 dynamic range
//      (300 > 255), (input + Height) overflows for EVERY voxel, so the saturating marker is 255 everywhere and
//      the reconstruction of that uniform ceiling is 255 everywhere. A wrapping cast would instead produce small
//      markers whose reconstruction stays near the input, so the uniform 255 output uniquely proves the cast
//      clamps rather than wraps. The test asserts the uniform output on the filter directly.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::HMinimaImageFilter: uint8 full-overflow clamps to type maximum", "[NXImageProcessing][HMinimaImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_Dx = 12;
  constexpr usize k_Dy = 12;
  constexpr usize k_Dz = 8;
  constexpr float64 k_Height = 300.0; // > uint8 dynamic range (255) -> every voxel's marker overflows
  const std::vector<uint8> pattern = rt::MakeGradientNoisePattern<uint8>(k_Dx, k_Dy, k_Dz);

  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<uint8>(dataStructure, k_Dx, k_Dy, k_Dz, pattern);
  HMinimaImageFilter filter;
  rt::RunReconstructionFilter<HMinimaImageFilter>(filter, dataStructure, inputPath, rt::HeightAndFullyConnectedParamSetter<HMinimaImageFilter>(), /*fullyConnected=*/false, k_Height);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<uint8>>(outputPath));
  const auto& outStore = dataStructure.getDataRefAs<DataArray<uint8>>(outputPath).getDataStoreRef();
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    REQUIRE(outStore.getValue(i) == uint8{255}); // clamped to NonnegativeMax, not wrapped to a small value
  }
}

// -----------------------------------------------------------------------------
// (5) Preflight: a non-finite Height (NaN or Inf) is rejected with the filter-local error code, because the
//     saturating (input + Height) marker offset is undefined for a non-finite value.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::HMinimaImageFilter: Preflight non-finite Height guard", "[NXImageProcessing][HMinimaImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  HMinimaImageFilter filter;

  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<uint8>(dataStructure, 8, 8, 8, rt::MakeGradientNoisePattern<uint8>(8, 8, 8));

  SECTION("finite Height preflights cleanly")
  {
    auto result = filter.preflight(dataStructure, makeHMinimaArguments(inputPath, 2.0));
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  }

  SECTION("invalid Heights are rejected before output creation")
  {
    const auto [height, expectedCode] =
        GENERATE(table<float64, int32>({{std::numeric_limits<float64>::quiet_NaN(), k_NonFiniteHeight}, {std::numeric_limits<float64>::infinity(), k_NonFiniteHeight}, {-1.0, k_NegativeHeight}}));
    CAPTURE(height);
    const auto args = makeHMinimaArguments(inputPath, height);
    const auto preflight = filter.preflight(dataStructure, args);
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while) The simplnx result assertion wraps its diagnostics in do/while.
    SIMPLNX_RESULT_REQUIRE_INVALID(preflight.output_actions);
    REQUIRE((preflight.output_actions.errors().size() == 1));
    REQUIRE((preflight.output_actions.errors().front().code == expectedCode));
    const auto execution = filter.execute(dataStructure, args);
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while) The simplnx result assertion wraps its diagnostics in do/while.
    SIMPLNX_RESULT_REQUIRE_INVALID(execution.result);
    REQUIRE((execution.result.errors().size() == 1));
    REQUIRE((execution.result.errors().front().code == expectedCode));
    REQUIRE((dataStructure.getData(inputPath.replaceName("Output")) == nullptr));
  }
}

TEST_CASE("NXImageProcessing::HMinimaImageFilter: FromSIMPLJson", "[NXImageProcessing][HMinimaImageFilter]")
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
TEST_CASE("NXImageProcessing::HMinimaImageFilter: ITK real-image golden (HMinima)", "[NXImageProcessing][ItkGolden][HMinimaImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<HMinimaImageFilter>("RA-Short.nrrd", "7778067eeb752b6ac396dd9e362e8346",
                                                        [](Arguments& args) { args.insertOrAssign(HMinimaImageFilter::k_Height_Key, std::make_any<float64>(2000.0)); });
}

// NOLINTBEGIN(bugprone-chained-comparison) Catch2 comparison decomposition triggers this check.
TEST_CASE("NXImageProcessing::HMinimaImageFilter: resident integration fractional minima produce watershed markers", "[NXImageProcessing][ImageFilterIntegration][Issue1874][HMinimaImageFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  unit_test::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  const auto input = rt::BuildImageFromPattern<uint8>(dataStructure, 7, 1, 1, {0, 1, 2, 3, 2, 1, 0});
  image_integration::SetGeometry(dataStructure);
  const ShapeType shape{1, 1, 7};
  REQUIRE(image_integration::Read<uint8>(dataStructure, input, shape, scope) == std::vector<uint8>{0, 1, 2, 3, 2, 1, 0});
  const auto converted = image_integration::ConvertToFloat32(dataStructure, input);
  REQUIRE(image_integration::Read<float32>(dataStructure, converted, shape, scope) == std::vector<float32>{0, 1, 2, 3, 2, 1, 0});
  const HMinimaImageFilter minimaFilter;
  auto args = image_integration::ImageArguments(minimaFilter, converted, "RaisedMinima");
  args.insertOrAssign(HMinimaImageFilter::k_Height_Key, std::make_any<float64>(0.5));
  args.insertOrAssign(HMinimaImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
  image_integration::Execute(minimaFilter, dataStructure, args, scope);
  const auto raised = input.replaceName("RaisedMinima");
  const std::vector<float32> expectedRaised{0.5F, 1, 2, 3, 2, 1, 0.5F};
  REQUIRE(image_integration::Read<float32>(dataStructure, raised, shape, scope) == expectedRaised);
  const RegionalMinimaImageFilter regional;
  args = image_integration::ImageArguments(regional, raised, "RegionalMinima");
  args.insertOrAssign(RegionalMinimaImageFilter::k_ForegroundValue_Key, std::make_any<float64>(1));
  args.insertOrAssign(RegionalMinimaImageFilter::k_BackgroundValue_Key, std::make_any<float64>(0));
  args.insertOrAssign(RegionalMinimaImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
  args.insertOrAssign(RegionalMinimaImageFilter::k_FlatIsMinima_Key, std::make_any<bool>(true));
  image_integration::Execute(regional, dataStructure, args, scope);
  const auto regionalPath = input.replaceName("RegionalMinima");
  REQUIRE(image_integration::Read<uint32>(dataStructure, regionalPath, shape, scope) == std::vector<uint32>{1, 0, 0, 0, 0, 0, 1});
  const auto markers = image_integration::Components(dataStructure, regionalPath, "Markers");
  const std::vector<uint32> expectedMarkers{1, 0, 0, 0, 0, 0, 2};
  REQUIRE(image_integration::Read<uint32>(dataStructure, markers, shape, scope) == expectedMarkers);
  const MorphologicalWatershedFromMarkersImageFilter watershed;
  args = image_integration::ImageArguments(watershed, raised, "Basins");
  args.insertOrAssign(MorphologicalWatershedFromMarkersImageFilter::k_MarkerImageDataPath_Key, std::make_any<DataPath>(markers));
  args.insertOrAssign(MorphologicalWatershedFromMarkersImageFilter::k_MarkWatershedLine_Key, std::make_any<bool>(true));
  args.insertOrAssign(MorphologicalWatershedFromMarkersImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
  image_integration::ExecuteChecked(watershed, dataStructure, args);
  const auto labels = image_integration::Read<uint32>(dataStructure, input.replaceName("Basins"), shape, scope);
  // The unique central ridge separates three cells in each basin. Zero marks the watershed line.
  REQUIRE(labels == std::vector<uint32>{1, 1, 1, 0, 2, 2, 2});
  REQUIRE(std::count(labels.begin(), labels.end(), uint32{1}) == 3);
  REQUIRE(std::count(labels.begin(), labels.end(), uint32{2}) == 3);
  REQUIRE(image_integration::Read<float32>(dataStructure, raised, shape, scope) == expectedRaised);
  REQUIRE(image_integration::Read<uint32>(dataStructure, markers, shape, scope) == expectedMarkers);
  REQUIRE(image_integration::Read<uint8>(dataStructure, input, shape, scope) == std::vector<uint8>{0, 1, 2, 3, 2, 1, 0});
  image_integration::CheckGeometry(dataStructure, {7, 1, 1});
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison)
