#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/ThresholdMaximumConnectedComponentsImageFilter.hpp"

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Filter/Parameters.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ThresholdMaximumConnectedComponentsEngine.hpp"
#include "simplnx/Utilities/MD5.hpp"

#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>
#include <nonstd/span.hpp>

#include <any>
#include <array>
#include <atomic>
#include <cmath>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
using TMCCFilter = ThresholdMaximumConnectedComponentsImageFilter;

// Sets the geometry, input, and output keys plus the four operation parameters on the Arguments. The helper runs
// preflight and execute, and requires that both steps succeed.
void RunTMCC(IFilter& filter, DataStructure& ds, const DataPath& inputPath, uint32 minimumObjectSizeInPixels, float64 upperBoundary, uint8 insideValue, uint8 outsideValue,
             const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(TMCCFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(TMCCFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(TMCCFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(TMCCFilter::k_MinimumObjectSizeInPixels_Key, std::make_any<uint32>(minimumObjectSizeInPixels));
  args.insertOrAssign(TMCCFilter::k_UpperBoundary_Key, std::make_any<float64>(upperBoundary));
  args.insertOrAssign(TMCCFilter::k_InsideValue_Key, std::make_any<uint8>(insideValue));
  args.insertOrAssign(TMCCFilter::k_OutsideValue_Key, std::make_any<uint8>(outsideValue));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}
} // namespace

namespace
{
Arguments thresholdArguments(const DataPath& inputPath, float64 cap = std::numeric_limits<float64>::infinity(), bool warn = false, uint8 inside = 1, uint8 outside = 0, uint32 minimumSize = 0)
{
  const ThresholdMaximumConnectedComponentsImageFilter filter;
  Arguments arguments = filter.getDefaultArguments();
  arguments.insertOrAssign(TMCCFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
  arguments.insertOrAssign(TMCCFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  arguments.insertOrAssign(TMCCFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  arguments.insertOrAssign(TMCCFilter::k_UpperBoundary_Key, std::make_any<float64>(cap));
  arguments.insertOrAssign(TMCCFilter::k_NonfiniteInputHandling_Key, std::make_any<ChoicesParameter::ValueType>(warn ? 1 : 0));
  arguments.insertOrAssign(TMCCFilter::k_InsideValue_Key, std::make_any<uint8>(inside));
  arguments.insertOrAssign(TMCCFilter::k_OutsideValue_Key, std::make_any<uint8>(outside));
  arguments.insertOrAssign(TMCCFilter::k_MinimumObjectSizeInPixels_Key, std::make_any<uint32>(minimumSize));
  return arguments;
}

void requireThresholdMask(const DataStructure& dataStructure, const DataPath& inputPath, const std::vector<uint8>& expected)
{
  const IDataArray* inputPtr = nullptr;
  const IDataArray* outputPtr = nullptr;
  REQUIRE_NOTHROW(inputPtr = &dataStructure.getDataRefAs<IDataArray>(inputPath));
  const DataPath outputPath = inputPath.replaceName("Output");
  REQUIRE_NOTHROW(outputPtr = &dataStructure.getDataRefAs<IDataArray>(outputPath));
  REQUIRE(outputPtr->getDataType() == DataType::uint8);
  REQUIRE(outputPtr->getTupleShape() == inputPtr->getTupleShape());
  REQUIRE(outputPtr->getComponentShape() == ShapeType{1});
  REQUIRE(outputPtr->getNumberOfTuples() == expected.size());
  const UInt8Array* maskPtr = nullptr;
  REQUIRE_NOTHROW(maskPtr = &dataStructure.getDataRefAs<UInt8Array>(outputPath));
  std::vector<uint8> actual(expected.size());
  const auto readResult = maskPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint8>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(readResult);
  REQUIRE(actual == expected);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

enum class ThresholdFaultType : uint8
{
  None,
  ReadError,
  BadAllocation,
  LengthError
};

template <class T>
class ThresholdTransferStore : public DataStore<T>
{
public:
  explicit ThresholdTransferStore(const std::vector<T>& values)
  : DataStore<T>(ShapeType{1, 1, values.size()}, ShapeType{1}, T{})
  {
    for(usize index = 0; index < values.size(); ++index)
    {
      DataStore<T>::setValue(index, values[index]);
    }
  }
  ~ThresholdTransferStore() noexcept override = default;
  ThresholdTransferStore(const ThresholdTransferStore&) = delete;
  ThresholdTransferStore(ThresholdTransferStore&&) = delete;
  ThresholdTransferStore& operator=(const ThresholdTransferStore&) = delete;
  ThresholdTransferStore& operator=(ThresholdTransferStore&&) = delete;

  IDataStore::StoreType getStoreType() const override
  {
    return IDataStore::StoreType::Empty; // Abstract bulk-transfer route only; this fixture has no disk backing.
  }
  Result<> copyIntoBuffer(usize start, nonstd::span<T> buffer) const override
  {
    ++m_Reads;
    if(m_Reads == m_FaultRead)
    {
      if(m_Fault == ThresholdFaultType::ReadError)
      {
        return MakeErrorResult(-28631, "Injected Threshold read error");
      }
      if(m_Fault == ThresholdFaultType::BadAllocation)
      {
        throw std::bad_alloc{};
      }
      if(m_Fault == ThresholdFaultType::LengthError)
      {
        throw std::length_error("Injected Threshold addressability failure");
      }
    }
    auto result = DataStore<T>::copyIntoBuffer(start, buffer);
    if(m_WarnOnRead && m_Reads == 1)
    {
      result.warnings().push_back(Warning{.code = -28633, .message = "Injected Threshold read warning"});
    }
    return result;
  }
  Result<> copyFromBuffer(usize start, nonstd::span<const T> buffer) override
  {
    ++m_Writes;
    if(m_FailWrite)
    {
      return MakeErrorResult(-28632, "Injected Threshold write error");
    }
    auto result = DataStore<T>::copyFromBuffer(start, buffer);
    if(m_CancelPtr != nullptr)
    {
      m_CancelPtr->store(true);
    }
    return result;
  }
  void setReadFault(usize read, ThresholdFaultType fault) noexcept
  {
    m_FaultRead = read;
    m_Fault = fault;
  }
  void failWrite() noexcept
  {
    m_FailWrite = true;
  }
  void warnOnRead() noexcept
  {
    m_WarnOnRead = true;
  }
  void cancelOnWrite(std::atomic_bool& cancel) noexcept
  {
    m_CancelPtr = &cancel;
  }
  usize reads() const noexcept
  {
    return m_Reads;
  }
  usize writes() const noexcept
  {
    return m_Writes;
  }

private:
  mutable usize m_Reads = 0;
  usize m_Writes = 0;
  usize m_FaultRead = 0;
  ThresholdFaultType m_Fault = ThresholdFaultType::None;
  bool m_FailWrite = false;
  bool m_WarnOnRead = false;
  std::atomic_bool* m_CancelPtr = nullptr;
};
} // namespace

TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: nonfinite policy is explicit and defaults to Error",
          "[ImageProcessing][Issue1874][ThresholdMaximumConnectedComponentsImageFilter][ThresholdSafeRed]")
{
  UnitTest::LoadPlugins();
  const ThresholdMaximumConnectedComponentsImageFilter filter;
  const Parameters parameters = filter.parameters();
  const std::string policyKey = "nonfinite_input_handling_index";

  // One choice covers image NaN and both infinities, including the no-finite-pixels fallback.
  // Inspect the public schema without passing nonfinite data to the baseline search.
  REQUIRE(parameters.contains(policyKey));
  const auto* policyPtr = dynamic_cast<const ChoicesParameter*>(parameters.at(policyKey).get());
  REQUIRE(policyPtr != nullptr);
  REQUIRE(policyPtr->humanName() == "Nonfinite Input Handling");
  REQUIRE(policyPtr->choices() == ChoicesParameter::Choices{"Error", "Warn and Continue"});
  REQUIRE(policyPtr->defaultIndex() == 0);
  const Arguments defaultArguments = filter.getDefaultArguments();
  REQUIRE(defaultArguments.value<ChoicesParameter::ValueType>(policyKey) == 0);
}

TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: initial midpoint includes the finite image minimum",
          "[ImageProcessing][Issue1874][ThresholdMaximumConnectedComponentsImageFilter][ThresholdSafeRed]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int32>(dataStructure, 2, 1, 1, std::vector<int32>{100, 102});
  const DataPath outputPath = inputPath.replaceName("Output");
  const ThresholdMaximumConnectedComponentsImageFilter filter;
  Arguments arguments = filter.getDefaultArguments();
  arguments.insertOrAssign(TMCCFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
  arguments.insertOrAssign(TMCCFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  arguments.insertOrAssign(TMCCFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  arguments.insertOrAssign(TMCCFilter::k_MinimumObjectSizeInPixels_Key, std::make_any<uint32>(0));
  arguments.insertOrAssign(TMCCFilter::k_UpperBoundary_Key, std::make_any<float64>(65536.0));
  arguments.insertOrAssign(TMCCFilter::k_InsideValue_Key, std::make_any<uint8>(1));
  arguments.insertOrAssign(TMCCFilter::k_OutsideValue_Key, std::make_any<uint8>(0));

  const auto preflightResult = filter.preflight(dataStructure, arguments);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = filter.execute(dataStructure, arguments);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(outputPath));
  REQUIRE(outputArrayPtr->getDataType() == DataType::uint8);
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 1, 2});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const UInt8Array* maskPtr = nullptr;
  REQUIRE_NOTHROW(maskPtr = &dataStructure.getDataRefAs<UInt8Array>(outputPath));

  // Width 2 keeps the approved stopping rule: midpoint 101 gives the literal inclusive-band mask [0, 1].
  // The baseline int32 arithmetic and the finite 65536 boundary conversion are defined for this fixture.
  CHECK(maskPtr->getValue(0) == uint8{0});
  CHECK(maskPtr->getValue(1) == uint8{1});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: native integer endpoints retain exact width-two masks",
                   "[ImageProcessing][Issue1874][ThresholdMaximumConnectedComponentsImageFilter]", int8, uint8, int16, uint16, int32, uint32, int64, uint64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const bool nearMaximum = GENERATE(false, true);
  const TestType low = nearMaximum ? static_cast<TestType>(std::numeric_limits<TestType>::max() - TestType{2}) : std::numeric_limits<TestType>::lowest();
  const TestType high = static_cast<TestType>(low + TestType{2});
  DataStructure dataStructure;
  const auto inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 2, 1, 1, {low, high});
  const ThresholdMaximumConnectedComponentsImageFilter filter;
  const auto result = filter.execute(dataStructure, thresholdArguments(inputPath));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(result.result.warnings().empty());
  // Native midpoint is low+1 even at int64/uint64 endpoints beyond Float64's exact integer range.
  requireThresholdMask(dataStructure, inputPath, {0, 1});
}

TEMPLATE_TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: full native integer spans keep the absolute-two heuristic",
                   "[ImageProcessing][Issue1874][ThresholdMaximumConnectedComponentsImageFilter]", int8, uint8, int16, uint16, int32, uint32, int64, uint64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const auto inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 2, 1, 1, {std::numeric_limits<TestType>::lowest(), std::numeric_limits<TestType>::max()});
  const ThresholdMaximumConnectedComponentsImageFilter filter;
  const auto result = filter.execute(dataStructure, thresholdArguments(inputPath));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(result.result.warnings().empty());
  // Two adjacent pixels always score one component. Left ties halve the odd span until width 1; midpoint is then the native minimum.
  requireThresholdMask(dataStructure, inputPath, {1, 1});
}

TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: caps clamp before 64-bit conversion and preserve lower truncation",
          "[ImageProcessing][Issue1874][ThresholdMaximumConnectedComponentsImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  SECTION("signed exclusive high and finite overflow")
  {
    const float64 cap = GENERATE(0x1p63, std::numeric_limits<float64>::max(), std::numeric_limits<float64>::infinity());
    DataStructure dataStructure;
    const auto inputPath = rt::BuildImageFromPattern<int64>(dataStructure, 2, 1, 1, {std::numeric_limits<int64>::max() - 2, std::numeric_limits<int64>::max()});
    const ThresholdMaximumConnectedComponentsImageFilter filter;
    const auto result = filter.execute(dataStructure, thresholdArguments(inputPath, cap));
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    REQUIRE(result.result.warnings().empty());
    requireThresholdMask(dataStructure, inputPath, {0, 1});
  }
  SECTION("unsigned exclusive high and finite overflow")
  {
    const float64 cap = GENERATE(0x1p64, std::numeric_limits<float64>::max(), std::numeric_limits<float64>::infinity());
    DataStructure dataStructure;
    const auto inputPath = rt::BuildImageFromPattern<uint64>(dataStructure, 2, 1, 1, {std::numeric_limits<uint64>::max() - 2, std::numeric_limits<uint64>::max()});
    const ThresholdMaximumConnectedComponentsImageFilter filter;
    const auto result = filter.execute(dataStructure, thresholdArguments(inputPath, cap));
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    REQUIRE(result.result.warnings().empty());
    requireThresholdMask(dataStructure, inputPath, {0, 1});
  }
  SECTION("unsigned negative fraction remains zero")
  {
    DataStructure dataStructure;
    const auto inputPath = rt::BuildImageFromPattern<uint8>(dataStructure, 2, 1, 1, {0, 2});
    const ThresholdMaximumConnectedComponentsImageFilter filter;
    const auto result = filter.execute(dataStructure, thresholdArguments(inputPath, -0.5));
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    REQUIRE(result.result.warnings().empty());
    requireThresholdMask(dataStructure, inputPath, {1, 0});
  }
  SECTION("signed low fraction truncates into range")
  {
    DataStructure dataStructure;
    const auto inputPath = rt::BuildImageFromPattern<int8>(dataStructure, 2, 1, 1, {-128, -126});
    const ThresholdMaximumConnectedComponentsImageFilter filter;
    const auto result = filter.execute(dataStructure, thresholdArguments(inputPath, -128.75));
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    REQUIRE(result.result.warnings().empty());
    requireThresholdMask(dataStructure, inputPath, {1, 0});
  }
}

TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: low and empty bands return warned Outside values",
          "[ImageProcessing][Issue1874][ThresholdMaximumConnectedComponentsImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const float64 cap = GENERATE(-1.0, -std::numeric_limits<float64>::infinity(), 99.0);
  const uint8 outside = GENERATE(uint8{0}, uint8{7});
  DataStructure dataStructure;
  const auto inputPath = rt::BuildImageFromPattern<uint64>(dataStructure, 2, 1, 1, {100, 102});
  const ThresholdMaximumConnectedComponentsImageFilter filter;
  const auto result = filter.execute(dataStructure, thresholdArguments(inputPath, cap, false, 1, outside));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(result.result.warnings().size() == 1);
  REQUIRE(result.result.warnings().front().code == -28615);
  requireThresholdMask(dataStructure, inputPath, {outside, outside});
}

TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: NaN Upper Boundary is a hard parameter error in both image modes",
          "[ImageProcessing][Issue1874][ThresholdMaximumConnectedComponentsImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const bool warn = GENERATE(false, true);
  DataStructure dataStructure;
  const auto inputPath = rt::BuildImageFromPattern<uint64>(dataStructure, 2, 1, 1, {100, 102});
  const ThresholdMaximumConnectedComponentsImageFilter filter;
  const auto result = filter.preflight(dataStructure, thresholdArguments(inputPath, std::numeric_limits<float64>::quiet_NaN(), warn));
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().front().code == -28610);
  REQUIRE(result.outputActions.errors().front().message.find("Upper Boundary") != std::string::npos);
  REQUIRE(dataStructure.getDataAs<IDataArray>(inputPath.replaceName("Output")) == nullptr);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: one image policy covers every nonfinite class and order",
          "[ImageProcessing][Issue1874][ThresholdMaximumConnectedComponentsImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const usize placement = GENERATE(usize{0}, usize{1}, usize{2});
  const bool warn = GENERATE(false, true);
  const uint8 outside = GENERATE(uint8{0}, uint8{7});
  std::vector<float64> values{0, 2};
  values.insert(values.begin() + static_cast<std::vector<float64>::difference_type>(placement),
                {std::numeric_limits<float64>::quiet_NaN(), -std::numeric_limits<float64>::infinity(), std::numeric_limits<float64>::infinity()});
  std::vector<uint8> expected{outside, 1};
  expected.insert(expected.begin() + static_cast<std::vector<uint8>::difference_type>(placement), {outside, outside, outside});
  DataStructure dataStructure;
  const auto inputPath = rt::BuildImageFromPattern<float64>(dataStructure, values.size(), 1, 1, values);
  const ThresholdMaximumConnectedComponentsImageFilter filter;
  const auto result = filter.execute(dataStructure, thresholdArguments(inputPath, std::numeric_limits<float64>::infinity(), warn, 1, outside));
  if(!warn)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
    REQUIRE(result.result.errors().front().code == -28611);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    REQUIRE(result.result.warnings().size() == 1);
    REQUIRE(result.result.warnings().front().code == -28614);
    REQUIRE(result.result.warnings().front().message.find("1 NaN, 1 negative infinity and 1 positive infinity") != std::string::npos);
    requireThresholdMask(dataStructure, inputPath, expected);
  }
}

TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: all nonfinite Warn skips search and calculates no threshold",
          "[ImageProcessing][Issue1874][ThresholdMaximumConnectedComponentsImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const int32 kind = GENERATE(0, 1, 2, 3);
  const uint8 outside = GENERATE(uint8{0}, uint8{7});
  const uint8 inside = GENERATE(uint8{0}, uint8{1});
  const std::array<float64, 3> nonfinite{std::numeric_limits<float64>::quiet_NaN(), -std::numeric_limits<float64>::infinity(), std::numeric_limits<float64>::infinity()};
  const std::vector<float64> values = kind == 3 ? std::vector<float64>{nonfinite.at(2), nonfinite.at(0), nonfinite.at(1)} : std::vector<float64>(3, nonfinite.at(static_cast<usize>(kind)));
  DataStructure dataStructure;
  const auto inputPath = rt::BuildImageFromPattern<float64>(dataStructure, 3, 1, 1, values);
  const ThresholdMaximumConnectedComponentsImageFilter filter;
  const auto result = filter.execute(dataStructure, thresholdArguments(inputPath, 65536, true, inside, outside, 10));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(result.result.warnings().size() == 1);
  REQUIRE(result.result.warnings().front().code == -28614);
  REQUIRE(result.result.warnings().front().message.find("no threshold was calculated") != std::string::npos);
  requireThresholdMask(dataStructure, inputPath, {outside, outside, outside});
}

TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: floating rounding distinguishes convergence from a repeated wide state",
          "[ImageProcessing][Issue1874][ThresholdMaximumConnectedComponentsImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const bool roundUp = GENERATE(false, true);
  DataStructure dataStructure;
  const auto inputPath = rt::BuildImageFromPattern<float32>(dataStructure, 2, 1, 1, roundUp ? std::vector<float32>{33554436.0F, 33554440.0F} : std::vector<float32>{33554432.0F, 33554436.0F});
  const ThresholdMaximumConnectedComponentsImageFilter filter;
  const auto result = filter.execute(dataStructure, thresholdArguments(inputPath));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(result.result.warnings().size() == (roundUp ? 1 : 0));
  if(roundUp)
  {
    REQUIRE(result.result.warnings().front().code == -28616);
  }
  // A lower rounded midpoint moves high down to low (width 0); only the upper-rounded state repeats at width 4.
  requireThresholdMask(dataStructure, inputPath, roundUp ? std::vector<uint8>{0, 1} : std::vector<uint8>{1, 1});
}

TEMPLATE_TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: opposite finite floating extrema keep finite search candidates",
                   "[ImageProcessing][Issue1874][ThresholdMaximumConnectedComponentsImageFilter]", float32, float64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const auto inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 2, 1, 1, {-std::numeric_limits<TestType>::max(), std::numeric_limits<TestType>::max()});
  const ThresholdMaximumConnectedComponentsImageFilter filter;
  const auto result = filter.execute(dataStructure, thresholdArguments(inputPath));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(result.result.warnings().size() == 1);
  REQUIRE(result.result.warnings().front().code == -28616);
  // Every score ties. Near -MAX, adjacent native endpoints are farther than 2 apart; ties-to-even retains the upper candidate.
  requireThresholdMask(dataStructure, inputPath, {0, 1});
}

TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: nonzero scoring minimum sizes and the fixed scale stop are preserved",
          "[ImageProcessing][Issue1874][ThresholdMaximumConnectedComponentsImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const ThresholdMaximumConnectedComponentsImageFilter filter;
  SECTION("strict-right score selects isolated bright peaks")
  {
    const auto inputPath = rt::BuildImageFromPattern<int32>(dataStructure, 8, 1, 1, {0, 4, 6, 4, 6, 4, 0, 8});
    const auto result = filter.execute(dataStructure, thresholdArguments(inputPath));
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    requireThresholdMask(dataStructure, inputPath, {0, 0, 1, 0, 1, 0, 0, 1});
  }
  SECTION("minimum size affects scoring but does not remove the final singleton")
  {
    const auto inputPath = rt::BuildImageFromPattern<int32>(dataStructure, 8, 1, 1, {0, 4, 6, 4, 6, 4, 0, 8});
    const auto result = filter.execute(dataStructure, thresholdArguments(inputPath, 65536, false, 1, 0, 2));
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    requireThresholdMask(dataStructure, inputPath, {0, 1, 1, 1, 1, 1, 0, 1});
  }
  SECTION("size exactly one qualifies for a minimum of one")
  {
    const auto inputPath = rt::BuildImageFromPattern<int32>(dataStructure, 8, 1, 1, {0, 4, 6, 4, 6, 4, 0, 8});
    const auto result = filter.execute(dataStructure, thresholdArguments(inputPath, 65536, false, 1, 0, 1));
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    requireThresholdMask(dataStructure, inputPath, {0, 0, 1, 0, 1, 0, 0, 1});
  }
  SECTION("zero Inside and nonzero Outside score the complement")
  {
    const auto inputPath = rt::BuildImageFromPattern<int32>(dataStructure, 8, 1, 1, {0, 4, 0, 4, 0, 4, 0, 8});
    const auto result = filter.execute(dataStructure, thresholdArguments(inputPath, 65536, false, 0, 1));
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    requireThresholdMask(dataStructure, inputPath, {1, 0, 1, 0, 1, 0, 1, 0});
  }
  SECTION("both nonzero score the whole image and ties select left")
  {
    const auto inputPath = rt::BuildImageFromPattern<int32>(dataStructure, 8, 1, 1, {0, 4, 6, 4, 6, 4, 0, 8});
    const auto result = filter.execute(dataStructure, thresholdArguments(inputPath, 65536, false, 1, 7));
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    requireThresholdMask(dataStructure, inputPath, {7, 1, 1, 1, 1, 1, 7, 1});
  }
  SECTION("binary width one still skips search")
  {
    const auto inputPath = rt::BuildImageFromPattern<int32>(dataStructure, 4, 1, 1, {0, 1, 0, 1});
    const auto result = filter.execute(dataStructure, thresholdArguments(inputPath));
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    requireThresholdMask(dataStructure, inputPath, {1, 1, 1, 1});
  }
  SECTION("constant negative data use their actual constant")
  {
    const auto inputPath = rt::BuildImageFromPattern<int32>(dataStructure, 2, 1, 1, {-5, -5});
    const auto result = filter.execute(dataStructure, thresholdArguments(inputPath));
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    requireThresholdMask(dataStructure, inputPath, {1, 1});
  }
  SECTION("normalized width one remains distinct from the scaled image")
  {
    const bool scaled = GENERATE(false, true);
    const auto inputPath = rt::BuildImageFromPattern<float64>(dataStructure, 8, 1, 1, scaled ? std::vector<float64>{0, 40, 10, 40, 10, 40, 0, 100} : std::vector<float64>{0, .4, .1, .4, .1, .4, 0, 1});
    const auto result = filter.execute(dataStructure, thresholdArguments(inputPath));
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    requireThresholdMask(dataStructure, inputPath, scaled ? std::vector<uint8>{0, 1, 0, 1, 0, 1, 0, 1} : std::vector<uint8>{0, 0, 0, 0, 0, 0, 0, 1});
  }
}

TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: old JSON receives Error and version two preserves explicit Warn",
          "[ImageProcessing][Issue1874][ThresholdMaximumConnectedComponentsImageFilter]")
{
  UnitTest::LoadPlugins();
  const ThresholdMaximumConnectedComponentsImageFilter filter;
  REQUIRE(filter.parametersVersion() == 2);
  auto json = filter.toJson(filter.getDefaultArguments());
  json.erase("nonfinite_input_handling_index");
  json["parameters_version"] = 1;
  const auto oldArguments = filter.fromJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(oldArguments);
  REQUIRE(oldArguments.value().value<ChoicesParameter::ValueType>(TMCCFilter::k_NonfiniteInputHandling_Key) == 0);
  REQUIRE(oldArguments.warnings().size() == 1);
  REQUIRE(oldArguments.warnings().front().code == -5432);
  auto arguments = filter.getDefaultArguments();
  arguments.insertOrAssign(TMCCFilter::k_NonfiniteInputHandling_Key, std::make_any<ChoicesParameter::ValueType>(1));
  const auto roundTrip = filter.fromJson(filter.toJson(arguments));
  SIMPLNX_RESULT_REQUIRE_VALID(roundTrip);
  REQUIRE(roundTrip.value().value<ChoicesParameter::ValueType>(TMCCFilter::k_NonfiniteInputHandling_Key) == 1);
}

TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: engine rejects image nonfinites before empty-cap output and distinguishes empty input",
          "[ImageProcessing][Issue1874][ThresholdMaximumConnectedComponentsImageFilter]")
{
  UnitTest::LoadPlugins();
  const std::atomic_bool shouldCancel{false};
  const float64 nonfinite = GENERATE(std::numeric_limits<float64>::quiet_NaN(), -std::numeric_limits<float64>::infinity(), std::numeric_limits<float64>::infinity());
  const ThresholdTransferStore<float64> input({0, nonfinite, 2});
  ThresholdTransferStore<uint8> output({99, 99, 99});
  const ImageProcessing::ThresholdMaxCCOptions options{.UpperBoundary = -std::numeric_limits<float64>::infinity()};
  const auto result = ImageProcessing::ApplyThresholdMaximumConnectedComponents(input, output, SizeVec3{3, 1, 1}, options, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().front().code == -28611);
  REQUIRE(input.reads() == 1);
  REQUIRE(output.writes() == 0);
  REQUIRE(output.getValue(0) == 99);
  const ThresholdTransferStore<float64> emptyInput(std::vector<float64>{});
  ThresholdTransferStore<uint8> emptyOutput(std::vector<uint8>{});
  const auto emptyResult = ImageProcessing::ApplyThresholdMaximumConnectedComponents(emptyInput, emptyOutput, SizeVec3{0, 1, 1}, options, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(emptyResult);
  REQUIRE(emptyResult.warnings().empty());
  REQUIRE(emptyInput.reads() == 0);
  REQUIRE(emptyOutput.writes() == 0);
}

TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: no-finite fallback avoids count and reread passes and retains warnings on write failure",
          "[ImageProcessing][Issue1874][ThresholdMaximumConnectedComponentsImageFilter]")
{
  UnitTest::LoadPlugins();
  const bool failWrite = GENERATE(false, true);
  const ThresholdTransferStore<float64> input({std::numeric_limits<float64>::quiet_NaN(), -std::numeric_limits<float64>::infinity(), std::numeric_limits<float64>::infinity()});
  ThresholdTransferStore<uint8> output({99, 99, 99});
  if(failWrite)
  {
    output.failWrite();
  }
  const std::atomic_bool shouldCancel{false};
  const ImageProcessing::ThresholdMaxCCOptions options{.OutsideValue = 7, .WarnOnNonfinite = true};
  const auto result = ImageProcessing::ApplyThresholdMaximumConnectedComponents(input, output, SizeVec3{3, 1, 1}, options, shouldCancel);
  REQUIRE(input.reads() == 1);
  REQUIRE(output.writes() == 1);
  REQUIRE(result.warnings().size() == 1);
  REQUIRE(result.warnings().front().code == -28614);
  if(failWrite)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -28632);
    REQUIRE(output.getValue(0) == 99);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(output.getValue(0) == 7);
    REQUIRE(output.getValue(2) == 7);
  }
}

TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: count failures remain strict under image Warn",
          "[ImageProcessing][Issue1874][ThresholdMaximumConnectedComponentsImageFilter]")
{
  UnitTest::LoadPlugins();
  const auto fault = GENERATE(ThresholdFaultType::ReadError, ThresholdFaultType::BadAllocation, ThresholdFaultType::LengthError);
  ThresholdTransferStore<float64> input({0, std::numeric_limits<float64>::quiet_NaN(), 4});
  input.setReadFault(2, fault); // Initial summary succeeds; the first component-count read fails.
  ThresholdTransferStore<uint8> output({99, 99, 99});
  const std::atomic_bool shouldCancel{false};
  const ImageProcessing::ThresholdMaxCCOptions options{.WarnOnNonfinite = true};
  const auto result = ImageProcessing::ApplyThresholdMaximumConnectedComponents(input, output, SizeVec3{3, 1, 1}, options, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  int32 expectedCode = -28631;
  if(fault == ThresholdFaultType::BadAllocation)
  {
    expectedCode = -8774;
  }
  else if(fault == ThresholdFaultType::LengthError)
  {
    expectedCode = -8776;
  }
  REQUIRE(result.errors().front().code == expectedCode);
  REQUIRE(result.warnings().size() == 1);
  REQUIRE(result.warnings().front().code == -28614);
  REQUIRE(output.writes() == 0);
}

TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: summary and final transfers preserve failures and collected diagnostics",
          "[ImageProcessing][Issue1874][ThresholdMaximumConnectedComponentsImageFilter]")
{
  UnitTest::LoadPlugins();
  const int32 stage = GENERATE(0, 1, 2, 3, 4, 5, 6);
  ThresholdTransferStore<float64> input({0, std::numeric_limits<float64>::quiet_NaN(), 2});
  ThresholdTransferStore<uint8> output({99, 99, 99});
  int32 expectedCode = -28631;
  if(stage == 0)
  {
    input.setReadFault(1, ThresholdFaultType::ReadError);
  }
  else if(stage == 1)
  {
    input.setReadFault(1, ThresholdFaultType::BadAllocation);
    expectedCode = -28617;
  }
  else if(stage == 2)
  {
    input.setReadFault(1, ThresholdFaultType::LengthError);
    expectedCode = -28618;
  }
  else if(stage == 3)
  {
    input.warnOnRead();
    input.setReadFault(2, ThresholdFaultType::ReadError);
  }
  else if(stage == 4)
  {
    output.failWrite();
    expectedCode = -28632;
  }
  else
  {
    input.warnOnRead();
    input.setReadFault(2, stage == 5 ? ThresholdFaultType::BadAllocation : ThresholdFaultType::LengthError);
    expectedCode = stage == 5 ? -28617 : -28618;
  }
  const std::atomic_bool shouldCancel{false};
  const ImageProcessing::ThresholdMaxCCOptions options{.WarnOnNonfinite = true};
  const auto result = ImageProcessing::ApplyThresholdMaximumConnectedComponents(input, output, SizeVec3{3, 1, 1}, options, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().front().code == expectedCode);
  REQUIRE(result.warnings().size() == (stage == 3 || stage >= 5 ? 2 : (stage == 4 ? 1 : 0)));
  if(stage == 3 || stage >= 5)
  {
    REQUIRE(result.warnings().at(0).code == -28633);
    REQUIRE(result.warnings().at(1).code == -28614);
  }
  REQUIRE(output.getValue(0) == 99);
}

TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: engine cancellation keeps warning Results without inventing a local cancellation error",
          "[ImageProcessing][Issue1874][ThresholdMaximumConnectedComponentsImageFilter]")
{
  UnitTest::LoadPlugins();
  const bool cancelBefore = GENERATE(false, true);
  std::atomic_bool shouldCancel{cancelBefore};
  const ThresholdTransferStore<float64> input({0, std::numeric_limits<float64>::quiet_NaN(), 2});
  ThresholdTransferStore<uint8> output({99, 99, 99});
  output.cancelOnWrite(shouldCancel);
  const ImageProcessing::ThresholdMaxCCOptions options{.WarnOnNonfinite = true};
  const auto result = ImageProcessing::ApplyThresholdMaximumConnectedComponents(input, output, SizeVec3{3, 1, 1}, options, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE(shouldCancel);
  REQUIRE(input.reads() == (cancelBefore ? 0 : 2));
  REQUIRE(output.writes() == (cancelBefore ? 0 : 1));
  REQUIRE(result.warnings().size() == (cancelBefore ? 0 : 1));
  if(!cancelBefore)
  {
    REQUIRE(result.warnings().front().code == -28614);
  }
}

TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: direct engine validates hard parameters and output shape before transfer",
          "[ImageProcessing][Issue1874][ThresholdMaximumConnectedComponentsImageFilter]")
{
  UnitTest::LoadPlugins();
  const bool nanParameter = GENERATE(false, true);
  const ThresholdTransferStore<float64> input({std::numeric_limits<float64>::quiet_NaN(), std::numeric_limits<float64>::infinity()});
  ThresholdTransferStore<uint8> output({99});
  const std::atomic_bool shouldCancel{false};
  const ImageProcessing::ThresholdMaxCCOptions options{.UpperBoundary = nanParameter ? std::numeric_limits<float64>::quiet_NaN() : 65536, .WarnOnNonfinite = true};
  const auto result = ImageProcessing::ApplyThresholdMaximumConnectedComponents(input, output, SizeVec3{2, 1, 1}, options, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().front().code == (nanParameter ? -28610 : -28612));
  REQUIRE(input.reads() == 0);
  REQUIRE(output.writes() == 0);
}

TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: resident and owned multi-block summaries use finite native extrema",
          "[ImageProcessing][Issue1874][ThresholdMaximumConnectedComponentsImageFilter]")
{
  UnitTest::LoadPlugins();
  constexpr usize count = 131073;
  std::vector<float32> values(count, 0);
  values.at(1) = 2;
  values.at(65536) = std::numeric_limits<float32>::quiet_NaN();
  values.at(count - 1) = std::numeric_limits<float32>::infinity();
  const std::atomic_bool shouldCancel{false};
  SECTION("owned bulk-transfer buffer")
  {
    const ThresholdTransferStore<float32> input(values);
    const auto result = ImageProcessing::detail::ScanThresholdMaxCC(input, true, shouldCancel);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(result.value().Minimum == 0);
    REQUIRE(result.value().Maximum == 2);
    REQUIRE(result.value().FiniteCount == count - 2);
    REQUIRE(result.value().NaNCount == 1);
    REQUIRE(result.value().PositiveInfinityCount == 1);
  }
  SECTION("direct resident span")
  {
    DataStore<float32> input(ShapeType{1, 1, count}, ShapeType{1}, 0.0F);
    const auto writeResult = input.copyFromBuffer(0, nonstd::span<const float32>(values.data(), values.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
    const auto result = ImageProcessing::detail::ScanThresholdMaxCC(input, true, shouldCancel);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(result.value().Minimum == 0);
    REQUIRE(result.value().Maximum == 2);
    REQUIRE(result.value().FiniteCount == count - 2);
    REQUIRE(result.value().NaNCount == 1);
    REQUIRE(result.value().PositiveInfinityCount == 1);
  }
}

TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: native span and cap math retain exact bounds and ordinary floating rounding",
          "[ImageProcessing][Issue1874][ThresholdMaximumConnectedComponentsImageFilter]")
{
  UnitTest::LoadPlugins();
  REQUIRE(ImageProcessing::detail::ThresholdMaxCCSpanExceedsTwo(std::numeric_limits<int8>::lowest(), std::numeric_limits<int8>::max()));
  REQUIRE(ImageProcessing::detail::ThresholdMaxCCSpanExceedsTwo(std::numeric_limits<int64>::lowest(), std::numeric_limits<int64>::max()));
  REQUIRE(ImageProcessing::detail::ThresholdMaxCCSpanExceedsTwo(-std::numeric_limits<float64>::max(), std::numeric_limits<float64>::max()));
  REQUIRE_FALSE(ImageProcessing::detail::ThresholdMaxCCSpanExceedsTwo(-1.0, 1.0));
  REQUIRE(ImageProcessing::detail::ConvertThresholdMaxCCCap<float32>(33554438.0).Value == 33554440.0F);
  REQUIRE(ImageProcessing::detail::ConvertThresholdMaxCCCap<float32>(33554434.0).Value == 33554432.0F);
  REQUIRE(ImageProcessing::detail::ConvertThresholdMaxCCCap<float32>(std::numeric_limits<float64>::max()).Value == std::numeric_limits<float32>::max());
  REQUIRE(ImageProcessing::detail::ConvertThresholdMaxCCCap<float32>(-std::numeric_limits<float64>::max()).Empty);
  REQUIRE(ImageProcessing::detail::ConvertThresholdMaxCCCap<int64>(std::nextafter(0x1p63, 0.0)).Value == std::numeric_limits<int64>::max() - 1023);
  REQUIRE(ImageProcessing::detail::ConvertThresholdMaxCCCap<uint64>(std::nextafter(0x1p64, 0.0)).Value == std::numeric_limits<uint64>::max() - 2047);
}

// -----------------------------------------------------------------------------
// (1) Preflight guards: a multi-component input is rejected (requireScalar); a valid scalar input (of ANY numeric
//     type, incl. float) produces a FIXED uint8 output array (AlwaysUInt8).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: preflight guards", "[ImageProcessing][ThresholdMaximumConnectedComponentsImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  SECTION("multi-component rejected")
  {
    DataStructure ds;
    const DataPath scalarPath = rt::BuildImageFromPattern<int32>(ds, 6, 6, 1, std::vector<int32>(36, 0));
    const DataPath vecPath({"Image Geometry", "CellData", "Vec"});
    auto vecStore = DataStoreUtilities::CreateDataStore<int32>(ds, vecPath, {1, 6, 6}, {3});
    const AttributeMatrix* cellMatrixPtr = nullptr;
    REQUIRE_NOTHROW(cellMatrixPtr = &ds.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"})));
    DataArray<int32>::Create(ds, "Vec", vecStore, cellMatrixPtr->getId());
    const DataPath geomPath = vecPath.getParent().getParent();
    TMCCFilter filter;
    Arguments args;
    args.insertOrAssign(TMCCFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(TMCCFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(vecPath));
    args.insertOrAssign(TMCCFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(TMCCFilter::k_MinimumObjectSizeInPixels_Key, std::make_any<uint32>(0u));
    args.insertOrAssign(TMCCFilter::k_UpperBoundary_Key, std::make_any<float64>(65536.0));
    args.insertOrAssign(TMCCFilter::k_InsideValue_Key, std::make_any<uint8>(uint8{1}));
    args.insertOrAssign(TMCCFilter::k_OutsideValue_Key, std::make_any<uint8>(uint8{0}));
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
  }
  SECTION("scalar input (incl. float) creates a uint8 output")
  {
    DataStructure ds;
    const std::vector<float32> field = rt::MakeGradientNoisePattern<float32>(12, 12, 1);
    const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, 12, 12, 1, field);
    TMCCFilter filter;
    RunTMCC(filter, ds, inputPath, /*minimumObjectSizeInPixels=*/0u, /*upperBoundary=*/65536.0, /*insideValue=*/1u, /*outsideValue=*/0u);
    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    const IDataArray* outputPtr = nullptr;
    REQUIRE_NOTHROW(outputPtr = &ds.getDataRefAs<IDataArray>(outputPath));
    REQUIRE(outputPtr->getDataType() == DataType::uint8);
    UnitTest::CheckArraysInheritTupleDims(ds);
  }
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden -- duplicates ITKThresholdMaximumConnectedComponentsImageTest.cpp on our ITK-free
// filter. The output is a fixed uint8 binary image (AlwaysUInt8). The durable golden is an md5 pin. The helper pins
// ForceInCore.
//   default:    cthead1.png, defaults (MinimumObjectSizeInPixels 0, UpperBoundary 65536, Inside 1, Outside 0).
//   parameters: cthead1.png, MinimumObjectSizeInPixels = 40 AND UpperBoundary = 150.
// The float fixture below has a separate corrected-midpoint regression with an active legacy reference.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: ITK real-image golden (default)", "[ImageProcessing][ItkGolden][ThresholdMaximumConnectedComponentsImageFilter]")
{
  UnitTest::LoadPlugins();
  rt::RunReconstructionMd5ItkGolden<ThresholdMaximumConnectedComponentsImageFilter>("cthead1.png", "c84b75c78c33844251a1095d9cbcffb9");
}

TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: ITK real-image golden (parameters)", "[ImageProcessing][ItkGolden][ThresholdMaximumConnectedComponentsImageFilter]")
{
  UnitTest::LoadPlugins();
  rt::RunReconstructionMd5ItkGolden<ThresholdMaximumConnectedComponentsImageFilter>("cthead1.png", "27c6cf8494fcc4e414f1c420e7a9ca6f", [](Arguments& args) {
    args.insertOrAssign(ThresholdMaximumConnectedComponentsImageFilter::k_MinimumObjectSizeInPixels_Key, std::make_any<uint32>(40u));
    args.insertOrAssign(ThresholdMaximumConnectedComponentsImageFilter::k_UpperBoundary_Key, std::make_any<float64>(150.0));
  });
}

TEST_CASE("ImageProcessing::ThresholdMaximumConnectedComponentsImageFilter: corrected-midpoint real-image regression (float)",
          "[ImageProcessing][Issue1874][ThresholdMaximumConnectedComponentsImageFilter][MidpointMigration]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath geometryPath({"Image Geometry"});
  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  const auto readResult = ip_golden::ReadInputImage(dataStructure, ip_golden::InputPath("RA-Float.nrrd"), geometryPath, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readResult);
  const IDataArray* inputArrayPtr = nullptr;
  REQUIRE_NOTHROW(inputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(inputPath));
  REQUIRE(inputArrayPtr->getDataType() == DataType::float32);
  REQUIRE(inputArrayPtr->getTupleShape() == ShapeType{64, 64, 64});
  REQUIRE(inputArrayPtr->getComponentShape() == ShapeType{1});
  REQUIRE(inputArrayPtr->getNumberOfTuples() == 262144);
  const Float32Array* inputPtr = nullptr;
  REQUIRE_NOTHROW(inputPtr = &dataStructure.getDataRefAs<Float32Array>(inputPath));

  // An independent six-neighbor set/stack trace reproduced both hashes twice; correcting only the seed
  // also reproduces the new mask. See the issue-1874 companion's supporting/threshold-ra-float-evidence.{py,json}.
  // These literal cutoffs classify the real fixture without re-running production search to create expectations.
  constexpr float32 legacyThreshold = 24164.05859375F;
  constexpr float32 correctedThreshold = 24166.107421875F;
  constexpr float32 upperBoundary = 65536.0F;
  std::vector<uint8> legacyMask(inputPtr->getNumberOfTuples());
  std::vector<uint8> correctedMask(inputPtr->getNumberOfTuples());
  usize legacyForeground = 0;
  usize correctedForeground = 0;
  usize changedPixels = 0;
  bool allFinite = true;
  float32 minimum = std::numeric_limits<float32>::max();
  float32 maximum = std::numeric_limits<float32>::lowest();
  for(usize index = 0; index < inputPtr->getNumberOfTuples(); ++index)
  {
    const float32 value = inputPtr->getValue(index);
    allFinite = allFinite && std::isfinite(value);
    if(value < minimum)
    {
      minimum = value;
    }
    if(value > maximum)
    {
      maximum = value;
    }
    const bool legacyInside = value >= legacyThreshold && value <= upperBoundary;
    const bool correctedInside = value >= correctedThreshold && value <= upperBoundary;
    legacyMask[index] = legacyInside ? uint8{1} : uint8{0};
    correctedMask[index] = correctedInside ? uint8{1} : uint8{0};
    legacyForeground += static_cast<usize>(legacyInside);
    correctedForeground += static_cast<usize>(correctedInside);
    changedPixels += static_cast<usize>(legacyInside != correctedInside);
  }
  REQUIRE(allFinite);
  REQUIRE(minimum == -1146.0F);
  REQUIRE(maximum == 32767.0F);
  REQUIRE(legacyForeground == 99487);
  REQUIRE(correctedForeground == 99328);
  REQUIRE(changedPixels == 159);

  MD5 legacyHash;
  legacyHash.update(legacyMask.data(), static_cast<MD5::size_type>(legacyMask.size()));
  legacyHash.finalize();
  REQUIRE(legacyHash.hexdigest() == "e475b27bd0dd66ede330c4eab93c17e9"); // Active reference to the legacy segmentation.
  MD5 correctedHash;
  correctedHash.update(correctedMask.data(), static_cast<MD5::size_type>(correctedMask.size()));
  correctedHash.finalize();
  REQUIRE(correctedHash.hexdigest() == "63d4c611a5633ba010f1857401b13150");

  const ThresholdMaximumConnectedComponentsImageFilter filter;
  const auto preflight = filter.preflight(dataStructure, thresholdArguments(inputPath, 65536.0));
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto result = filter.execute(dataStructure, thresholdArguments(inputPath, 65536.0));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(result.result.warnings().empty());
  requireThresholdMask(dataStructure, inputPath, correctedMask);
  REQUIRE(ip_golden::ComputeMd5Hash(dataStructure, inputPath.replaceName("Output")) == "63d4c611a5633ba010f1857401b13150");
  // Independent flood-fill evidence gives 3715 legacy versus 3712 corrected components. The retained search
  // can select fewer components after the arithmetic fix; this regression is not an ITK-parity or optimum claim.
}
