#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/ExpImageFilter.hpp"
#include "ImageProcessing/utils/MathOutputUtilities.hpp"

#include "simplnx/Common/Aliases.hpp"
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
#include <cmath>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

using namespace nx::core;

TEMPLATE_TEST_CASE("ImageProcessing::ExpImageFilter: integer input produces float64 growth and decay", "[ImageProcessing][Issue1874][ExpImageFilter]", int8, uint8, int16, uint16, int32, uint32, int64,
                   uint64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  std::vector<TestType> values = {0, 1, 100};
  if constexpr(std::is_signed_v<TestType>)
  {
    values.push_back(-1);
    values.push_back(std::numeric_limits<TestType>::lowest());
  }
  values.push_back(std::numeric_limits<TestType>::max());

  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(dataStructure, values);
  const DataPath output = input.replaceName("Output");
  Arguments args;
  args.insertOrAssign(ExpImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(ExpImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(ExpImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));

  const ExpImageFilter filter;
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.value().actions.size() == 1);
  const auto* actionPtr = dynamic_cast<const CreateArrayAction*>(preflightResult.outputActions.value().actions.front().get());
  REQUIRE(actionPtr != nullptr);
  // Stop the baseline before exp(100) can undergo an out-of-range conversion to an integer.
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
  REQUIRE(store.getValue(1) == Approx(2.71828182845904523536).epsilon(1.0e-15));
  REQUIRE(store.getValue(2) == Approx(2.68811714181613544841e43).epsilon(1.0e-15));
  if constexpr(std::is_signed_v<TestType>)
  {
    REQUIRE(store.getValue(3) == Approx(0.36787944117144232160).epsilon(1.0e-15));
    if constexpr(std::is_same_v<TestType, int8>)
    {
      REQUIRE(store.getValue(4) == Approx(2.57220937264241482684e-56).epsilon(1.0e-15));
    }
    else
    {
      REQUIRE(store.getValue(4) == 0.0);
    }
  }
  const float64 maximumResult = store.getValue(values.size() - 1);
  if constexpr(std::is_same_v<TestType, int8>)
  {
    REQUIRE(maximumResult == Approx(1.43020799583481044656e55).epsilon(1.0e-15));
  }
  else if constexpr(std::is_same_v<TestType, uint8>)
  {
    REQUIRE(maximumResult == Approx(5.56023164772767512139e110).epsilon(1.0e-15));
  }
  else
  {
    REQUIRE(std::isinf(maximumResult));
    REQUIRE(maximumResult > 0.0);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::ExpImageFilter: floating IEEE values retain growth decay and range behavior", "[ImageProcessing][Issue1874][ExpImageFilter]", float32, float64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(dataStructure, {std::numeric_limits<TestType>::quiet_NaN(),
                                                                                   std::numeric_limits<TestType>::infinity(),
                                                                                   -std::numeric_limits<TestType>::infinity(),
                                                                                   TestType{0},
                                                                                   -TestType{0},
                                                                                   100,
                                                                                   -128,
                                                                                   1000,
                                                                                   -1000,
                                                                                   -100,
                                                                                   -103,
                                                                                   -104,
                                                                                   10,
                                                                                   88,
                                                                                   89,
                                                                                   709,
                                                                                   710,
                                                                                   -744,
                                                                                   -745,
                                                                                   -746});
  const DataPath output = ip_test::RunPointwiseFilter<ExpImageFilter>(dataStructure, input);
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(output));
  REQUIRE(outputArrayPtr->getDataType() == GetDataType<TestType>());
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 1, 20});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const DataArray<TestType>* exponentialPtr = nullptr;
  REQUIRE_NOTHROW(exponentialPtr = &dataStructure.getDataRefAs<DataArray<TestType>>(output));
  const auto& store = exponentialPtr->getDataStoreRef();
  REQUIRE(std::isnan(store.getValue(0)));
  REQUIRE(store.getValue(1) == std::numeric_limits<TestType>::infinity());
  REQUIRE(store.getValue(2) == 0);
  REQUIRE_FALSE(std::signbit(store.getValue(2)));
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
    REQUIRE(store.getValue(6) == Approx(2.57220937264241482684e-56).epsilon(1.0e-15));
    REQUIRE(store.getValue(9) == Approx(3.72007597602083596296e-44).epsilon(1.0e-15));
    REQUIRE(store.getValue(10) == Approx(1.85211676951797542963e-45).epsilon(1.0e-15));
    REQUIRE(store.getValue(11) == Approx(6.81355682154529853676e-46).epsilon(1.0e-15));
    REQUIRE(store.getValue(12) == Approx(22026.46579480671651696).epsilon(1.0e-15));
    REQUIRE(store.getValue(13) == Approx(1.65163625499400185553e38).epsilon(1.0e-15));
    REQUIRE(store.getValue(14) == Approx(4.48961281917434524628e38).epsilon(1.0e-15));
    REQUIRE(store.getValue(15) == Approx(8.21840746155497218924e307).epsilon(1.0e-15));
    REQUIRE(store.getValue(17) == 2 * std::numeric_limits<float64>::denorm_min());
    REQUIRE(store.getValue(18) == std::numeric_limits<float64>::denorm_min());
    REQUIRE(store.getValue(19) == 0);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::ExpImageFilter: mapped multi-component output preserves tuple shape", "[ImageProcessing][Issue1874][ExpImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath input = ip_test::BuildRampImage<int16>(dataStructure, 2, 0, 0);
  Int16Array* inputArrayPtr = nullptr;
  REQUIRE_NOTHROW(inputArrayPtr = &dataStructure.getDataRefAs<Int16Array>(input));
  auto inputStore = DataStoreUtilities::CreateDataStore<int16>(dataStructure, input, ShapeType{2, 2, 2}, ShapeType{2});
  REQUIRE(inputStore != nullptr);
  const auto replaceResult = inputArrayPtr->setDataStore(inputStore);
  SIMPLNX_RESULT_REQUIRE_VALID(replaceResult);
  for(usize index = 0; index < 16; ++index)
  {
    inputStore->setValue(index, static_cast<int16>(static_cast<int32>(index % 4) - 1));
  }

  Arguments args;
  args.insertOrAssign(ExpImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(ExpImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(ExpImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const ExpImageFilter filter;
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
    REQUIRE(store.getValue(index) == Approx(0.36787944117144232160).epsilon(1.0e-15));
    REQUIRE(store.getValue(index + 1) == 1.0);
    REQUIRE(store.getValue(index + 2) == Approx(2.71828182845904523536).epsilon(1.0e-15));
    REQUIRE(store.getValue(index + 3) == Approx(7.38905609893065022723).epsilon(1.0e-15));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::ExpImageFilter: floating result conversion handles finite narrowing boundaries", "[ImageProcessing][Issue1874][ExpImageFilter]")
{
  UnitTest::LoadPlugins();
  constexpr float64 finiteLimit = std::numeric_limits<float32>::max();
  const float64 outside = std::nextafter(finiteLimit, std::numeric_limits<float64>::infinity());
  const float64 inside = std::nextafter(finiteLimit, float64{0});
  REQUIRE(ImageProcessing::ConvertMathResult<float32>(inside) == std::numeric_limits<float32>::max());
  REQUIRE(ImageProcessing::ConvertMathResult<float32>(finiteLimit) == std::numeric_limits<float32>::max());
  REQUIRE(ImageProcessing::ConvertMathResult<float32>(outside) == std::numeric_limits<float32>::infinity());
  REQUIRE(ImageProcessing::ConvertMathResult<float32>(-inside) == -std::numeric_limits<float32>::max());
  REQUIRE(ImageProcessing::ConvertMathResult<float32>(-finiteLimit) == -std::numeric_limits<float32>::max());
  REQUIRE(ImageProcessing::ConvertMathResult<float32>(-outside) == -std::numeric_limits<float32>::infinity());
  REQUIRE(std::isnan(ImageProcessing::ConvertMathResult<float32>(std::numeric_limits<float64>::quiet_NaN())));
  REQUIRE(ImageProcessing::ConvertMathResult<float32>(std::numeric_limits<float64>::infinity()) == std::numeric_limits<float32>::infinity());
  REQUIRE(ImageProcessing::ConvertMathResult<float32>(-std::numeric_limits<float64>::infinity()) == -std::numeric_limits<float32>::infinity());
  REQUIRE(ImageProcessing::ConvertMathResult<float32>(0.0) == 0.0f);
  REQUIRE_FALSE(std::signbit(ImageProcessing::ConvertMathResult<float32>(0.0)));
  REQUIRE(std::signbit(ImageProcessing::ConvertMathResult<float32>(-0.0)));
  REQUIRE(ImageProcessing::ConvertMathResult<float32>(std::numeric_limits<float32>::denorm_min()) == std::numeric_limits<float32>::denorm_min());
  REQUIRE(ImageProcessing::ConvertMathResult<float64>(outside) == outside);
  REQUIRE(ImageProcessing::ConvertMathResult<float64>(std::numeric_limits<float64>::denorm_min()) == std::numeric_limits<float64>::denorm_min());
  REQUIRE(std::signbit(ImageProcessing::ConvertMathResult<float64>(-0.0)));
}

TEMPLATE_TEST_CASE("ImageProcessing::ExpImageFilter: floating input retains type for safe finite values", "[ImageProcessing][Issue1874][ExpImageFilter]", float32, float64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(dataStructure, {-1, 0, 1});
  const DataPath output = input.replaceName("Output");
  Arguments args;
  args.insertOrAssign(ExpImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(ExpImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(ExpImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));

  const ExpImageFilter filter;
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
  REQUIRE(store.getValue(0) == Approx(0.36787944117144232160).epsilon(relativeTolerance));
  REQUIRE(store.getValue(1) == 1);
  REQUIRE(store.getValue(2) == Approx(2.71828182845904523536).epsilon(relativeTolerance));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// exp(1000) exceeds the float64 finite range and produces positive infinity.
// ConvertMathResult retains infinity and explicitly handles finite results above the float32 range.
TEST_CASE("ImageProcessing::ExpImageFilter: exp of a large input overflows to +inf (domain edge)", "[ImageProcessing][ExpImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(ds, {1000.0f});
  const DataPath output = ip_test::RunPointwiseFilter<ExpImageFilter>(ds, input);

  const Float32Array* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &ds.getDataRefAs<Float32Array>(output));
  const auto& out = outputArrayPtr->getDataStoreRef();
  REQUIRE(std::isinf(out.getValue(0)));
  REQUIRE(out.getValue(0) > 0.0f); // +inf
  UnitTest::CheckArraysInheritTupleDims(ds);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKExpImageTest.cpp(defaults): Ramp-Zero-One-Float.nrrd
// (float32) through OUR reader, run OUR filter (defaults), then compare with the committed baseline @0.01.
// See AcosImageFilterTest.cpp for the full rationale.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ExpImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][ExpImageFilter]")
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
  args.insertOrAssign(ExpImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(ExpImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(ExpImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const ExpImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // DURABLE golden: baseline compare @ 0.01.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath("BasicFilters_ExpImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
