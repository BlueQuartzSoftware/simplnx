#include "C15CompatibilityTestUtils.hpp"
#include "ItkGoldenTestUtils.hpp"
#include "NXImageProcessing/Filters/ExpImageFilter.hpp"
#include "NXImageProcessing/Filters/Log10ImageFilter.hpp"
#include "NXImageProcessing/Filters/LogImageFilter.hpp"
#include "NXImageProcessing/Filters/SqrtImageFilter.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/ExpNegativeImageFilter.hpp"

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
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <any>
#include <cmath>
#include <limits>
#include <numbers>
#include <string>
#include <type_traits>
#include <vector>

using namespace nx::core;

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
TEMPLATE_TEST_CASE("NXImageProcessing::ExpNegativeImageFilter: integer input produces float64 decay", "[NXImageProcessing][Issue1874][ExpNegativeImageFilter]", int8, uint8, int16, uint16, int32,
                   uint32, int64, uint64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  const auto values = [] {
    if constexpr(std::is_signed_v<TestType>)
    {
      return std::vector<TestType>{0, 1, std::numeric_limits<TestType>::max(), -1, std::numeric_limits<TestType>::lowest()};
    }
    else
    {
      return std::vector<TestType>{0, 1, std::numeric_limits<TestType>::max()};
    }
  }();
  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(dataStructure, values);
  const DataPath output = input.replaceName("Output");
  Arguments args;
  args.insertOrAssign(ExpNegativeImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(ExpNegativeImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(ExpNegativeImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));

  const ExpNegativeImageFilter filter;
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.value().actions.size() == 1);
  const auto* actionPtr = dynamic_cast<const CreateArrayAction*>(preflightResult.outputActions.value().actions.front().get());
  REQUIRE(actionPtr != nullptr);
  // Stop before an extreme negative input could overflow an integer output.
  REQUIRE(actionPtr->type() == DataType::float64);
  REQUIRE(actionPtr->dims() == ShapeType{1, 1, values.size()});
  REQUIRE(actionPtr->componentDims() == ShapeType{1});

  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(output));
  REQUIRE(outputArrayPtr->getDataType() == DataType::float64);
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 1, values.size()});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const Float64Array* exponentialPtr = nullptr;
  REQUIRE_NOTHROW(exponentialPtr = &dataStructure.getDataRefAs<Float64Array>(output));
  const auto& store = exponentialPtr->getDataStoreRef();
  REQUIRE(store.getValue(0) == 1.0);
  REQUIRE(store.getValue(1) == Approx(0.36787944117144232160).epsilon(1.0e-15));
  if constexpr(std::is_same_v<TestType, int8>)
  {
    REQUIRE(store.getValue(2) > 0.0);
    REQUIRE(store.getValue(2) == Approx(6.99198999664591705080e-56).epsilon(1.0e-15));
  }
  else if constexpr(std::is_same_v<TestType, uint8>)
  {
    REQUIRE(store.getValue(2) > 0.0);
    REQUIRE(store.getValue(2) == Approx(1.79848622027946356313e-111).epsilon(1.0e-15));
  }
  else
  {
    REQUIRE(store.getValue(2) == 0.0);
    REQUIRE_FALSE(std::signbit(store.getValue(2)));
  }
  if constexpr(std::is_signed_v<TestType>)
  {
    REQUIRE(store.getValue(3) == Approx(2.71828182845904523536).epsilon(1.0e-15));
    if constexpr(std::is_same_v<TestType, int8>)
    {
      REQUIRE(store.getValue(4) == Approx(3.88770840599459509222e55).epsilon(1.0e-15));
    }
    else
    {
      REQUIRE(store.getValue(4) == std::numeric_limits<float64>::infinity());
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// NOLINTBEGIN(bugprone-chained-comparison) Catch2 comparison decomposition triggers this check.
TEMPLATE_TEST_CASE("NXImageProcessing::ExpNegativeImageFilter: floating IEEE values retain sign reversed range behavior", "[NXImageProcessing][Issue1874][ExpNegativeImageFilter]", float32, float64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(dataStructure, {std::numeric_limits<TestType>::quiet_NaN(),
                                                                                   std::numeric_limits<TestType>::infinity(),
                                                                                   -std::numeric_limits<TestType>::infinity(),
                                                                                   TestType{0},
                                                                                   -TestType{0},
                                                                                   -100,
                                                                                   128,
                                                                                   -1000,
                                                                                   1000,
                                                                                   100,
                                                                                   103,
                                                                                   104,
                                                                                   -10,
                                                                                   -88,
                                                                                   -89,
                                                                                   -709,
                                                                                   -710,
                                                                                   744,
                                                                                   745,
                                                                                   746});
  const DataPath output = ip_test::RunPointwiseFilter<ExpNegativeImageFilter>(dataStructure, input);
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(output));
  REQUIRE(outputArrayPtr->getDataType() == GetDataType<TestType>());
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 1, 20});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const DataArray<TestType>* exponentialPtr = nullptr;
  REQUIRE_NOTHROW(exponentialPtr = &dataStructure.getDataRefAs<DataArray<TestType>>(output));
  const auto& store = exponentialPtr->getDataStoreRef();
  REQUIRE(std::isnan(store.getValue(0)));
  REQUIRE(store.getValue(1) == 0);
  REQUIRE_FALSE(std::signbit(store.getValue(1)));
  REQUIRE(store.getValue(2) == std::numeric_limits<TestType>::infinity());
  REQUIRE(store.getValue(3) == 1);
  REQUIRE(store.getValue(4) == 1);
  REQUIRE(store.getValue(7) == std::numeric_limits<TestType>::infinity());
  REQUIRE(store.getValue(8) == 0);
  REQUIRE_FALSE(std::signbit(store.getValue(8)));
  REQUIRE(store.getValue(16) == std::numeric_limits<TestType>::infinity());
  if constexpr(std::is_same_v<TestType, float32>)
  {
    REQUIRE(store.getValue(5) == std::numeric_limits<float32>::infinity());
    REQUIRE(store.getValue(6) == 0);
    REQUIRE(store.getValue(9) == 3.7835058536770060915e-44f);
    REQUIRE(store.getValue(10) == std::numeric_limits<float32>::denorm_min());
    REQUIRE(store.getValue(11) == 0);
    REQUIRE(store.getValue(12) == Approx(22026.46579480671651696).epsilon(1.0e-7));
    REQUIRE(store.getValue(13) == Approx(1.65163625499400185553e38).epsilon(1.0e-7));
    REQUIRE(store.getValue(14) == std::numeric_limits<float32>::infinity());
    REQUIRE(store.getValue(15) == std::numeric_limits<float32>::infinity());
    REQUIRE(store.getValue(17) == 0);
    REQUIRE(store.getValue(18) == 0);
    REQUIRE(store.getValue(19) == 0);
  }
  else
  {
    REQUIRE(store.getValue(5) == Approx(2.68811714181613544841e43).epsilon(1.0e-15));
    REQUIRE(store.getValue(6) > 0);
    REQUIRE(store.getValue(6) == Approx(2.57220937264241482684e-56).epsilon(1.0e-15));
    REQUIRE(store.getValue(9) > 0);
    REQUIRE(store.getValue(9) == Approx(3.72007597602083596296e-44).epsilon(1.0e-15));
    REQUIRE(store.getValue(10) > 0);
    REQUIRE(store.getValue(10) == Approx(1.85211676951797542963e-45).epsilon(1.0e-15));
    REQUIRE(store.getValue(11) > 0);
    REQUIRE(store.getValue(11) == Approx(6.81355682154529853676e-46).epsilon(1.0e-15));
    REQUIRE(store.getValue(12) == Approx(22026.46579480671651696).epsilon(1.0e-15));
    REQUIRE(store.getValue(13) == Approx(1.65163625499400185553e38).epsilon(1.0e-15));
    REQUIRE(store.getValue(14) == Approx(4.48961281917434524628e38).epsilon(1.0e-15));
    REQUIRE(store.getValue(15) == Approx(8.21840746155497218924e307).epsilon(1.0e-15));
    // exp(-744) < 2d, exp(-745) < d, and exp(-746) < d/2, where d = 2^-1074.
    // Accept zero or bounded subnormal results without requiring a particular libm rounding.
    for(usize index = 17; index < 20; ++index)
    {
      const float64 value = store.getValue(index);
      const float64 upperBound = index == 17 ? 2 * std::numeric_limits<float64>::denorm_min() : std::numeric_limits<float64>::denorm_min();
      CAPTURE(index);
      REQUIRE(std::isfinite(value));
      REQUIRE(value >= 0);
      REQUIRE_FALSE(std::signbit(value));
      REQUIRE(value <= upperBound);
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison)

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
TEST_CASE("NXImageProcessing::ExpNegativeImageFilter: mapped multi component output preserves tuple shape", "[NXImageProcessing][Issue1874][ExpNegativeImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath input = ip_test::BuildRampImage<int16>(dataStructure, 2, 0, 0);
  Int16Array* inputArrayPtr = nullptr;
  REQUIRE_NOTHROW(inputArrayPtr = &dataStructure.getDataRefAs<Int16Array>(input));
  auto inputStorePtr = DataStoreUtilities::CreateDataStore<int16>(dataStructure, input, ShapeType{2, 2, 2}, ShapeType{2});
  REQUIRE(inputStorePtr != nullptr);
  const auto replaceResult = inputArrayPtr->setDataStore(inputStorePtr);
  SIMPLNX_RESULT_REQUIRE_VALID(replaceResult);
  for(usize index = 0; index < 16; ++index)
  {
    inputStorePtr->setValue(index, static_cast<int16>(static_cast<int32>(index % 4) - 1));
  }
  Arguments args;
  args.insertOrAssign(ExpNegativeImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(ExpNegativeImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(ExpNegativeImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const ExpNegativeImageFilter filter;
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.value().actions.size() == 1);
  const auto* actionPtr = dynamic_cast<const CreateArrayAction*>(preflightResult.outputActions.value().actions.front().get());
  REQUIRE(actionPtr != nullptr);
  REQUIRE(actionPtr->type() == DataType::float64);
  REQUIRE(actionPtr->dims() == ShapeType{2, 2, 2});
  REQUIRE(actionPtr->componentDims() == ShapeType{2});
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const DataPath output = input.replaceName("Output");
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(output));
  REQUIRE(outputArrayPtr->getDataType() == DataType::float64);
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{2, 2, 2});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{2});
  const Float64Array* exponentialPtr = nullptr;
  REQUIRE_NOTHROW(exponentialPtr = &dataStructure.getDataRefAs<Float64Array>(output));
  const auto& store = exponentialPtr->getDataStoreRef();
  for(usize index = 0; index < 16; index += 4)
  {
    REQUIRE(store.getValue(index) == Approx(2.71828182845904523536).epsilon(1.0e-15));
    REQUIRE(store.getValue(index + 1) == 1.0);
    REQUIRE(store.getValue(index + 2) == Approx(0.36787944117144232160).epsilon(1.0e-15));
    REQUIRE(store.getValue(index + 3) == Approx(0.13533528323661269189).epsilon(1.0e-15));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
TEMPLATE_TEST_CASE("NXImageProcessing::ExpNegativeImageFilter: floating input retains type for safe finite values", "[NXImageProcessing][Issue1874][ExpNegativeImageFilter]", float32, float64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(dataStructure, {-1, 0, 1});
  const DataPath output = input.replaceName("Output");
  Arguments args;
  args.insertOrAssign(ExpNegativeImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(ExpNegativeImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(ExpNegativeImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));

  const ExpNegativeImageFilter filter;
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.value().actions.size() == 1);
  const auto* actionPtr = dynamic_cast<const CreateArrayAction*>(preflightResult.outputActions.value().actions.front().get());
  REQUIRE(actionPtr != nullptr);
  REQUIRE(actionPtr->type() == GetDataType<TestType>());
  REQUIRE(actionPtr->dims() == ShapeType{1, 1, 3});
  REQUIRE(actionPtr->componentDims() == ShapeType{1});

  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(output));
  REQUIRE(outputArrayPtr->getDataType() == GetDataType<TestType>());
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 1, 3});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const DataArray<TestType>* exponentialPtr = nullptr;
  REQUIRE_NOTHROW(exponentialPtr = &dataStructure.getDataRefAs<DataArray<TestType>>(output));
  const auto& store = exponentialPtr->getDataStoreRef();
  constexpr float64 relativeTolerance = std::is_same_v<TestType, float32> ? 1.0e-7 : 1.0e-15;
  REQUIRE(store.getValue(0) == Approx(2.71828182845904523536).epsilon(relativeTolerance));
  REQUIRE(store.getValue(1) == 1);
  REQUIRE(store.getValue(2) == Approx(0.36787944117144232160).epsilon(relativeTolerance));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// exp(-(-1000)) exceeds the float64 finite range and produces positive infinity.
// The shared math conversion also handles finite results above the float32 range explicitly.
TEST_CASE("NXImageProcessing::ExpNegativeImageFilter: exp(-x) of a large negative input overflows to +inf (domain edge)", "[NXImageProcessing][ExpNegativeImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(ds, {-1000.0f});
  const DataPath output = ip_test::RunPointwiseFilter<ExpNegativeImageFilter>(ds, input);

  const Float32Array* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &ds.getDataRefAs<Float32Array>(output));
  const auto& out = outputArrayPtr->getDataStoreRef();
  REQUIRE(std::isinf(out.getValue(0)));
  REQUIRE(out.getValue(0) > 0.0f); // exp(-(-1000)) = exp(1000) = +inf
  UnitTest::CheckArraysInheritTupleDims(ds);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKExpNegativeImageTest.cpp(defaults):
// Ramp-Zero-One-Float.nrrd (float32) through OUR reader, run OUR filter (defaults), then compare with the
// committed baseline @0.01. See AcosImageFilterTest.cpp for the full rationale.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::ExpNegativeImageFilter: ITK real-image golden (defaults)", "[NXImageProcessing][ItkGolden][ExpNegativeImageFilter]")
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
  args.insertOrAssign(ExpNegativeImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(ExpNegativeImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(ExpNegativeImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const ExpNegativeImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // DURABLE golden: baseline compare @ 0.01.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath("BasicFilters_ExpNegativeImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}

// NOLINTBEGIN(bugprone-chained-comparison) Catch2 comparison decomposition triggers this check.
TEST_CASE("NXImageProcessing::C15Compatibility resident exponential and domain masks feed components", "[NXImageProcessing][C15Compatibility][Issue1874][ExpNegativeImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const c15_test::StoreExpectation expectedStore{IDataStore::StoreType::InMemory};
  const usize variant = GENERATE(usize{0}, usize{1}, usize{2}, usize{3}, usize{4}, usize{5}, usize{6});
  CAPTURE(variant);
  DataStructure dataStructure;
  DataPath input;
  DataPath quantitative;
  std::vector<float64> expected;
  std::vector<uint8> selected;
  std::vector<uint32> components;
  float64 lower = 0;
  float64 upper = 2;
  const auto nan = std::numeric_limits<float64>::quiet_NaN();
  const auto inf = std::numeric_limits<float64>::infinity();
  switch(variant)
  {
  case 0:
    input = ip_test::BuildScalarValuesImage<int16>(dataStructure, {0, 1, 2});
    quantitative = c15_test::Pointwise<ExpNegativeImageFilter>(dataStructure, input, "Quantitative");
    expected = {1, 0.36787944117144232160, 0.13533528323661269189};
    lower = 0.5;
    upper = 1;
    selected = {1, 0, 0};
    components = {1, 0, 0};
    REQUIRE(c15_test::Read<int16>(dataStructure, input, {1, 1, 3}, expectedStore) == std::vector<int16>{0, 1, 2});
    break;
  case 1:
    input = ip_test::BuildScalarValuesImage<int16>(dataStructure, {0, 1});
    quantitative = c15_test::Pointwise<ExpImageFilter>(dataStructure, input, "Quantitative");
    expected = {1, std::numbers::e};
    lower = 1;
    selected = {1, 0};
    components = {1, 0};
    REQUIRE(c15_test::Read<int16>(dataStructure, input, {1, 1, 2}, expectedStore) == std::vector<int16>{0, 1});
    break;
  case 2:
    input = ip_test::BuildScalarValuesImage<float64>(dataStructure, {0, 1000, nan, -inf, inf});
    quantitative = c15_test::Pointwise<ExpImageFilter>(dataStructure, input, "Quantitative");
    expected = {1, inf, nan, 0, inf};
    selected = {1, 0, 0, 1, 0};
    components = {1, 0, 0, 2, 0};
    break;
  case 3:
    input = ip_test::BuildScalarValuesImage<float64>(dataStructure, {1, 0, -1, inf, nan});
    quantitative = c15_test::Pointwise<LogImageFilter>(dataStructure, input, "Quantitative");
    expected = {0, -inf, nan, inf, nan};
    lower = -1;
    upper = 1;
    selected = {1, 0, 0, 0, 0};
    components = {1, 0, 0, 0, 0};
    break;
  case 4:
    input = ip_test::BuildScalarValuesImage<float64>(dataStructure, {1, 10, 0, -1});
    quantitative = c15_test::Pointwise<Log10ImageFilter>(dataStructure, input, "Quantitative");
    expected = {0, 1, -inf, nan};
    upper = 1;
    selected = {1, 1, 0, 0};
    components = {1, 1, 0, 0};
    break;
  case 5:
    input = ip_test::BuildScalarValuesImage<float64>(dataStructure, {0, 4, -1, inf, nan});
    quantitative = c15_test::Pointwise<SqrtImageFilter>(dataStructure, input, "Quantitative");
    expected = {0, 2, nan, inf, nan};
    selected = {1, 1, 0, 0, 0};
    components = {1, 1, 0, 0, 0};
    break;
  default:
    input = ip_test::BuildScalarValuesImage<float64>(dataStructure, {0, 1, 2});
    quantitative = c15_test::Pointwise<ExpNegativeImageFilter>(dataStructure, input, "Quantitative");
    expected = {1, 0.36787944117144232160, 0.13533528323661269189};
    lower = 0.5;
    upper = 1;
    selected = {1, 0, 0};
    components = {1, 0, 0};
    break;
  }
  const ShapeType shape{1, 1, expected.size()};
  auto checkQuantitative = [&] {
    const auto values = c15_test::Read<float64>(dataStructure, quantitative, shape, expectedStore);
    for(usize index = 0; index < expected.size(); ++index)
    {
      CAPTURE(index);
      if(std::isnan(expected[index]))
      {
        REQUIRE(std::isnan(values[index]));
      }
      else if(std::isinf(expected[index]) || expected[index] == 0)
      {
        REQUIRE(values[index] == expected[index]);
      }
      else
      {
        REQUIRE(values[index] == Approx(expected[index]).epsilon(1.0e-15));
      }
    }
  };
  if(variant >= 2)
  {
    REQUIRE(c15_test::Read<float64>(dataStructure, input, shape, expectedStore).size() == expected.size());
  }
  checkQuantitative();
  const auto mask = c15_test::Select(dataStructure, quantitative, lower, upper, "Selected");
  REQUIRE(c15_test::Read<uint8>(dataStructure, mask, shape, expectedStore) == selected);
  const auto labels = c15_test::Components(dataStructure, mask, "Components");
  REQUIRE(c15_test::Read<uint32>(dataStructure, labels, shape, expectedStore) == components);
  checkQuantitative();
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison)
