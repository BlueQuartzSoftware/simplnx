#include "SquareImageFilterTestUtils.hpp"

#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/ConnectedComponentImageFilter.hpp"
#include "NXImageProcessing/Filters/ReadMhaFileFilter.hpp"
#include "NXImageProcessing/Filters/SquareImageFilter.hpp"
#include "NXImageProcessing/Filters/WriteImageFilter.hpp"
#include "NXImageProcessing/NXImageProcessing_test_dirs.hpp"

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/AlgorithmTestScope.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <catch2/catch.hpp>
#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <any>
#include <array>
#include <atomic>
#include <cmath>
#include <filesystem>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
// This resident test double selects the buffered range-scan route and signals the first completed read.
class SquareRangeReadStore final : public DataStore<int64>
{
public:
  explicit SquareRangeReadStore(std::function<void(Result<>&)> onFirstRead)
  : DataStore<int64>(ShapeType{1, 1, 2}, ShapeType{1}, 5)
  , m_OnFirstRead(std::move(onFirstRead))
  {
  }

  ~SquareRangeReadStore() noexcept override = default;
  SquareRangeReadStore(const SquareRangeReadStore&) = delete;
  SquareRangeReadStore(SquareRangeReadStore&&) = delete;
  SquareRangeReadStore& operator=(const SquareRangeReadStore&) = delete;
  SquareRangeReadStore& operator=(SquareRangeReadStore&&) = delete;

  IDataStore::StoreType getStoreType() const override
  {
    return IDataStore::StoreType::Empty;
  }

  Result<> copyIntoBuffer(usize offset, nonstd::span<int64> values) const override
  {
    auto result = DataStore<int64>::copyIntoBuffer(offset, values);
    if(result.valid() && ++m_ReadCount == 1)
    {
      m_OnFirstRead(result);
    }
    return result;
  }

  [[nodiscard]] usize readCount() const
  {
    return m_ReadCount;
  }

private:
  std::function<void(Result<>&)> m_OnFirstRead;
  mutable usize m_ReadCount = 0;
};

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
DataPath runSquareWithType(DataStructure& dataStructure, const DataPath& inputPath, DataType expectedType, const ShapeType& tupleShape, const ShapeType& componentShape)
{
  const DataPath outputPath = ip_test::RunMappedPointwiseFilter<SquareImageFilter>(dataStructure, inputPath, expectedType, tupleShape, componentShape);
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(outputPath));
  REQUIRE(outputArrayPtr->getDataType() == expectedType);
  REQUIRE(outputArrayPtr->getTupleShape() == tupleShape);
  REQUIRE(outputArrayPtr->getComponentShape() == componentShape);
  return outputPath;
}

template <typename InputType, typename OutputType>
void requireSquareValues(const std::vector<InputType>& values, const std::vector<OutputType>& expected)
{
  REQUIRE(values.size() == expected.size());
  DataStructure dataStructure;
  const DataPath inputPath = ip_test::BuildScalarValuesImage<InputType>(dataStructure, values);
  const DataPath outputPath = runSquareWithType(dataStructure, inputPath, GetDataType<OutputType>(), ShapeType{1, 1, values.size()}, ShapeType{1});
  const DataArray<OutputType>* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<DataArray<OutputType>>(outputPath));
  const auto& store = outputArrayPtr->getDataStoreRef();
  for(usize index = 0; index < expected.size(); ++index)
  {
    REQUIRE(store.getValue(index) == expected.at(index));
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

template <class InputType>
void requireSquareOverflow(InputType value, int32 expectedCode, const char* expectedBound, InputType otherValue = 1)
{
  DataStructure dataStructure;
  const DataPath inputPath = ip_test::BuildScalarValuesImage<InputType>(dataStructure, {0, value, otherValue});
  const auto args = ip_test::PointwiseFilterArguments<SquareImageFilter>(inputPath);
  const SquareImageFilter filter;
  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors().front().code == expectedCode);
  REQUIRE(executeResult.result.errors().front().message.find(inputPath.toString()) != std::string::npos);
  REQUIRE(executeResult.result.errors().front().message.find(std::to_string(value)) != std::string::npos);
  REQUIRE(executeResult.result.errors().front().message.find(expectedBound) != std::string::npos);
  REQUIRE(executeResult.result.errors().front().message.find("Convert the input to float64") != std::string::npos);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

TEST_CASE("NXImageProcessing::SquareImageFilter: smaller integer types widen for exact squares", "[NXImageProcessing][Issue1874][SquareImageFilter]")
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  SECTION("int8 to int16")
  {
    requireSquareValues<int8, int16>({0, 1, -1, 16, -128, 127}, {0, 1, 1, 256, 16384, 16129});
  }
  SECTION("uint8 to uint16")
  {
    requireSquareValues<uint8, uint16>({0, 1, 16, 225, 255}, {0, 1, 256, 50625, 65025});
  }
  SECTION("int16 to int32")
  {
    requireSquareValues<int16, int32>({0, -16, 16, -32768, 32767}, {0, 256, 256, 1073741824, 1073676289});
  }
  SECTION("uint16 to uint32")
  {
    requireSquareValues<uint16, uint32>({0, 16, 4097, 65535}, {0, 256, 16785409, 4294836225U});
  }
  SECTION("int32 to int64")
  {
    requireSquareValues<int32, int64>({0, -16, 16, std::numeric_limits<int32>::min(), std::numeric_limits<int32>::max()}, {0, 256, 256, 4611686018427387904LL, 4611686014132420609LL});
  }
  SECTION("uint32 to uint64")
  {
    requireSquareValues<uint32, uint64>({0, 16, 4097, 4294967295U}, {0, 256, 16785409, 18446744065119617025ULL});
  }
}

TEST_CASE("NXImageProcessing::SquareImageFilter: retained 64-bit integers preserve low bits at safe bounds", "[NXImageProcessing][Issue1874][SquareImageFilter]")
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  SECTION("int64 signed bounds")
  {
    // Adjacent signed endpoints distinguish exact integer products from rounded Float64 multiplication.
    requireSquareValues<int64, int64>({0, -1, 1, -3037000498LL, 3037000498LL, -3037000499LL, 3037000499LL},
                                      {0, 1, 1, 9223372024852248004LL, 9223372024852248004LL, 9223372030926249001LL, 9223372030926249001LL});
  }
  SECTION("uint64 bound")
  {
    requireSquareValues<uint64, uint64>({0, 1, 4294967294ULL, 4294967295ULL}, {0, 1, 18446744056529682436ULL, 18446744065119617025ULL});
  }
}

TEST_CASE("NXImageProcessing::SquareImageFilter: 64-bit overflow inputs return actionable errors", "[NXImageProcessing][Issue1874][SquareImageFilter]")
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  SECTION("Signed minimum")
  {
    requireSquareOverflow<int64>(std::numeric_limits<int64>::min(), -29001, "-3037000499");
  }
  SECTION("Signed negative adjacent overflow")
  {
    requireSquareOverflow<int64>(-3037000500LL, -29001, "-3037000499");
  }
  SECTION("Signed minimum is reported first when both bounds fail")
  {
    requireSquareOverflow<int64>(-3037000500LL, -29001, "-3037000499", 3037000500LL);
  }
  SECTION("Signed positive adjacent overflow")
  {
    requireSquareOverflow<int64>(3037000500LL, -29002, "3037000499");
  }
  SECTION("Signed maximum")
  {
    requireSquareOverflow<int64>(std::numeric_limits<int64>::max(), -29002, "3037000499");
  }
  SECTION("Unsigned adjacent overflow")
  {
    requireSquareOverflow<uint64>(4294967296ULL, -29003, "4294967295");
  }
  SECTION("Unsigned maximum")
  {
    requireSquareOverflow<uint64>(std::numeric_limits<uint64>::max(), -29003, "4294967295");
  }
}

TEMPLATE_TEST_CASE("NXImageProcessing::SquareImageFilter: empty 64-bit inputs retain empty output", "[NXImageProcessing][Issue1874][SquareImageFilter]", int64, uint64)
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  requireSquareValues<TestType, TestType>({}, {});
}

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
TEST_CASE("NXImageProcessing::SquareImageFilter: cancellation during the 64-bit range scan leaves the output untouched", "[NXImageProcessing][Issue1874][SquareImageFilter]")
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath inputPath = ip_test::BuildScalarValuesImage<int64>(dataStructure, {5, 5});
  const DataPath outputPath = inputPath.replaceName("Output");
  std::atomic_bool shouldCancel = false;
  constexpr int64 sentinel = -7;
  const auto inputStorePtr = std::make_shared<SquareRangeReadStore>([&](Result<>&) {
    // Full-overwrite outputs can begin uninitialized. Set a known value before testing for writes.
    auto* outputPtr = dataStructure.getDataAs<Int64ArrayType>(outputPath);
    REQUIRE(outputPtr != nullptr);
    outputPtr->fill(sentinel);
    shouldCancel = true;
  });
  auto* inputPtr = dataStructure.getDataAs<Int64ArrayType>(inputPath);
  REQUIRE(inputPtr != nullptr);
  const auto replace = inputPtr->setDataStore(inputStorePtr);
  SIMPLNX_RESULT_REQUIRE_VALID(replace);
  const SquareImageFilter filter;
  const auto args = ip_test::PointwiseFilterArguments<SquareImageFilter>(inputPath);
  const auto result = filter.execute(dataStructure, args, nullptr, {}, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().size() == 1);
  REQUIRE(result.result.errors().front().code == -1);
  REQUIRE(inputStorePtr->readCount() == 1);
  const auto* outputPtr = dataStructure.getDataAs<Int64ArrayType>(outputPath);
  REQUIRE(outputPtr != nullptr);
  const auto& output = outputPtr->getDataStoreRef();
  REQUIRE(output.getValue(0) == sentinel);
  REQUIRE(output.getValue(1) == sentinel);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::SquareImageFilter: a failed 64-bit range read preserves its error and warnings", "[NXImageProcessing][Issue1874][SquareImageFilter]")
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath inputPath = ip_test::BuildScalarValuesImage<int64>(dataStructure, {5, 5});
  std::atomic_bool shouldCancel{false};
  const bool cancelWithError = GENERATE(false, true);
  const auto inputStorePtr = std::make_shared<SquareRangeReadStore>([&](Result<>& result) {
    shouldCancel = cancelWithError;
    result = MakeErrorResult(-101, "Injected range read error");
    result.warnings().push_back(Warning{.code = -102, .message = "First read warning"});
    result.warnings().push_back(Warning{.code = -103, .message = "Second read warning"});
  });
  Int64ArrayType* inputPtr = nullptr;
  REQUIRE_NOTHROW(inputPtr = &dataStructure.getDataRefAs<Int64ArrayType>(inputPath));
  const auto replace = inputPtr->setDataStore(inputStorePtr);
  SIMPLNX_RESULT_REQUIRE_VALID(replace);

  const SquareImageFilter filter;
  const auto result = filter.execute(dataStructure, ip_test::PointwiseFilterArguments<SquareImageFilter>(inputPath), nullptr, {}, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(inputStorePtr->readCount() == 1);
  REQUIRE(result.result.errors().size() == 1);
  REQUIRE(result.result.errors().front().code == -101);
  REQUIRE(result.result.errors().front().message == "Injected range read error");
  REQUIRE(result.result.warnings().size() == 2);
  REQUIRE(result.result.warnings().front().code == -102);
  REQUIRE(result.result.warnings().front().message == "First read warning");
  REQUIRE(result.result.warnings().back().code == -103);
  REQUIRE(result.result.warnings().back().message == "Second read warning");
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::SquareImageFilter: a successful 64-bit range read preserves its warning", "[NXImageProcessing][Issue1874][SquareImageFilter]")
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath inputPath = ip_test::BuildScalarValuesImage<int64>(dataStructure, {5, 5});
  const auto inputStorePtr = std::make_shared<SquareRangeReadStore>([](Result<>& readResult) { readResult.warnings().push_back(Warning{.code = -102, .message = "Range scan read warning"}); });
  const bool overflow = GENERATE(false, true);
  CAPTURE(overflow);
  if(overflow)
  {
    inputStorePtr->setValue(1, 3037000500LL);
  }
  auto* inputArrayPtr = dataStructure.getDataAs<Int64ArrayType>(inputPath);
  REQUIRE(inputArrayPtr != nullptr);
  const auto replaceResult = inputArrayPtr->setDataStore(inputStorePtr);
  SIMPLNX_RESULT_REQUIRE_VALID(replaceResult);

  const SquareImageFilter filter;
  const auto result = filter.execute(dataStructure, ip_test::PointwiseFilterArguments<SquareImageFilter>(inputPath));
  if(overflow)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
    REQUIRE(result.result.errors().size() == 1);
    REQUIRE(result.result.errors().front().code == -29002);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    const auto* outputPtr = dataStructure.getDataAs<Int64ArrayType>(inputPath.replaceName("Output"));
    REQUIRE(outputPtr != nullptr);
    REQUIRE((*outputPtr)[0] == 25);
    REQUIRE((*outputPtr)[1] == 25);
  }
  REQUIRE(inputStorePtr->readCount() == 1);
  REQUIRE(result.result.warnings().size() == 1);
  REQUIRE(result.result.warnings().front().code == -102);
  REQUIRE(result.result.warnings().front().message == "Range scan read warning");
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
TEST_CASE("NXImageProcessing::SquareImageFilter: widened multi-component output preserves tuple shape", "[NXImageProcessing][Issue1874][SquareImageFilter]")
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath inputPath = ip_test::BuildRampImage<int16>(dataStructure, 2, 0, 0);
  Int16ArrayType* inputArrayPtr = nullptr;
  REQUIRE_NOTHROW(inputArrayPtr = &dataStructure.getDataRefAs<Int16ArrayType>(inputPath));
  const auto inputStorePtr = data_store_utilities::CreateDataStore<int16>(dataStructure, inputPath, ShapeType{2, 2, 2}, ShapeType{2});
  REQUIRE(inputStorePtr != nullptr);
  const auto replaceResult = inputArrayPtr->setDataStore(inputStorePtr);
  SIMPLNX_RESULT_REQUIRE_VALID(replaceResult);
  constexpr std::array<int16, 4> values = {-32768, -16, 16, 32767};
  constexpr std::array<int32, 4> expected = {1073741824, 256, 256, 1073676289};
  for(usize index = 0; index < 16; ++index)
  {
    inputStorePtr->setValue(index, values.at(index % values.size()));
  }
  const DataPath outputPath = runSquareWithType(dataStructure, inputPath, DataType::int32, ShapeType{2, 2, 2}, ShapeType{2});
  const Int32ArrayType* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<Int32ArrayType>(outputPath));
  const auto& store = outputArrayPtr->getDataStoreRef();
  for(usize index = 0; index < 16; ++index)
  {
    REQUIRE(store.getValue(index) == expected.at(index % expected.size()));
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// NOLINTBEGIN(bugprone-chained-comparison) Catch2 comparison decomposition triggers this check.
TEMPLATE_TEST_CASE("NXImageProcessing::SquareImageFilter: floating inputs retain type and safe IEEE values", "[NXImageProcessing][Issue1874][SquareImageFilter]", float32, float64)
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  const TestType infinity = std::numeric_limits<TestType>::infinity();
  DataStructure dataStructure;
  const DataPath inputPath =
      ip_test::BuildScalarValuesImage<TestType>(dataStructure, {0, -TestType{0}, -2, 2, infinity, -infinity, std::numeric_limits<TestType>::quiet_NaN(), TestType{1.5}, TestType{-0.25}});
  const DataPath outputPath = runSquareWithType(dataStructure, inputPath, GetDataType<TestType>(), ShapeType{1, 1, 9}, ShapeType{1});
  const DataArray<TestType>* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<DataArray<TestType>>(outputPath));
  const auto& store = outputArrayPtr->getDataStoreRef();
  REQUIRE(store.getValue(0) == 0);
  REQUIRE_FALSE(std::signbit(store.getValue(0)));
  REQUIRE(store.getValue(1) == 0);
  REQUIRE_FALSE(std::signbit(store.getValue(1)));
  REQUIRE(store.getValue(2) == 4);
  REQUIRE(store.getValue(3) == 4);
  REQUIRE(store.getValue(4) == infinity);
  REQUIRE(store.getValue(5) == infinity);
  REQUIRE(std::isnan(store.getValue(6)));
  REQUIRE(store.getValue(7) == TestType{2.25});
  REQUIRE(store.getValue(8) == TestType{0.0625});
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison)

// NOLINTBEGIN(bugprone-chained-comparison) Catch2 comparison decomposition triggers this check.
TEMPLATE_TEST_CASE("NXImageProcessing::SquareImageFilter: floating overflow is positive infinity and underflow is positive zero", "[NXImageProcessing][Issue1874][SquareImageFilter]", float32, float64)
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const TestType maximum = std::numeric_limits<TestType>::max();
  const DataPath inputPath = ip_test::BuildScalarValuesImage<TestType>(dataStructure, {maximum, -maximum, std::numeric_limits<TestType>::min(), std::numeric_limits<TestType>::denorm_min()});
  const DataPath outputPath = runSquareWithType(dataStructure, inputPath, GetDataType<TestType>(), ShapeType{1, 1, 4}, ShapeType{1});
  const DataArray<TestType>* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<DataArray<TestType>>(outputPath));
  const auto& store = outputArrayPtr->getDataStoreRef();
  REQUIRE(store.getValue(0) == std::numeric_limits<TestType>::infinity());
  REQUIRE(store.getValue(1) == std::numeric_limits<TestType>::infinity());
  REQUIRE(store.getValue(2) == 0);
  REQUIRE_FALSE(std::signbit(store.getValue(2)));
  REQUIRE(store.getValue(3) == 0);
  REQUIRE_FALSE(std::signbit(store.getValue(3)));
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison)

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
// This finite float32 input squares beyond the largest finite float32 value.
TEST_CASE("NXImageProcessing::SquareImageFilter: square near sqrt(FLT_MAX) overflows to +inf (domain edge)", "[NXImageProcessing][SquareImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(dataStructure, {1.0e20f});
  const DataPath output = ip_test::RunPointwiseFilter<SquareImageFilter>(dataStructure, input);

  const auto* outputPtr = dataStructure.getDataAs<DataArray<float32>>(output);
  REQUIRE(outputPtr != nullptr);
  const auto& out = outputPtr->getDataStoreRef();
  REQUIRE(std::isinf(out.getValue(0)));
  REQUIRE(out.getValue(0) > 0.0f); // (1e20)^2 = 1e40 > FLT_MAX -> +inf
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image md5 golden (plan Sec.3/Sec.4/Sec.6). Mirrors ITKSquareImageTest.cpp(defaults): read
// Ramp-Zero-One-Float.nrrd (float32) through OUR reader, run OUR SquareImageFilter (defaults), then our output's
// md5 must EQUAL ITK's committed hash. A match confirms the durable golden byte-for-byte (no re-freeze needed).
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::SquareImageFilter: ITK real-image golden (defaults)", "[NXImageProcessing][ItkGolden][SquareImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath input = geom.createChildPath("CellData").createChildPath("Input");
  const DataPath output = geom.createChildPath("CellData").createChildPath("Output");

  DataStructure dataStructure;
  const auto readInput = ip_golden::ReadInputImage(dataStructure, ip_golden::InputPath("Ramp-Zero-One-Float.nrrd"), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);

  Arguments args;
  args.insertOrAssign(SquareImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(SquareImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(SquareImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const SquareImageFilter filter;
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // DURABLE golden: md5-validity-first (plan Sec.4).
  const std::string ourMd5 = ip_golden::ComputeMd5Hash(dataStructure, output);
  INFO(fmt::format("SquareImageFilter defaults md5: ours='{}' committed='{}'", ourMd5, "314065b457b66e102b9cafd7c49be6b3"));
  REQUIRE(ourMd5 == "314065b457b66e102b9cafd7c49be6b3");

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

namespace
{
// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
template <typename T>
void checkPixels(const DataStructure& dataStructure, const DataPath& path, const std::vector<T>& expected, const ip_test::SquareStoreCheckType& checkStore, const fs::path& reopenedFile = {})
{
  const auto* arrayPtr = dataStructure.getDataAs<DataArray<T>>(path);
  REQUIRE(arrayPtr != nullptr);
  REQUIRE(arrayPtr->getTupleShape() == ShapeType{1, 1, 4});
  REQUIRE(arrayPtr->getComponentShape() == ShapeType{1});
  checkStore(*arrayPtr, reopenedFile);
  std::vector<T> pixels(arrayPtr->getSize());
  const auto read = arrayPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<T>(pixels.data(), pixels.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(read);
  REQUIRE(pixels == expected);
}

void checkSavedSquares(const fs::path& path, const DataPath& square, const std::vector<int64>& expected)
{
  auto file = hdf5::FileIO::ReadFile(path);
  REQUIRE(file.isValid());
  auto dataset = file.openDataset("DataStructure/" + square.toString());
  REQUIRE(dataset.getId() > 0);
  const auto type = dataset.getDataType();
  SIMPLNX_RESULT_REQUIRE_VALID(type);
  REQUIRE(type.value() == DataType::int64);
  REQUIRE(dataset.getDimensions() == ShapeType{1, 1, 4, 1});
  std::vector<int64> pixels(expected.size());
  const auto read = dataset.readIntoSpan<int64>(nonstd::span<int64>(pixels.data(), pixels.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(read);
  REQUIRE(pixels == expected);
}

void checkMetaImage(const fs::path& path, const std::vector<int64>& expected, const ip_test::SquareStoreCheckType& checkStore)
{
  const ReadMhaFileFilter reader;
  auto args = reader.getDefaultArguments();
  args.insertOrAssign(ReadMhaFileFilter::k_InputFilePath_Key, std::make_any<fs::path>(path));
  args.insertOrAssign(ReadMhaFileFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(ReadMhaFileFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>("CellData"));
  args.insertOrAssign(ReadMhaFileFilter::k_ImageDataArrayName_Key, std::make_any<std::string>("Pixels"));
  DataStructure raw;
  const auto result = reader.execute(raw, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  checkPixels(raw, DataPath({"Image Geometry", "CellData", "Pixels"}), expected, checkStore);
  const auto* planePtr = raw.getDataAs<ImageGeom>(DataPath({"Image Geometry"}));
  REQUIRE(planePtr != nullptr);
  REQUIRE(planePtr->getDimensions() == SizeVec3Type{4, 1, 1});
  REQUIRE(planePtr->getSpacing() == FloatVec3Type{1, 2, 1});
  REQUIRE(planePtr->getOrigin() == FloatVec3Type{4, 5, 0});
  unit_test::CheckArraysInheritTupleDims(raw);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)
} // namespace

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
void ip_test::checkSquareConsumers(const SquareStoreCheckType& checkStore, const std::function<void(const std::function<void()>&)>& runWriter, std::shared_ptr<const IDataStoreFormatResolver> resolver)
{
  // Keep the temporary directory alive until all arrays reopened from it have released their handles.
  const auto temporary = AtomicFile::Create(fs::path(unit_test::k_BinaryTestOutputDir.view()) / "square.dream3d");
  SIMPLNX_RESULT_REQUIRE_VALID(temporary);
  const auto dream3d = temporary.value().tempFilePath();
  const auto mha = dream3d.parent_path() / "square.mha";

  DataStructure source;
  const DataPath geomPath({"Image Geometry"});
  const std::vector<int32> original{0, 2147483647, 0, 2};
  const auto input = BuildScalarValuesImage<int32>(source, original);
  auto* geomPtr = source.getDataAs<ImageGeom>(geomPath);
  REQUIRE(geomPtr != nullptr);
  const auto spacing = geomPtr->setSpacing({1, 2, 3});
  SIMPLNX_RESULT_REQUIRE_VALID(spacing);
  const auto origin = geomPtr->setOrigin({4, 5, 6});
  SIMPLNX_RESULT_REQUIRE_VALID(origin);
  geomPtr->setUnits(IGeometry::LengthUnit::Millimeter);

  const SquareImageFilter filter;
  auto squareArgs = PointwiseFilterArguments<SquareImageFilter>(input);
  squareArgs.insertOrAssign(SquareImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("ExactSquare"));
  const auto result = filter.execute(source, squareArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const auto square = input.replaceName("ExactSquare");
  // (2^31 - 1)^2 exceeds the exact integer precision of Float64.
  const std::vector<int64> expected{0, 4611686014132420609LL, 0, 4};
  checkPixels(source, square, expected, checkStore);
  const auto save = dream3d::WriteFile(dream3d, source);
  SIMPLNX_RESULT_REQUIRE_VALID(save);
  checkSavedSquares(dream3d, square, expected);
  auto load = dream3d::LoadDataStructure(dream3d, std::move(resolver));
  SIMPLNX_RESULT_REQUIRE_VALID(load);
  auto& reloaded = load.value();
  checkPixels(reloaded, square, expected, checkStore, dream3d);
  const auto* reloadedGeomPtr = reloaded.getDataAs<ImageGeom>(geomPath);
  REQUIRE(reloadedGeomPtr != nullptr);
  REQUIRE(reloadedGeomPtr->getDimensions() == SizeVec3Type{4, 1, 1});
  REQUIRE(reloadedGeomPtr->getSpacing() == FloatVec3Type{1, 2, 3});
  REQUIRE(reloadedGeomPtr->getOrigin() == FloatVec3Type{4, 5, 6});
  REQUIRE(reloadedGeomPtr->getUnits() == IGeometry::LengthUnit::Millimeter);

  const WriteImageFilter writer;
  auto writerArgs = writer.getDefaultArguments();
  writerArgs.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  writerArgs.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(square));
  writerArgs.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<fs::path>(mha));
  writerArgs.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(0));
  writerArgs.insertOrAssign(WriteImageFilter::k_CreateColorTable_Key, std::make_any<bool>(false));
  runWriter([&] {
    const auto write = writer.execute(reloaded, writerArgs);
    SIMPLNX_RESULT_REQUIRE_VALID(write.result);
  });
  checkMetaImage(mha, expected, checkStore);

  const ConnectedComponentImageFilter components;
  const auto componentArgs = PointwiseFilterArguments<ConnectedComponentImageFilter>(square);
  const auto connected = components.execute(reloaded, componentArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(connected.result);
  checkPixels(reloaded, square.replaceName("Output"), std::vector<uint32>{0, 1, 0, 2}, checkStore);
  checkPixels(reloaded, square, expected, checkStore, dream3d);
  checkPixels(source, square, expected, checkStore);
  checkPixels(source, input, original, checkStore);
  unit_test::CheckArraysInheritTupleDims(source);
  unit_test::CheckArraysInheritTupleDims(reloaded);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

TEST_CASE("NXImageProcessing::SquareImageFilter: resident integration widened Square persists and feeds integer components",
          "[NXImageProcessing][ImageFilterIntegration][Issue1874][SquareImageFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  unit_test::AlgorithmTestScope scope(scenario);
  ip_test::checkSquareConsumers([&scope](const IDataArray& array, const fs::path&) { scope.requireExpectedStore(array); }, [&scope](const std::function<void()>& write) { scope.execute(write); },
                                nullptr);
}
