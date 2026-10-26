#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/AsinImageFilter.hpp"

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Preferences.hpp"
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
DataPath runMappedIntegerAsin(DataStructure& dataStructure, const DataPath& inputPath, const ShapeType& tupleShape, const ShapeType& componentShape = {1})
{
  const AsinImageFilter filter;
  Arguments args;
  args.insertOrAssign(AsinImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(AsinImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(AsinImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.value().actions.size() == 1);
  const auto* actionPtr = dynamic_cast<const CreateArrayAction*>(preflightResult.outputActions.value().actions.front().get());
  REQUIRE(actionPtr != nullptr);
  // Stop an unfixed implementation before an invalid integer value can reach a NaN-to-integer conversion.
  REQUIRE(actionPtr->type() == DataType::float32);
  REQUIRE(actionPtr->dims() == tupleShape);
  REQUIRE(actionPtr->componentDims() == componentShape);
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  return inputPath.replaceName("Output");
}
} // namespace

// The principal angle at integer one is pi/2 radians for every integer width and signedness.
TEMPLATE_TEST_CASE("ImageProcessing::AsinImageFilter: integer one produces float32 radians", "[ImageProcessing][Issue1874][AsinImageFilter]", int8, uint8, int16, uint16, int32, uint32, int64, uint64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath inputPath = ip_test::BuildScalarValuesImage<TestType>(dataStructure, {1});
  const DataPath outputPath = runMappedIntegerAsin(dataStructure, inputPath, {1, 1, 1});
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(outputPath));
  REQUIRE(outputArrayPtr->getDataType() == DataType::float32);
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 1, 1});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const Float32Array* anglesPtr = nullptr;
  REQUIRE_NOTHROW(anglesPtr = &dataStructure.getDataRefAs<Float32Array>(outputPath));
  REQUIRE(anglesPtr->getDataStoreRef().getValue(0) == Approx(1.57079632679489661923).epsilon(0).margin(1.0e-7));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::AsinImageFilter: integer endpoints and invalid values produce floating angles", "[ImageProcessing][Issue1874][AsinImageFilter]", int8, uint8, int16, uint16, int32,
                   uint32, int64, uint64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  std::vector<TestType> inputValues = {0, 1, 2, std::numeric_limits<TestType>::max()};
  if constexpr(std::is_signed_v<TestType>)
  {
    inputValues.push_back(-1);
    inputValues.push_back(std::numeric_limits<TestType>::lowest());
  }
  DataStructure dataStructure;
  const DataPath inputPath = ip_test::BuildScalarValuesImage<TestType>(dataStructure, inputValues);
  const DataPath outputPath = runMappedIntegerAsin(dataStructure, inputPath, {1, 1, inputValues.size()});
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(outputPath));
  REQUIRE(outputArrayPtr->getDataType() == DataType::float32);
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 1, inputValues.size()});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const Float32Array* anglesPtr = nullptr;
  REQUIRE_NOTHROW(anglesPtr = &dataStructure.getDataRefAs<Float32Array>(outputPath));
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
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::AsinImageFilter: floating input retains type and principal angles", "[ImageProcessing][Issue1874][AsinImageFilter]", float32, float64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
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
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::AsinImageFilter: adjacent domain boundaries remain distinct", "[ImageProcessing][Issue1874][AsinImageFilter]", float32, float64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
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
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::AsinImageFilter: mapped multi-component output preserves tuple shape", "[ImageProcessing][Issue1874][AsinImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath inputPath = ip_test::BuildRampImage<int16>(dataStructure, 2, 0, 0);
  Int16Array* inputArrayPtr = nullptr;
  REQUIRE_NOTHROW(inputArrayPtr = &dataStructure.getDataRefAs<Int16Array>(inputPath));
  auto inputStore = DataStoreUtilities::CreateDataStore<int16>(dataStructure, inputPath, ShapeType{2, 2, 2}, ShapeType{2});
  REQUIRE(inputStore != nullptr);
  const auto replaceResult = inputArrayPtr->setDataStore(inputStore);
  SIMPLNX_RESULT_REQUIRE_VALID(replaceResult);
  constexpr std::array<int16, 4> inputValues = {-1, 0, 1, 2};
  for(usize index = 0; index < 16; ++index)
  {
    inputStore->setValue(index, inputValues.at(index % inputValues.size()));
  }
  const DataPath outputPath = runMappedIntegerAsin(dataStructure, inputPath, {2, 2, 2}, {2});
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(outputPath));
  REQUIRE(outputArrayPtr->getDataType() == DataType::float32);
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{2, 2, 2});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{2});
  const Float32Array* anglesPtr = nullptr;
  REQUIRE_NOTHROW(anglesPtr = &dataStructure.getDataRefAs<Float32Array>(outputPath));
  const auto& angleStore = anglesPtr->getDataStoreRef();
  for(usize index = 0; index < 16; index += 4)
  {
    REQUIRE(angleStore.getValue(index) == Approx(-1.57079632679489661923).epsilon(0).margin(1.0e-7));
    REQUIRE(angleStore.getValue(index + 1) == 0.0f);
    REQUIRE(angleStore.getValue(index + 2) == Approx(1.57079632679489661923).epsilon(0).margin(1.0e-7));
    REQUIRE(std::isnan(angleStore.getValue(index + 3)));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// PART A domain edge (documents ITK-faithful behavior): asin is undefined for |x|>1. The op is
// static_cast<U>(std::asin(static_cast<double>(x))) -- ITK's equally-unguarded Asin functor -- so asin(2) is NaN.
TEST_CASE("ImageProcessing::AsinImageFilter: asin of |x|>1 is NaN (domain edge)", "[ImageProcessing][AsinImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(ds, {2.0f});
  const DataPath output = ip_test::RunPointwiseFilter<AsinImageFilter>(ds, input);

  const Float32Array* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &ds.getDataRefAs<Float32Array>(output));
  REQUIRE(std::isnan(outputArrayPtr->getDataStoreRef().getValue(0)));
  UnitTest::CheckArraysInheritTupleDims(ds);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKAsinImageTest.cpp(defaults): Ramp-Zero-One-Float.nrrd
// (float32) through OUR reader, run OUR filter (defaults), then compare with the committed baseline @0.01 via
// ip_golden::CompareImages. See AcosImageFilterTest.cpp for the full rationale.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::AsinImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][AsinImageFilter]")
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
  args.insertOrAssign(AsinImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(AsinImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(AsinImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  AsinImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // DURABLE golden: baseline compare @ 0.01.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath("BasicFilters_AsinImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
