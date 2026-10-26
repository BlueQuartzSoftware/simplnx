#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/LogImageFilter.hpp"

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
DataPath runMappedIntegerLog(DataStructure& dataStructure, const DataPath& input, const ShapeType& tupleShape, const ShapeType& componentShape)
{
  const LogImageFilter filter;
  Arguments args;
  args.insertOrAssign(LogImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(LogImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(LogImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.value().actions.size() == 1);
  const auto* actionPtr = dynamic_cast<const CreateArrayAction*>(preflightResult.outputActions.value().actions.front().get());
  REQUIRE(actionPtr != nullptr);
  // Stop before zero or negative integer input can produce an unsafe nonfinite-to-integer cast.
  REQUIRE(actionPtr->type() == DataType::float64);
  REQUIRE(actionPtr->dims() == tupleShape);
  REQUIRE(actionPtr->componentDims() == componentShape);
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  return input.replaceName("Output");
}
} // namespace

TEMPLATE_TEST_CASE("ImageProcessing::LogImageFilter: integer values produce float64 natural logarithms", "[ImageProcessing][Issue1874][LogImageFilter]", int8, uint8, int16, uint16, int32, uint32,
                   int64, uint64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  constexpr int32 highestPower = std::numeric_limits<TestType>::digits - 1;
  constexpr TestType highestPowerOfTwo = static_cast<TestType>(TestType{1} << highestPower);
  std::vector<TestType> values = {1, 2, 0, std::numeric_limits<TestType>::max(), highestPowerOfTwo};
  if constexpr(std::is_signed_v<TestType>)
  {
    values.push_back(-1);
    values.push_back(std::numeric_limits<TestType>::lowest());
  }
  const ShapeType tupleShape = {1, 1, values.size()};
  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(dataStructure, values);
  const DataPath output = runMappedIntegerLog(dataStructure, input, tupleShape, {1});
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(output));
  REQUIRE(outputArrayPtr->getDataType() == DataType::float64);
  REQUIRE(outputArrayPtr->getTupleShape() == tupleShape);
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const Float64Array* logarithmsPtr = nullptr;
  REQUIRE_NOTHROW(logarithmsPtr = &dataStructure.getDataRefAs<Float64Array>(output));
  const auto& store = logarithmsPtr->getDataStoreRef();
  REQUIRE(store.getValue(0) == 0.0);
  REQUIRE(store.getValue(1) == Approx(0.69314718055994530942).epsilon(0).margin(1.0e-15));
  REQUIRE(std::isinf(store.getValue(2)));
  REQUIRE(std::signbit(store.getValue(2)));
  // 2^(digits-1) <= maximum < 2^digits. Rounding can make the upper logarithm equal for 64-bit types.
  constexpr float64 logTwo = 0.69314718055994530942;
  constexpr float64 lowerBound = highestPower * logTwo;
  constexpr float64 upperBound = std::numeric_limits<TestType>::digits * logTwo;
  REQUIRE(std::isfinite(store.getValue(3)));
  REQUIRE(store.getValue(3) >= lowerBound);
  REQUIRE(store.getValue(3) <= upperBound);
  REQUIRE(store.getValue(4) == Approx(lowerBound).epsilon(0).margin(1.0e-14));
  if constexpr(std::is_signed_v<TestType>)
  {
    REQUIRE(std::isnan(store.getValue(5)));
    REQUIRE(std::isnan(store.getValue(6)));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::LogImageFilter: floating input retains type and IEEE domain results", "[ImageProcessing][Issue1874][LogImageFilter]", float32, float64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath input =
      ip_test::BuildScalarValuesImage<TestType>(dataStructure, {1, 2, TestType{0}, -TestType{0}, -1, std::numeric_limits<TestType>::quiet_NaN(), std::numeric_limits<TestType>::infinity(),
                                                                -std::numeric_limits<TestType>::infinity(), std::numeric_limits<TestType>::max(), std::numeric_limits<TestType>::lowest()});
  const DataPath output = input.replaceName("Output");
  const LogImageFilter filter;
  Arguments args;
  args.insertOrAssign(LogImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(LogImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(LogImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.value().actions.size() == 1);
  const auto* actionPtr = dynamic_cast<const CreateArrayAction*>(preflightResult.outputActions.value().actions.front().get());
  REQUIRE(actionPtr != nullptr);
  REQUIRE(actionPtr->type() == GetDataType<TestType>());
  REQUIRE(actionPtr->dims() == ShapeType{1, 1, 10});
  REQUIRE(actionPtr->componentDims() == ShapeType{1});
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(output));
  REQUIRE(outputArrayPtr->getDataType() == GetDataType<TestType>());
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 1, 10});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const DataArray<TestType>* logarithmsPtr = nullptr;
  REQUIRE_NOTHROW(logarithmsPtr = &dataStructure.getDataRefAs<DataArray<TestType>>(output));
  const auto& store = logarithmsPtr->getDataStoreRef();
  constexpr float64 logarithmTolerance = std::is_same_v<TestType, float32> ? 3.0e-8 : 1.0e-15;
  REQUIRE(store.getValue(0) == TestType{0});
  REQUIRE(store.getValue(1) == Approx(0.69314718055994530942).epsilon(0).margin(logarithmTolerance));
  for(usize index = 2; index < 4; ++index)
  {
    REQUIRE(std::isinf(store.getValue(index)));
    REQUIRE(std::signbit(store.getValue(index)));
  }
  REQUIRE(std::isnan(store.getValue(4)));
  REQUIRE(std::isnan(store.getValue(5)));
  REQUIRE(std::isinf(store.getValue(6)));
  REQUIRE_FALSE(std::signbit(store.getValue(6)));
  REQUIRE(std::isnan(store.getValue(7)));
  REQUIRE(std::isfinite(store.getValue(8)));
  REQUIRE(store.getValue(8) > TestType{0});
  REQUIRE(std::isnan(store.getValue(9)));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::LogImageFilter: positive floating boundaries retain finite logarithms", "[ImageProcessing][Issue1874][LogImageFilter]", float32, float64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(dataStructure, {std::numeric_limits<TestType>::min(), std::numeric_limits<TestType>::denorm_min(),
                                                                                   std::nextafter(TestType{1}, TestType{0}), std::nextafter(TestType{1}, std::numeric_limits<TestType>::infinity())});
  const DataPath output = ip_test::RunPointwiseFilter<LogImageFilter>(dataStructure, input);
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(output));
  REQUIRE(outputArrayPtr->getDataType() == GetDataType<TestType>());
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 1, 4});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const DataArray<TestType>* logarithmsPtr = nullptr;
  REQUIRE_NOTHROW(logarithmsPtr = &dataStructure.getDataRefAs<DataArray<TestType>>(output));
  const auto& store = logarithmsPtr->getDataStoreRef();
  // IEEE binary types have minimum normal 2^(min_exponent-1) and minimum subnormal 2^(min_exponent-digits).
  constexpr float64 logTwo = 0.69314718055994530942;
  constexpr float64 normalLogarithm = (std::numeric_limits<TestType>::min_exponent - 1) * logTwo;
  constexpr float64 subnormalLogarithm = (std::numeric_limits<TestType>::min_exponent - std::numeric_limits<TestType>::digits) * logTwo;
  constexpr float64 logarithmTolerance = std::is_same_v<TestType, float32> ? 1.0e-5 : 1.0e-12;
  REQUIRE(store.getValue(0) == Approx(normalLogarithm).epsilon(0).margin(logarithmTolerance));
  REQUIRE(store.getValue(1) == Approx(subnormalLogarithm).epsilon(0).margin(logarithmTolerance));
  REQUIRE(std::isfinite(store.getValue(2)));
  REQUIRE(store.getValue(2) < TestType{0});
  REQUIRE(std::isfinite(store.getValue(3)));
  REQUIRE(store.getValue(3) > TestType{0});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::LogImageFilter: mapped multi-component output preserves tuple shape", "[ImageProcessing][Issue1874][LogImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath input = ip_test::BuildRampImage<int16>(dataStructure, 2, 1, 0);
  Int16Array* inputArrayPtr = nullptr;
  REQUIRE_NOTHROW(inputArrayPtr = &dataStructure.getDataRefAs<Int16Array>(input));
  auto storePtr = DataStoreUtilities::CreateDataStore<int16>(dataStructure, input, ShapeType{2, 2, 2}, ShapeType{2});
  REQUIRE(storePtr != nullptr);
  const auto replaceResult = inputArrayPtr->setDataStore(storePtr);
  SIMPLNX_RESULT_REQUIRE_VALID(replaceResult);
  constexpr std::array<int16, 4> values = {1, 2, 0, -1};
  for(usize index = 0; index < 16; ++index)
  {
    storePtr->setValue(index, values.at(index % values.size()));
  }
  const DataPath output = runMappedIntegerLog(dataStructure, input, {2, 2, 2}, {2});
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(output));
  REQUIRE(outputArrayPtr->getDataType() == DataType::float64);
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{2, 2, 2});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{2});
  const Float64Array* logarithmsPtr = nullptr;
  REQUIRE_NOTHROW(logarithmsPtr = &dataStructure.getDataRefAs<Float64Array>(output));
  const auto& result = logarithmsPtr->getDataStoreRef();
  for(usize index = 0; index < 16; index += 4)
  {
    REQUIRE(result.getValue(index) == 0.0);
    REQUIRE(result.getValue(index + 1) == Approx(0.69314718055994530942).epsilon(0).margin(1.0e-15));
    REQUIRE(std::isinf(result.getValue(index + 2)));
    REQUIRE(std::signbit(result.getValue(index + 2)));
    REQUIRE(std::isnan(result.getValue(index + 3)));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Floating domain behavior: log(x<0) is NaN and log(0) is -infinity.
TEST_CASE("ImageProcessing::LogImageFilter: log of negative is NaN and log(0) is -inf (domain edge)", "[ImageProcessing][LogImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(ds, {-2.0f, 0.0f});
  const DataPath output = ip_test::RunPointwiseFilter<LogImageFilter>(ds, input);

  const Float32Array* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &ds.getDataRefAs<Float32Array>(output));
  const auto& out = outputArrayPtr->getDataStoreRef();
  REQUIRE(std::isnan(out.getValue(0))); // log(-2) -> NaN
  REQUIRE(std::isinf(out.getValue(1))); // log(0)  -> -inf
  REQUIRE(out.getValue(1) < 0.0f);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKLogImageTest.cpp(defaults): Ramp-Zero-One-Float.nrrd
// (float32) through OUR reader, run OUR filter (defaults), then compare with the committed baseline @0.01.
// log(0) at the ramp's zero produces -inf; ip_golden::CompareImages mirrors ITK's own max-abs-diff comparator, which
// tolerates the -inf tail. See AcosImageFilterTest.cpp for the full rationale.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::LogImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][LogImageFilter]")
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
  args.insertOrAssign(LogImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(LogImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(LogImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const LogImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // DURABLE golden: baseline compare @ 0.01.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath("BasicFilters_LogImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
