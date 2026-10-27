#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/HConvexImageFilter.hpp"

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/UnitTest/AlgorithmTestScope.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/MorphologicalReconstructionEngine.hpp"

#include <catch2/catch.hpp>
#include <nonstd/span.hpp>

#include <any>
#include <array>
#include <atomic>
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
// Filter-local error codes for the Height guards (kept in sync with HConvexImageFilter.cpp).
constexpr int32 k_NonFiniteHeight = -8553;
constexpr int32 k_NegativeHeight = -8556;

Arguments makeHConvexArguments(const DataPath& inputPath, float64 height, bool fullyConnected = false)
{
  Arguments args;
  args.insertOrAssign(HConvexImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(HConvexImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(HConvexImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(HConvexImageFilter::k_Height_Key, std::make_any<float64>(height));
  args.insertOrAssign(HConvexImageFilter::k_FullyConnected_Key, std::make_any<bool>(fullyConnected));
  return args;
}

class HConvexTransferStore : public DataStore<int16>
{
public:
  explicit HConvexTransferStore(int16 initialValue)
  : DataStore<int16>(ShapeType{1, 1, 3}, ShapeType{1}, initialValue)
  {
  }
  ~HConvexTransferStore() noexcept override = default;
  HConvexTransferStore(const HConvexTransferStore&) = delete;
  HConvexTransferStore(HConvexTransferStore&&) = delete;
  HConvexTransferStore& operator=(const HConvexTransferStore&) = delete;
  HConvexTransferStore& operator=(HConvexTransferStore&&) = delete;

  Result<> copyIntoBuffer(usize start, nonstd::span<int16> buffer) const override
  {
    if(m_FailRead)
    {
      return MakeErrorResult(-18741001, "HConvex test input transfer failed.");
    }
    auto result = DataStore<int16>::copyIntoBuffer(start, buffer);
    result.warnings().push_back(Warning{.code = 18741001, .message = "HConvex test input transfer warning."});
    return result;
  }

  Result<> copyFromBuffer(usize start, nonstd::span<const int16> buffer) override
  {
    if(m_FailWrite)
    {
      return MakeErrorResult(-18741002, "HConvex test output transfer failed.");
    }
    auto result = DataStore<int16>::copyFromBuffer(start, buffer);
    result.warnings().push_back(Warning{.code = 18741002, .message = "HConvex test output transfer warning."});
    return result;
  }

  void setFailRead(bool failRead)
  {
    m_FailRead = failRead;
  }

  void setFailWrite(bool failWrite)
  {
    m_FailWrite = failWrite;
  }

private:
  bool m_FailRead = false;
  bool m_FailWrite = false;
};
} // namespace

TEMPLATE_TEST_CASE("ImageProcessing::HConvexImageFilter: integer fractional Height fails preflight", "[ImageProcessing][Issue1874][HConvexImageFilter]", int8, uint8, int16, uint16, int32, uint32,
                   int64, uint64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  const float64 height = GENERATE(0.5, 2.5);
  CAPTURE(height);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 3, 1, 1, {0, 2, 0});
  const HConvexImageFilter filter;

  // Integer images require a whole Height. Stop at preflight: the baseline must never execute this request.
  const auto preflightResult = filter.preflight(dataStructure, makeHConvexArguments(inputPath, height));
  REQUIRE(preflightResult.outputActions.invalid());
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors().front().code == -1874101);
}

TEMPLATE_TEST_CASE("ImageProcessing::HConvexImageFilter: zero Height gives zero contrast with unchanged type", "[ImageProcessing][Issue1874][HConvexImageFilter]", int8, uint8, int16, uint16, int32,
                   uint32, int64, uint64, float32, float64)
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
  const bool fullyConnected = GENERATE(false, true);
  CAPTURE(fullyConnected);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 3, 2, 1, {1, 3, 2, 4, 0, 5});
  const Arguments args = makeHConvexArguments(inputPath, 0.0, fullyConnected);
  const HConvexImageFilter filter;
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
  REQUIRE(output.getDataType() == GetDataType<TestType>());
  REQUIRE(output.getTupleShape() == ShapeType{1, 2, 3});
  REQUIRE(output.getComponentShape() == ShapeType{1});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<TestType>>(outputPath));
  const auto& outputStore = dataStructure.getDataRefAs<DataArray<TestType>>(outputPath).getDataStoreRef();
  REQUIRE(outputStore.getSize() == 6);
  // Height 0 makes the marker equal to the mask. Reconstruction equals input, so input minus reconstruction is 0.
  for(usize index = 0; index < outputStore.getSize(); ++index)
  {
    CAPTURE(index);
    REQUIRE(outputStore.getValue(index) == static_cast<TestType>(0));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::HConvexImageFilter: floating fractional Height retains fractional contrast", "[ImageProcessing][Issue1874][HConvexImageFilter]", float32, float64)
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
  const bool fullyConnected = GENERATE(false, true);
  CAPTURE(fullyConnected);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 3, 3, 1, {0, 0, 0, 0, 2, 0, 0, 0, 0});
  const Arguments args = makeHConvexArguments(inputPath, 0.5, fullyConnected);
  const HConvexImageFilter filter;
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const DataPath outputPath = inputPath.replaceName("Output");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(outputPath));
  const auto& output = dataStructure.getDataRefAs<IDataArray>(outputPath);
  REQUIRE(output.getDataType() == GetDataType<TestType>());
  REQUIRE(output.getTupleShape() == ShapeType{1, 3, 3});
  REQUIRE(output.getComponentShape() == ShapeType{1});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<TestType>>(outputPath));
  const auto& outputStore = dataStructure.getDataRefAs<DataArray<TestType>>(outputPath).getDataStoreRef();
  // For an isolated height-2 peak on zero background, reconstruction is 1.5 at the peak and 0 elsewhere.
  const std::array<TestType, 9> expected = {0, 0, 0, 0, 0.5, 0, 0, 0, 0};
  REQUIRE(outputStore.getSize() == expected.size());
  for(usize index = 0; index < expected.size(); ++index)
  {
    CAPTURE(index);
    REQUIRE(outputStore.getValue(index) == expected.at(index));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::HConvexImageFilter: native 64-bit unit contrast above double precision", "[ImageProcessing][Issue1874][HConvexImageFilter]", int64, uint64)
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
  constexpr TestType inputValue = TestType{1} << 54;
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 1, 1, 1, {inputValue});
  const HConvexImageFilter filter;
  const Arguments args = makeHConvexArguments(inputPath, 1.0);
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const DataPath outputPath = inputPath.replaceName("Output");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(outputPath));
  const auto& output = dataStructure.getDataRefAs<IDataArray>(outputPath);
  REQUIRE(output.getDataType() == GetDataType<TestType>());
  REQUIRE(output.getTupleShape() == ShapeType{1, 1, 1});
  REQUIRE(output.getComponentShape() == ShapeType{1});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<TestType>>(outputPath));
  const auto& outputStore = dataStructure.getDataRefAs<DataArray<TestType>>(outputPath).getDataStoreRef();
  REQUIRE(outputStore.getSize() == 1);
  // A one-pixel marker has no neighbor that can raise it: reconstruction is inputValue - 1, so contrast is 1.
  // The baseline double conversion rounds the marker back to inputValue. Both values are safely inside T's range.
  REQUIRE(outputStore.getValue(0) == TestType{1});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::HConvexImageFilter: exact signed 64-bit marker boundaries in both directions", "[ImageProcessing][Issue1874][HConvexImageFilter]")
{
  UnitTest::LoadPlugins();
  constexpr int64 minimum = std::numeric_limits<int64>::lowest();
  constexpr int64 maximum = std::numeric_limits<int64>::max();
  const bool lower = GENERATE(false, true);
  std::array<int64, 5> values = {minimum, minimum + 1, -1, 0, maximum};
  const std::array<int64, 5> original = {minimum, minimum + 1, -1, 0, maximum};
  ImageProcessing::ReconstructionMarkerOptions options;
  options.source = lower ? ImageProcessing::ReconstructionMarkerSource::MaskMinusHeight : ImageProcessing::ReconstructionMarkerSource::MaskPlusHeight;
  options.height = std::ldexp(1.0, 63);
  options.ExactIntegerHeight = true;
  ImageProcessing::detail::ApplyReconstructionMarkerOptions<int64>(nonstd::span<int64>(values.data(), values.size()), 0, {5, 1, 1}, options);
  // MIN + 2^63 = 0; MAX - 2^63 = -1. Other values saturate at the corresponding native bound.
  const std::array<int64, 5> expected = lower ? std::array<int64, 5>{minimum, minimum, minimum, minimum, -1} : std::array<int64, 5>{0, 1, maximum, maximum, maximum};
  REQUIRE(values == expected);
  DataStore<int64> input(ShapeType{5}, ShapeType{1}, 0);
  DataStore<int64> marker(ShapeType{5}, ShapeType{1}, 0);
  const auto inputResult = input.copyFromBuffer(0, nonstd::span<const int64>(original.data(), original.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(inputResult);
  const std::atomic_bool shouldCancel = false;
  const auto markerResult = ImageProcessing::detail::BuildReconstructionMarker<int64>(
      input, marker, {5, 1, 1}, lower ? ImageProcessing::detail::ReconMarker::MinusHeight : ImageProcessing::detail::ReconMarker::PlusHeight, options.height, shouldCancel, true);
  SIMPLNX_RESULT_REQUIRE_VALID(markerResult);
  for(usize index = 0; index < expected.size(); ++index)
  {
    REQUIRE(marker.getValue(index) == expected.at(index));
  }
}

TEMPLATE_TEST_CASE("ImageProcessing::HConvexImageFilter: exact integer marker zero unit and huge Heights", "[ImageProcessing][Issue1874][HConvexImageFilter]", int8, uint8, int16, uint16, int32,
                   uint32, int64, uint64)
{
  UnitTest::LoadPlugins();
  constexpr TestType minimum = std::numeric_limits<TestType>::lowest();
  constexpr TestType maximum = std::numeric_limits<TestType>::max();
  const bool lower = GENERATE(false, true);
  const float64 height = GENERATE(0.0, 1.0, std::ldexp(1.0, 64), std::numeric_limits<float64>::max());
  CAPTURE(lower, height);
  std::array<TestType, 3> values = {minimum, 0, maximum};
  ImageProcessing::ReconstructionMarkerOptions options;
  options.source = lower ? ImageProcessing::ReconstructionMarkerSource::MaskMinusHeight : ImageProcessing::ReconstructionMarkerSource::MaskPlusHeight;
  options.height = height;
  options.ExactIntegerHeight = true;
  ImageProcessing::detail::ApplyReconstructionMarkerOptions<TestType>(nonstd::span<TestType>(values.data(), values.size()), 0, {3, 1, 1}, options);
  if(height == 0.0)
  {
    const std::array<TestType, 3> expected = {minimum, 0, maximum};
    REQUIRE(values == expected);
  }
  else if(height == 1.0)
  {
    const TestType zeroLowered = std::is_signed_v<TestType> ? static_cast<TestType>(-1) : TestType{0};
    const std::array<TestType, 3> expected =
        lower ? std::array<TestType, 3>{minimum, zeroLowered, static_cast<TestType>(maximum - 1)} : std::array<TestType, 3>{static_cast<TestType>(minimum + 1), 1, maximum};
    REQUIRE(values == expected);
  }
  else
  {
    const TestType bound = lower ? minimum : maximum;
    const std::array<TestType, 3> expected = {bound, bound, bound};
    REQUIRE(values == expected);
  }
}

TEST_CASE("ImageProcessing::HConvexImageFilter: exact marker arithmetic requires explicit opt-in", "[ImageProcessing][Issue1874][HConvexImageFilter]")
{
  UnitTest::LoadPlugins();
  constexpr int64 inputValue = (int64{1} << 54) + 1;
  constexpr int64 expectedExistingValue = int64{1} << 54;
  std::array<int64, 1> values = {inputValue};
  ImageProcessing::ReconstructionMarkerOptions options;
  options.source = ImageProcessing::ReconstructionMarkerSource::MaskPlusHeight;
  options.height = 0.0;
  ImageProcessing::detail::ApplyReconstructionMarkerOptions<int64>(nonstd::span<int64>(values.data(), values.size()), 0, {1, 1, 1}, options);
  REQUIRE(values.at(0) == expectedExistingValue);
  const DataStore<int64> input(ShapeType{1}, ShapeType{1}, inputValue);
  DataStore<int64> marker(ShapeType{1}, ShapeType{1}, 0);
  const std::atomic_bool shouldCancel = false;
  const auto markerResult = ImageProcessing::detail::BuildReconstructionMarker<int64>(input, marker, {1, 1, 1}, ImageProcessing::detail::ReconMarker::MinusHeight, 0.0, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(markerResult);
  REQUIRE(marker.getValue(0) == expectedExistingValue);
}

TEST_CASE("ImageProcessing::HConvexImageFilter: exact marker Height immediately below 2^64", "[ImageProcessing][Issue1874][HConvexImageFilter]")
{
  UnitTest::LoadPlugins();
  ImageProcessing::ReconstructionMarkerOptions options;
  options.source = ImageProcessing::ReconstructionMarkerSource::MaskMinusHeight;
  options.height = 0x1.fffffffffffffp63;
  options.ExactIntegerHeight = true;
  std::array<uint64, 1> unsignedValues = {std::numeric_limits<uint64>::max()};
  ImageProcessing::detail::ApplyReconstructionMarkerOptions<uint64>(nonstd::span<uint64>(unsignedValues.data(), unsignedValues.size()), 0, {1, 1, 1}, options);
  REQUIRE(unsignedValues.at(0) == uint64{2047});
  std::array<int64, 1> loweredValues = {std::numeric_limits<int64>::max()};
  ImageProcessing::detail::ApplyReconstructionMarkerOptions<int64>(nonstd::span<int64>(loweredValues.data(), loweredValues.size()), 0, {1, 1, 1}, options);
  REQUIRE(loweredValues.at(0) == -9223372036854773761LL);
  options.source = ImageProcessing::ReconstructionMarkerSource::MaskPlusHeight;
  std::array<int64, 1> raisedValues = {std::numeric_limits<int64>::lowest()};
  ImageProcessing::detail::ApplyReconstructionMarkerOptions<int64>(nonstd::span<int64>(raisedValues.data(), raisedValues.size()), 0, {1, 1, 1}, options);
  REQUIRE(raisedValues.at(0) == 9223372036854773760LL);
}

TEMPLATE_TEST_CASE("ImageProcessing::HConvexImageFilter: signed contrast overflow and adjacent representable contrast", "[ImageProcessing][Issue1874][HConvexImageFilter]", int8, int16, int32, int64)
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
  const bool overflow = GENERATE(false, true);
  const bool fullyConnected = GENERATE(false, true);
  CAPTURE(overflow, fullyConnected);
  constexpr TestType minimum = std::numeric_limits<TestType>::lowest();
  constexpr TestType maximum = std::numeric_limits<TestType>::max();
  const TestType peak = overflow ? maximum : TestType{-1};
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 3, 1, 1, {minimum, peak, minimum});
  const HConvexImageFilter filter;
  // Height exceeds the entire native range, so reconstruction is the minimum everywhere.
  const Arguments args = makeHConvexArguments(inputPath, std::ldexp(1.0, static_cast<int32>(8 * sizeof(TestType))), fullyConnected);
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  if(overflow)
  {
    REQUIRE(executeResult.result.invalid());
    REQUIRE(executeResult.result.errors().size() == 1);
    REQUIRE(executeResult.result.errors().front().code == -1874102);
    REQUIRE(executeResult.result.errors().front().message.find("tuple 1") != std::string::npos);
    REQUIRE(executeResult.result.errors().front().message.find(inputPath.toString()) != std::string::npos);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    const DataPath outputPath = inputPath.replaceName("Output");
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(outputPath));
    const auto& output = dataStructure.getDataRefAs<IDataArray>(outputPath);
    REQUIRE(output.getDataType() == GetDataType<TestType>());
    REQUIRE(output.getTupleShape() == ShapeType{1, 1, 3});
    REQUIRE(output.getComponentShape() == ShapeType{1});
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<TestType>>(outputPath));
    const auto& outputStore = dataStructure.getDataRefAs<DataArray<TestType>>(outputPath).getDataStoreRef();
    REQUIRE(outputStore.getSize() == 3);
    REQUIRE(outputStore.getValue(0) == 0);
    REQUIRE(outputStore.getValue(1) == maximum);
    REQUIRE(outputStore.getValue(2) == 0);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::HConvexImageFilter: zero Height gives zero contrast at native integer bounds", "[ImageProcessing][Issue1874][HConvexImageFilter]", int8, uint8, int16, uint16,
                   int32, uint32, int64, uint64)
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 3, 1, 1, {std::numeric_limits<TestType>::lowest(), 0, std::numeric_limits<TestType>::max()});
  const HConvexImageFilter filter;
  const auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, makeHConvexArguments(inputPath, 0.0));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const DataPath outputPath = inputPath.replaceName("Output");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(outputPath));
  const auto& output = dataStructure.getDataRefAs<IDataArray>(outputPath);
  REQUIRE(output.getDataType() == GetDataType<TestType>());
  REQUIRE(output.getTupleShape() == ShapeType{1, 1, 3});
  REQUIRE(output.getComponentShape() == ShapeType{1});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<TestType>>(outputPath));
  const auto& outputStore = dataStructure.getDataRefAs<DataArray<TestType>>(outputPath).getDataStoreRef();
  REQUIRE(outputStore.getSize() == 3);
  for(usize index = 0; index < outputStore.getSize(); ++index)
  {
    REQUIRE(outputStore.getValue(index) == 0);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::HConvexImageFilter: whole Height contrast is stable under a signed baseline shift", "[ImageProcessing][Issue1874][HConvexImageFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
  const int32 background = GENERATE(-50, 50);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, 3, 1, 1, {background, background + 20, background});
  const HConvexImageFilter filter;
  const auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, makeHConvexArguments(inputPath, 10.0));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const DataPath outputPath = inputPath.replaceName("Output");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(outputPath));
  REQUIRE(dataStructure.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::int32);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<int32>>(outputPath));
  const auto& outputStore = dataStructure.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();
  REQUIRE(outputStore.getSize() == 3);
  REQUIRE(outputStore.getValue(0) == 0);
  REQUIRE(outputStore.getValue(1) == 10);
  REQUIRE(outputStore.getValue(2) == 0);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::HConvexImageFilter: chunked contrast preserves transfer Results and cancellation", "[ImageProcessing][Issue1874][HConvexImageFilter]")
{
  UnitTest::LoadPlugins();
  HConvexTransferStore input(2);
  HConvexTransferStore reconstruction(1);
  HConvexTransferStore output(-1);
  std::atomic_bool shouldCancel = false;
  const DataPath inputPath({"Input"});
  SECTION("successful chunks retain all transfer warnings")
  {
    const auto result = ImageProcessing::detail::SubtractHConvexContrastChunked<int16>(input, reconstruction, output, inputPath, shouldCancel, 2);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(result.warnings().size() == 6);
    for(usize index = 0; index < 3; ++index)
    {
      REQUIRE(output.getValue(index) == 1);
    }
  }
  SECTION("input failure returns its error before output writes")
  {
    input.setFailRead(true);
    const auto result = ImageProcessing::detail::SubtractHConvexContrastChunked<int16>(input, reconstruction, output, inputPath, shouldCancel, 2);
    REQUIRE(result.invalid());
    REQUIRE(result.errors().size() == 1);
    REQUIRE(result.errors().front().code == -18741001);
    REQUIRE(output.getValue(0) == -1);
  }
  SECTION("reconstruction failure retains earlier input warning")
  {
    reconstruction.setFailRead(true);
    const auto result = ImageProcessing::detail::SubtractHConvexContrastChunked<int16>(input, reconstruction, output, inputPath, shouldCancel, 2);
    REQUIRE(result.invalid());
    REQUIRE(result.errors().size() == 1);
    REQUIRE(result.errors().front().code == -18741001);
    REQUIRE(result.warnings().size() == 1);
    REQUIRE(output.getValue(0) == -1);
  }
  SECTION("output failure retains both input warnings")
  {
    output.setFailWrite(true);
    const auto result = ImageProcessing::detail::SubtractHConvexContrastChunked<int16>(input, reconstruction, output, inputPath, shouldCancel, 2);
    REQUIRE(result.invalid());
    REQUIRE(result.errors().size() == 1);
    REQUIRE(result.errors().front().code == -18741002);
    REQUIRE(result.warnings().size() == 2);
    REQUIRE(output.getValue(0) == -1);
  }
  SECTION("pre-cancelled work leaves output unchanged")
  {
    shouldCancel = true;
    const auto result = ImageProcessing::detail::SubtractHConvexContrastChunked<int16>(input, reconstruction, output, inputPath, shouldCancel, 2);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(result.warnings().empty());
    REQUIRE(output.getValue(0) == -1);
  }
}

// -----------------------------------------------------------------------------
// (2) Hand-computed behavior on a small 2D image: a flat background maps to 0; a bright bump whose contrast is
//     >= Height maps to exactly Height (its dome is capped); a bright bump whose contrast is < Height maps to its
//     own contrast. HConvex(f, h) = f - HMaxima(f, h) = min(contrast, h) at each regional maximum, 0 elsewhere.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::HConvexImageFilter: hand-computed dome heights", "[ImageProcessing][HConvexImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 9;
  constexpr usize DY = 9;
  constexpr usize DZ = 1;
  constexpr int32 background = 50;
  constexpr int32 tallBump = 70;    // contrast 20 (>= Height) -> dome capped to Height (10)
  constexpr int32 shallowBump = 55; // contrast 5  (<  Height) -> dome equals its own contrast (5)
  constexpr float64 height = 10.0;

  std::vector<int32> pattern(DX * DY * DZ, background);
  pattern[rt::FlatIndex(2, 2, 0, DX, DY)] = tallBump;    // isolated regional maximum (all face-neighbors are background)
  pattern[rt::FlatIndex(6, 6, 0, DX, DY)] = shallowBump; // isolated regional maximum, separated from the tall bump by background

  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);

  HConvexImageFilter filter;
  rt::RunReconstructionFilter<HConvexImageFilter>(filter, ds, inputPath, rt::HeightAndFullyConnectedParamSetter<HConvexImageFilter>(), /*fullyConnected=*/false, height);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();

  REQUIRE(outStore.getValue(rt::FlatIndex(2, 2, 0, DX, DY)) == static_cast<int32>(height)); // tall dome capped to Height (10)
  REQUIRE(outStore.getValue(rt::FlatIndex(6, 6, 0, DX, DY)) == (shallowBump - background)); // shallow dome == contrast (5)
  REQUIRE(outStore.getValue(rt::FlatIndex(4, 4, 0, DX, DY)) == 0);                          // background -> 0
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 2, 0, DX, DY)) == 0);                          // neighbor of the tall bump -> 0
}

// -----------------------------------------------------------------------------
// (2b) Both-algorithm-paths coverage: the SAME hand-computed dome-height oracle as test (2), run under BOTH the
//      in-core and out-of-core reconstruction ALGORITHM paths on in-memory stores (selected by
//      SIMPLNX_TEST_ALGORITHM_PATH) via UnitTest::AlgorithmTestScope. HConvex reconstructs the (input - Height)
//      marker (which routes through DispatchAlgorithm) and then subtracts it from the input, so the scope proves
//      both reconstruction implementations feed the correct dome; the hand-computed expected values are the
//      non-circular oracle (not the filter's own in-core run).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::HConvexImageFilter: hand-computed dome heights (both algorithm paths)", "[ImageProcessing][HConvexImageFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  constexpr usize DX = 9;
  constexpr usize DY = 9;
  constexpr usize DZ = 1;
  constexpr int32 background = 50;
  constexpr int32 tallBump = 70;    // contrast 20 (>= Height) -> dome capped to Height (10)
  constexpr int32 shallowBump = 55; // contrast 5  (<  Height) -> dome equals its own contrast (5)
  constexpr float64 height = 10.0;

  std::vector<int32> pattern(DX * DY * DZ, background);
  pattern[rt::FlatIndex(2, 2, 0, DX, DY)] = tallBump;    // isolated regional maximum (all face-neighbors are background)
  pattern[rt::FlatIndex(6, 6, 0, DX, DY)] = shallowBump; // isolated regional maximum, separated from the tall bump by background

  UnitTest::AlgorithmTestScope scope(scenario);
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, pattern);
  scope.requireExpectedStore(ds.getDataRefAs<IDataArray>(inputPath));

  HConvexImageFilter filter;
  Arguments args;
  args.insertOrAssign(HConvexImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(HConvexImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(HConvexImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(HConvexImageFilter::k_Height_Key, std::make_any<float64>(height));
  args.insertOrAssign(HConvexImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));

  auto executeResult = scope.executeFilter(filter, ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = ds.getDataRefAs<DataArray<int32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  REQUIRE(outStore.getValue(rt::FlatIndex(2, 2, 0, DX, DY)) == static_cast<int32>(height)); // tall dome capped to Height (10)
  REQUIRE(outStore.getValue(rt::FlatIndex(6, 6, 0, DX, DY)) == (shallowBump - background)); // shallow dome == contrast (5)
  REQUIRE(outStore.getValue(rt::FlatIndex(4, 4, 0, DX, DY)) == 0);                          // background -> 0
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 2, 0, DX, DY)) == 0);                          // neighbor of the tall bump -> 0
}

// -----------------------------------------------------------------------------
// (2c) Height == 0 no-op: HConvex(f, 0) = f - HMaxima(f, 0). The (input - 0) marker equals the input, so its
//      reconstruction-by-dilation converges to the input and the h-dome (input - input) is 0 at every voxel.
//      Preflight accepts Height 0 (only a negative Height is rejected); assert the entire output is zero.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::HConvexImageFilter: Height 0 yields an all-zero output", "[ImageProcessing][HConvexImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 12, DY = 12, DZ = 3;
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(ds, DX, DY, DZ, rt::MakeGradientNoisePattern<int32>(DX, DY, DZ));

  HConvexImageFilter filter;
  rt::RunReconstructionFilter<HConvexImageFilter>(filter, ds, inputPath, rt::HeightAndFullyConnectedParamSetter<HConvexImageFilter>(), /*fullyConnected=*/false, /*height=*/0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    INFO("Height-0 h-dome index=" << i);
    REQUIRE(outStore.getValue(i) == 0);
  }
}

// -----------------------------------------------------------------------------
// (2d) Height beyond the dynamic range saturates the marker: on a uint8 image a Height larger than 255 drives the
//      saturating (input - Height) marker to the type minimum (0) at every voxel, whose reconstruction-by-dilation
//      is 0 everywhere, so HConvex = input - 0 = input. Assert the output equals the input exactly (the saturating
//      marker cast, and the input-minus-reconstruction difference, are both well-defined at this extreme).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::HConvexImageFilter: Height beyond dynamic range returns the input (uint8 saturation)", "[ImageProcessing][HConvexImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 12, DY = 12, DZ = 3;
  const std::vector<uint8> pattern = rt::MakeGradientNoisePattern<uint8>(DX, DY, DZ);
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<uint8>(ds, DX, DY, DZ, pattern);

  HConvexImageFilter filter;
  rt::RunReconstructionFilter<HConvexImageFilter>(filter, ds, inputPath, rt::HeightAndFullyConnectedParamSetter<HConvexImageFilter>(), /*fullyConnected=*/false, /*height=*/1000.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<uint8>>(outputPath).getDataStoreRef();
  REQUIRE(outStore.getSize() == pattern.size());
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    INFO("saturated h-dome index=" << i);
    REQUIRE(outStore.getValue(i) == pattern[i]);
  }
}

// -----------------------------------------------------------------------------
// (2e) Preflight scalar guard: a multi-component (non-scalar) input is rejected (requireScalar=true; the grayscale
//      reconstruction operator is defined on scalar images). HConvex uniquely lacked this coverage that its siblings
//      GrayscaleFillhole/GrayscaleGrindPeak have; the input ArraySelectionParameter imposes no component-shape
//      restriction, so the guard lives in preflightImpl and returns k_NonScalarInput (-8550).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::HConvexImageFilter: Preflight scalar guard (multi-component rejected)", "[ImageProcessing][HConvexImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath scalarPath = rt::BuildImageFromPattern<uint8>(ds, 8, 8, 8, rt::MakeGradientNoisePattern<uint8>(8, 8, 8)); // creates the geometry + CellData
  const DataPath vecPath({"Image Geometry", "CellData", "Vec"});
  auto vecStore = DataStoreUtilities::CreateDataStore<uint8>(ds, vecPath, {8, 8, 8}, {2});
  const auto& cellAM = ds.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
  DataArray<uint8>::Create(ds, "Vec", vecStore, cellAM.getId());

  HConvexImageFilter filter;
  Arguments args;
  args.insertOrAssign(HConvexImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(HConvexImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(vecPath));
  args.insertOrAssign(HConvexImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(HConvexImageFilter::k_Height_Key, std::make_any<float64>(2.0));
  args.insertOrAssign(HConvexImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
  const auto result = filter.preflight(ds, args);
  REQUIRE(result.outputActions.invalid());
  REQUIRE(result.outputActions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
}

// -----------------------------------------------------------------------------
// (4) Preflight: a non-finite Height (NaN or Inf) is rejected with the filter-local error code, because the
//     saturating (input - Height) marker offset is undefined for a non-finite value.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::HConvexImageFilter: Preflight non-finite Height guard", "[ImageProcessing][HConvexImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  HConvexImageFilter filter;

  auto makeArgs = [](const DataPath& inputPath, float64 height) {
    Arguments args;
    args.insertOrAssign(HConvexImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(HConvexImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(HConvexImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(HConvexImageFilter::k_Height_Key, std::make_any<float64>(height));
    args.insertOrAssign(HConvexImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
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

TEST_CASE("ImageProcessing::HConvexImageFilter: FromSIMPLJson", "[ImageProcessing][HConvexImageFilter]")
{
  const nlohmann::json json = {{"Height", 3.5},
                               {"FullyConnected", true},
                               {"SelectedCellArrayPath", {{"Data Container Name", "Image Geometry"}, {"Attribute Matrix Name", "CellData"}, {"Data Array Name", "Input"}}},
                               {"NewCellArrayName", "HConv"}};

  Result<Arguments> result = HConvexImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments& args = result.value();
  REQUIRE(args.value<float64>(HConvexImageFilter::k_Height_Key) == 3.5);
  REQUIRE(args.value<bool>(HConvexImageFilter::k_FullyConnected_Key) == true);
  REQUIRE(args.value<DataPath>(HConvexImageFilter::k_InputImageGeomPath_Key) == DataPath({"Image Geometry"}));
  REQUIRE(args.value<DataPath>(HConvexImageFilter::k_InputImageDataPath_Key) == DataPath({"Image Geometry", "CellData", "Input"}));
  REQUIRE(args.value<std::string>(HConvexImageFilter::k_OutputImageArrayName_Key) == "HConv");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image md5 golden -- duplicates ITKHConvexImageTest.cpp(HConvex): RA-Short.nrrd (int16),
// Height 10000. (B) live-ITK bit-exact + (A) md5-validity-first vs the committed hash.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::HConvexImageFilter: ITK real-image golden (HConvex)", "[ImageProcessing][ItkGolden][HConvexImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<HConvexImageFilter>("RA-Short.nrrd", "f3a7b95a51710d51b3b73e0eb77eb1eb",
                                                        [](Arguments& args) { args.insertOrAssign(HConvexImageFilter::k_Height_Key, std::make_any<float64>(10000.0)); });
}
