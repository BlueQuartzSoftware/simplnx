#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/SquareImageFilter.hpp"

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
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <any>
#include <array>
#include <atomic>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

using namespace nx::core;

namespace
{
DataPath runSquareWithType(DataStructure& dataStructure, const DataPath& inputPath, DataType expectedType, const ShapeType& tupleShape, const ShapeType& componentShape)
{
  Arguments args;
  args.insertOrAssign(SquareImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(SquareImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(SquareImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const SquareImageFilter filter;
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.value().actions.size() == 1);
  const auto* actionPtr = dynamic_cast<const CreateArrayAction*>(preflightResult.outputActions.value().actions.front().get());
  REQUIRE(actionPtr != nullptr);
  // A hard preflight guard stops the old same-width output before any narrowing overflow or typed lookup.
  REQUIRE(actionPtr->type() == expectedType);
  REQUIRE(actionPtr->dims() == tupleShape);
  REQUIRE(actionPtr->componentDims() == componentShape);

  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const DataPath outputPath = inputPath.replaceName("Output");
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
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors().front().code == expectedCode);
  REQUIRE(executeResult.result.errors().front().message.find(inputPath.toString()) != std::string::npos);
  REQUIRE(executeResult.result.errors().front().message.find(std::to_string(value)) != std::string::npos);
  REQUIRE(executeResult.result.errors().front().message.find("float64") != std::string::npos);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace

TEST_CASE("ImageProcessing::SquareImageFilter: smaller integer types widen for exact squares", "[ImageProcessing][Issue1874][SquareImageFilter]")
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

TEST_CASE("ImageProcessing::SquareImageFilter: retained 64-bit integers preserve low bits at safe bounds", "[ImageProcessing][Issue1874][SquareImageFilter]")
{
  UnitTest::LoadPlugins();
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

TEST_CASE("ImageProcessing::SquareImageFilter: 64-bit overflow inputs return actionable errors", "[ImageProcessing][Issue1874][SquareImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  SECTION("Signed minimum")
  {
    requireSquareOverflow<int64>(std::numeric_limits<int64>::min(), -1874091);
  }
  SECTION("Signed negative adjacent overflow")
  {
    requireSquareOverflow<int64>(-3037000500LL, -1874091);
  }
  SECTION("Signed positive adjacent overflow")
  {
    requireSquareOverflow<int64>(3037000500LL, -1874092);
  }
  SECTION("Signed maximum")
  {
    requireSquareOverflow<int64>(std::numeric_limits<int64>::max(), -1874092);
  }
  SECTION("Unsigned adjacent overflow")
  {
    requireSquareOverflow<uint64>(4294967296ULL, -1874093);
  }
  SECTION("Unsigned maximum")
  {
    requireSquareOverflow<uint64>(std::numeric_limits<uint64>::max(), -1874093);
  }
}

TEMPLATE_TEST_CASE("ImageProcessing::SquareImageFilter: empty 64-bit inputs retain empty output", "[ImageProcessing][Issue1874][SquareImageFilter]", int64, uint64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  requireSquareValues<TestType, TestType>({}, {});
}

TEST_CASE("ImageProcessing::SquareImageFilter: cancellation preserves the filter cancellation result", "[ImageProcessing][Issue1874][SquareImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath inputPath = ip_test::BuildScalarValuesImage<int64>(dataStructure, {std::numeric_limits<int64>::min()});
  Arguments args;
  args.insertOrAssign(SquareImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(SquareImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(SquareImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const SquareImageFilter filter;
  const std::atomic_bool shouldCancel = true;
  const auto executeResult = filter.execute(dataStructure, args, nullptr, {}, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors().front().code == -1);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::SquareImageFilter: widened multi-component output preserves tuple shape", "[ImageProcessing][Issue1874][SquareImageFilter]")
{
  UnitTest::LoadPlugins();
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

TEMPLATE_TEST_CASE("ImageProcessing::SquareImageFilter: floating inputs retain type and safe IEEE values", "[ImageProcessing][Issue1874][SquareImageFilter]", float32, float64)
{
  UnitTest::LoadPlugins();
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

TEMPLATE_TEST_CASE("ImageProcessing::SquareImageFilter: floating overflow is positive infinity and underflow is positive zero", "[ImageProcessing][Issue1874][SquareImageFilter]", float32, float64)
{
  UnitTest::LoadPlugins();
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

// -----------------------------------------------------------------------------
// ITK-sourced real-image md5 golden (plan Sec.3/Sec.4/Sec.6). Mirrors ITKSquareImageTest.cpp(defaults): read
// Ramp-Zero-One-Float.nrrd (float32) through OUR reader, run OUR SquareImageFilter (defaults), then our output's
// md5 must EQUAL ITK's committed hash. A match confirms the durable golden byte-for-byte (no re-freeze needed).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SquareImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][SquareImageFilter]")
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
