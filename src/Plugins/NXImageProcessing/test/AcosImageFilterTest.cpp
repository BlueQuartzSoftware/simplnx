#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/AcosImageFilter.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
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

// An angle at zero is pi/2 radians, independently of the input's integer width or signedness.
TEMPLATE_TEST_CASE("NXImageProcessing::AcosImageFilter: integer zero produces float32 radians", "[NXImageProcessing][Issue1874][AcosImageFilter]", int8, uint8, int16, uint16, int32, uint32, int64,
                   uint64)
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(dataStructure, {0});
  const DataPath output = ip_test::RunMappedPointwiseFilter<AcosImageFilter>(dataStructure, input, DataType::float32, {1, 1, 1}, {1});
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(output));
  REQUIRE(outputArrayPtr->getDataType() == DataType::float32);
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 1, 1});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const Float32ArrayType* anglesPtr = nullptr;
  REQUIRE_NOTHROW(anglesPtr = &dataStructure.getDataRefAs<Float32ArrayType>(output));
  REQUIRE(anglesPtr->getDataStoreRef().getValue(0) == Approx(1.5707963267948966).epsilon(0).margin(1.0e-7));

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("NXImageProcessing::AcosImageFilter: floating input retains type and propagates domain NaN", "[NXImageProcessing][Issue1874][AcosImageFilter]", float32, float64)
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(
      dataStructure, {-1, 0, 1, 2, -2, std::numeric_limits<TestType>::quiet_NaN(), std::numeric_limits<TestType>::infinity(), -std::numeric_limits<TestType>::infinity()});
  const DataPath output = ip_test::RunPointwiseFilter<AcosImageFilter>(dataStructure, input);
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(output));
  REQUIRE(outputArrayPtr->getDataType() == GetDataType<TestType>());
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 1, 8});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const DataArray<TestType>* anglesPtr = nullptr;
  REQUIRE_NOTHROW(anglesPtr = &dataStructure.getDataRefAs<DataArray<TestType>>(output));
  const auto& store = anglesPtr->getDataStoreRef();
  constexpr float64 angleTolerance = std::is_same_v<TestType, float32> ? 1.0e-7 : 1.0e-15;
  REQUIRE(store.getValue(0) == Approx(3.14159265358979323846).epsilon(0).margin(angleTolerance));
  REQUIRE(store.getValue(1) == Approx(1.57079632679489661923).epsilon(0).margin(angleTolerance));
  REQUIRE(store.getValue(2) == 0);
  for(usize index = 3; index < 8; ++index)
  {
    REQUIRE(std::isnan(store.getValue(index)));
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("NXImageProcessing::AcosImageFilter: integer endpoints and invalid values produce floating angles", "[NXImageProcessing][Issue1874][AcosImageFilter]", int8, uint8, int16, uint16,
                   int32, uint32, int64, uint64)
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  const auto values = [] {
    if constexpr(std::is_signed_v<TestType>)
    {
      return std::vector<TestType>{0, 1, 2, std::numeric_limits<TestType>::max(), -1, std::numeric_limits<TestType>::lowest()};
    }
    else
    {
      return std::vector<TestType>{0, 1, 2, std::numeric_limits<TestType>::max()};
    }
  }();

  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(dataStructure, values);
  const DataPath output = ip_test::RunMappedPointwiseFilter<AcosImageFilter>(dataStructure, input, DataType::float32, {1, 1, values.size()}, {1});
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(output));
  REQUIRE(outputArrayPtr->getDataType() == DataType::float32);
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 1, values.size()});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const Float32ArrayType* anglesPtr = nullptr;
  REQUIRE_NOTHROW(anglesPtr = &dataStructure.getDataRefAs<Float32ArrayType>(output));
  const auto& store = anglesPtr->getDataStoreRef();
  REQUIRE(store.getValue(0) == Approx(1.57079632679489661923).epsilon(0).margin(1.0e-7));
  REQUIRE(store.getValue(1) == 0.0f);
  REQUIRE(std::isnan(store.getValue(2)));
  REQUIRE(std::isnan(store.getValue(3)));
  if constexpr(std::is_signed_v<TestType>)
  {
    REQUIRE(store.getValue(4) == Approx(3.14159265358979323846).epsilon(0).margin(1.0e-7));
    REQUIRE(std::isnan(store.getValue(5)));
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("NXImageProcessing::AcosImageFilter: adjacent domain boundaries remain distinct", "[NXImageProcessing][Issue1874][AcosImageFilter]", float32, float64)
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  const TestType inside = std::nextafter(TestType{1}, TestType{0});
  const TestType outside = std::nextafter(TestType{1}, std::numeric_limits<TestType>::infinity());
  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(dataStructure, {inside, -inside, outside, -outside});
  const DataPath output = ip_test::RunPointwiseFilter<AcosImageFilter>(dataStructure, input);
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(output));
  REQUIRE(outputArrayPtr->getDataType() == GetDataType<TestType>());
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 1, 4});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const DataArray<TestType>* anglesPtr = nullptr;
  REQUIRE_NOTHROW(anglesPtr = &dataStructure.getDataRefAs<DataArray<TestType>>(output));
  const auto& store = anglesPtr->getDataStoreRef();
  // The two interior values must give finite, non-endpoint angles; exterior values must not be clamped.
  REQUIRE(store.getValue(0) > 0);
  REQUIRE(store.getValue(0) < 1.0e-3);
  REQUIRE(store.getValue(1) > 3.14);
  REQUIRE(store.getValue(1) < 3.14159265358979323846);
  REQUIRE(std::isnan(store.getValue(2)));
  REQUIRE(std::isnan(store.getValue(3)));
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::AcosImageFilter: mapped multi-component output preserves tuple shape", "[NXImageProcessing][Issue1874][AcosImageFilter]")
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath input = ip_test::BuildRampImage<int16>(dataStructure, 2, 0, 0);
  Int16ArrayType* inputArrayPtr = nullptr;
  REQUIRE_NOTHROW(inputArrayPtr = &dataStructure.getDataRefAs<Int16ArrayType>(input));
  auto store = data_store_utilities::CreateDataStore<int16>(dataStructure, input, ShapeType{2, 2, 2}, ShapeType{2});
  REQUIRE(store != nullptr);
  const auto replaceResult = inputArrayPtr->setDataStore(store);
  SIMPLNX_RESULT_REQUIRE_VALID(replaceResult);
  for(usize index = 0; index < 16; ++index)
  {
    store->setValue(index, static_cast<int16>(static_cast<int32>(index % 4) - 1));
  }
  const DataPath output = ip_test::RunMappedPointwiseFilter<AcosImageFilter>(dataStructure, input, DataType::float32, {2, 2, 2}, {2});
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(output));
  REQUIRE(outputArrayPtr->getDataType() == DataType::float32);
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{2, 2, 2});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{2});
  const Float32ArrayType* anglesPtr = nullptr;
  REQUIRE_NOTHROW(anglesPtr = &dataStructure.getDataRefAs<Float32ArrayType>(output));
  const auto& result = anglesPtr->getDataStoreRef();
  for(usize index = 0; index < 16; index += 4)
  {
    REQUIRE(result.getValue(index) == Approx(3.14159265358979323846).epsilon(0).margin(1.0e-7));
    REQUIRE(result.getValue(index + 1) == Approx(1.57079632679489661923).epsilon(0).margin(1.0e-7));
    REQUIRE(result.getValue(index + 2) == 0.0f);
    REQUIRE(std::isnan(result.getValue(index + 3)));
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

// PART A domain edge (documents ITK-faithful behavior; does NOT "fix" it): acos is undefined for |x|>1. The op is
// static_cast<U>(std::acos(static_cast<double>(x))) -- exactly ITK's equally-unguarded Acos functor -- so acos(2)
// is IEEE NaN. This pins that observed output.
TEST_CASE("NXImageProcessing::AcosImageFilter: acos of |x|>1 is NaN (domain edge)", "[NXImageProcessing][AcosImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(dataStructure, {2.0f});
  const DataPath output = ip_test::RunPointwiseFilter<AcosImageFilter>(dataStructure, input);

  const Float32ArrayType* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<Float32ArrayType>(output));
  const auto& out = outputArrayPtr->getDataStoreRef();
  REQUIRE(std::isnan(out.getValue(0)));

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKAcosImageTest.cpp(defaults): read
// Ramp-Zero-One-Float.nrrd (float32) through OUR ITK-free reader, run OUR AcosImageFilter with the ITK case's
// params (defaults), then compare our output to ITK's committed baseline .nrrd (read through the SAME reader) at
// the ITK test's tolerance 0.01 via ip_golden::CompareImages. CompareImages dispatches on the runtime type and --
// unlike CompareDataArrays -- tolerates the non-finite tails these functors can emit.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::AcosImageFilter: ITK real-image golden (defaults)", "[NXImageProcessing][ItkGolden][AcosImageFilter]")
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
  args.insertOrAssign(AcosImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(AcosImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(AcosImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  AcosImageFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // DURABLE golden: baseline compare @ 0.01 (SameAsInput output owned by 'geom').
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(dataStructure, ip_golden::BaselinePath("BasicFilters_AcosImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(dataStructure, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
