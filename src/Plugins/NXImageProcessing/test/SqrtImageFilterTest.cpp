#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/SqrtImageFilter.hpp"

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
TEMPLATE_TEST_CASE("NXImageProcessing::SqrtImageFilter: integer values produce float64 square roots", "[NXImageProcessing][SqrtImageFilter]", int8, uint8, int16, uint16, int32, uint32, int64, uint64)
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  const auto values = [] {
    if constexpr(std::is_signed_v<TestType>)
    {
      return std::vector<TestType>{0, 2, 4, std::numeric_limits<TestType>::max(), -1, std::numeric_limits<TestType>::lowest()};
    }
    else
    {
      return std::vector<TestType>{0, 2, 4, std::numeric_limits<TestType>::max()};
    }
  }();
  const ShapeType tupleShape = {1, 1, values.size()};
  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(dataStructure, values);
  const DataPath output = ip_test::RunMappedPointwiseFilter<SqrtImageFilter>(dataStructure, input, DataType::float64, tupleShape, {1});
  const Float64ArrayType* rootsPtr = nullptr;
  REQUIRE_NOTHROW(rootsPtr = &dataStructure.getDataRefAs<Float64ArrayType>(output));
  REQUIRE(rootsPtr->getDataType() == DataType::float64);
  REQUIRE(rootsPtr->getTupleShape() == tupleShape);
  REQUIRE(rootsPtr->getComponentShape() == ShapeType{1});
  REQUIRE(rootsPtr->getStoreType() == IDataStore::StoreType::InMemory);
  const auto& store = rootsPtr->getDataStoreRef();
  REQUIRE(store.getValue(0) == 0.0);
  REQUIRE(store.getValue(1) == Approx(1.4142135623730950488).epsilon(0).margin(3.0e-16));
  REQUIRE(store.getValue(2) == 2.0);
  // Independent values at each native integer maximum, in order of increasing width (8, 16, 32, 64 bits).
  constexpr std::array<float64, 4> signedMaxima = {11.269427669584644, 181.0165738268184132, 46340.9500010519849, 3037000499.9760496923};
  constexpr std::array<float64, 4> unsignedMaxima = {15.968719422671311, 255.998046867549362, 65535.9999923706055, 4294967296.0};
  constexpr usize widthIndex = static_cast<usize>(std::countr_zero(sizeof(TestType)));
  constexpr float64 expectedMaximum = std::is_signed_v<TestType> ? signedMaxima.at(widthIndex) : unsignedMaxima.at(widthIndex);
  REQUIRE_THAT(store.getValue(3), Catch::Matchers::WithinULP(expectedMaximum, 1));
  if constexpr(std::is_signed_v<TestType>)
  {
    REQUIRE(std::isnan(store.getValue(4)));
    REQUIRE(std::isnan(store.getValue(5)));
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("NXImageProcessing::SqrtImageFilter: floating input retains type and IEEE domain results", "[NXImageProcessing][SqrtImageFilter]", float32, float64)
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(
      dataStructure, {TestType{0}, -TestType{0}, 2, 4, -1, std::numeric_limits<TestType>::quiet_NaN(), std::numeric_limits<TestType>::infinity(), -std::numeric_limits<TestType>::infinity(),
                      std::numeric_limits<TestType>::max(), std::numeric_limits<TestType>::lowest(), std::numeric_limits<TestType>::min(), -std::numeric_limits<TestType>::min()});
  const DataPath output = ip_test::RunMappedPointwiseFilter<SqrtImageFilter>(dataStructure, input, GetDataType<TestType>(), {1, 1, 12}, {1});
  const DataArray<TestType>* rootsPtr = nullptr;
  REQUIRE_NOTHROW(rootsPtr = &dataStructure.getDataRefAs<DataArray<TestType>>(output));
  REQUIRE(rootsPtr->getDataType() == GetDataType<TestType>());
  REQUIRE(rootsPtr->getTupleShape() == ShapeType{1, 1, 12});
  REQUIRE(rootsPtr->getComponentShape() == ShapeType{1});
  REQUIRE(rootsPtr->getStoreType() == IDataStore::StoreType::InMemory);
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
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("NXImageProcessing::SqrtImageFilter: floating boundary roots match independent binary values", "[NXImageProcessing][SqrtImageFilter]", float32, float64)
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(DataStorageMode::ForceInCore, 0);
  constexpr TestType smallestNormal = std::numeric_limits<TestType>::min();
  constexpr TestType infinity = std::numeric_limits<TestType>::infinity();
  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(
      dataStructure, {std::numeric_limits<TestType>::denorm_min(), -std::numeric_limits<TestType>::denorm_min(), smallestNormal, std::nextafter(smallestNormal, TestType{0}),
                      std::nextafter(smallestNormal, infinity), TestType{1}, std::nextafter(TestType{1}, TestType{0}), std::nextafter(TestType{1}, infinity), std::numeric_limits<TestType>::max()});
  const DataPath output = ip_test::RunMappedPointwiseFilter<SqrtImageFilter>(dataStructure, input, GetDataType<TestType>(), {1, 1, 9}, {1});
  const DataArray<TestType>* rootsPtr = nullptr;
  REQUIRE_NOTHROW(rootsPtr = &dataStructure.getDataRefAs<DataArray<TestType>>(output));
  REQUIRE(rootsPtr->getDataType() == GetDataType<TestType>());
  REQUIRE(rootsPtr->getTupleShape() == ShapeType{1, 1, 9});
  REQUIRE(rootsPtr->getComponentShape() == ShapeType{1});
  REQUIRE(rootsPtr->getStoreType() == IDataStore::StoreType::InMemory);
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
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::SqrtImageFilter: promoted output preserves tuples and components", "[NXImageProcessing][SqrtImageFilter]")
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
  constexpr std::array<int16, 4> values = {0, 2, 4, -1};
  const auto writeResult = inputStore->copyFromBuffer(0, nonstd::span<const int16>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);

  const auto output = ip_test::RunMappedPointwiseFilter<SqrtImageFilter>(dataStructure, input, DataType::float64, tupleShape, componentShape);
  const Float64ArrayType* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<Float64ArrayType>(output));
  REQUIRE(outputArrayPtr->getTupleShape() == tupleShape);
  REQUIRE(outputArrayPtr->getComponentShape() == componentShape);
  const auto& store = outputArrayPtr->getDataStoreRef();
  REQUIRE(store.getValue(0) == 0.0);
  REQUIRE(store.getValue(1) == Approx(1.4142135623730950488).epsilon(1.0e-15));
  REQUIRE(store.getValue(2) == 2.0);
  REQUIRE(std::isnan(store.getValue(3)));
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)

// Negative inputs yield NaN.
TEST_CASE("NXImageProcessing::SqrtImageFilter: sqrt of a negative is NaN (domain edge)", "[NXImageProcessing][SqrtImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(dataStructure, {-4.0f});
  const DataPath output = ip_test::RunPointwiseFilter<SqrtImageFilter>(dataStructure, input);

  const auto& out = dataStructure.getDataRefAs<DataArray<float32>>(output).getDataStoreRef();
  REQUIRE(std::isnan(out.getValue(0)));
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKSqrtImageTest.cpp(defaults): Ramp-Zero-One-Float.nrrd
// (float32) through OUR reader, run OUR filter (defaults), then compare with the committed baseline @0.01.
// See AcosImageFilterTest.cpp for the full rationale.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::SqrtImageFilter: ITK real-image golden (defaults)", "[NXImageProcessing][ItkGolden][SqrtImageFilter]")
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
  args.insertOrAssign(SqrtImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(SqrtImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(SqrtImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  SqrtImageFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // DURABLE golden: baseline compare @ 0.01.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(dataStructure, ip_golden::BaselinePath("BasicFilters_SqrtImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(dataStructure, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
