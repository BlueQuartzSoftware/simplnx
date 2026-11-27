#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/Log10ImageFilter.hpp"

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>
#include <nonstd/span.hpp>

#include <any>
#include <array>
#include <bit>
#include <cmath>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

using namespace nx::core;

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros expand to these constructs.

TEMPLATE_TEST_CASE("NXImageProcessing::Log10ImageFilter: integer values produce float64 logarithms and domain results", "[NXImageProcessing][Log10ImageFilter]", int8, uint8, int16, uint16, int32,
                   uint32, int64, uint64)
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  const auto values = [] {
    if constexpr(std::is_signed_v<TestType>)
    {
      return std::vector<TestType>{1, 10, 100, 2, 0, std::numeric_limits<TestType>::max(), -1, std::numeric_limits<TestType>::lowest()};
    }
    else
    {
      return std::vector<TestType>{1, 10, 100, 2, 0, std::numeric_limits<TestType>::max()};
    }
  }();

  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(dataStructure, values);
  const DataPath output = ip_test::RunMappedPointwiseFilter<Log10ImageFilter>(dataStructure, input, DataType::float64, {1, 1, values.size()}, {1});
  const Float64ArrayType* logarithmsPtr = nullptr;
  REQUIRE_NOTHROW(logarithmsPtr = &dataStructure.getDataRefAs<Float64ArrayType>(output));
  REQUIRE(logarithmsPtr->getDataType() == DataType::float64);
  REQUIRE(logarithmsPtr->getTupleShape() == ShapeType{1, 1, values.size()});
  REQUIRE(logarithmsPtr->getComponentShape() == ShapeType{1});
  REQUIRE(logarithmsPtr->getStoreType() == IDataStore::StoreType::InMemory);
  const auto& store = logarithmsPtr->getDataStoreRef();
  REQUIRE(store.getValue(0) == 0.0);
  REQUIRE(store.getValue(1) == 1.0);
  REQUIRE(store.getValue(2) == 2.0);
  REQUIRE(store.getValue(3) == Approx(0.30102999566398119521).epsilon(0).margin(1.0e-15));
  REQUIRE(std::isinf(store.getValue(4)));
  REQUIRE(std::signbit(store.getValue(4)));
  // Independent values at each native integer maximum, in order of increasing width (8, 16, 32, 64 bits).
  constexpr std::array<float64, 4> signedMaxima = {2.103803720955957, 4.515436681141699, 9.331929865381182, 18.964889726830815};
  constexpr std::array<float64, 4> unsignedMaxima = {2.406540180433955, 4.81647330376525, 9.632959861146281, 19.265919722494797};
  constexpr usize widthIndex = static_cast<usize>(std::countr_zero(sizeof(TestType)));
  constexpr float64 expectedMaximum = std::is_signed_v<TestType> ? signedMaxima.at(widthIndex) : unsignedMaxima.at(widthIndex);
  REQUIRE_THAT(store.getValue(5), Catch::Matchers::WithinULP(expectedMaximum, 1));
  if constexpr(std::is_signed_v<TestType>)
  {
    REQUIRE(std::isnan(store.getValue(6)));
    REQUIRE(std::isnan(store.getValue(7)));
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("NXImageProcessing::Log10ImageFilter: floating input retains type and IEEE domain behavior", "[NXImageProcessing][Log10ImageFilter]", float32, float64)
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(dataStructure, {1, 10, 100, 2, TestType{0.0}, -TestType{0.0}, -1, std::numeric_limits<TestType>::quiet_NaN(),
                                                                                   std::numeric_limits<TestType>::infinity(), -std::numeric_limits<TestType>::infinity()});
  const DataPath output = ip_test::RunMappedPointwiseFilter<Log10ImageFilter>(dataStructure, input, GetDataType<TestType>(), {1, 1, 10}, {1});
  const DataArray<TestType>* logarithmsPtr = nullptr;
  REQUIRE_NOTHROW(logarithmsPtr = &dataStructure.getDataRefAs<DataArray<TestType>>(output));
  REQUIRE(logarithmsPtr->getDataType() == GetDataType<TestType>());
  REQUIRE(logarithmsPtr->getTupleShape() == ShapeType{1, 1, 10});
  REQUIRE(logarithmsPtr->getComponentShape() == ShapeType{1});
  REQUIRE(logarithmsPtr->getStoreType() == IDataStore::StoreType::InMemory);
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
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("NXImageProcessing::Log10ImageFilter: positive floating boundaries have finite ordered logarithms", "[NXImageProcessing][Log10ImageFilter]", float32, float64)
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath input =
      ip_test::BuildScalarValuesImage<TestType>(dataStructure, {std::numeric_limits<TestType>::denorm_min(), std::numeric_limits<TestType>::min(), std::numeric_limits<TestType>::max(),
                                                                std::nextafter(TestType{1}, TestType{0}), TestType{1}, std::nextafter(TestType{1}, TestType{2})});
  const DataPath output = ip_test::RunMappedPointwiseFilter<Log10ImageFilter>(dataStructure, input, GetDataType<TestType>(), {1, 1, 6}, {1});
  const DataArray<TestType>* logarithmsPtr = nullptr;
  REQUIRE_NOTHROW(logarithmsPtr = &dataStructure.getDataRefAs<DataArray<TestType>>(output));
  REQUIRE(logarithmsPtr->getDataType() == GetDataType<TestType>());
  REQUIRE(logarithmsPtr->getTupleShape() == ShapeType{1, 1, 6});
  REQUIRE(logarithmsPtr->getComponentShape() == ShapeType{1});
  REQUIRE(logarithmsPtr->getStoreType() == IDataStore::StoreType::InMemory);
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
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::Log10ImageFilter: promoted output preserves tuples and components", "[NXImageProcessing][Log10ImageFilter]")
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const auto input = ip_test::BuildScalarValuesImage<int16>(dataStructure, {0, 0});
  Int16ArrayType* inputArrayPtr = nullptr;
  REQUIRE_NOTHROW(inputArrayPtr = &dataStructure.getDataRefAs<Int16ArrayType>(input));
  const ShapeType tupleShape{1, 1, 2};
  const ShapeType componentShape{2};
  auto inputStore = data_store_utilities::CreateDataStore<int16>(dataStructure, input, tupleShape, componentShape);
  REQUIRE(inputStore != nullptr);
  const auto replaceResult = inputArrayPtr->setDataStore(inputStore);
  SIMPLNX_RESULT_REQUIRE_VALID(replaceResult);
  constexpr std::array<int16, 4> values = {1, 10, 100, 2};
  const auto writeResult = inputStore->copyFromBuffer(0, nonstd::span<const int16>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);

  const auto output = ip_test::RunMappedPointwiseFilter<Log10ImageFilter>(dataStructure, input, DataType::float64, tupleShape, componentShape);
  const Float64ArrayType* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<Float64ArrayType>(output));
  REQUIRE(outputArrayPtr->getTupleShape() == tupleShape);
  REQUIRE(outputArrayPtr->getComponentShape() == componentShape);
  const auto& store = outputArrayPtr->getDataStoreRef();
  REQUIRE(store.getValue(0) == 0.0);
  REQUIRE(store.getValue(1) == 1.0);
  REQUIRE(store.getValue(2) == 2.0);
  REQUIRE(store.getValue(3) == Approx(0.30102999566398119521).epsilon(1.0e-15));
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// Negative inputs yield NaN and zero yields negative infinity.
TEST_CASE("NXImageProcessing::Log10ImageFilter: log10 of negative is NaN and log10(0) is -inf (domain edge)", "[NXImageProcessing][Log10ImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(dataStructure, {-2.0f, 0.0f});
  const DataPath output = ip_test::RunPointwiseFilter<Log10ImageFilter>(dataStructure, input);

  const auto& out = dataStructure.getDataRefAs<DataArray<float32>>(output).getDataStoreRef();
  REQUIRE(std::isnan(out.getValue(0))); // log10(-2) -> NaN
  REQUIRE(std::isinf(out.getValue(1))); // log10(0)  -> -inf
  REQUIRE(out.getValue(1) < 0.0f);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKLog10ImageTest.cpp(defaults): Ramp-Zero-One-Float.nrrd
// (float32) through OUR reader, run OUR filter (defaults), then compare with the committed baseline @0.01.
// log10(0) at the ramp's zero produces -inf; ip_golden::CompareImages mirrors ITK's own max-abs-diff comparator, which
// tolerates the -inf tail. See AcosImageFilterTest.cpp for the full rationale.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::Log10ImageFilter: ITK real-image golden (defaults)", "[NXImageProcessing][ItkGolden][Log10ImageFilter]")
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
  args.insertOrAssign(Log10ImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(Log10ImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(Log10ImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  Log10ImageFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // DURABLE golden: baseline compare @ 0.01.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(dataStructure, ip_golden::BaselinePath("BasicFilters_Log10ImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(dataStructure, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
