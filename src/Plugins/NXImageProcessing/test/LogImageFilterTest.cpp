#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/LogImageFilter.hpp"

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
#include <numbers>
#include <string>
#include <type_traits>
#include <vector>

using namespace nx::core;

// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros expand to these constructs.
TEMPLATE_TEST_CASE("NXImageProcessing::LogImageFilter: integer values produce float64 natural logarithms", "[NXImageProcessing][LogImageFilter]", int8, uint8, int16, uint16, int32, uint32, int64,
                   uint64)
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  constexpr int32 highestPower = std::numeric_limits<TestType>::digits - 1;
  constexpr TestType highestPowerOfTwo = static_cast<TestType>(TestType{1} << highestPower);
  const auto values = [] {
    if constexpr(std::is_signed_v<TestType>)
    {
      return std::vector<TestType>{1, 2, 0, std::numeric_limits<TestType>::max(), highestPowerOfTwo, -1, std::numeric_limits<TestType>::lowest()};
    }
    else
    {
      return std::vector<TestType>{1, 2, 0, std::numeric_limits<TestType>::max(), highestPowerOfTwo};
    }
  }();
  const ShapeType tupleShape = {1, 1, values.size()};
  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(dataStructure, values);
  const DataPath output = ip_test::RunMappedPointwiseFilter<LogImageFilter>(dataStructure, input, DataType::float64, tupleShape, {1});
  const Float64ArrayType* logarithmsPtr = nullptr;
  REQUIRE_NOTHROW(logarithmsPtr = &dataStructure.getDataRefAs<Float64ArrayType>(output));
  REQUIRE(logarithmsPtr->getDataType() == DataType::float64);
  REQUIRE(logarithmsPtr->getTupleShape() == tupleShape);
  REQUIRE(logarithmsPtr->getComponentShape() == ShapeType{1});
  REQUIRE(logarithmsPtr->getStoreType() == IDataStore::StoreType::InMemory);
  const auto& store = logarithmsPtr->getDataStoreRef();
  REQUIRE(store.getValue(0) == 0.0);
  REQUIRE(store.getValue(1) == Approx(0.69314718055994530942).epsilon(0).margin(1.0e-15));
  REQUIRE(std::isinf(store.getValue(2)));
  REQUIRE(std::signbit(store.getValue(2)));
  // Independent values at each native integer maximum, in order of increasing width (8, 16, 32, 64 bits).
  constexpr std::array<float64, 4> signedMaxima = {4.844187086458591, 10.397177190355384, 21.487562596892644, 43.66827237527655};
  constexpr std::array<float64, 4> unsignedMaxima = {5.541263545158426, 11.090339630053647, 22.18070977768542, 44.3614195558365};
  constexpr usize widthIndex = static_cast<usize>(std::countr_zero(sizeof(TestType)));
  constexpr float64 expectedMaximum = std::is_signed_v<TestType> ? signedMaxima.at(widthIndex) : unsignedMaxima.at(widthIndex);
  REQUIRE_THAT(store.getValue(3), Catch::Matchers::WithinULP(expectedMaximum, 1));
  constexpr float64 logPowerOfTwo = highestPower * std::numbers::ln2;
  REQUIRE(store.getValue(4) == Approx(logPowerOfTwo).epsilon(0).margin(1.0e-14));
  if constexpr(std::is_signed_v<TestType>)
  {
    REQUIRE(std::isnan(store.getValue(5)));
    REQUIRE(std::isnan(store.getValue(6)));
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("NXImageProcessing::LogImageFilter: floating input retains type and IEEE domain results", "[NXImageProcessing][LogImageFilter]", float32, float64)
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath input =
      ip_test::BuildScalarValuesImage<TestType>(dataStructure, {1, 2, TestType{0}, -TestType{0}, -1, std::numeric_limits<TestType>::quiet_NaN(), std::numeric_limits<TestType>::infinity(),
                                                                -std::numeric_limits<TestType>::infinity(), std::numeric_limits<TestType>::max(), std::numeric_limits<TestType>::lowest()});
  const DataPath output = ip_test::RunMappedPointwiseFilter<LogImageFilter>(dataStructure, input, GetDataType<TestType>(), {1, 1, 10}, {1});
  const DataArray<TestType>* logarithmsPtr = nullptr;
  REQUIRE_NOTHROW(logarithmsPtr = &dataStructure.getDataRefAs<DataArray<TestType>>(output));
  REQUIRE(logarithmsPtr->getDataType() == GetDataType<TestType>());
  REQUIRE(logarithmsPtr->getTupleShape() == ShapeType{1, 1, 10});
  REQUIRE(logarithmsPtr->getComponentShape() == ShapeType{1});
  REQUIRE(logarithmsPtr->getStoreType() == IDataStore::StoreType::InMemory);
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
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("NXImageProcessing::LogImageFilter: positive floating boundaries retain finite logarithms", "[NXImageProcessing][LogImageFilter]", float32, float64)
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(dataStructure, {std::numeric_limits<TestType>::min(), std::numeric_limits<TestType>::denorm_min(),
                                                                                   std::nextafter(TestType{1}, TestType{0}), std::nextafter(TestType{1}, std::numeric_limits<TestType>::infinity())});
  const DataPath output = ip_test::RunMappedPointwiseFilter<LogImageFilter>(dataStructure, input, GetDataType<TestType>(), {1, 1, 4}, {1});
  const DataArray<TestType>* logarithmsPtr = nullptr;
  REQUIRE_NOTHROW(logarithmsPtr = &dataStructure.getDataRefAs<DataArray<TestType>>(output));
  REQUIRE(logarithmsPtr->getDataType() == GetDataType<TestType>());
  REQUIRE(logarithmsPtr->getTupleShape() == ShapeType{1, 1, 4});
  REQUIRE(logarithmsPtr->getComponentShape() == ShapeType{1});
  REQUIRE(logarithmsPtr->getStoreType() == IDataStore::StoreType::InMemory);
  const auto& store = logarithmsPtr->getDataStoreRef();
  // IEEE binary types have minimum normal 2^(min_exponent-1) and minimum subnormal 2^(min_exponent-digits).
  constexpr float64 logTwo = std::numbers::ln2;
  constexpr float64 normalLogarithm = (std::numeric_limits<TestType>::min_exponent - 1) * logTwo;
  constexpr float64 subnormalLogarithm = (std::numeric_limits<TestType>::min_exponent - std::numeric_limits<TestType>::digits) * logTwo;
  constexpr float64 logarithmTolerance = std::is_same_v<TestType, float32> ? 1.0e-5 : 1.0e-12;
  REQUIRE(store.getValue(0) == Approx(normalLogarithm).epsilon(0).margin(logarithmTolerance));
  REQUIRE(store.getValue(1) == Approx(subnormalLogarithm).epsilon(0).margin(logarithmTolerance));
  // log(1 +/- epsilon) is within one epsilon squared of its first-order approximation at these spacings.
  constexpr float64 epsilon = std::numeric_limits<TestType>::epsilon();
  REQUIRE(store.getValue(2) == Approx(-epsilon / 2).epsilon(0).margin(epsilon * epsilon));
  REQUIRE(store.getValue(3) == Approx(epsilon).epsilon(0).margin(epsilon * epsilon));
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::LogImageFilter: promoted output preserves tuples and components", "[NXImageProcessing][LogImageFilter]")
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
  constexpr std::array<int16, 4> values = {1, 2, 0, -1};
  const auto writeResult = inputStore->copyFromBuffer(0, nonstd::span<const int16>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);

  const auto output = ip_test::RunMappedPointwiseFilter<LogImageFilter>(dataStructure, input, DataType::float64, tupleShape, componentShape);
  const Float64ArrayType* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<Float64ArrayType>(output));
  REQUIRE(outputArrayPtr->getTupleShape() == tupleShape);
  REQUIRE(outputArrayPtr->getComponentShape() == componentShape);
  const auto& store = outputArrayPtr->getDataStoreRef();
  REQUIRE(store.getValue(0) == 0.0);
  REQUIRE(store.getValue(1) == Approx(0.69314718055994530942).epsilon(1.0e-15));
  REQUIRE(store.getValue(2) == -std::numeric_limits<float64>::infinity());
  REQUIRE(std::isnan(store.getValue(3)));
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// Negative inputs yield NaN and zero yields negative infinity.
TEST_CASE("NXImageProcessing::LogImageFilter: log of negative is NaN and log(0) is -inf (domain edge)", "[NXImageProcessing][LogImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(dataStructure, {-2.0f, 0.0f});
  const DataPath output = ip_test::RunPointwiseFilter<LogImageFilter>(dataStructure, input);

  const auto& out = dataStructure.getDataRefAs<DataArray<float32>>(output).getDataStoreRef();
  REQUIRE(std::isnan(out.getValue(0))); // log(-2) -> NaN
  REQUIRE(std::isinf(out.getValue(1))); // log(0)  -> -inf
  REQUIRE(out.getValue(1) < 0.0f);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKLogImageTest.cpp(defaults): Ramp-Zero-One-Float.nrrd
// (float32) through OUR reader, run OUR filter (defaults), then compare with the committed baseline @0.01.
// log(0) at the ramp's zero produces -inf; ip_golden::CompareImages mirrors ITK's own max-abs-diff comparator, which
// tolerates the -inf tail. See AcosImageFilterTest.cpp for the full rationale.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::LogImageFilter: ITK real-image golden (defaults)", "[NXImageProcessing][ItkGolden][LogImageFilter]")
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
  args.insertOrAssign(LogImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(LogImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(LogImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  LogImageFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // DURABLE golden: baseline compare @ 0.01.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(dataStructure, ip_golden::BaselinePath("BasicFilters_LogImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(dataStructure, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
