#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/Log10ImageFilter.hpp"

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <any>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

using namespace nx::core;

namespace
{
// Independent decimal constants for log10(2^digits - 1), rounded to float64.
template <class T>
constexpr float64 maximumIntegerLog10()
{
  constexpr auto digits = std::numeric_limits<T>::digits;
  if constexpr(digits == 7)
  {
    return 2.1038037209559568;
  }
  else if constexpr(digits == 8)
  {
    return 2.406540180433955;
  }
  else if constexpr(digits == 15)
  {
    return 4.515436681141699;
  }
  else if constexpr(digits == 16)
  {
    return 4.81647330376525;
  }
  else if constexpr(digits == 31)
  {
    return 9.331929865381183;
  }
  else if constexpr(digits == 32)
  {
    return 9.632959861146281;
  }
  else if constexpr(digits == 63)
  {
    return 18.964889726830815;
  }
  else
  {
    static_assert(digits == 64);
    return 19.265919722494797;
  }
}
} // namespace

TEMPLATE_TEST_CASE("ImageProcessing::Log10ImageFilter: integer values produce float64 logarithms and domain results", "[ImageProcessing][Issue1874][Log10ImageFilter]", int8, uint8, int16, uint16,
                   int32, uint32, int64, uint64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  std::vector<TestType> values = {1, 10, 100, 2, 0, std::numeric_limits<TestType>::max()};
  if constexpr(std::is_signed_v<TestType>)
  {
    values.push_back(-1);
    values.push_back(std::numeric_limits<TestType>::lowest());
  }

  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(dataStructure, values);
  const DataPath output = input.replaceName("Output");
  Arguments args;
  args.insertOrAssign(Log10ImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(Log10ImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(Log10ImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const Log10ImageFilter filter;
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.value().actions.size() == 1);
  const auto* actionPtr = dynamic_cast<const CreateArrayAction*>(preflightResult.outputActions.value().actions.front().get());
  REQUIRE(actionPtr != nullptr);
  // Stop the baseline before zero or a negative input can produce a nonfinite-to-integer cast.
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
  const Float64Array* logarithmsPtr = nullptr;
  REQUIRE_NOTHROW(logarithmsPtr = &dataStructure.getDataRefAs<Float64Array>(output));
  const auto& store = logarithmsPtr->getDataStoreRef();
  REQUIRE(store.getValue(0) == 0.0);
  REQUIRE(store.getValue(1) == 1.0);
  REQUIRE(store.getValue(2) == 2.0);
  REQUIRE(store.getValue(3) == Approx(0.30102999566398119521).epsilon(0).margin(1.0e-15));
  REQUIRE(std::isinf(store.getValue(4)));
  REQUIRE(std::signbit(store.getValue(4)));
  REQUIRE(store.getValue(5) == Approx(maximumIntegerLog10<TestType>()).epsilon(0).margin(1.0e-14));
  if constexpr(std::is_signed_v<TestType>)
  {
    REQUIRE(std::isnan(store.getValue(6)));
    REQUIRE(std::isnan(store.getValue(7)));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::Log10ImageFilter: floating input retains type and IEEE domain behavior", "[ImageProcessing][Issue1874][Log10ImageFilter]", float32, float64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(dataStructure, {1, 10, 100, 2, TestType{0.0}, -TestType{0.0}, -1, std::numeric_limits<TestType>::quiet_NaN(),
                                                                                   std::numeric_limits<TestType>::infinity(), -std::numeric_limits<TestType>::infinity()});
  const DataPath output = input.replaceName("Output");
  Arguments args;
  args.insertOrAssign(Log10ImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(Log10ImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(Log10ImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const Log10ImageFilter filter;
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
  REQUIRE(store.getValue(0) == 0);
  REQUIRE(store.getValue(1) == 1);
  REQUIRE(store.getValue(2) == 2);
  constexpr float64 absoluteTolerance = std::is_same_v<TestType, float32> ? 3.0e-8 : 1.0e-15;
  REQUIRE(store.getValue(3) == Approx(0.30102999566398119521).epsilon(0).margin(absoluteTolerance));
  for(const usize index : {usize{4}, usize{5}})
  {
    REQUIRE(std::isinf(store.getValue(index)));
    REQUIRE(std::signbit(store.getValue(index)));
  }
  REQUIRE(std::isnan(store.getValue(6)));
  REQUIRE(std::isnan(store.getValue(7)));
  REQUIRE(std::isinf(store.getValue(8)));
  REQUIRE_FALSE(std::signbit(store.getValue(8)));
  REQUIRE(std::isnan(store.getValue(9)));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::Log10ImageFilter: positive floating boundaries have finite ordered logarithms", "[ImageProcessing][Issue1874][Log10ImageFilter]", float32, float64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath input =
      ip_test::BuildScalarValuesImage<TestType>(dataStructure, {std::numeric_limits<TestType>::denorm_min(), std::numeric_limits<TestType>::min(), std::numeric_limits<TestType>::max(),
                                                                std::nextafter(TestType{1}, TestType{0}), TestType{1}, std::nextafter(TestType{1}, TestType{2})});
  const DataPath output = ip_test::RunPointwiseFilter<Log10ImageFilter>(dataStructure, input);
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(output));
  REQUIRE(outputArrayPtr->getDataType() == GetDataType<TestType>());
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 1, 6});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const DataArray<TestType>* logarithmsPtr = nullptr;
  REQUIRE_NOTHROW(logarithmsPtr = &dataStructure.getDataRefAs<DataArray<TestType>>(output));
  const auto& store = logarithmsPtr->getDataStoreRef();
  // The two lower limits are exact powers of two: log10(2^n) = n * log10(2).
  constexpr float64 log10Two = 0.30102999566398119521;
  constexpr float64 expectedSubnormal = (std::numeric_limits<TestType>::min_exponent - std::numeric_limits<TestType>::digits) * log10Two;
  constexpr float64 expectedNormal = (std::numeric_limits<TestType>::min_exponent - 1) * log10Two;
  constexpr float64 expectedMaximum = std::is_same_v<TestType, float32> ? 38.531839419103626 : 308.25471555991675;
  constexpr float64 limitTolerance = std::is_same_v<TestType, float32> ? 5.0e-6 : 5.0e-13;
  REQUIRE(std::isfinite(store.getValue(0)));
  REQUIRE(store.getValue(0) == Approx(expectedSubnormal).epsilon(0).margin(limitTolerance));
  REQUIRE(std::isfinite(store.getValue(1)));
  REQUIRE(store.getValue(1) == Approx(expectedNormal).epsilon(0).margin(limitTolerance));
  REQUIRE(std::isfinite(store.getValue(2)));
  REQUIRE(store.getValue(2) == Approx(expectedMaximum).epsilon(0).margin(limitTolerance));
  constexpr float64 expectedBelowOne = std::is_same_v<TestType, float32> ? -2.588596909327576e-8 : -4.8216373327664354e-17;
  constexpr float64 expectedAboveOne = std::is_same_v<TestType, float32> ? 5.177193355766362e-8 : 9.64327466553287e-17;
  constexpr float64 adjacentTolerance = std::is_same_v<TestType, float32> ? 5.0e-15 : 1.0e-30;
  REQUIRE(store.getValue(3) == Approx(expectedBelowOne).epsilon(0).margin(adjacentTolerance));
  REQUIRE(store.getValue(3) < 0);
  REQUIRE(store.getValue(4) == 0);
  REQUIRE(store.getValue(5) == Approx(expectedAboveOne).epsilon(0).margin(adjacentTolerance));
  REQUIRE(store.getValue(5) > 0);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::Log10ImageFilter: integer output preserves tuple and component shapes", "[ImageProcessing][Issue1874][Log10ImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image Geometry");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({2, 1, 1});
  const ShapeType tupleShape{1, 1, 2};
  const ShapeType componentShape{2};
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, "CellData", tupleShape, imageGeomPtr->getId());
  REQUIRE(cellDataPtr != nullptr);
  imageGeomPtr->setCellData(*cellDataPtr);
  const DataPath input({"Image Geometry", "CellData", "Input"});
  auto storePtr = DataStoreUtilities::CreateDataStore<int16>(dataStructure, input, tupleShape, componentShape);
  REQUIRE(storePtr != nullptr);
  auto* inputArrayPtr = DataArray<int16>::Create(dataStructure, "Input", storePtr, cellDataPtr->getId());
  REQUIRE(inputArrayPtr != nullptr);
  const std::vector<int16> values{1, 10, 100, 2};
  for(usize index = 0; index < values.size(); ++index)
  {
    inputArrayPtr->getDataStoreRef().setValue(index, values.at(index));
  }
  Arguments args;
  args.insertOrAssign(Log10ImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(Log10ImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(Log10ImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const Log10ImageFilter filter;
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.value().actions.size() == 1);
  const auto* actionPtr = dynamic_cast<const CreateArrayAction*>(preflightResult.outputActions.value().actions.front().get());
  REQUIRE(actionPtr != nullptr);
  REQUIRE(actionPtr->type() == DataType::float64);
  REQUIRE(actionPtr->dims() == tupleShape);
  REQUIRE(actionPtr->componentDims() == componentShape);
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const DataPath output = input.replaceName("Output");
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(output));
  REQUIRE(outputArrayPtr->getDataType() == DataType::float64);
  REQUIRE(outputArrayPtr->getTupleShape() == tupleShape);
  REQUIRE(outputArrayPtr->getComponentShape() == componentShape);
  const Float64Array* logarithmsPtr = nullptr;
  REQUIRE_NOTHROW(logarithmsPtr = &dataStructure.getDataRefAs<Float64Array>(output));
  const auto& outputStore = logarithmsPtr->getDataStoreRef();
  REQUIRE(outputStore.getValue(0) == 0.0);
  REQUIRE(outputStore.getValue(1) == 1.0);
  REQUIRE(outputStore.getValue(2) == 2.0);
  REQUIRE(outputStore.getValue(3) == Approx(0.30102999566398119521).epsilon(0).margin(1.0e-15));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Negative input produces NaN; zero produces negative infinity. Floating output represents both results.
TEST_CASE("ImageProcessing::Log10ImageFilter: log10 of negative is NaN and log10(0) is -inf (domain edge)", "[ImageProcessing][Log10ImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(ds, {-2.0f, 0.0f});
  const DataPath output = ip_test::RunPointwiseFilter<Log10ImageFilter>(ds, input);

  const Float32Array* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &ds.getDataRefAs<Float32Array>(output));
  const auto& out = outputArrayPtr->getDataStoreRef();
  REQUIRE(std::isnan(out.getValue(0))); // log10(-2) -> NaN
  REQUIRE(std::isinf(out.getValue(1))); // log10(0)  -> -inf
  REQUIRE(out.getValue(1) < 0.0f);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKLog10ImageTest.cpp(defaults): Ramp-Zero-One-Float.nrrd
// (float32) through OUR reader, run OUR filter (defaults), then compare with the committed baseline @0.01.
// log10(0) at the ramp's zero produces -inf; ip_golden::CompareImages mirrors ITK's own max-abs-diff comparator, which
// tolerates the -inf tail. See AcosImageFilterTest.cpp for the full rationale.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::Log10ImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][Log10ImageFilter]")
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
  args.insertOrAssign(Log10ImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(Log10ImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(Log10ImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const Log10ImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // DURABLE golden: baseline compare @ 0.01.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath("BasicFilters_Log10ImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
