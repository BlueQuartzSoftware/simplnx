#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/HMinimaImageFilter.hpp"

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
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
  Arguments args;
  args.insertOrAssign(HMinimaImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(HMinimaImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(HMinimaImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(HMinimaImageFilter::k_Height_Key, std::make_any<float64>(height));
  args.insertOrAssign(HMinimaImageFilter::k_FullyConnected_Key, std::make_any<bool>(fullyConnected));
  return args;
}
} // namespace

TEMPLATE_TEST_CASE("ImageProcessing::HMinimaImageFilter: integer fractional Height fails preflight", "[ImageProcessing][Issue1874][HMinimaImageFilter]", int8, uint8, int16, uint16, int32, uint32,
                   int64, uint64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  const float64 height = GENERATE(0.5, 2.5);
  CAPTURE(height);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 3, 1, 1, {0, 2, 0});
  const HMinimaImageFilter filter;
  const auto wholeHeightPreflightResult = filter.preflight(dataStructure, makeHMinimaArguments(inputPath, 2.0));
  SIMPLNX_RESULT_REQUIRE_VALID(wholeHeightPreflightResult.outputActions);

  // Integer images require a whole Height. Stop at preflight: the baseline must never execute this request.
  const auto preflightResult = filter.preflight(dataStructure, makeHMinimaArguments(inputPath, height));
  REQUIRE(preflightResult.outputActions.invalid());
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors().front().code == -1874111);
  REQUIRE(preflightResult.outputActions.errors().front().message.find(inputPath.toString()) != std::string::npos);
  REQUIRE(preflightResult.outputActions.errors().front().message.find(height == 0.5 ? "0.5" : "2.5") != std::string::npos);
}

TEMPLATE_TEST_CASE("ImageProcessing::HMinimaImageFilter: zero Height preserves values and type", "[ImageProcessing][Issue1874][HMinimaImageFilter]", int8, uint8, int16, uint16, int32, uint32, int64,
                   uint64, float32, float64)
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
  const bool fullyConnected = GENERATE(false, true);
  CAPTURE(fullyConnected);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 3, 2, 1, {1, 3, 2, 4, 0, 5});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(inputPath));
  algorithmTestScope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));
  const Arguments args = makeHMinimaArguments(inputPath, 0.0, fullyConnected);
  const HMinimaImageFilter filter;
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.value().actions.size() == 1);
  const auto* actionPtr = dynamic_cast<const CreateArrayAction*>(preflightResult.outputActions.value().actions.front().get());
  REQUIRE(actionPtr != nullptr);
  REQUIRE(actionPtr->type() == GetDataType<TestType>());
  REQUIRE(actionPtr->dims() == ShapeType{1, 2, 3});
  REQUIRE(actionPtr->componentDims() == ShapeType{1});

  const auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const DataPath outputPath = inputPath.replaceName("Output");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(outputPath));
  const auto& output = dataStructure.getDataRefAs<IDataArray>(outputPath);
  algorithmTestScope.requireExpectedStore(output);
  REQUIRE(output.getDataType() == GetDataType<TestType>());
  REQUIRE(output.getTupleShape() == ShapeType{1, 2, 3});
  REQUIRE(output.getComponentShape() == ShapeType{1});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<TestType>>(outputPath));
  const auto& outputStore = dataStructure.getDataRefAs<DataArray<TestType>>(outputPath).getDataStoreRef();
  REQUIRE(outputStore.getSize() == 6);
  // Height 0 makes the marker equal to the mask, so reconstruction preserves every input value.
  const std::array<TestType, 6> expected = {1, 3, 2, 4, 0, 5};
  for(usize index = 0; index < outputStore.getSize(); ++index)
  {
    CAPTURE(index);
    REQUIRE(outputStore.getValue(index) == expected.at(index));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::HMinimaImageFilter: floating fractional Height retains fractional minima", "[ImageProcessing][Issue1874][HMinimaImageFilter]", float32, float64)
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
  const bool fullyConnected = GENERATE(false, true);
  CAPTURE(fullyConnected);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 3, 3, 1, {2, 2, 2, 2, 0, 2, 2, 2, 2});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(inputPath));
  algorithmTestScope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));
  const Arguments args = makeHMinimaArguments(inputPath, 0.5, fullyConnected);
  const HMinimaImageFilter filter;
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const DataPath outputPath = inputPath.replaceName("Output");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(outputPath));
  const auto& output = dataStructure.getDataRefAs<IDataArray>(outputPath);
  algorithmTestScope.requireExpectedStore(output);
  REQUIRE(output.getDataType() == GetDataType<TestType>());
  REQUIRE(output.getTupleShape() == ShapeType{1, 3, 3});
  REQUIRE(output.getComponentShape() == ShapeType{1});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<TestType>>(outputPath));
  const auto& outputStore = dataStructure.getDataRefAs<DataArray<TestType>>(outputPath).getDataStoreRef();
  // For an isolated zero-valued minimum on background 2, erosion reconstructs 0.5 at the minimum and 2 elsewhere.
  const std::array<TestType, 9> expected = {2, 2, 2, 2, 0.5, 2, 2, 2, 2};
  REQUIRE(outputStore.getSize() == expected.size());
  for(usize index = 0; index < expected.size(); ++index)
  {
    CAPTURE(index);
    REQUIRE(outputStore.getValue(index) == expected.at(index));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::HMinimaImageFilter: native 64-bit marker preserves units above double precision", "[ImageProcessing][Issue1874][HMinimaImageFilter]", int64, uint64)
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
  constexpr TestType inputValue = TestType{1} << 54;
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 1, 1, 1, {inputValue});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(inputPath));
  algorithmTestScope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));
  const HMinimaImageFilter filter;
  const Arguments args = makeHMinimaArguments(inputPath, 1.0);
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const DataPath outputPath = inputPath.replaceName("Output");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(outputPath));
  const auto& output = dataStructure.getDataRefAs<IDataArray>(outputPath);
  algorithmTestScope.requireExpectedStore(output);
  REQUIRE(output.getDataType() == GetDataType<TestType>());
  REQUIRE(output.getTupleShape() == ShapeType{1, 1, 1});
  REQUIRE(output.getComponentShape() == ShapeType{1});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<TestType>>(outputPath));
  const auto& outputStore = dataStructure.getDataRefAs<DataArray<TestType>>(outputPath).getDataStoreRef();
  REQUIRE(outputStore.getSize() == 1);
  // A one-pixel marker has no neighbor that can lower it: reconstruction remains the exact inputValue + 1.
  // The baseline double conversion rounds the marker back to inputValue. Both values are safely inside T's range.
  REQUIRE(outputStore.getValue(0) == inputValue + TestType{1});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::HMinimaImageFilter: zero Height preserves native integer bounds", "[ImageProcessing][Issue1874][HMinimaImageFilter]", int8, uint8, int16, uint16, int32, uint32,
                   int64, uint64)
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool fullyConnected = GENERATE(false, true);
  CAPTURE(scenario, fullyConnected);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
  constexpr TestType minimum = std::numeric_limits<TestType>::lowest();
  constexpr TestType maximum = std::numeric_limits<TestType>::max();
  const std::array<TestType, 4> expected = {minimum, minimum + TestType{1}, maximum - TestType{1}, maximum};
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 2, 2, 1, {minimum, minimum + TestType{1}, maximum - TestType{1}, maximum});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(inputPath));
  algorithmTestScope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));
  const HMinimaImageFilter filter;
  const auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, makeHMinimaArguments(inputPath, 0.0, fullyConnected));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const DataPath outputPath = inputPath.replaceName("Output");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(outputPath));
  const auto& output = dataStructure.getDataRefAs<IDataArray>(outputPath);
  algorithmTestScope.requireExpectedStore(output);
  REQUIRE(output.getDataType() == GetDataType<TestType>());
  REQUIRE(output.getTupleShape() == ShapeType{1, 2, 2});
  REQUIRE(output.getComponentShape() == ShapeType{1});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<TestType>>(outputPath));
  const auto& outputStore = dataStructure.getDataRefAs<DataArray<TestType>>(outputPath).getDataStoreRef();
  REQUIRE(outputStore.getSize() == expected.size());
  for(usize index = 0; index < expected.size(); ++index)
  {
    CAPTURE(index);
    REQUIRE(outputStore.getValue(index) == expected.at(index));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::HMinimaImageFilter: whole Height raises or saturates native integer markers", "[ImageProcessing][Issue1874][HMinimaImageFilter]", int8, uint8, int16, uint16,
                   int32, uint32, int64, uint64)
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool fullyConnected = GENERATE(false, true);
  const bool atMaximum = GENERATE(false, true);
  const float64 height = GENERATE(1.0, std::ldexp(1.0, 64), std::numeric_limits<float64>::max());
  CAPTURE(scenario, fullyConnected, atMaximum, height);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
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
  const DataPath outputPath = inputPath.replaceName("Output");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(outputPath));
  const auto& output = dataStructure.getDataRefAs<IDataArray>(outputPath);
  algorithmTestScope.requireExpectedStore(output);
  REQUIRE(output.getDataType() == GetDataType<TestType>());
  REQUIRE(output.getTupleShape() == ShapeType{1, 1, 1});
  REQUIRE(output.getComponentShape() == ShapeType{1});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<TestType>>(outputPath));
  const auto& outputStore = dataStructure.getDataRefAs<DataArray<TestType>>(outputPath).getDataStoreRef();
  REQUIRE(outputStore.getSize() == 1);
  REQUIRE(outputStore.getValue(0) == expected);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::HMinimaImageFilter: large whole Height retains exact native 64-bit markers", "[ImageProcessing][Issue1874][HMinimaImageFilter]", int64, uint64)
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool fullyConnected = GENERATE(false, true);
  const float64 height = GENERATE(std::ldexp(1.0, 63), 0x1.fffffffffffffp63);
  CAPTURE(scenario, fullyConnected, height);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 1, 1, 1, {std::numeric_limits<TestType>::lowest()});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(inputPath));
  algorithmTestScope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));
  const HMinimaImageFilter filter;
  const auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, makeHMinimaArguments(inputPath, height, fullyConnected));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const DataPath outputPath = inputPath.replaceName("Output");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(outputPath));
  const auto& output = dataStructure.getDataRefAs<IDataArray>(outputPath);
  algorithmTestScope.requireExpectedStore(output);
  REQUIRE(output.getDataType() == GetDataType<TestType>());
  REQUIRE(output.getTupleShape() == ShapeType{1, 1, 1});
  REQUIRE(output.getComponentShape() == ShapeType{1});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<TestType>>(outputPath));
  const auto& outputStore = dataStructure.getDataRefAs<DataArray<TestType>>(outputPath).getDataStoreRef();
  REQUIRE(outputStore.getSize() == 1);
  // The second Height is exactly 2^64 - 2048, the largest float64 Height below 2^64.
  if constexpr(std::is_signed_v<TestType>)
  {
    REQUIRE(outputStore.getValue(0) == (height == std::ldexp(1.0, 63) ? int64{0} : int64{9223372036854773760LL}));
  }
  else
  {
    REQUIRE(outputStore.getValue(0) == (height == std::ldexp(1.0, 63) ? uint64{9223372036854775808ULL} : uint64{18446744073709549568ULL}));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::HMinimaImageFilter: whole Height is stable under a signed baseline shift", "[ImageProcessing][Issue1874][HMinimaImageFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const bool fullyConnected = GENERATE(false, true);
  const bool positiveBaseline = GENERATE(false, true);
  CAPTURE(scenario, fullyConnected, positiveBaseline);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
  const std::vector<int32> input = positiveBaseline ? std::vector<int32>{50, 30, 50} : std::vector<int32>{-50, -70, -50};
  const std::array<int32, 3> expected = positiveBaseline ? std::array<int32, 3>{50, 40, 50} : std::array<int32, 3>{-50, -60, -50};
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, 3, 1, 1, input);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(inputPath));
  algorithmTestScope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));
  const HMinimaImageFilter filter;
  const auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, makeHMinimaArguments(inputPath, 10.0, fullyConnected));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const DataPath outputPath = inputPath.replaceName("Output");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(outputPath));
  const auto& output = dataStructure.getDataRefAs<IDataArray>(outputPath);
  algorithmTestScope.requireExpectedStore(output);
  REQUIRE(output.getDataType() == DataType::int32);
  REQUIRE(output.getTupleShape() == ShapeType{1, 1, 3});
  REQUIRE(output.getComponentShape() == ShapeType{1});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<int32>>(outputPath));
  const auto& outputStore = dataStructure.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();
  REQUIRE(outputStore.getSize() == expected.size());
  for(usize index = 0; index < expected.size(); ++index)
  {
    CAPTURE(index);
    REQUIRE(outputStore.getValue(index) == expected.at(index));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
  REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<int32>>(outputPath));
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
  REQUIRE_NOTHROW(ds.getDataRefAs<IDataArray>(inputPath));
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
  REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<int32>>(outputPath));
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
  REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<int32>>(outputPath));
  const auto& outStore = ds.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();
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
  REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<uint8>>(outputPath));
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
  UnitTest::LoadPlugins();
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
  UnitTest::LoadPlugins();
  rt::RunReconstructionMd5ItkGolden<HMinimaImageFilter>("RA-Short.nrrd", "7778067eeb752b6ac396dd9e362e8346",
                                                        [](Arguments& args) { args.insertOrAssign(HMinimaImageFilter::k_Height_Key, std::make_any<float64>(2000.0)); });
}
