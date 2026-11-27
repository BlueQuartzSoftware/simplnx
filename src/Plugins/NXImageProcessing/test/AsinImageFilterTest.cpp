#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/AsinImageFilter.hpp"

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
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

// The principal angle at integer one is pi/2 radians for every integer width and signedness.
TEMPLATE_TEST_CASE("NXImageProcessing::AsinImageFilter: integer one produces float32 radians", "[NXImageProcessing][Issue1874][AsinImageFilter]", int8, uint8, int16, uint16, int32, uint32, int64,
                   uint64)
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath inputPath = ip_test::BuildScalarValuesImage<TestType>(dataStructure, {1});
  const DataPath outputPath = ip_test::RunMappedPointwiseFilter<AsinImageFilter>(dataStructure, inputPath, DataType::float32, {1, 1, 1}, {1});
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(outputPath));
  REQUIRE(outputArrayPtr->getDataType() == DataType::float32);
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 1, 1});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const Float32ArrayType* anglesPtr = nullptr;
  REQUIRE_NOTHROW(anglesPtr = &dataStructure.getDataRefAs<Float32ArrayType>(outputPath));
  REQUIRE(anglesPtr->getDataStoreRef().getValue(0) == Approx(1.57079632679489661923).epsilon(0).margin(1.0e-7));
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("NXImageProcessing::AsinImageFilter: integer endpoints and invalid values produce floating angles", "[NXImageProcessing][Issue1874][AsinImageFilter]", int8, uint8, int16, uint16,
                   int32, uint32, int64, uint64)
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  const auto inputValues = [] {
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
  const DataPath inputPath = ip_test::BuildScalarValuesImage<TestType>(dataStructure, inputValues);
  const DataPath outputPath = ip_test::RunMappedPointwiseFilter<AsinImageFilter>(dataStructure, inputPath, DataType::float32, {1, 1, inputValues.size()}, {1});
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(outputPath));
  REQUIRE(outputArrayPtr->getDataType() == DataType::float32);
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 1, inputValues.size()});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const Float32ArrayType* anglesPtr = nullptr;
  REQUIRE_NOTHROW(anglesPtr = &dataStructure.getDataRefAs<Float32ArrayType>(outputPath));
  const auto& angleStore = anglesPtr->getDataStoreRef();
  REQUIRE(angleStore.getValue(0) == 0.0f);
  REQUIRE(angleStore.getValue(1) == Approx(1.57079632679489661923).epsilon(0).margin(1.0e-7));
  REQUIRE(std::isnan(angleStore.getValue(2)));
  REQUIRE(std::isnan(angleStore.getValue(3)));
  if constexpr(std::is_signed_v<TestType>)
  {
    REQUIRE(angleStore.getValue(4) == Approx(-1.57079632679489661923).epsilon(0).margin(1.0e-7));
    REQUIRE(std::isnan(angleStore.getValue(5)));
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("NXImageProcessing::AsinImageFilter: floating input retains type and principal angles", "[NXImageProcessing][Issue1874][AsinImageFilter]", float32, float64)
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath inputPath = ip_test::BuildScalarValuesImage<TestType>(dataStructure, {-1, -0.5, -TestType{0}, TestType{0}, 0.5, 1, 2, -2, std::numeric_limits<TestType>::quiet_NaN(),
                                                                                       std::numeric_limits<TestType>::infinity(), -std::numeric_limits<TestType>::infinity()});
  const DataPath outputPath = ip_test::RunPointwiseFilter<AsinImageFilter>(dataStructure, inputPath);
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(outputPath));
  REQUIRE(outputArrayPtr->getDataType() == GetDataType<TestType>());
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 1, 11});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const DataArray<TestType>* anglesPtr = nullptr;
  REQUIRE_NOTHROW(anglesPtr = &dataStructure.getDataRefAs<DataArray<TestType>>(outputPath));
  const auto& angleStore = anglesPtr->getDataStoreRef();
  constexpr float64 angleTolerance = std::is_same_v<TestType, float32> ? 1.0e-7 : 1.0e-15;
  REQUIRE(angleStore.getValue(0) == Approx(-1.57079632679489661923).epsilon(0).margin(angleTolerance));
  REQUIRE(angleStore.getValue(1) == Approx(-0.52359877559829887308).epsilon(0).margin(angleTolerance));
  REQUIRE(angleStore.getValue(2) == 0);
  REQUIRE(std::signbit(angleStore.getValue(2)));
  REQUIRE(angleStore.getValue(3) == 0);
  REQUIRE_FALSE(std::signbit(angleStore.getValue(3)));
  REQUIRE(angleStore.getValue(4) == Approx(0.52359877559829887308).epsilon(0).margin(angleTolerance));
  REQUIRE(angleStore.getValue(5) == Approx(1.57079632679489661923).epsilon(0).margin(angleTolerance));
  for(usize index = 6; index < 11; ++index)
  {
    REQUIRE(std::isnan(angleStore.getValue(index)));
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("NXImageProcessing::AsinImageFilter: adjacent domain boundaries remain distinct", "[NXImageProcessing][Issue1874][AsinImageFilter]", float32, float64)
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  const TestType insideValue = std::nextafter(TestType{1}, TestType{0});
  const TestType outsideValue = std::nextafter(TestType{1}, std::numeric_limits<TestType>::infinity());
  DataStructure dataStructure;
  const DataPath inputPath = ip_test::BuildScalarValuesImage<TestType>(dataStructure, {insideValue, -insideValue, outsideValue, -outsideValue});
  const DataPath outputPath = ip_test::RunPointwiseFilter<AsinImageFilter>(dataStructure, inputPath);
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(outputPath));
  REQUIRE(outputArrayPtr->getDataType() == GetDataType<TestType>());
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 1, 4});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const DataArray<TestType>* anglesPtr = nullptr;
  REQUIRE_NOTHROW(anglesPtr = &dataStructure.getDataRefAs<DataArray<TestType>>(outputPath));
  const auto& angleStore = anglesPtr->getDataStoreRef();
  REQUIRE(angleStore.getValue(0) > 1.57);
  REQUIRE(angleStore.getValue(0) < 1.57079632679489661923);
  REQUIRE(angleStore.getValue(1) < -1.57);
  REQUIRE(angleStore.getValue(1) > -1.57079632679489661923);
  REQUIRE(std::isnan(angleStore.getValue(2)));
  REQUIRE(std::isnan(angleStore.getValue(3)));
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::AsinImageFilter: mapped multi-component output preserves tuple shape", "[NXImageProcessing][Issue1874][AsinImageFilter]")
{
  Application::GetOrCreateInstance();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath inputPath = ip_test::BuildRampImage<int16>(dataStructure, 2, 0, 0);
  Int16ArrayType* inputArrayPtr = nullptr;
  REQUIRE_NOTHROW(inputArrayPtr = &dataStructure.getDataRefAs<Int16ArrayType>(inputPath));
  auto inputStore = data_store_utilities::CreateDataStore<int16>(dataStructure, inputPath, ShapeType{2, 2, 2}, ShapeType{2});
  REQUIRE(inputStore != nullptr);
  const auto replaceResult = inputArrayPtr->setDataStore(inputStore);
  SIMPLNX_RESULT_REQUIRE_VALID(replaceResult);
  constexpr std::array<int16, 4> inputValues = {-1, 0, 1, 2};
  for(usize index = 0; index < 16; ++index)
  {
    inputStore->setValue(index, inputValues.at(index % inputValues.size()));
  }
  const DataPath outputPath = ip_test::RunMappedPointwiseFilter<AsinImageFilter>(dataStructure, inputPath, DataType::float32, {2, 2, 2}, {2});
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(outputPath));
  REQUIRE(outputArrayPtr->getDataType() == DataType::float32);
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{2, 2, 2});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{2});
  const Float32ArrayType* anglesPtr = nullptr;
  REQUIRE_NOTHROW(anglesPtr = &dataStructure.getDataRefAs<Float32ArrayType>(outputPath));
  const auto& angleStore = anglesPtr->getDataStoreRef();
  for(usize index = 0; index < 16; index += 4)
  {
    REQUIRE(angleStore.getValue(index) == Approx(-1.57079632679489661923).epsilon(0).margin(1.0e-7));
    REQUIRE(angleStore.getValue(index + 1) == 0.0f);
    REQUIRE(angleStore.getValue(index + 2) == Approx(1.57079632679489661923).epsilon(0).margin(1.0e-7));
    REQUIRE(std::isnan(angleStore.getValue(index + 3)));
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

// PART A domain edge (documents ITK-faithful behavior): asin is undefined for |x|>1. The op is
// static_cast<U>(std::asin(static_cast<double>(x))) -- ITK's equally-unguarded Asin functor -- so asin(2) is NaN.
TEST_CASE("NXImageProcessing::AsinImageFilter: asin of |x|>1 is NaN (domain edge)", "[NXImageProcessing][AsinImageFilter]")
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(dataStructure, {2.0f});
  const DataPath output = ip_test::RunPointwiseFilter<AsinImageFilter>(dataStructure, input);

  const Float32ArrayType* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<Float32ArrayType>(output));
  REQUIRE(std::isnan(outputArrayPtr->getDataStoreRef().getValue(0)));
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKAsinImageTest.cpp(defaults): Ramp-Zero-One-Float.nrrd
// (float32) through OUR reader, run OUR filter (defaults), then compare with the committed baseline @0.01 via
// ip_golden::CompareImages. See AcosImageFilterTest.cpp for the full rationale.
// -----------------------------------------------------------------------------
TEST_CASE("NXImageProcessing::AsinImageFilter: ITK real-image golden (defaults)", "[NXImageProcessing][ItkGolden][AsinImageFilter]")
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
  args.insertOrAssign(AsinImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(AsinImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(AsinImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  AsinImageFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // DURABLE golden: baseline compare @ 0.01.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(dataStructure, ip_golden::BaselinePath("BasicFilters_AsinImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(dataStructure, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
