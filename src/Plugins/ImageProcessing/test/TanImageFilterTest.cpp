#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/TanImageFilter.hpp"

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Preferences.hpp"
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
#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

using namespace nx::core;

TEMPLATE_TEST_CASE("ImageProcessing::TanImageFilter: integer radians produce signed fractional float32 responses", "[ImageProcessing][Issue1874][TanImageFilter]", int8, uint8, int16, uint16, int32,
                   uint32, int64, uint64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  std::vector<TestType> values = {0, 1, 3};
  if constexpr(std::is_signed_v<TestType>)
  {
    values.push_back(-1);
  }
  const usize twoRadianIndex = values.size();
  values.push_back(2);
  const usize firstExtremum = values.size();
  values.push_back(std::numeric_limits<TestType>::lowest());
  values.push_back(std::numeric_limits<TestType>::max());
  if constexpr(sizeof(TestType) == sizeof(int64))
  {
    constexpr TestType exactDoubleInteger = TestType{9007199254740992ULL};
    values.push_back(exactDoubleInteger);
    values.push_back(exactDoubleInteger + 1);
  }
  DataStructure dataStructure;
  const DataPath inputPath = ip_test::BuildScalarValuesImage<TestType>(dataStructure, values);
  const ShapeType tupleShape = {1, 1, values.size()};
  Arguments args;
  args.insertOrAssign(TanImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(TanImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(TanImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const TanImageFilter filter;
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.value().actions.size() == 1);
  const auto* actionPtr = dynamic_cast<const CreateArrayAction*>(preflightResult.outputActions.value().actions.front().get());
  REQUIRE(actionPtr != nullptr);
  // Stop the baseline before a negative tangent could be converted to an unsigned integer.
  REQUIRE(actionPtr->type() == DataType::float32);
  REQUIRE(actionPtr->dims() == tupleShape);
  REQUIRE(actionPtr->componentDims() == ShapeType{1});

  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const DataPath outputPath = inputPath.replaceName("Output");
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(outputPath));
  REQUIRE(outputArrayPtr->getDataType() == DataType::float32);
  REQUIRE(outputArrayPtr->getTupleShape() == tupleShape);
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const Float32Array* responsesPtr = nullptr;
  REQUIRE_NOTHROW(responsesPtr = &dataStructure.getDataRefAs<Float32Array>(outputPath));
  const auto& store = responsesPtr->getDataStoreRef();
  REQUIRE(store.getValue(0) == 0.0f);
  REQUIRE(store.getValue(1) == Approx(1.5574077246549022305).epsilon(0).margin(1.0e-7));
  REQUIRE(store.getValue(2) == Approx(-0.1425465430742778053).epsilon(0).margin(1.0e-8));
  if constexpr(std::is_signed_v<TestType>)
  {
    REQUIRE(store.getValue(3) == Approx(-1.5574077246549022305).epsilon(0).margin(1.0e-7));
  }
  REQUIRE(store.getValue(twoRadianIndex) == Approx(-2.18503986326151899164).epsilon(0).margin(2.0e-7));
  // Native integer endpoints are accepted, without claiming exact phase after conversion to double.
  REQUIRE(std::isfinite(store.getValue(firstExtremum)));
  REQUIRE(std::isfinite(store.getValue(firstExtremum + 1)));
  if constexpr(sizeof(TestType) == sizeof(int64))
  {
    // The approved double argument conversion maps 2^53 and 2^53+1 to the same argument.
    REQUIRE(std::isfinite(store.getValue(values.size() - 2)));
    REQUIRE(store.getValue(values.size() - 2) == store.getValue(values.size() - 1));
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::TanImageFilter: floating responses retain type signed zero and finite near-pole values", "[ImageProcessing][Issue1874][TanImageFilter]", float32, float64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure dataStructure;
  const DataPath inputPath = ip_test::BuildScalarValuesImage<TestType>(dataStructure, {0, -TestType{0}, 1, -1, 3, std::numeric_limits<TestType>::quiet_NaN(), std::numeric_limits<TestType>::infinity(),
                                                                                       -std::numeric_limits<TestType>::infinity(), static_cast<TestType>(1.5707963267948966),
                                                                                       std::numeric_limits<TestType>::lowest(), std::numeric_limits<TestType>::max()});
  Arguments args;
  args.insertOrAssign(TanImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(TanImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(TanImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const TanImageFilter filter;
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.value().actions.size() == 1);
  const auto* actionPtr = dynamic_cast<const CreateArrayAction*>(preflightResult.outputActions.value().actions.front().get());
  REQUIRE(actionPtr != nullptr);
  REQUIRE(actionPtr->type() == GetDataType<TestType>());
  REQUIRE(actionPtr->dims() == ShapeType{1, 1, 11});
  REQUIRE(actionPtr->componentDims() == ShapeType{1});

  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const DataPath outputPath = inputPath.replaceName("Output");
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(outputPath));
  REQUIRE(outputArrayPtr->getDataType() == GetDataType<TestType>());
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 1, 11});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const DataArray<TestType>* responsesPtr = nullptr;
  REQUIRE_NOTHROW(responsesPtr = &dataStructure.getDataRefAs<DataArray<TestType>>(outputPath));
  const auto& store = responsesPtr->getDataStoreRef();
  REQUIRE(store.getValue(0) == 0);
  REQUIRE_FALSE(std::signbit(store.getValue(0)));
  REQUIRE(store.getValue(1) == 0);
  REQUIRE(std::signbit(store.getValue(1)));
  constexpr float64 responseTolerance = std::is_same_v<TestType, float32> ? 1.0e-7 : 1.0e-15;
  REQUIRE(store.getValue(2) == Approx(1.5574077246549022305).epsilon(0).margin(responseTolerance));
  REQUIRE(store.getValue(3) == Approx(-1.5574077246549022305).epsilon(0).margin(responseTolerance));
  REQUIRE(store.getValue(4) == Approx(-0.1425465430742778053).epsilon(0).margin(responseTolerance));
  for(usize index = 5; index < 8; ++index)
  {
    REQUIRE(std::isnan(store.getValue(index)));
  }
  // Independent high-precision sine/cosine references at the exact stored half-pi approximations.
  // The two stored arguments are on opposite sides of the pole and must not be excluded.
  REQUIRE(std::isfinite(store.getValue(8)));
  if constexpr(std::is_same_v<TestType, float32>)
  {
    REQUIRE(store.getValue(8) == Approx(-22877332.428856459874).epsilon(1.0e-7));
  }
  else
  {
    REQUIRE(store.getValue(8) == Approx(16331239353195369.756).epsilon(1.0e-15));
  }
  REQUIRE(std::isfinite(store.getValue(9)));
  REQUIRE(std::isfinite(store.getValue(10)));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::TanImageFilter: integer vector tuples preserve shape and component order", "[ImageProcessing][Issue1874][TanImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure dataStructure;
  const ShapeType tupleShape = {1, 2, 2};
  const ShapeType componentShape = {2};
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image Geometry");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({2, 2, 1});
  auto* cellDataPtr = AttributeMatrix::Create(dataStructure, "CellData", tupleShape, imageGeomPtr->getId());
  REQUIRE(cellDataPtr != nullptr);
  imageGeomPtr->setCellData(*cellDataPtr);
  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  auto inputStorePtr = DataStoreUtilities::CreateDataStore<int16>(dataStructure, inputPath, tupleShape, componentShape);
  REQUIRE(inputStorePtr != nullptr);
  auto* inputArrayPtr = DataArray<int16>::Create(dataStructure, "Input", inputStorePtr, cellDataPtr->getId());
  REQUIRE(inputArrayPtr != nullptr);
  constexpr std::array<int16, 8> values = {0, 1, 3, -1, 1, 0, -1, 3};
  for(usize index = 0; index < values.size(); ++index)
  {
    inputArrayPtr->getDataStoreRef().setValue(index, values.at(index));
  }

  Arguments args;
  args.insertOrAssign(TanImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(TanImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(TanImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const TanImageFilter filter;
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.value().actions.size() == 1);
  const auto* actionPtr = dynamic_cast<const CreateArrayAction*>(preflightResult.outputActions.value().actions.front().get());
  REQUIRE(actionPtr != nullptr);
  REQUIRE(actionPtr->type() == DataType::float32);
  REQUIRE(actionPtr->dims() == tupleShape);
  REQUIRE(actionPtr->componentDims() == componentShape);

  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const DataPath outputPath = inputPath.replaceName("Output");
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(outputPath));
  REQUIRE(outputArrayPtr->getDataType() == DataType::float32);
  REQUIRE(outputArrayPtr->getTupleShape() == tupleShape);
  REQUIRE(outputArrayPtr->getComponentShape() == componentShape);
  const Float32Array* responsesPtr = nullptr;
  REQUIRE_NOTHROW(responsesPtr = &dataStructure.getDataRefAs<Float32Array>(outputPath));
  constexpr std::array<float64, 8> expected = {0, 1.5574077246549022305, -0.1425465430742778053, -1.5574077246549022305, 1.5574077246549022305, 0, -1.5574077246549022305, -0.1425465430742778053};
  for(usize index = 0; index < expected.size(); ++index)
  {
    REQUIRE(responsesPtr->getDataStoreRef().getValue(index) == Approx(expected.at(index)).epsilon(0).margin(1.0e-7));
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKTanImageTest.cpp(defaults): Ramp-Zero-One-Float.nrrd
// (float32) through OUR reader, run OUR filter (defaults), then compare with the committed baseline @0.01.
// See AcosImageFilterTest.cpp for the full rationale.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::TanImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][TanImageFilter]")
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
  args.insertOrAssign(TanImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(TanImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(TanImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  TanImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // DURABLE golden: baseline compare @ 0.01.
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath("BasicFilters_TanImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
