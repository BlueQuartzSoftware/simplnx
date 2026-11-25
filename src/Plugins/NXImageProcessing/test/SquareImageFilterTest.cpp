#include "ImageFilterIntegrationTestUtils.hpp"

#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/SquareImageFilter.hpp"
#include "NXImageProcessing/Filters/WriteImageFilter.hpp"

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
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/UnitTest/AlgorithmTestScope.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"

#include <catch2/catch.hpp>
#include <nonstd/span.hpp>

#include <any>
#include <array>
#include <atomic>
#include <cmath>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace nx::core;

namespace
{
// This resident test double selects the buffered range-scan route and signals the first completed read.
class SquareRangeReadStore : public DataStore<int64>
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

  usize readCount() const
  {
    return m_ReadCount;
  }

private:
  std::function<void(Result<>&)> m_OnFirstRead;
  mutable usize m_ReadCount = 0;
};

DataPath runSquareWithType(DataStructure& dataStructure, const DataPath& inputPath, DataType expectedType, const ShapeType& tupleShape, const ShapeType& componentShape)
{
  const DataPath outputPath = ip_test::RunMappedPointwiseFilter<SquareImageFilter>(dataStructure, inputPath, expectedType, tupleShape, componentShape);
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(outputPath));
  // NOLINTNEXTLINE(bugprone-chained-comparison) Catch2 comparison decomposition.
  REQUIRE(outputArrayPtr->getDataType() == expectedType);
  // NOLINTNEXTLINE(bugprone-chained-comparison) Catch2 comparison decomposition.
  REQUIRE(outputArrayPtr->getTupleShape() == tupleShape);
  // NOLINTNEXTLINE(bugprone-chained-comparison) Catch2 comparison decomposition.
  REQUIRE(outputArrayPtr->getComponentShape() == componentShape);
  return outputPath;
}

template <typename InputType, typename OutputType>
void requireSquareValues(const std::vector<InputType>& values, const std::vector<OutputType>& expected)
{
  // NOLINTNEXTLINE(bugprone-chained-comparison) Catch2 comparison decomposition.
  REQUIRE(values.size() == expected.size());
  DataStructure dataStructure;
  const DataPath inputPath = ip_test::BuildScalarValuesImage<InputType>(dataStructure, values);
  const DataPath outputPath = runSquareWithType(dataStructure, inputPath, GetDataType<OutputType>(), ShapeType{1, 1, values.size()}, ShapeType{1});
  const DataArray<OutputType>* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<DataArray<OutputType>>(outputPath));
  const auto& store = outputArrayPtr->getDataStoreRef();
  for(usize index = 0; index < expected.size(); ++index)
  {
    // NOLINTNEXTLINE(bugprone-chained-comparison) Catch2 comparison decomposition.
    REQUIRE(store.getValue(index) == expected.at(index));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

template <class InputType>
void requireSquareOverflow(InputType value, int32 expectedCode)
{
  DataStructure dataStructure;
  const DataPath inputPath = ip_test::BuildScalarValuesImage<InputType>(dataStructure, {0, value, 1});
  Arguments args;
  args.insertOrAssign(SquareImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(SquareImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(SquareImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const SquareImageFilter filter;
  const auto executeResult = filter.execute(dataStructure, args);
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while) simplnx assertion macro.
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  // NOLINTNEXTLINE(bugprone-chained-comparison) Catch2 comparison decomposition.
  REQUIRE(executeResult.result.errors().size() == 1);
  // NOLINTNEXTLINE(bugprone-chained-comparison) Catch2 comparison decomposition.
  REQUIRE(executeResult.result.errors().front().code == expectedCode);
  // NOLINTNEXTLINE(bugprone-chained-comparison) Catch2 comparison decomposition.
  REQUIRE(executeResult.result.errors().front().message.find(inputPath.toString()) != std::string::npos);
  // NOLINTNEXTLINE(bugprone-chained-comparison) Catch2 comparison decomposition.
  REQUIRE(executeResult.result.errors().front().message.find(std::to_string(value)) != std::string::npos);
  // NOLINTNEXTLINE(bugprone-chained-comparison) Catch2 comparison decomposition.
  REQUIRE(executeResult.result.errors().front().message.find("float64") != std::string::npos);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace

TEST_CASE("NXImageProcessing::SquareImageFilter: smaller integer types widen for exact squares", "[NXImageProcessing][Issue1874][SquareImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
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
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  SECTION("int64 signed bounds")
  {
    // These exact products and the baseline double products fit int64. No overflowing input reaches the baseline.
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
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  SECTION("Signed minimum")
  {
    requireSquareOverflow<int64>(std::numeric_limits<int64>::min(), -29001);
  }
  SECTION("Signed negative adjacent overflow")
  {
    requireSquareOverflow<int64>(-3037000500LL, -29001);
  }
  SECTION("Signed positive adjacent overflow")
  {
    requireSquareOverflow<int64>(3037000500LL, -29002);
  }
  SECTION("Signed maximum")
  {
    requireSquareOverflow<int64>(std::numeric_limits<int64>::max(), -29002);
  }
  SECTION("Unsigned adjacent overflow")
  {
    requireSquareOverflow<uint64>(4294967296ULL, -29003);
  }
  SECTION("Unsigned maximum")
  {
    requireSquareOverflow<uint64>(std::numeric_limits<uint64>::max(), -29003);
  }
}

TEMPLATE_TEST_CASE("NXImageProcessing::SquareImageFilter: empty 64-bit inputs retain empty output", "[NXImageProcessing][Issue1874][SquareImageFilter]", int64, uint64)
{
  Application::GetOrCreateInstance();
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  requireSquareValues<TestType, TestType>({}, {});
}

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
TEST_CASE("NXImageProcessing::SquareImageFilter: cancellation during the 64-bit range scan leaves the output untouched", "[NXImageProcessing][Issue1874][SquareImageFilter]")
{
  Application::GetOrCreateInstance();
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath inputPath = ip_test::BuildScalarValuesImage<int64>(dataStructure, {5, 5});
  const DataPath outputPath = inputPath.replaceName("Output");
  std::atomic_bool shouldCancel = false;
  constexpr int64 sentinel = -7;
  auto inputStore = std::make_shared<SquareRangeReadStore>([&](Result<>&) {
    // Full-overwrite outputs can begin uninitialized. Set a known value before testing for writes.
    dataStructure.getDataRefAs<Int64Array>(outputPath).getDataStoreRef().fill(sentinel);
    shouldCancel = true;
  });
  const auto replace = dataStructure.getDataRefAs<Int64Array>(inputPath).setDataStore(inputStore);
  SIMPLNX_RESULT_REQUIRE_VALID(replace);
  const SquareImageFilter filter;
  const auto args = ip_test::PointwiseFilterArguments<SquareImageFilter>(inputPath);
  const auto result = filter.execute(dataStructure, args, nullptr, {}, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().size() == 1);
  REQUIRE(result.result.errors().front().code == -1);
  REQUIRE(inputStore->readCount() == 1);
  const auto& output = dataStructure.getDataRefAs<Int64Array>(outputPath).getDataStoreRef();
  REQUIRE(output.getValue(0) == sentinel);
  REQUIRE(output.getValue(1) == sentinel);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::SquareImageFilter: a failed 64-bit range read preserves its error and warnings", "[NXImageProcessing][Issue1874][SquareImageFilter]")
{
  Application::GetOrCreateInstance();
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath inputPath = ip_test::BuildScalarValuesImage<int64>(dataStructure, {5, 5});
  auto inputStore = std::make_shared<SquareRangeReadStore>([](Result<>& result) {
    result = MakeErrorResult(-101, "Injected range read error");
    result.warnings().push_back(Warning{.code = -102, .message = "First read warning"});
    result.warnings().push_back(Warning{.code = -103, .message = "Second read warning"});
  });
  Int64Array* inputPtr = nullptr;
  REQUIRE_NOTHROW(inputPtr = &dataStructure.getDataRefAs<Int64Array>(inputPath));
  const auto replace = inputPtr->setDataStore(inputStore);
  SIMPLNX_RESULT_REQUIRE_VALID(replace);

  const SquareImageFilter filter;
  const auto result = filter.execute(dataStructure, ip_test::PointwiseFilterArguments<SquareImageFilter>(inputPath));
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(inputStore->readCount() == 1);
  REQUIRE(result.result.errors().size() == 1);
  REQUIRE(result.result.errors().front().code == -101);
  REQUIRE(result.result.warnings().size() == 2);
  REQUIRE(result.result.warnings().front().code == -102);
  REQUIRE(result.result.warnings().front().message == "First read warning");
  REQUIRE(result.result.warnings().back().code == -103);
  REQUIRE(result.result.warnings().back().message == "Second read warning");
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::SquareImageFilter: a successful 64-bit range read preserves its warning", "[NXImageProcessing][Issue1874][SquareImageFilter]")
{
  Application::GetOrCreateInstance();
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath inputPath = ip_test::BuildScalarValuesImage<int64>(dataStructure, {5, 5});
  auto inputStorePtr = std::make_shared<SquareRangeReadStore>([](Result<>& readResult) { readResult.warnings().push_back(Warning{.code = -102, .message = "Range scan read warning"}); });
  const bool overflow = GENERATE(false, true);
  CAPTURE(overflow);
  if(overflow)
  {
    inputStorePtr->setValue(1, 3037000500LL);
  }
  auto* inputArrayPtr = dataStructure.getDataAs<Int64Array>(inputPath);
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
    const auto* outputPtr = dataStructure.getDataAs<Int64Array>(inputPath.replaceName("Output"));
    REQUIRE(outputPtr != nullptr);
    REQUIRE((*outputPtr)[0] == 25);
    REQUIRE((*outputPtr)[1] == 25);
  }
  REQUIRE(inputStorePtr->readCount() == 1);
  REQUIRE(result.result.warnings().size() == 1);
  REQUIRE(result.result.warnings().front().code == -102);
  REQUIRE(result.result.warnings().front().message == "Range scan read warning");
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
TEST_CASE("NXImageProcessing::SquareImageFilter: widened multi-component output preserves tuple shape", "[NXImageProcessing][Issue1874][SquareImageFilter]")
{
  Application::GetOrCreateInstance();
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath inputPath = ip_test::BuildRampImage<int16>(dataStructure, 2, 0, 0);
  Int16Array* inputArrayPtr = nullptr;
  REQUIRE_NOTHROW(inputArrayPtr = &dataStructure.getDataRefAs<Int16Array>(inputPath));
  const auto inputStorePtr = DataStoreUtilities::CreateDataStore<int16>(dataStructure, inputPath, ShapeType{2, 2, 2}, ShapeType{2});
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
  const Int32Array* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<Int32Array>(outputPath));
  const auto& store = outputArrayPtr->getDataStoreRef();
  for(usize index = 0; index < 16; ++index)
  {
    REQUIRE(store.getValue(index) == expected.at(index % expected.size()));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// NOLINTBEGIN(bugprone-chained-comparison) Catch2 comparison decomposition triggers this check.
TEMPLATE_TEST_CASE("NXImageProcessing::SquareImageFilter: floating inputs retain type and safe IEEE values", "[NXImageProcessing][Issue1874][SquareImageFilter]", float32, float64)
{
  Application::GetOrCreateInstance();
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  const TestType infinity = std::numeric_limits<TestType>::infinity();
  DataStructure dataStructure;
  const DataPath inputPath = ip_test::BuildScalarValuesImage<TestType>(dataStructure, {0, -TestType{0}, -2, 2, infinity, -infinity, std::numeric_limits<TestType>::quiet_NaN()});
  const DataPath outputPath = runSquareWithType(dataStructure, inputPath, GetDataType<TestType>(), ShapeType{1, 1, 7}, ShapeType{1});
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
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison)

// NOLINTBEGIN(bugprone-chained-comparison) Catch2 comparison decomposition triggers this check.
TEMPLATE_TEST_CASE("NXImageProcessing::SquareImageFilter: floating overflow is positive infinity and underflow is positive zero", "[NXImageProcessing][Issue1874][SquareImageFilter]", float32, float64)
{
  Application::GetOrCreateInstance();
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
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
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison)

// -----------------------------------------------------------------------------
// ITK-sourced real-image md5 golden (plan Sec.3/Sec.4/Sec.6). Mirrors ITKSquareImageTest.cpp(defaults): read
// Ramp-Zero-One-Float.nrrd (float32) through OUR reader, run OUR SquareImageFilter (defaults), then our output's
// md5 must EQUAL ITK's committed hash. A match confirms the durable golden byte-for-byte (no re-freeze needed).
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::SquareImageFilter: ITK real-image golden (defaults)", "[NXImageProcessing][ItkGolden][SquareImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath input = geom.createChildPath("CellData").createChildPath("Input");
  const DataPath output = geom.createChildPath("CellData").createChildPath("Output");

  DataStructure ds;
  const auto readInput = ip_golden::ReadInputImage(ds, ip_golden::InputPath("Ramp-Zero-One-Float.nrrd"), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);

  Arguments args;
  args.insertOrAssign(SquareImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(SquareImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(SquareImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const SquareImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // DURABLE golden: md5-validity-first (plan Sec.4).
  const std::string ourMd5 = ip_golden::ComputeMd5Hash(ds, output);
  INFO(fmt::format("SquareImageFilter defaults md5: ours='{}' committed='{}'", ourMd5, "314065b457b66e102b9cafd7c49be6b3"));
  REQUIRE(ourMd5 == "314065b457b66e102b9cafd7c49be6b3");

  UnitTest::CheckArraysInheritTupleDims(ds);
}

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
TEST_CASE("NXImageProcessing::SquareImageFilter: resident integration widened Square persists and feeds integer components",
          "[NXImageProcessing][ImageFilterIntegration][Issue1874][SquareImageFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope scope(scenario);
  const image_integration::Directory output;
  DataStructure source;
  const std::vector<int32> original{0, 2147483647, 0, 2};
  const auto input = ip_test::BuildScalarValuesImage<int32>(source, original);
  image_integration::SetGeometry(source);
  REQUIRE(image_integration::Read<int32>(source, input, {1, 1, 4}, scope) == original);
  const auto square = image_integration::Pointwise<SquareImageFilter>(source, input, "ExactSquare");
  // (2^31-1)^2 is exact in Int64 and cannot be transported exactly through Float64.
  const std::vector<int64> expected{0, 4611686014132420609LL, 0, 4};
  REQUIRE(image_integration::Read<int64>(source, square, {1, 1, 4}, scope) == expected);
  const auto save = DREAM3D::WriteFile(output.path() / "square.dream3d", source);
  SIMPLNX_RESULT_REQUIRE_VALID(save);
  image_integration::CheckSaved<int64>(output.path() / "square.dream3d", square, {1, 1, 4}, expected);
  auto load = DREAM3D::LoadDataStructure(output.path() / "square.dream3d");
  SIMPLNX_RESULT_REQUIRE_VALID(load);
  auto& reloaded = load.value();
  REQUIRE(image_integration::Read<int64>(reloaded, square, {1, 1, 4}, scope) == expected);
  image_integration::CheckGeometry(reloaded, {4, 1, 1});
  const WriteImageFilter writer;
  image_integration::Execute(writer, reloaded, image_integration::WriteArguments(square, output.path() / "square.mha"), scope);
  auto raw = image_integration::ReadMha(output.path() / "square.mha");
  REQUIRE(image_integration::Read<int64>(raw, DataPath({"Image Geometry", "CellData", "Pixels"}), {1, 1, 4}, scope) == expected);
  const ImageGeom* rawGeometryPtr = nullptr;
  REQUIRE_NOTHROW(rawGeometryPtr = &raw.getDataRefAs<ImageGeom>(DataPath({"Image Geometry"})));
  REQUIRE(rawGeometryPtr->getDimensions() == SizeVec3{4, 1, 1});
  REQUIRE(rawGeometryPtr->getSpacing() == FloatVec3{1, 2, 1});
  REQUIRE(rawGeometryPtr->getOrigin() == FloatVec3{4, 5, 0});
  const auto labels = image_integration::Components(reloaded, square, "Components");
  REQUIRE(image_integration::Read<uint32>(reloaded, labels, {1, 1, 4}, scope) == std::vector<uint32>{0, 1, 0, 2});
  REQUIRE(image_integration::Read<int64>(reloaded, square, {1, 1, 4}, scope) == expected);
  REQUIRE(image_integration::Read<int64>(source, square, {1, 1, 4}, scope) == expected);
  REQUIRE(image_integration::Read<int32>(source, input, {1, 1, 4}, scope) == original);
  UnitTest::CheckArraysInheritTupleDims(source);
  UnitTest::CheckArraysInheritTupleDims(reloaded);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)
