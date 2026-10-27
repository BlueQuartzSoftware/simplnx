#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/SqrtImageFilter.hpp"

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
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
#include <cmath>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

using namespace nx::core;

namespace
{
DataPath runMappedIntegerSqrt(DataStructure& dataStructure, const DataPath& input, const ShapeType& tupleShape, const ShapeType& componentShape)
{
  const SqrtImageFilter filter;
  Arguments args;
  args.insertOrAssign(SqrtImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(SqrtImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(SqrtImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.value().actions.size() == 1);
  const auto* actionPtr = dynamic_cast<const CreateArrayAction*>(preflightResult.outputActions.value().actions.front().get());
  REQUIRE(actionPtr != nullptr);
  // Stop before negative integer input can produce an unsafe NaN-to-integer cast.
  REQUIRE(actionPtr->type() == DataType::float64);
  REQUIRE(actionPtr->dims() == tupleShape);
  REQUIRE(actionPtr->componentDims() == componentShape);
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  return input.replaceName("Output");
}
} // namespace

TEMPLATE_TEST_CASE("ImageProcessing::SqrtImageFilter: integer values produce float64 square roots", "[ImageProcessing][Issue1874][SqrtImageFilter]", int8, uint8, int16, uint16, int32, uint32, int64,
                   uint64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  std::vector<TestType> values = {0, 2, 4, std::numeric_limits<TestType>::max()};
  if constexpr(std::is_signed_v<TestType>)
  {
    values.push_back(-1);
    values.push_back(std::numeric_limits<TestType>::lowest());
  }
  const ShapeType tupleShape = {1, 1, values.size()};
  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(dataStructure, values);
  const DataPath output = runMappedIntegerSqrt(dataStructure, input, tupleShape, {1});
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(output));
  REQUIRE(outputArrayPtr->getDataType() == DataType::float64);
  REQUIRE(outputArrayPtr->getTupleShape() == tupleShape);
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const Float64Array* rootsPtr = nullptr;
  REQUIRE_NOTHROW(rootsPtr = &dataStructure.getDataRefAs<Float64Array>(output));
  const auto& store = rootsPtr->getDataStoreRef();
  REQUIRE(store.getValue(0) == 0.0);
  REQUIRE(store.getValue(1) == Approx(1.4142135623730950488).epsilon(0).margin(3.0e-16));
  REQUIRE(store.getValue(2) == 2.0);
  REQUIRE(std::isfinite(store.getValue(3)));
  REQUIRE(store.getValue(3) > 2.0);
  REQUIRE(store.getValue(3) <= 4294967296.0);
  if constexpr(std::is_same_v<TestType, int8>)
  {
    REQUIRE(store.getValue(3) == Approx(11.269427669584644).epsilon(1.0e-15));
  }
  if constexpr(std::is_same_v<TestType, uint8>)
  {
    REQUIRE(store.getValue(3) == Approx(15.968719422671311).epsilon(1.0e-15));
  }
  if constexpr(std::is_same_v<TestType, int16>)
  {
    REQUIRE(store.getValue(3) == Approx(181.0165738268184132).epsilon(1.0e-15));
  }
  if constexpr(std::is_same_v<TestType, uint16>)
  {
    REQUIRE(store.getValue(3) == Approx(255.998046867549362).epsilon(1.0e-15));
  }
  if constexpr(std::is_same_v<TestType, int32>)
  {
    REQUIRE(store.getValue(3) == Approx(46340.9500010519849).epsilon(1.0e-15));
  }
  if constexpr(std::is_same_v<TestType, uint32>)
  {
    REQUIRE(store.getValue(3) == Approx(65535.9999923706055).epsilon(1.0e-15));
  }
  if constexpr(std::is_same_v<TestType, int64>)
  {
    // Independent high-precision root of 9223372036854775807, with one float64 ULP of tolerance.
    REQUIRE(store.getValue(3) == Approx(3037000499.9760496923).epsilon(0).margin(5.0e-7));
  }
  if constexpr(std::is_same_v<TestType, uint64>)
  {
    // sqrt(18446744073709551615) rounds to 2^32 in float64.
    REQUIRE(store.getValue(3) == 4294967296.0);
  }
  if constexpr(std::is_signed_v<TestType>)
  {
    REQUIRE(std::isnan(store.getValue(4)));
    REQUIRE(std::isnan(store.getValue(5)));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::SqrtImageFilter: floating input retains type and IEEE domain results", "[ImageProcessing][Issue1874][SqrtImageFilter]", float32, float64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(
      dataStructure, {TestType{0}, -TestType{0}, 2, 4, -1, std::numeric_limits<TestType>::quiet_NaN(), std::numeric_limits<TestType>::infinity(), -std::numeric_limits<TestType>::infinity(),
                      std::numeric_limits<TestType>::max(), std::numeric_limits<TestType>::lowest(), std::numeric_limits<TestType>::min(), -std::numeric_limits<TestType>::min()});
  const DataPath output = input.replaceName("Output");
  const SqrtImageFilter filter;
  Arguments args;
  args.insertOrAssign(SqrtImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(SqrtImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(SqrtImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.value().actions.size() == 1);
  const auto* actionPtr = dynamic_cast<const CreateArrayAction*>(preflightResult.outputActions.value().actions.front().get());
  REQUIRE(actionPtr != nullptr);
  REQUIRE(actionPtr->type() == GetDataType<TestType>());
  REQUIRE(actionPtr->dims() == ShapeType{1, 1, 12});
  REQUIRE(actionPtr->componentDims() == ShapeType{1});
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(output));
  REQUIRE(outputArrayPtr->getDataType() == GetDataType<TestType>());
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 1, 12});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const DataArray<TestType>* rootsPtr = nullptr;
  REQUIRE_NOTHROW(rootsPtr = &dataStructure.getDataRefAs<DataArray<TestType>>(output));
  const auto& store = rootsPtr->getDataStoreRef();
  constexpr float64 rootTolerance = std::is_same_v<TestType, float32> ? 1.0e-7 : 3.0e-16;
  REQUIRE(store.getValue(0) == TestType{0});
  REQUIRE_FALSE(std::signbit(store.getValue(0)));
  REQUIRE(store.getValue(1) == TestType{0});
  REQUIRE(std::signbit(store.getValue(1)));
  REQUIRE(store.getValue(2) == Approx(1.4142135623730950488).epsilon(0).margin(rootTolerance));
  REQUIRE(store.getValue(3) == TestType{2});
  REQUIRE(std::isnan(store.getValue(4)));
  REQUIRE(std::isnan(store.getValue(5)));
  REQUIRE(std::isinf(store.getValue(6)));
  REQUIRE_FALSE(std::signbit(store.getValue(6)));
  REQUIRE(std::isnan(store.getValue(7)));
  REQUIRE(std::isfinite(store.getValue(8)));
  REQUIRE(store.getValue(8) > TestType{2});
  REQUIRE(std::isnan(store.getValue(9)));
  REQUIRE(std::isfinite(store.getValue(10)));
  REQUIRE(store.getValue(10) > TestType{0});
  REQUIRE(store.getValue(10) < TestType{1});
  REQUIRE(std::isnan(store.getValue(11)));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::SqrtImageFilter: floating boundary roots match independent binary values", "[ImageProcessing][Issue1874][SqrtImageFilter]", float32, float64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  constexpr TestType smallestNormal = std::numeric_limits<TestType>::min();
  constexpr TestType infinity = std::numeric_limits<TestType>::infinity();
  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(
      dataStructure, {std::numeric_limits<TestType>::denorm_min(), -std::numeric_limits<TestType>::denorm_min(), smallestNormal, std::nextafter(smallestNormal, TestType{0}),
                      std::nextafter(smallestNormal, infinity), TestType{1}, std::nextafter(TestType{1}, TestType{0}), std::nextafter(TestType{1}, infinity), std::numeric_limits<TestType>::max()});
  const DataPath output = input.replaceName("Output");
  const SqrtImageFilter filter;
  Arguments args;
  args.insertOrAssign(SqrtImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(SqrtImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(SqrtImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.value().actions.size() == 1);
  const auto* actionPtr = dynamic_cast<const CreateArrayAction*>(preflightResult.outputActions.value().actions.front().get());
  REQUIRE(actionPtr != nullptr);
  REQUIRE(actionPtr->type() == GetDataType<TestType>());
  REQUIRE(actionPtr->dims() == ShapeType{1, 1, 9});
  REQUIRE(actionPtr->componentDims() == ShapeType{1});
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(output));
  REQUIRE(outputArrayPtr->getDataType() == GetDataType<TestType>());
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 1, 9});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const DataArray<TestType>* rootsPtr = nullptr;
  REQUIRE_NOTHROW(rootsPtr = &dataStructure.getDataRefAs<DataArray<TestType>>(output));
  const auto& store = rootsPtr->getDataStoreRef();

  // Exact binary literals follow square roots of powers of two and nearest-value rounding.
  // The float32 smallest subnormal is 2^-149; its root is sqrt(2) * 2^-75.
  constexpr TestType subnormalRoot = static_cast<TestType>(std::is_same_v<TestType, float32> ? 0x1.6a09e6p-75 : 0x1p-537);
  constexpr TestType normalRoot = static_cast<TestType>(std::is_same_v<TestType, float32> ? 0x1p-63 : 0x1p-511);
  constexpr TestType belowNormalRoot = static_cast<TestType>(std::is_same_v<TestType, float32> ? 0x1.fffffep-64 : 0x1.fffffffffffffp-512);
  constexpr TestType belowOneRoot = static_cast<TestType>(std::is_same_v<TestType, float32> ? 0x1.fffffep-1 : 0x1.fffffffffffffp-1);
  constexpr TestType maximumRoot = static_cast<TestType>(std::is_same_v<TestType, float32> ? 0x1.fffffep63 : 0x1.fffffffffffffp511);
  REQUIRE(store.getValue(0) == subnormalRoot);
  REQUIRE(std::isnan(store.getValue(1)));
  REQUIRE(store.getValue(2) == normalRoot);
  REQUIRE(store.getValue(3) == belowNormalRoot);
  REQUIRE(store.getValue(4) == normalRoot);
  REQUIRE(store.getValue(5) == TestType{1});
  REQUIRE(store.getValue(6) == belowOneRoot);
  REQUIRE(store.getValue(7) == TestType{1});
  REQUIRE(store.getValue(8) == maximumRoot);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::SqrtImageFilter: mapped multi-component output preserves tuple shape", "[ImageProcessing][Issue1874][SqrtImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath input = ip_test::BuildRampImage<int16>(dataStructure, 2, 0, 0);
  Int16Array* inputArrayPtr = nullptr;
  REQUIRE_NOTHROW(inputArrayPtr = &dataStructure.getDataRefAs<Int16Array>(input));
  auto storePtr = DataStoreUtilities::CreateDataStore<int16>(dataStructure, input, ShapeType{2, 2, 2}, ShapeType{2});
  REQUIRE(storePtr != nullptr);
  const auto replaceResult = inputArrayPtr->setDataStore(storePtr);
  SIMPLNX_RESULT_REQUIRE_VALID(replaceResult);
  constexpr std::array<int16, 4> values = {0, 2, 4, -1};
  for(usize index = 0; index < 16; ++index)
  {
    storePtr->setValue(index, values.at(index % values.size()));
  }
  const DataPath output = runMappedIntegerSqrt(dataStructure, input, {2, 2, 2}, {2});
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(output));
  REQUIRE(outputArrayPtr->getDataType() == DataType::float64);
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{2, 2, 2});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{2});
  const Float64Array* rootsPtr = nullptr;
  REQUIRE_NOTHROW(rootsPtr = &dataStructure.getDataRefAs<Float64Array>(output));
  const auto& result = rootsPtr->getDataStoreRef();
  for(usize index = 0; index < 16; index += 4)
  {
    REQUIRE(result.getValue(index) == 0.0);
    REQUIRE(result.getValue(index + 1) == Approx(1.4142135623730950488).epsilon(0).margin(3.0e-16));
    REQUIRE(result.getValue(index + 2) == 2.0);
    REQUIRE(std::isnan(result.getValue(index + 3)));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Negative floating input produces a representable NaN.
TEST_CASE("ImageProcessing::SqrtImageFilter: sqrt of a negative is NaN (domain edge)", "[ImageProcessing][SqrtImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(ds, {-4.0f});
  const DataPath output = ip_test::RunPointwiseFilter<SqrtImageFilter>(ds, input);

  const Float32Array* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &ds.getDataRefAs<Float32Array>(output));
  const auto& out = outputArrayPtr->getDataStoreRef();
  REQUIRE(std::isnan(out.getValue(0)));
  UnitTest::CheckArraysInheritTupleDims(ds);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKSqrtImageTest.cpp(defaults): Ramp-Zero-One-Float.nrrd
// (float32) through OUR reader, run OUR filter (defaults), then compare with the committed baseline @0.01.
// See AcosImageFilterTest.cpp for the full rationale.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SqrtImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][SqrtImageFilter]")
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
  args.insertOrAssign(SqrtImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(SqrtImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(SqrtImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const SqrtImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // DURABLE golden: baseline compare @ 0.01.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath("BasicFilters_SqrtImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
