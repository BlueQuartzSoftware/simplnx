#include "ImageFilterIntegrationTestUtils.hpp"

#include "ReconstructionFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/HConvexImageFilter.hpp"
#include "NXImageProcessing/utils/HConvexUtilities.hpp"
#include "NXImageProcessing/utils/ImageFilterExecutorDetail.hpp"
#include "NXImageProcessing/utils/ImageFilterExecutors.hpp"

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/AlgorithmTestScope.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingOperationTypes.hpp"
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
#include <utility>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

static_assert(std::is_constructible_v<ImageProcessing::ExactReconstructionHeight, uint64>);
static_assert(!std::is_constructible_v<ImageProcessing::ExactReconstructionHeight, float64>);

using ReconstructionExecutorType = Result<> (*)(DataStructure&, const DataPath&, const DataPath&, const DataPath&, ImageProcessing::ReconstructOp, bool, ImageProcessing::detail::ReconMarker, float64,
                                                const std::atomic_bool&, const IFilter::MessageHandler&, bool);
static_assert(std::is_same_v<decltype(&ImageProcessing::ExecuteMorphologicalReconstructionImageFilter), ReconstructionExecutorType>);

namespace
{
// Filter-local error codes for the Height guards (kept in sync with HConvexImageFilter.cpp).
constexpr int32 k_NonFiniteHeight = -8553;
constexpr int32 k_NegativeHeight = -8556;

Arguments makeHConvexArguments(const DataPath& inputPath, float64 height, bool fullyConnected = false)
{
  auto args = image_integration::ImageArguments(HConvexImageFilter{}, inputPath, "Output");
  args.insertOrAssign(HConvexImageFilter::k_Height_Key, std::make_any<float64>(height));
  args.insertOrAssign(HConvexImageFilter::k_FullyConnected_Key, std::make_any<bool>(fullyConnected));
  return args;
}

constexpr int32 k_InputReadCode = 1;
constexpr int32 k_ReconstructionReadCode = 2;
constexpr int32 k_OutputWriteCode = 3;

class HConvexTransferStore : public DataStore<int16>
{
public:
  explicit HConvexTransferStore(const std::array<int16, 3>& values, int32 readCode = 0)
  : DataStore<int16>(ShapeType{1, 1, 3}, ShapeType{1}, int16{0})
  , m_ReadCode(readCode)
  {
    for(usize index = 0; index < values.size(); ++index)
    {
      setValue(index, values.at(index));
    }
  }
  ~HConvexTransferStore() noexcept override = default;
  HConvexTransferStore(const HConvexTransferStore&) = delete;
  HConvexTransferStore(HConvexTransferStore&&) = delete;
  HConvexTransferStore& operator=(const HConvexTransferStore&) = delete;
  HConvexTransferStore& operator=(HConvexTransferStore&&) = delete;

  Result<> copyIntoBuffer(usize start, nonstd::span<int16> buffer) const override
  {
    m_TransferRanges.emplace_back(start, buffer.size());
    if(m_ShouldFail)
    {
      return MakeErrorResult(-m_ReadCode, "HConvex test input transfer failed.");
    }
    auto result = DataStore<int16>::copyIntoBuffer(start, buffer);
    result.warnings().push_back(Warning{.code = m_ReadCode, .message = "HConvex test input transfer warning."});
    return result;
  }

  Result<> copyFromBuffer(usize start, nonstd::span<const int16> buffer) override
  {
    m_TransferRanges.emplace_back(start, buffer.size());
    if(m_ShouldFail)
    {
      return MakeErrorResult(-k_OutputWriteCode, "HConvex test output transfer failed.");
    }
    auto result = DataStore<int16>::copyFromBuffer(start, buffer);
    if(result.valid() && m_CancelAfterWritePtr != nullptr)
    {
      m_CancelAfterWritePtr->store(true);
    }
    result.warnings().push_back(Warning{.code = k_OutputWriteCode, .message = "HConvex test output transfer warning."});
    return result;
  }

  void failTransfers()
  {
    m_ShouldFail = true;
  }

  void cancelAfterWrite(std::atomic_bool& shouldCancel)
  {
    m_CancelAfterWritePtr = &shouldCancel;
  }

  const std::vector<std::pair<usize, usize>>& transferRanges() const
  {
    return m_TransferRanges;
  }

private:
  const int32 m_ReadCode;
  bool m_ShouldFail = false;
  std::atomic_bool* m_CancelAfterWritePtr = nullptr;
  mutable std::vector<std::pair<usize, usize>> m_TransferRanges;
};
} // namespace

TEMPLATE_TEST_CASE("NXImageProcessing::HConvexImageFilter: integer fractional Height fails preflight", "[NXImageProcessing][Issue1874][HConvexImageFilter]", int8, uint8, int16, uint16, int32, uint32,
                   int64, uint64)
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  const float64 height = GENERATE(0.5, 2.5);
  CAPTURE(height);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 3, 1, 1, {0, 2, 0});
  const HConvexImageFilter filter;

  const Arguments args = makeHConvexArguments(inputPath, height);
  const auto preflightResult = filter.preflight(dataStructure, args);
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while) simplnx assertion macro.
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions);
  REQUIRE((preflightResult.output_actions.errors().size() == 1));
  REQUIRE((preflightResult.output_actions.errors().front().code == -29101));
  REQUIRE((preflightResult.output_actions.errors().front().message.find(inputPath.toString()) != std::string::npos));
  REQUIRE((preflightResult.output_actions.errors().front().message.find(height == 0.5 ? "0.5" : "2.5") != std::string::npos));
  const auto executeResult = filter.execute(dataStructure, args);
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while) The simplnx result assertion wraps its diagnostics in do/while.
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE((executeResult.result.errors().size() == 1));
  REQUIRE((executeResult.result.errors().front().code == preflightResult.output_actions.errors().front().code));
  REQUIRE((dataStructure.getData(inputPath.replaceName("Output")) == nullptr));
}

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
TEMPLATE_TEST_CASE("NXImageProcessing::HConvexImageFilter: zero Height gives zero contrast with unchanged type", "[NXImageProcessing][Issue1874][HConvexImageFilter]", int8, uint8, int16, uint16,
                   int32, uint32, int64, uint64, float32, float64)
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);
  const bool fullyConnected = GENERATE(false, true);
  CAPTURE(fullyConnected);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 3, 2, 1, {1, 3, 2, 4, 0, 5});
  const Arguments args = makeHConvexArguments(inputPath, 0.0, fullyConnected);
  const HConvexImageFilter filter;
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
  // Reconstruction equals the input at zero height, so every contrast is zero.
  const auto actual = image_integration::Read<TestType>(dataStructure, inputPath.replaceName("Output"), {1, 2, 3}, algorithmTestScope);
  REQUIRE(actual == std::vector<TestType>(6, 0));
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
TEMPLATE_TEST_CASE("NXImageProcessing::HConvexImageFilter: floating fractional Height retains fractional contrast", "[NXImageProcessing][Issue1874][HConvexImageFilter]", float32, float64)
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);
  const bool fullyConnected = GENERATE(false, true);
  CAPTURE(fullyConnected);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 3, 3, 1, {0, 0, 0, 0, 2, 0, 0, 0, 0});
  const Arguments args = makeHConvexArguments(inputPath, 0.5, fullyConnected);
  const HConvexImageFilter filter;
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  const auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  // The height-2 peak reconstructs to 1.5, giving 0.5 contrast against its original value.
  const auto actual = image_integration::Read<TestType>(dataStructure, inputPath.replaceName("Output"), {1, 3, 3}, algorithmTestScope);
  REQUIRE(actual == std::vector<TestType>{0, 0, 0, 0, 0.5, 0, 0, 0, 0});
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
TEMPLATE_TEST_CASE("NXImageProcessing::HConvexImageFilter: native 64-bit contrast for unit and large Heights above double precision", "[NXImageProcessing][Issue1874][HConvexImageFilter]", int64,
                   uint64)
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);
  constexpr float64 belowUint64Limit = 0x1.fffffffffffffp63;
  const float64 height = GENERATE(1.0, 0x1.fffffffffffffp63, std::ldexp(1.0, 64), std::numeric_limits<float64>::max());
  CAPTURE(height);
  constexpr TestType maximum = std::numeric_limits<TestType>::max();
  // A signed input of -1 gives a representable contrast even if the marker reaches the native minimum.
  constexpr TestType largeHeightInput = std::is_signed_v<TestType> ? static_cast<TestType>(-1) : maximum;
  const TestType inputValue = height == 1.0 ? (TestType{1} << 54) : largeHeightInput;
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 1, 1, 1, {inputValue});
  const HConvexImageFilter filter;
  const Arguments args = makeHConvexArguments(inputPath, height);
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  const auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  // A single pixel retains its marker. With the largest Height below 2^64 the unsigned marker stays at 2047.
  TestType expected = maximum;
  if(height == 1.0)
  {
    expected = TestType{1};
  }
  else if(!std::is_signed_v<TestType> && height == belowUint64Limit)
  {
    expected = maximum - TestType{2047};
  }
  const auto actual = image_integration::Read<TestType>(dataStructure, inputPath.replaceName("Output"), {1, 1, 1}, algorithmTestScope);
  REQUIRE(actual == std::vector<TestType>{expected});
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
TEST_CASE("NXImageProcessing::HConvexImageFilter: exact signed 64-bit marker boundaries in both directions", "[NXImageProcessing][Issue1874][HConvexImageFilter]")
{
  constexpr int64 minimum = std::numeric_limits<int64>::lowest();
  constexpr int64 maximum = std::numeric_limits<int64>::max();
  const bool lower = GENERATE(false, true);
  std::array<int64, 5> values = {minimum, minimum + 1, -1, 0, maximum};
  const std::array<int64, 5> original = {minimum, minimum + 1, -1, 0, maximum};
  ImageProcessing::ReconstructionMarkerOptions options;
  options.source = lower ? ImageProcessing::ReconstructionMarkerSource::MaskMinusHeight : ImageProcessing::ReconstructionMarkerSource::MaskPlusHeight;
  options.height = ImageProcessing::ExactReconstructionHeight{uint64{1} << 63};
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
      input, marker, {5, 1, 1}, lower ? ImageProcessing::detail::ReconMarker::MinusHeight : ImageProcessing::detail::ReconMarker::PlusHeight, options.height, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(markerResult);
  for(usize index = 0; index < expected.size(); ++index)
  {
    REQUIRE(marker.getValue(index) == expected.at(index));
  }
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// NOLINTBEGIN(bugprone-chained-comparison) Catch2 comparison decomposition triggers this check.
TEMPLATE_TEST_CASE("NXImageProcessing::HConvexImageFilter: exact integer marker zero unit and huge Heights", "[NXImageProcessing][Issue1874][HConvexImageFilter]", int8, uint8, int16, uint16, int32,
                   uint32, int64, uint64)
{
  constexpr TestType minimum = std::numeric_limits<TestType>::lowest();
  constexpr TestType maximum = std::numeric_limits<TestType>::max();
  const bool lower = GENERATE(false, true);
  const uint64 height = GENERATE(uint64{0}, uint64{1}, std::numeric_limits<uint64>::max());
  CAPTURE(lower, height);
  std::array<TestType, 3> values = {minimum, 0, maximum};
  ImageProcessing::ReconstructionMarkerOptions options;
  options.source = lower ? ImageProcessing::ReconstructionMarkerSource::MaskMinusHeight : ImageProcessing::ReconstructionMarkerSource::MaskPlusHeight;
  options.height = ImageProcessing::ExactReconstructionHeight{height};
  ImageProcessing::detail::ApplyReconstructionMarkerOptions<TestType>(nonstd::span<TestType>(values.data(), values.size()), 0, {3, 1, 1}, options);
  if(height == 0)
  {
    const std::array<TestType, 3> expected = {minimum, 0, maximum};
    REQUIRE(values == expected);
  }
  else if(height == 1)
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
// NOLINTEND(bugprone-chained-comparison)

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
TEST_CASE("NXImageProcessing::HConvexImageFilter: exact marker arithmetic requires explicit opt-in", "[NXImageProcessing][Issue1874][HConvexImageFilter]")
{
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
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// NOLINTBEGIN(bugprone-chained-comparison) Catch2 comparison decomposition triggers this check.
TEST_CASE("NXImageProcessing::HConvexImageFilter: exact marker Height immediately below 2^64", "[NXImageProcessing][Issue1874][HConvexImageFilter]")
{
  ImageProcessing::ReconstructionMarkerOptions options;
  options.source = ImageProcessing::ReconstructionMarkerSource::MaskMinusHeight;
  options.height = ImageProcessing::ExactReconstructionHeight{std::numeric_limits<uint64>::max() - uint64{2047}};
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
// NOLINTEND(bugprone-chained-comparison)

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
TEMPLATE_TEST_CASE("NXImageProcessing::HConvexImageFilter: signed contrast overflow and adjacent representable contrast", "[NXImageProcessing][Issue1874][HConvexImageFilter]", int8, int16, int32,
                   int64)
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);
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
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  const auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  if(overflow)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors().size() == 1);
    REQUIRE(executeResult.result.errors().front().code == -29102);
    const auto& message = executeResult.result.errors().front().message;
    REQUIRE(message.find("tuple 1 of") != std::string::npos);
    if constexpr(std::is_same_v<TestType, int8>)
    {
      REQUIRE(message.find("(input 127, reconstruction -128)") != std::string::npos);
    }
    REQUIRE(message.find(inputPath.toString()) != std::string::npos);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    const auto actual = image_integration::Read<TestType>(dataStructure, inputPath.replaceName("Output"), {1, 1, 3}, algorithmTestScope);
    REQUIRE(actual == std::vector<TestType>{0, maximum, 0});
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
TEMPLATE_TEST_CASE("NXImageProcessing::HConvexImageFilter: zero Height gives zero contrast at native integer bounds", "[NXImageProcessing][Issue1874][HConvexImageFilter]", int8, uint8, int16, uint16,
                   int32, uint32, int64, uint64)
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 3, 1, 1, {std::numeric_limits<TestType>::lowest(), 0, std::numeric_limits<TestType>::max()});
  const HConvexImageFilter filter;
  const auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, makeHConvexArguments(inputPath, 0.0));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const auto actual = image_integration::Read<TestType>(dataStructure, inputPath.replaceName("Output"), {1, 1, 3}, algorithmTestScope);
  REQUIRE(actual == std::vector<TestType>{0, 0, 0});
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
TEST_CASE("NXImageProcessing::HConvexImageFilter: whole Height contrast is stable under a signed baseline shift", "[NXImageProcessing][Issue1874][HConvexImageFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope algorithmTestScope(scenario);
  const int32 background = GENERATE(-50, 50);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, 3, 1, 1, {background, background + 20, background});
  const HConvexImageFilter filter;
  const auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, makeHConvexArguments(inputPath, 10.0));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const auto actual = image_integration::Read<int32>(dataStructure, inputPath.replaceName("Output"), {1, 1, 3}, algorithmTestScope);
  REQUIRE(actual == std::vector<int32>{0, 10, 0});
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
TEST_CASE("NXImageProcessing::HConvexImageFilter: chunked contrast preserves transfer Results and cancellation", "[NXImageProcessing][Issue1874][HConvexImageFilter]")
{
  std::atomic_bool shouldCancel = false;
  HConvexTransferStore input({2, 2, 2}, k_InputReadCode);
  HConvexTransferStore reconstruction({1, 1, 1}, k_ReconstructionReadCode);
  HConvexTransferStore output({-1, -1, -1});
  std::array<int16, 3> expectedOutput{-1, -1, -1};
  std::vector<int32> expectedWarnings;
  int32 expectedError = 0;

  SECTION("successful chunks retain all transfer warnings")
  {
    // A two-pixel chunk and a one-pixel tail each perform two reads followed by a write.
    expectedWarnings = {k_InputReadCode, k_ReconstructionReadCode, k_OutputWriteCode, k_InputReadCode, k_ReconstructionReadCode, k_OutputWriteCode};
    expectedOutput = {1, 1, 1};
  }
  SECTION("input failure returns its error before output writes")
  {
    input.failTransfers();
    expectedError = -k_InputReadCode;
  }
  SECTION("reconstruction failure retains the earlier input warning")
  {
    reconstruction.failTransfers();
    expectedError = -k_ReconstructionReadCode;
    expectedWarnings = {k_InputReadCode};
  }
  SECTION("output failure retains both read warnings")
  {
    output.failTransfers();
    expectedError = -k_OutputWriteCode;
    expectedWarnings = {k_InputReadCode, k_ReconstructionReadCode};
  }
  SECTION("cancelling after the first write preserves the committed page")
  {
    output.cancelAfterWrite(shouldCancel);
    expectedOutput = {1, 1, -1};
    expectedWarnings = {k_InputReadCode, k_ReconstructionReadCode, k_OutputWriteCode};
  }
  SECTION("pre-cancelled work leaves output unchanged")
  {
    shouldCancel = true;
  }

  const auto result = ImageProcessing::detail::SubtractHConvexContrastChunked<int16>(input, reconstruction, output, DataPath({"Input"}), shouldCancel, 2);
  if(expectedError == 0)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 1);
    REQUIRE(result.errors().front().code == expectedError);
  }
  REQUIRE(result.warnings().size() == expectedWarnings.size());
  for(usize index = 0; index < expectedWarnings.size(); ++index)
  {
    REQUIRE(result.warnings().at(index).code == expectedWarnings.at(index));
  }
  for(usize index = 0; index < output.getSize(); ++index)
  {
    REQUIRE(output.getValue(index) == expectedOutput.at(index));
  }

  if(shouldCancel.load())
  {
    std::vector<std::pair<usize, usize>> expectedTransfers;
    if(!expectedWarnings.empty())
    {
      expectedTransfers.emplace_back(0, 2);
    }
    REQUIRE(input.transferRanges() == expectedTransfers);
    REQUIRE(reconstruction.transferRanges() == expectedTransfers);
    REQUIRE(output.transferRanges() == expectedTransfers);
  }
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 compares within its expression decomposition and simplnx result checks use do/while.
TEST_CASE("NXImageProcessing::HConvexImageFilter: overflow in the aliased contrast tail preserves the committed page", "[NXImageProcessing][Issue1874][HConvexImageFilter]")
{
  const HConvexTransferStore input({5, 5, 32767}, k_InputReadCode);
  HConvexTransferStore reconstruction({1, 1, -1}, k_ReconstructionReadCode);
  const std::atomic_bool shouldCancel = false;
  const auto result = ImageProcessing::detail::SubtractHConvexContrastChunked<int16>(input, reconstruction, reconstruction, DataPath({"Input"}), shouldCancel, 2);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  REQUIRE(result.errors().front().code == -29102);
  const auto& message = result.errors().front().message;
  REQUIRE(message.find("tuple 2 of") != std::string::npos);
  REQUIRE(message.find("(input 32767, reconstruction -1)") != std::string::npos);
  const std::array<int32, 5> expectedWarnings{k_InputReadCode, k_ReconstructionReadCode, k_OutputWriteCode, k_InputReadCode, k_ReconstructionReadCode};
  REQUIRE(result.warnings().size() == expectedWarnings.size());
  for(usize index = 0; index < expectedWarnings.size(); ++index)
  {
    REQUIRE(result.warnings().at(index).code == expectedWarnings.at(index));
  }
  // The last page was read but never written. Reconstruction still holds that page's original marker.
  const std::vector<std::pair<usize, usize>> inputTransfers{{0, 2}, {2, 1}};
  const std::vector<std::pair<usize, usize>> reconstructionTransfers{{0, 2}, {0, 2}, {2, 1}};
  REQUIRE(input.transferRanges() == inputTransfers);
  REQUIRE(reconstruction.transferRanges() == reconstructionTransfers);
  const std::array<int16, 3> expected{4, 4, -1};
  for(usize index = 0; index < expected.size(); ++index)
  {
    REQUIRE(reconstruction.getValue(index) == expected.at(index));
  }
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// -----------------------------------------------------------------------------
// (2) Hand-computed behavior on a small 2D image: a flat background maps to 0; a bright bump whose contrast is
//     >= Height maps to exactly Height (its dome is capped); a bright bump whose contrast is < Height maps to its
//     own contrast. HConvex(f, h) = f - HMaxima(f, h) = min(contrast, h) at each regional maximum, 0 elsewhere.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::HConvexImageFilter: hand-computed dome heights", "[NXImageProcessing][HConvexImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_Dx = 9;
  constexpr usize k_Dy = 9;
  constexpr usize k_Dz = 1;
  constexpr int32 k_Background = 50;
  constexpr int32 k_TallBump = 70;    // contrast 20 (>= Height) -> dome capped to Height (10)
  constexpr int32 k_ShallowBump = 55; // contrast 5  (<  Height) -> dome equals its own contrast (5)
  constexpr float64 k_Height = 10.0;

  std::vector<int32> pattern(k_Dx * k_Dy * k_Dz, k_Background);
  pattern[rt::FlatIndex(2, 2, 0, k_Dx, k_Dy)] = k_TallBump;    // isolated regional maximum (all face-neighbors are background)
  pattern[rt::FlatIndex(6, 6, 0, k_Dx, k_Dy)] = k_ShallowBump; // isolated regional maximum, separated from the tall bump by background

  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_Dx, k_Dy, k_Dz, pattern);

  HConvexImageFilter filter;
  rt::RunReconstructionFilter<HConvexImageFilter>(filter, dataStructure, inputPath, rt::HeightAndFullyConnectedParamSetter<HConvexImageFilter>(), /*fullyConnected=*/false, k_Height);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = dataStructure.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();

  REQUIRE(outStore.getValue(rt::FlatIndex(2, 2, 0, k_Dx, k_Dy)) == static_cast<int32>(k_Height));   // tall dome capped to Height (10)
  REQUIRE(outStore.getValue(rt::FlatIndex(6, 6, 0, k_Dx, k_Dy)) == (k_ShallowBump - k_Background)); // shallow dome == contrast (5)
  REQUIRE(outStore.getValue(rt::FlatIndex(4, 4, 0, k_Dx, k_Dy)) == 0);                              // background -> 0
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 2, 0, k_Dx, k_Dy)) == 0);                              // neighbor of the tall bump -> 0
}

// -----------------------------------------------------------------------------
// (2b) Both-algorithm-paths coverage: the SAME hand-computed dome-height oracle as test (2), run under BOTH the
//      in-core and out-of-core reconstruction ALGORITHM paths on in-memory stores (selected by
//      SIMPLNX_TEST_ALGORITHM_PATH) via UnitTest::AlgorithmTestScope. HConvex reconstructs the (input - Height)
//      marker (which routes through DispatchAlgorithm) and then subtracts it from the input, so the scope proves
//      both reconstruction implementations feed the correct dome; the hand-computed expected values are the
//      non-circular oracle (not the filter's own in-core run).
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::HConvexImageFilter: hand-computed dome heights (both algorithm paths)", "[NXImageProcessing][HConvexImageFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  constexpr usize k_Dx = 9;
  constexpr usize k_Dy = 9;
  constexpr usize k_Dz = 1;
  constexpr int32 k_Background = 50;
  constexpr int32 k_TallBump = 70;    // contrast 20 (>= Height) -> dome capped to Height (10)
  constexpr int32 k_ShallowBump = 55; // contrast 5  (<  Height) -> dome equals its own contrast (5)
  constexpr float64 k_Height = 10.0;

  std::vector<int32> pattern(k_Dx * k_Dy * k_Dz, k_Background);
  pattern[rt::FlatIndex(2, 2, 0, k_Dx, k_Dy)] = k_TallBump;    // isolated regional maximum (all face-neighbors are background)
  pattern[rt::FlatIndex(6, 6, 0, k_Dx, k_Dy)] = k_ShallowBump; // isolated regional maximum, separated from the tall bump by background

  unit_test::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_Dx, k_Dy, k_Dz, pattern);
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(inputPath));

  HConvexImageFilter filter;
  Arguments args;
  args.insertOrAssign(HConvexImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(HConvexImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(HConvexImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(HConvexImageFilter::k_Height_Key, std::make_any<float64>(k_Height));
  args.insertOrAssign(HConvexImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  auto& outArray = dataStructure.getDataRefAs<DataArray<int32>>(outputPath);
  scope.requireExpectedStore(outArray);
  const auto& outStore = outArray.getDataStoreRef();
  REQUIRE(outStore.getValue(rt::FlatIndex(2, 2, 0, k_Dx, k_Dy)) == static_cast<int32>(k_Height));   // tall dome capped to Height (10)
  REQUIRE(outStore.getValue(rt::FlatIndex(6, 6, 0, k_Dx, k_Dy)) == (k_ShallowBump - k_Background)); // shallow dome == contrast (5)
  REQUIRE(outStore.getValue(rt::FlatIndex(4, 4, 0, k_Dx, k_Dy)) == 0);                              // background -> 0
  REQUIRE(outStore.getValue(rt::FlatIndex(3, 2, 0, k_Dx, k_Dy)) == 0);                              // neighbor of the tall bump -> 0
}

// -----------------------------------------------------------------------------
// (2c) Height == 0 no-op: HConvex(f, 0) = f - HMaxima(f, 0). The (input - 0) marker equals the input, so its
//      reconstruction-by-dilation converges to the input and the h-dome (input - input) is 0 at every voxel.
//      Preflight accepts Height 0 (only a negative Height is rejected); assert the entire output is zero.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::HConvexImageFilter: Height 0 yields an all-zero output", "[NXImageProcessing][HConvexImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_Dx = 12;
  constexpr usize k_Dy = 12;
  constexpr usize k_Dz = 3;
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, k_Dx, k_Dy, k_Dz, rt::MakeGradientNoisePattern<int32>(k_Dx, k_Dy, k_Dz));

  HConvexImageFilter filter;
  rt::RunReconstructionFilter<HConvexImageFilter>(filter, dataStructure, inputPath, rt::HeightAndFullyConnectedParamSetter<HConvexImageFilter>(), /*fullyConnected=*/false, /*height=*/0.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = dataStructure.getDataRefAs<DataArray<int32>>(outputPath).getDataStoreRef();
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
TEST_CASE("NXImageProcessing::HConvexImageFilter: Height beyond dynamic range returns the input (uint8 saturation)", "[NXImageProcessing][HConvexImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize k_Dx = 12;
  constexpr usize k_Dy = 12;
  constexpr usize k_Dz = 3;
  const std::vector<uint8> pattern = rt::MakeGradientNoisePattern<uint8>(k_Dx, k_Dy, k_Dz);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<uint8>(dataStructure, k_Dx, k_Dy, k_Dz, pattern);

  HConvexImageFilter filter;
  rt::RunReconstructionFilter<HConvexImageFilter>(filter, dataStructure, inputPath, rt::HeightAndFullyConnectedParamSetter<HConvexImageFilter>(), /*fullyConnected=*/false, /*height=*/1000.0);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = dataStructure.getDataRefAs<DataArray<uint8>>(outputPath).getDataStoreRef();
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
TEST_CASE("NXImageProcessing::HConvexImageFilter: Preflight scalar guard (multi-component rejected)", "[NXImageProcessing][HConvexImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure dataStructure;
  const DataPath scalarPath = rt::BuildImageFromPattern<uint8>(dataStructure, 8, 8, 8, rt::MakeGradientNoisePattern<uint8>(8, 8, 8)); // creates the geometry + CellData
  const DataPath vecPath({"Image Geometry", "CellData", "Vec"});
  auto vecStore = data_store_utilities::CreateDataStore<uint8>(dataStructure, vecPath, {8, 8, 8}, {2});
  const auto& cellAM = dataStructure.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
  DataArray<uint8>::Create(dataStructure, "Vec", vecStore, cellAM.getId());

  HConvexImageFilter filter;
  Arguments args;
  args.insertOrAssign(HConvexImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(HConvexImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(vecPath));
  args.insertOrAssign(HConvexImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(HConvexImageFilter::k_Height_Key, std::make_any<float64>(2.0));
  args.insertOrAssign(HConvexImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
  const auto result = filter.preflight(dataStructure, args);
  REQUIRE(result.output_actions.invalid());
  REQUIRE(result.output_actions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
}

// -----------------------------------------------------------------------------
// (4) Preflight: a non-finite Height (NaN or Inf) is rejected with the filter-local error code, because the
//     saturating (input - Height) marker offset is undefined for a non-finite value.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::HConvexImageFilter: Preflight non-finite Height guard", "[NXImageProcessing][HConvexImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  HConvexImageFilter filter;

  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<uint8>(dataStructure, 8, 8, 8, rt::MakeGradientNoisePattern<uint8>(8, 8, 8));

  SECTION("finite Height preflights cleanly")
  {
    auto result = filter.preflight(dataStructure, makeHConvexArguments(inputPath, 2.0));
    SIMPLNX_RESULT_REQUIRE_VALID(result.output_actions);
  }

  SECTION("invalid Heights are rejected before output creation")
  {
    const auto [height, expectedCode] =
        GENERATE(table<float64, int32>({{std::numeric_limits<float64>::quiet_NaN(), k_NonFiniteHeight}, {std::numeric_limits<float64>::infinity(), k_NonFiniteHeight}, {-1.0, k_NegativeHeight}}));
    CAPTURE(height);
    const auto args = makeHConvexArguments(inputPath, height);
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

TEST_CASE("NXImageProcessing::HConvexImageFilter: FromSIMPLJson", "[NXImageProcessing][HConvexImageFilter]")
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
TEST_CASE("NXImageProcessing::HConvexImageFilter: ITK real-image golden (HConvex)", "[NXImageProcessing][ItkGolden][HConvexImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<HConvexImageFilter>("RA-Short.nrrd", "f3a7b95a51710d51b3b73e0eb77eb1eb",
                                                        [](Arguments& args) { args.insertOrAssign(HConvexImageFilter::k_Height_Key, std::make_any<float64>(10000.0)); });
}

// NOLINTBEGIN(bugprone-chained-comparison) Catch2 comparison decomposition triggers this check.
TEST_CASE("NXImageProcessing::HConvexImageFilter: resident integration exact widening supports fractional HConvex height", "[NXImageProcessing][ImageFilterIntegration][Issue1874][HConvexImageFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  unit_test::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  const std::vector<uint8> original{2, 2, 2, 2, 4, 2, 2, 2, 2};
  const auto input = recon_test::BuildImageFromPattern<uint8>(dataStructure, 3, 3, 1, original);
  image_integration::SetGeometry(dataStructure);
  const auto converted = image_integration::ConvertToFloat32(dataStructure, input);
  const std::vector<float32> exactConverted{2, 2, 2, 2, 4, 2, 2, 2, 2};
  REQUIRE(image_integration::Read<float32>(dataStructure, converted, {1, 3, 3}, scope) == exactConverted);
  const HConvexImageFilter filter;
  auto args = image_integration::ImageArguments(filter, converted, "Contrast");
  args.insertOrAssign(HConvexImageFilter::k_Height_Key, std::make_any<float64>(0.5));
  args.insertOrAssign(HConvexImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
  image_integration::Execute(filter, dataStructure, args, scope);
  REQUIRE(image_integration::Read<float32>(dataStructure, input.replaceName("Contrast"), {1, 3, 3}, scope) == std::vector<float32>{0, 0, 0, 0, 0.5F, 0, 0, 0, 0});
  REQUIRE(image_integration::Read<float32>(dataStructure, converted, {1, 3, 3}, scope) == exactConverted);
  REQUIRE(image_integration::Read<uint8>(dataStructure, input, {1, 3, 3}, scope) == original);
  image_integration::CheckGeometry(dataStructure, {3, 3, 1});
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison)
