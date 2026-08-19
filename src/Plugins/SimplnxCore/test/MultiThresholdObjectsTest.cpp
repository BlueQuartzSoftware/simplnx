#include "SimplnxCore/Filters/MultiThresholdObjectsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

using namespace nx::core;
using namespace nx::core::Constants;
namespace fs = std::filesystem;

namespace
{
const std::string k_TestArrayFloatName = "TestArrayFloat";
const std::string k_TestArrayIntName = "TestArrayInt";
const std::string k_ThresholdArrayName = "ThresholdArray";
const std::string k_MultiComponentArrayName = "MultiComponent";

const DataPath k_ImageCellDataName({k_ImageGeometry, k_CellData});
const DataPath k_TestArrayFloatPath = k_ImageCellDataName.createChildPath(k_TestArrayFloatName);
const DataPath k_TestArrayIntPath = k_ImageCellDataName.createChildPath(k_TestArrayIntName);
const DataPath k_MultiComponentArrayPath = k_ImageCellDataName.createChildPath(k_MultiComponentArrayName);
const DataPath k_ThresholdArrayPath = k_ImageCellDataName.createChildPath(k_ThresholdArrayName);

const DataPath k_MismatchingTuplesArrayPath({"MismatchingTuplesArray"});

constexpr int8 k_TupleCount = 8;
constexpr int8 k_MultiComponentCount = 3;

constexpr float64 k_FloatValueIncrement = 0.01;

// The "Valid Execution, Mask DataType" test thresholds the float fixture (InputFloatValue(i) == (i + 1) *
// k_FloatValueIncrement) with GreaterThan. Tuple i is true exactly when i >= k_MaskTypeFirstTrueTuple, so the
// comparison value is derived from the split rather than the other way around.
constexpr usize k_MaskTypeFirstTrueTuple = 5;
constexpr float64 k_MaskTypeComparisonValue = k_MaskTypeFirstTrueTuple * k_FloatValueIncrement;
static_assert(k_MaskTypeFirstTrueTuple > 0 && k_MaskTypeFirstTrueTuple < static_cast<usize>(k_TupleCount),
              "The comparison value must split the fixture so both the false and true branches are exercised");

constexpr std::array<bool, k_TupleCount> k_ExemplarInt4{0, 0, 0, 0, 0, 1, 1, 1};
constexpr std::array<bool, k_TupleCount> k_ExemplarFloat02{0, 1, 0, 0, 0, 0, 0, 0};

constexpr int32 InputIntValue(int32 index)
{
  return index;
}

constexpr int32 InputIntComponentValue(int32 tuple, int32 component)
{
  return (tuple + component) % 2 == 0 ? -tuple : tuple;
}

constexpr float64 InputFloatValue(int32 index)
{
  return (index + 1) * k_FloatValueIncrement;
}

DataStructure CreateTestDataStructure()
{
  DataStructure dataStructure;
  // Create two test arrays, a float array and a int array
  // Set up geometry for tuples, a cuboid with dimensions k_TupleCount, 1, 1
  ImageGeom* image = ImageGeom::Create(dataStructure, k_ImageGeometry);
  std::vector<usize> dims = {k_TupleCount, 1, 1};
  image->setDimensions(dims);

  ShapeType tDims = {k_TupleCount};
  ShapeType cDims = {1};
  ShapeType cDimsMulti = {k_MultiComponentCount};

  AttributeMatrix* am = AttributeMatrix::Create(dataStructure, k_CellData, tDims, image->getId());
  Float32Array* data = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, k_TestArrayFloatName, tDims, cDims, am->getId());
  Int32Array* data1 = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, k_TestArrayIntName, tDims, cDims, am->getId());
  Int32Array* multiComponentData = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, k_MultiComponentArrayName, tDims, cDimsMulti, am->getId());

  // Mismatched tuple count throws an error.
  // This is not true for mismatched component shapes.
  Float32Array* problemArray = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, k_MismatchingTuplesArrayPath.getTargetName(), std::vector<usize>{10}, cDims);
  problemArray->fill(2.0);

  usize numComponents = multiComponentData->getNumberOfComponents();

  // Fill the float array with {.01,.02,.03,.04,.05,.06,.07,.08}
  // Fill the int array with {0,1,2,3,4,5,6,7}
  // Fill multi-component array with {{0, 0, 0}, {1, -1, 1}, {-2, 2, -2}, {3, -3, 3}, {-4, 4, -4}, {5, -5, 5}, {-6, 6, -6}, {7, -7, 7}}
  for(usize i = 0; i < k_TupleCount; i++)
  {
    (*data)[i] = InputFloatValue(i); // float array
    (*data1)[i] = InputIntValue(i);  // int array

    for(usize j = 0; j < k_MultiComponentCount; j++)
    {
      multiComponentData->setComponent(i, j, InputIntComponentValue(i, j));
    }
  }
  return dataStructure;
}

template <typename T>
void SetArrayValues(DataArray<T>& dataArray)
{
  auto& dataStore = dataArray.getDataStoreRef();
  usize count = dataStore.size();
  for(usize i = 0; i < count; i++)
  {
    dataStore[i] = static_cast<T>(i);
  }
}

DataStructure CreateTestDataStructure2()
{
  DataStructure dataStructure;
  // Create two test arrays, a float array and a int array
  // Set up geometry for tuples, a cuboid with dimensions k_TupleCount, 1, 1
  ImageGeom* image = ImageGeom::Create(dataStructure, k_ImageGeometry);
  std::vector<usize> dims = {k_TupleCount, 1, 1};
  image->setDimensions(dims);

  ShapeType tDims = {k_TupleCount};
  ShapeType cDims = {1};
  ShapeType cDimsMulti = {k_MultiComponentCount};

  AttributeMatrix* am = AttributeMatrix::Create(dataStructure, k_CellData, tDims, image->getId());
  auto* int8Array = Int8Array::CreateWithStore<Int8DataStore>(dataStructure, "int8", tDims, cDims, am->getId());
  auto* int16Array = Int16Array::CreateWithStore<Int16DataStore>(dataStructure, "int16", tDims, cDims, am->getId());
  auto* int32Array = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "int32", tDims, cDims, am->getId());
  auto* int64Array = Int64Array::CreateWithStore<Int64DataStore>(dataStructure, "int64", tDims, cDims, am->getId());
  auto* uint8Array = UInt8Array::CreateWithStore<UInt8DataStore>(dataStructure, "uint8", tDims, cDims, am->getId());
  auto* uint16Array = UInt16Array::CreateWithStore<UInt16DataStore>(dataStructure, "uint16", tDims, cDims, am->getId());
  auto* uint32Array = UInt32Array::CreateWithStore<UInt32DataStore>(dataStructure, "uint32", tDims, cDims, am->getId());
  auto* uint64Array = UInt64Array::CreateWithStore<UInt64DataStore>(dataStructure, "uint64", tDims, cDims, am->getId());
  auto* float32Array = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, "float32", tDims, cDims, am->getId());
  auto* float64Array = Float64Array::CreateWithStore<Float64DataStore>(dataStructure, "float64", tDims, cDims, am->getId());
  auto* boolArray = BoolArray::CreateWithStore<BoolDataStore>(dataStructure, "bool", tDims, cDims, am->getId());

  SetArrayValues<int8>(*int8Array);
  SetArrayValues<int16>(*int16Array);
  SetArrayValues<int32>(*int32Array);
  SetArrayValues<int64>(*int64Array);
  SetArrayValues<uint8>(*uint8Array);
  SetArrayValues<uint16>(*uint16Array);
  SetArrayValues<uint32>(*uint32Array);
  SetArrayValues<uint64>(*uint64Array);
  SetArrayValues<float32>(*float32Array);
  SetArrayValues<float64>(*float64Array);
  SetArrayValues<bool>(*boolArray);

  return dataStructure;
}

/**
 * @brief Creates a single threshold for the filter to use.
 * @param arrayPath Input DataArray path
 * @param comparisonType type of comparison
 * @param value Value to threshold against
 * @param isInverted Should the threshold output be inverted
 * componentIndex Component index of the array to threshold against.
 */
ArrayThresholdSet CreateSingleThreshold(const DataPath& arrayPath, ArrayThreshold::ComparisonType comparisonType, double value, bool isInverted, int componentIndex)
{
  ArrayThresholdSet thresholdSet;
  auto threshold = std::make_shared<ArrayThreshold>();
  threshold->setArrayPath(arrayPath);
  threshold->setComparisonType(comparisonType);
  threshold->setComparisonValue(value);
  threshold->setComponentIndex(componentIndex);
  threshold->setInverted(isInverted);
  thresholdSet.setArrayThresholds({threshold});

  return thresholdSet;
}

/**
 * @brief Runs the MultiThresholdObjects filter on the provided threshold set
 * @param dataStructure
 * @param thresholdSet ThresholdSet to use for the MultiThresholdObjectsFilter
 */
void RunThresholdSetTest(DataStructure& dataStructure, ArrayThresholdSet thresholdSet)
{
  MultiThresholdObjectsFilter filter;
  Arguments args;

  args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::boolean));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Require that the mask array only has one component
  const auto* thresholdArrayPtr = dataStructure.getDataAs<BoolArray>(k_ThresholdArrayPath);
  REQUIRE(thresholdArrayPtr != nullptr);

  REQUIRE(thresholdArrayPtr->getNumberOfComponents() == 1);
}

/**
 * @brief Runs the MultiThresholdObjects filter on the provided DataStructure using a single array threshold.
 * @param dataStructure
 * @param arrayPath Path to use for the threshold DataArray
 * @param comparisonType Type of comparison to perform
 * @param value Value to threshold against
 * @param isInverted should the output mask value be inverted
 * @param componentIndex Which component of the array the threshold should use.
 */
void RunSingleThresholdTest(DataStructure& dataStructure, const DataPath& arrayPath, ArrayThreshold::ComparisonType comparisonType, double value, bool isInverted, int32 componentIndex = 0)
{
  auto thresholdSet = CreateSingleThreshold(arrayPath, comparisonType, value, isInverted, componentIndex);
  RunThresholdSetTest(dataStructure, thresholdSet);
}

// Integer checks
bool ExpectedIntSingleComponentMask(ArrayThreshold::ComparisonType comparisonType, int32 i, double thresholdValue, bool isInverted)
{
  bool expected = false;

  // The filter truncates the comparison value to the input array's type and compares in that type, matching
  // legacy DREAM3D. A threshold of 5.5 against an int32 array therefore compares against 5.
  const int32 comparisonValue = static_cast<int32>(thresholdValue);

  switch(comparisonType)
  {
  case ArrayThreshold::ComparisonType::GreaterThan:
    expected = InputIntValue(i) > comparisonValue;
    break;
  case ArrayThreshold::ComparisonType::LessThan:
    expected = InputIntValue(i) < comparisonValue;
    break;
  case ArrayThreshold::ComparisonType::Operator_Equal:
    expected = InputIntValue(i) == comparisonValue;
    break;
  case ArrayThreshold::ComparisonType::Operator_NotEqual:
    expected = InputIntValue(i) != comparisonValue;
    break;
  }

  if(isInverted)
  {
    expected = !expected;
  }
  return expected;
}

void CheckIntTestDataSingleComponent(const DataStructure& dataStructure, ArrayThreshold::ComparisonType comparisonType, double thresholdValue, bool isInverted)
{
  const auto* thresholdArrayPtr = dataStructure.getDataAs<BoolArray>(k_ThresholdArrayPath);
  REQUIRE(thresholdArrayPtr != nullptr);

  auto& thresholdStore = thresholdArrayPtr->getDataStoreRef();

  for(usize i = 0; i < k_TupleCount; i++)
  {
    REQUIRE(thresholdStore[i] == ExpectedIntSingleComponentMask(comparisonType, i, thresholdValue, isInverted));
  }
}

// Floating point checks
bool ExpectedFloatSingleComponentMask(ArrayThreshold::ComparisonType comparisonType, int32 i, double thresholdValue, bool isInverted)
{
  bool expected = false;

  // The target array is a Float32Array and the filter truncates the comparison value to the array's type
  // before comparing, so the oracle rounds both operands through float32 the same way. Comparing the raw
  // float64 values instead would diverge on exact-equality thresholds such as 0.03.
  const float32 inputValue = static_cast<float32>(InputFloatValue(i));
  const float32 comparisonValue = static_cast<float32>(thresholdValue);

  switch(comparisonType)
  {
  case ArrayThreshold::ComparisonType::GreaterThan:
    expected = inputValue > comparisonValue;
    break;
  case ArrayThreshold::ComparisonType::LessThan:
    expected = inputValue < comparisonValue;
    break;
  case ArrayThreshold::ComparisonType::Operator_Equal:
    expected = inputValue == comparisonValue;
    break;
  case ArrayThreshold::ComparisonType::Operator_NotEqual:
    expected = inputValue != comparisonValue;
    break;
  }

  if(isInverted)
  {
    expected = !expected;
  }
  return expected;
}

void CheckFloatTestDataSingleComponent(const DataStructure& dataStructure, ArrayThreshold::ComparisonType comparisonType, double thresholdValue, bool isInverted)
{
  const auto* thresholdArrayPtr = dataStructure.getDataAs<BoolArray>(k_ThresholdArrayPath);
  REQUIRE(thresholdArrayPtr != nullptr);

  auto& thresholdStore = thresholdArrayPtr->getDataStoreRef();

  for(usize i = 0; i < k_TupleCount; i++)
  {
    REQUIRE(thresholdStore[i] == ExpectedFloatSingleComponentMask(comparisonType, i, thresholdValue, isInverted));
  }
}

// Multi-component checks
bool ExpectedIntMultiComponentMask(ArrayThreshold::ComparisonType comparisonType, int32 i, double thresholdValue, bool isInverted, int32 componentIndex)
{
  bool expected = false;

  // Same comparison-value truncation as the single-component int oracle above.
  const int32 comparisonValue = static_cast<int32>(thresholdValue);

  switch(comparisonType)
  {
  case ArrayThreshold::ComparisonType::GreaterThan:
    expected = InputIntComponentValue(i, componentIndex) > comparisonValue;
    break;
  case ArrayThreshold::ComparisonType::LessThan:
    expected = InputIntComponentValue(i, componentIndex) < comparisonValue;
    break;
  case ArrayThreshold::ComparisonType::Operator_Equal:
    expected = InputIntComponentValue(i, componentIndex) == comparisonValue;
    break;
  case ArrayThreshold::ComparisonType::Operator_NotEqual:
    expected = InputIntComponentValue(i, componentIndex) != comparisonValue;
    break;
  }

  if(isInverted)
  {
    expected = !expected;
  }
  return expected;
}

void CheckIntTestDataMultiComponent(const DataStructure& dataStructure, ArrayThreshold::ComparisonType comparisonType, double thresholdValue, bool isInverted, int32 componentIndex)
{
  const auto* thresholdArrayPtr = dataStructure.getDataAs<BoolArray>(k_ThresholdArrayPath);
  REQUIRE(thresholdArrayPtr != nullptr);

  auto& thresholdStore = thresholdArrayPtr->getDataStoreRef();

  for(usize i = 0; i < k_TupleCount; i++)
  {
    REQUIRE(thresholdStore[i] == ExpectedIntMultiComponentMask(comparisonType, i, thresholdValue, isInverted, componentIndex));
  }
}

template <typename T>
float64 GetOutOfBoundsMinimumValue()
{
  if constexpr(std::is_unsigned_v<T>)
  {
    return -1.0;
  }
  else if constexpr(std::is_floating_point_v<T>)
  {
    return static_cast<float64>(-std::numeric_limits<T>::max()) * 2;
  }

  return static_cast<float64>(std::numeric_limits<T>::min()) * 2;
}

template <typename T>
float64 GetOutOfBoundsMaximumValue()
{
  return static_cast<float64>(std::numeric_limits<T>::max()) * 2;
}
} // namespace

void CheckExemplar(const DataStructure& dataStructure, const std::array<bool, k_TupleCount>& exemplarMask)
{
  const auto* thresholdArrayPtr = dataStructure.getDataAs<BoolArray>(k_ThresholdArrayPath);
  REQUIRE(thresholdArrayPtr != nullptr);
  auto& thresholdStore = thresholdArrayPtr->getDataStoreRef();

  for(usize i = 0; i < k_TupleCount; i++)
  {
    REQUIRE(thresholdStore[i] == exemplarMask[i]);
  }
}

TEST_CASE("SimplnxCore::MultiThresholdObjects: Exemplar Single Thresholds: Int", "[SimplnxCore][MultiThresholdObjectsFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateTestDataStructure();
  const DataPath targetArray = k_TestArrayIntPath;
  double thresholdValue = 4.0;
  bool isInverted = false;

  RunSingleThresholdTest(dataStructure, targetArray, ArrayThreshold::ComparisonType::GreaterThan, thresholdValue, isInverted);
  CheckExemplar(dataStructure, k_ExemplarInt4);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::MultiThresholdObjects: Exemplar Single Thresholds: Float", "[SimplnxCore][MultiThresholdObjectsFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateTestDataStructure();
  const DataPath targetArray = k_TestArrayFloatPath;
  double thresholdValue = 0.02;
  bool isInverted = false;

  RunSingleThresholdTest(dataStructure, targetArray, ArrayThreshold::ComparisonType::Operator_Equal, thresholdValue, isInverted);
  CheckExemplar(dataStructure, k_ExemplarFloat02);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::MultiThresholdObjects: Valid Single Thresholds: Int", "[SimplnxCore][MultiThresholdObjectsFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateTestDataStructure();
  const DataPath targetArray = k_TestArrayIntPath;
  double thresholdValue = GENERATE(-1.0, 0.0, 1.0, 2.0, 3.0, 4.0, 22.0, 5.5);
  bool isInverted = GENERATE(false, true);

  SECTION("ArrayThreshold: >")
  {
    RunSingleThresholdTest(dataStructure, targetArray, ArrayThreshold::ComparisonType::GreaterThan, thresholdValue, isInverted);
    CheckIntTestDataSingleComponent(dataStructure, ArrayThreshold::ComparisonType::GreaterThan, thresholdValue, isInverted);
  }

  SECTION("ArrayThreshold: <")
  {
    RunSingleThresholdTest(dataStructure, targetArray, ArrayThreshold::ComparisonType::LessThan, thresholdValue, isInverted);
    CheckIntTestDataSingleComponent(dataStructure, ArrayThreshold::ComparisonType::LessThan, thresholdValue, isInverted);
  }

  SECTION("ArrayThreshold: ==")
  {
    RunSingleThresholdTest(dataStructure, targetArray, ArrayThreshold::ComparisonType::Operator_Equal, thresholdValue, isInverted);
    CheckIntTestDataSingleComponent(dataStructure, ArrayThreshold::ComparisonType::Operator_Equal, thresholdValue, isInverted);
  }
  SECTION("ArrayThreshold: !=")
  {
    RunSingleThresholdTest(dataStructure, targetArray, ArrayThreshold::ComparisonType::Operator_NotEqual, thresholdValue, isInverted);
    CheckIntTestDataSingleComponent(dataStructure, ArrayThreshold::ComparisonType::Operator_NotEqual, thresholdValue, isInverted);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::MultiThresholdObjects: Valid Single Thresholds: Float", "[SimplnxCore][MultiThresholdObjectsFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateTestDataStructure();
  const DataPath targetArray = k_TestArrayFloatPath;
  double thresholdValue = GENERATE(0.0, 0.01, 0.02, 0.03, 0.04, 26.2);
  bool isInverted = GENERATE(false, true);

  SECTION("ArrayThreshold: >")
  {
    RunSingleThresholdTest(dataStructure, targetArray, ArrayThreshold::ComparisonType::GreaterThan, thresholdValue, isInverted);
    CheckFloatTestDataSingleComponent(dataStructure, ArrayThreshold::ComparisonType::GreaterThan, thresholdValue, isInverted);
  }

  SECTION("ArrayThreshold: <")
  {
    RunSingleThresholdTest(dataStructure, targetArray, ArrayThreshold::ComparisonType::LessThan, thresholdValue, isInverted);
    CheckFloatTestDataSingleComponent(dataStructure, ArrayThreshold::ComparisonType::LessThan, thresholdValue, isInverted);
  }

  SECTION("ArrayThreshold: ==")
  {
    RunSingleThresholdTest(dataStructure, targetArray, ArrayThreshold::ComparisonType::Operator_Equal, thresholdValue, isInverted);
    CheckFloatTestDataSingleComponent(dataStructure, ArrayThreshold::ComparisonType::Operator_Equal, thresholdValue, isInverted);
  }
  SECTION("ArrayThreshold: !=")
  {
    RunSingleThresholdTest(dataStructure, targetArray, ArrayThreshold::ComparisonType::Operator_NotEqual, thresholdValue, isInverted);
    CheckFloatTestDataSingleComponent(dataStructure, ArrayThreshold::ComparisonType::Operator_NotEqual, thresholdValue, isInverted);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::MultiThresholdObjects: Valid Single Thresholds: Int Multi-Component", "[SimplnxCore][MultiThresholdObjectsFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateTestDataStructure();
  const DataPath targetArray = k_MultiComponentArrayPath;
  double thresholdValue = GENERATE(-1.0, 0.0, 1.0, 2.0, 3.0, 4.0, 22.0, 5.5);
  bool isInverted = GENERATE(false, true);
  int32 componentIndex = GENERATE(0, 1, 2);

  SECTION("ArrayThreshold: >")
  {
    RunSingleThresholdTest(dataStructure, targetArray, ArrayThreshold::ComparisonType::GreaterThan, thresholdValue, isInverted, componentIndex);
    CheckIntTestDataMultiComponent(dataStructure, ArrayThreshold::ComparisonType::GreaterThan, thresholdValue, isInverted, componentIndex);
  }

  SECTION("ArrayThreshold: <")
  {
    RunSingleThresholdTest(dataStructure, targetArray, ArrayThreshold::ComparisonType::LessThan, thresholdValue, isInverted, componentIndex);
    CheckIntTestDataMultiComponent(dataStructure, ArrayThreshold::ComparisonType::LessThan, thresholdValue, isInverted, componentIndex);
  }

  SECTION("ArrayThreshold: ==")
  {
    RunSingleThresholdTest(dataStructure, targetArray, ArrayThreshold::ComparisonType::Operator_Equal, thresholdValue, isInverted, componentIndex);
    CheckIntTestDataMultiComponent(dataStructure, ArrayThreshold::ComparisonType::Operator_Equal, thresholdValue, isInverted, componentIndex);
  }
  SECTION("ArrayThreshold: !=")
  {
    RunSingleThresholdTest(dataStructure, targetArray, ArrayThreshold::ComparisonType::Operator_NotEqual, thresholdValue, isInverted, componentIndex);
    CheckIntTestDataMultiComponent(dataStructure, ArrayThreshold::ComparisonType::Operator_NotEqual, thresholdValue, isInverted, componentIndex);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

/**
 * @brief Creates a single threshold for the filter to use.
 * @param arrayPath Input DataArray path
 * @param comparisonType type of comparison
 * @param value Value to threshold against
 * @param isInverted Should the threshold output be inverted
 * componentIndex Component index of the array to threshold against.
 * unionOperator Union operator to apply on the threshold. Defaults to And
 */
std::shared_ptr<ArrayThreshold> CreateArrayThreshold(const DataPath& arrayPath, ArrayThreshold::ComparisonType comparisonType, double value, bool isInverted, int componentIndex,
                                                     ArrayThreshold::UnionOperator unionOperator = ArrayThreshold::UnionOperator::And)
{
  auto threshold = std::make_shared<ArrayThreshold>();
  threshold->setArrayPath(arrayPath);
  threshold->setComparisonType(comparisonType);
  threshold->setComparisonValue(value);
  threshold->setComponentIndex(componentIndex);
  threshold->setInverted(isInverted);
  threshold->setUnionOperator(unionOperator);

  return threshold;
}

ArrayThresholdSet CreateThresholdSet1()
{
  ArrayThresholdSet thresholdSet;

  // Threshold: Int > 2
  auto threshold1 = CreateArrayThreshold(k_TestArrayIntPath, ArrayThreshold::ComparisonType::GreaterThan, 2.0, false, 0, ArrayThreshold::UnionOperator::And);
  // Threshold: Float < 0.025
  auto threshold2 = CreateArrayThreshold(k_TestArrayFloatPath, ArrayThreshold::ComparisonType::LessThan, 0.025, false, 0, ArrayThreshold::UnionOperator::And);
  // Threshold: Int[1] > 0.0 : inverted
  auto threshold3 = CreateArrayThreshold(k_MultiComponentArrayPath, ArrayThreshold::ComparisonType::GreaterThan, 0.0, true, 1, ArrayThreshold::UnionOperator::And);

  thresholdSet.setArrayThresholds({threshold1, threshold2, threshold3});

  return thresholdSet;
}

bool ExpectedThresholdSet1Mask(usize index, bool inverted)
{
  bool expectedThreshold1 = ExpectedIntSingleComponentMask(ArrayThreshold::ComparisonType::GreaterThan, index, 2.0, false);
  bool expectedThreshold2 = ExpectedFloatSingleComponentMask(ArrayThreshold::ComparisonType::LessThan, index, 0.025, false);
  bool expectedThreshold3 = ExpectedIntMultiComponentMask(ArrayThreshold::ComparisonType::GreaterThan, index, 0.0, true, 1);

  bool expected = expectedThreshold1 && expectedThreshold2 && expectedThreshold3;
  if(inverted)
  {
    expected = !expected;
  }
  return expected;
}

ArrayThresholdSet CreateThresholdSet2()
{
  ArrayThresholdSet thresholdSet;

  // Threshold: Int == 1
  auto threshold1 = CreateArrayThreshold(k_TestArrayIntPath, ArrayThreshold::ComparisonType::Operator_Equal, 1.0, false, 0, ArrayThreshold::UnionOperator::And);
  // Threshold: Float != 5.0
  auto threshold2 = CreateArrayThreshold(k_TestArrayFloatPath, ArrayThreshold::ComparisonType::Operator_NotEqual, 5.0, false, 0, ArrayThreshold::UnionOperator::Or);
  // Threshold: Int[0] < 0.0 : inverted
  auto threshold3 = CreateArrayThreshold(k_MultiComponentArrayPath, ArrayThreshold::ComparisonType::LessThan, 0.0, true, 0, ArrayThreshold::UnionOperator::And);

  thresholdSet.setArrayThresholds({threshold1, threshold2, threshold3});

  return thresholdSet;
}

bool ExpectedThresholdSet2Mask(usize index, bool inverted)
{
  bool expectedThreshold1 = ExpectedIntSingleComponentMask(ArrayThreshold::ComparisonType::Operator_Equal, index, 1.0, false);
  bool expectedThreshold2 = ExpectedFloatSingleComponentMask(ArrayThreshold::ComparisonType::Operator_NotEqual, index, 5.0, false);
  bool expectedThreshold3 = ExpectedIntMultiComponentMask(ArrayThreshold::ComparisonType::LessThan, index, 0.0, true, 0);

  bool expected = (expectedThreshold1 || expectedThreshold2) && expectedThreshold3;
  if(inverted)
  {
    expected = !expected;
  }
  return expected;
}

ArrayThresholdSet CreateThresholdSet3()
{
  ArrayThresholdSet thresholdSet;

  auto set1 = std::make_shared<ArrayThresholdSet>(CreateThresholdSet1());
  auto set2 = std::make_shared<ArrayThresholdSet>(CreateThresholdSet2());

  thresholdSet.setArrayThresholds({set1, set2});

  return thresholdSet;
}

ArrayThresholdSet CreateThresholdSet4()
{
  ArrayThresholdSet thresholdSet;

  auto set1 = std::make_shared<ArrayThresholdSet>(CreateThresholdSet1());
  auto set2 = std::make_shared<ArrayThresholdSet>(CreateThresholdSet2());
  set2->setUnionOperator(ArrayThreshold::UnionOperator::Or);

  thresholdSet.setArrayThresholds({set1, set2});

  return thresholdSet;
}

ArrayThresholdSet CreateThresholdSet5()
{
  ArrayThresholdSet thresholdSet;

  auto set1 = std::make_shared<ArrayThresholdSet>(CreateThresholdSet1());
  auto set2 = std::make_shared<ArrayThresholdSet>(CreateThresholdSet2());
  set2->setUnionOperator(ArrayThreshold::UnionOperator::Or);
  set2->setInverted(true);

  thresholdSet.setArrayThresholds({set1, set2});

  return thresholdSet;
}

/**
 * @brief Creates a ThresholdSet whose children mix a leaf ArrayThreshold with a sibling nested ArrayThresholdSet.
 *
 * This is the shape that triggered MultiThresholdObjectsFilter-D1, where the mask came back all-false
 * regardless of input. Every other CreateThresholdSet* helper passes either all leaves or all nested sets, so
 * this shape had no in-repo coverage.
 */
ArrayThresholdSet CreateThresholdSet6()
{
  ArrayThresholdSet thresholdSet;

  // Threshold: Int > 2
  auto leafThreshold = CreateArrayThreshold(k_TestArrayIntPath, ArrayThreshold::ComparisonType::GreaterThan, 2.0, false, 0, ArrayThreshold::UnionOperator::And);
  auto nestedSet = std::make_shared<ArrayThresholdSet>(CreateThresholdSet2());

  thresholdSet.setArrayThresholds({leafThreshold, nestedSet});

  return thresholdSet;
}

/**
 * @brief Creates a ThresholdSet mixing a leaf ArrayThreshold with a sibling *inverted* nested ArrayThresholdSet.
 *
 * This is the shape that triggered MultiThresholdObjectsFilter-D2, where inversion of a nested set reversed
 * the tuple order instead of flipping each tuple's value.
 */
ArrayThresholdSet CreateThresholdSet7()
{
  ArrayThresholdSet thresholdSet;

  // Threshold: Int > 2
  auto leafThreshold = CreateArrayThreshold(k_TestArrayIntPath, ArrayThreshold::ComparisonType::GreaterThan, 2.0, false, 0, ArrayThreshold::UnionOperator::And);
  auto nestedSet = std::make_shared<ArrayThresholdSet>(CreateThresholdSet2());
  nestedSet->setUnionOperator(ArrayThreshold::UnionOperator::Or);
  nestedSet->setInverted(true);

  thresholdSet.setArrayThresholds({leafThreshold, nestedSet});

  return thresholdSet;
}

void CheckThresholdSet1(DataStructure& dataStructure, bool inverted)
{
  const auto* thresholdArrayPtr = dataStructure.getDataAs<BoolArray>(k_ThresholdArrayPath);
  REQUIRE(thresholdArrayPtr != nullptr);

  auto& thresholdStore = thresholdArrayPtr->getDataStoreRef();

  for(usize i = 0; i < k_TupleCount; i++)
  {
    REQUIRE(thresholdStore[i] == ExpectedThresholdSet1Mask(i, inverted));
  }
}

void CheckThresholdSet2(DataStructure& dataStructure, bool inverted)
{
  const auto* thresholdArrayPtr = dataStructure.getDataAs<BoolArray>(k_ThresholdArrayPath);
  REQUIRE(thresholdArrayPtr != nullptr);

  auto& thresholdStore = thresholdArrayPtr->getDataStoreRef();

  for(usize i = 0; i < k_TupleCount; i++)
  {
    REQUIRE(thresholdStore[i] == ExpectedThresholdSet2Mask(i, inverted));
  }
}

void CheckThresholdSet3(DataStructure& dataStructure, bool inverted)
{
  const auto* thresholdArrayPtr = dataStructure.getDataAs<BoolArray>(k_ThresholdArrayPath);
  REQUIRE(thresholdArrayPtr != nullptr);

  auto& thresholdStore = thresholdArrayPtr->getDataStoreRef();

  for(usize i = 0; i < k_TupleCount; i++)
  {
    bool expectedMask1 = ExpectedThresholdSet1Mask(i, false);
    bool expectedMask2 = ExpectedThresholdSet2Mask(i, false);

    bool expected = expectedMask1 && expectedMask2;
    if(inverted)
    {
      expected = !expected;
    }

    REQUIRE(thresholdStore[i] == expected);
  }
}

void CheckThresholdSet4(DataStructure& dataStructure, bool inverted)
{
  const auto* thresholdArrayPtr = dataStructure.getDataAs<BoolArray>(k_ThresholdArrayPath);
  REQUIRE(thresholdArrayPtr != nullptr);

  auto& thresholdStore = thresholdArrayPtr->getDataStoreRef();

  for(usize i = 0; i < k_TupleCount; i++)
  {
    bool expectedMask1 = ExpectedThresholdSet1Mask(i, false);
    bool expectedMask2 = ExpectedThresholdSet2Mask(i, false);

    bool expected = expectedMask1 || expectedMask2;
    if(inverted)
    {
      expected = !expected;
    }

    REQUIRE(thresholdStore[i] == expected);
  }
}

void CheckThresholdSet5(DataStructure& dataStructure, bool inverted)
{
  const auto* thresholdArrayPtr = dataStructure.getDataAs<BoolArray>(k_ThresholdArrayPath);
  REQUIRE(thresholdArrayPtr != nullptr);

  auto& thresholdStore = thresholdArrayPtr->getDataStoreRef();

  for(usize i = 0; i < k_TupleCount; i++)
  {
    bool expectedMask1 = ExpectedThresholdSet1Mask(i, false);
    bool expectedMask2 = ExpectedThresholdSet2Mask(i, true);

    bool expected = expectedMask1 || expectedMask2;
    if(inverted)
    {
      expected = !expected;
    }

    REQUIRE(thresholdStore[i] == expected);
  }
}

void CheckThresholdSet6(DataStructure& dataStructure, bool inverted)
{
  const auto* thresholdArrayPtr = dataStructure.getDataAs<BoolArray>(k_ThresholdArrayPath);
  REQUIRE(thresholdArrayPtr != nullptr);

  auto& thresholdStore = thresholdArrayPtr->getDataStoreRef();

  for(usize i = 0; i < k_TupleCount; i++)
  {
    // The leaf is the first child, so it seeds the set's accumulator; the nested set then combines with its
    // own And union operator.
    bool expectedLeaf = ExpectedIntSingleComponentMask(ArrayThreshold::ComparisonType::GreaterThan, i, 2.0, false);
    bool expectedNested = ExpectedThresholdSet2Mask(i, false);

    bool expected = expectedLeaf && expectedNested;
    if(inverted)
    {
      expected = !expected;
    }

    REQUIRE(thresholdStore[i] == expected);
  }
}

void CheckThresholdSet7(DataStructure& dataStructure, bool inverted)
{
  const auto* thresholdArrayPtr = dataStructure.getDataAs<BoolArray>(k_ThresholdArrayPath);
  REQUIRE(thresholdArrayPtr != nullptr);

  auto& thresholdStore = thresholdArrayPtr->getDataStoreRef();

  for(usize i = 0; i < k_TupleCount; i++)
  {
    bool expectedLeaf = ExpectedIntSingleComponentMask(ArrayThreshold::ComparisonType::GreaterThan, i, 2.0, false);
    // The nested set is inverted, which must flip each tuple's value rather than reverse the tuple order.
    bool expectedNested = ExpectedThresholdSet2Mask(i, true);

    bool expected = expectedLeaf || expectedNested;
    if(inverted)
    {
      expected = !expected;
    }

    REQUIRE(thresholdStore[i] == expected);
  }
}

TEST_CASE("SimplnxCore::MultiThresholdObjects: Valid Threshold Sets", "[SimplnxCore][MultiThresholdObjectsFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateTestDataStructure();
  bool isInverted = GENERATE(false, true);

  SECTION("ArraySet 1")
  {
    auto thresholdSet = CreateThresholdSet1();
    thresholdSet.setInverted(isInverted);
    RunThresholdSetTest(dataStructure, thresholdSet);
    CheckThresholdSet1(dataStructure, isInverted);
  }

  SECTION("ArraySet 2")
  {
    auto thresholdSet = CreateThresholdSet2();
    thresholdSet.setInverted(isInverted);
    RunThresholdSetTest(dataStructure, thresholdSet);
    CheckThresholdSet2(dataStructure, isInverted);
  }

  SECTION("ArraySet 3")
  {
    auto thresholdSet = CreateThresholdSet3();
    thresholdSet.setInverted(isInverted);
    RunThresholdSetTest(dataStructure, thresholdSet);
    CheckThresholdSet3(dataStructure, isInverted);
  }

  SECTION("ArraySet 4")
  {
    auto thresholdSet = CreateThresholdSet4();
    thresholdSet.setInverted(isInverted);
    RunThresholdSetTest(dataStructure, thresholdSet);
    CheckThresholdSet4(dataStructure, isInverted);
  }

  SECTION("ArraySet 5")
  {
    auto thresholdSet = CreateThresholdSet5();
    thresholdSet.setInverted(isInverted);
    RunThresholdSetTest(dataStructure, thresholdSet);
    CheckThresholdSet5(dataStructure, isInverted);
  }

  // Regression coverage for MultiThresholdObjectsFilter-D1: a set mixing a leaf threshold with a sibling
  // nested set produced an all-false mask regardless of input.
  SECTION("ArraySet 6: leaf + nested set")
  {
    auto thresholdSet = CreateThresholdSet6();
    thresholdSet.setInverted(isInverted);
    RunThresholdSetTest(dataStructure, thresholdSet);
    CheckThresholdSet6(dataStructure, isInverted);
  }

  // Regression coverage for MultiThresholdObjectsFilter-D2: an inverted nested set reversed the tuple order
  // instead of flipping each tuple's value.
  SECTION("ArraySet 7: leaf + inverted nested set")
  {
    auto thresholdSet = CreateThresholdSet7();
    thresholdSet.setInverted(isInverted);
    RunThresholdSetTest(dataStructure, thresholdSet);
    CheckThresholdSet7(dataStructure, isInverted);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Invalid executions

TEST_CASE("SimplnxCore::MultiThresholdObjects: Invalid Execution", "[SimplnxCore][MultiThresholdObjectsFilter]")
{
  UnitTest::LoadPlugins();

  MultiThresholdObjectsFilter filter;
  DataStructure dataStructure = CreateTestDataStructure();
  Arguments args;
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));

  SECTION("Empty ArrayThresholdSet")
  {
    ArrayThresholdSet thresholdSet;

    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  }
  SECTION("Empty ArrayThreshold DataPath")
  {
    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  }
  SECTION("Out of Bounds Component Index")
  {
    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_TestArrayFloatPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    threshold->setComponentIndex(1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  }
  SECTION("Mismatching Tuples in Threshold Arrays")
  {
    ArrayThresholdSet thresholdSet;
    auto threshold1 = std::make_shared<ArrayThreshold>();
    threshold1->setArrayPath(k_TestArrayFloatPath);
    threshold1->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold1->setComparisonValue(0.1);
    auto threshold2 = std::make_shared<ArrayThreshold>();
    threshold2->setArrayPath(k_MismatchingTuplesArrayPath);
    threshold2->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold2->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold1, threshold2});

    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("SimplnxCore::MultiThresholdObjects: Invalid Execution - Out of Bounds Custom Values", "[SimplnxCore][MultiThresholdObjectsFilter]", int8, uint8, int16, uint16, int32, uint32,
                   int64, uint64, float32)
{
  UnitTest::LoadPlugins();

  MultiThresholdObjectsFilter filter;
  DataStructure dataStructure = CreateTestDataStructure();
  Arguments args;

  float64 trueValue;
  float64 falseValue;
  int32 code;

  SECTION("True Value < Minimum")
  {
    trueValue = GetOutOfBoundsMinimumValue<TestType>();
    falseValue = 1;
    code = MultiThresholdObjectsFilter::ErrorCodes::CustomTrueOutOfBounds;
  }

  SECTION("False Value < Minimum")
  {
    trueValue = 1;
    falseValue = GetOutOfBoundsMinimumValue<TestType>();
    code = MultiThresholdObjectsFilter::ErrorCodes::CustomFalseOutOfBounds;
  }

  SECTION("True Value > Maximum")
  {
    trueValue = GetOutOfBoundsMaximumValue<TestType>();
    falseValue = 1;
    code = MultiThresholdObjectsFilter::ErrorCodes::CustomTrueOutOfBounds;
  }

  SECTION("False Value > Maximum")
  {
    trueValue = 1;
    falseValue = GetOutOfBoundsMaximumValue<TestType>();
    code = MultiThresholdObjectsFilter::ErrorCodes::CustomFalseOutOfBounds;
  }

  ArrayThresholdSet thresholdSet;
  auto threshold = std::make_shared<ArrayThreshold>();
  threshold->setArrayPath(k_TestArrayIntPath);
  threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
  threshold->setComparisonValue(15);
  thresholdSet.setArrayThresholds({threshold});

  args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_UseCustomTrueValue, std::make_any<bool>(true));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CustomTrueValue, std::make_any<float64>(trueValue));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_UseCustomFalseValue, std::make_any<bool>(true));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CustomFalseValue, std::make_any<float64>(falseValue));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(GetDataType<TestType>()));

  // Preflight the filter
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == code);
}

TEST_CASE("SimplnxCore::MultiThresholdObjects: Invalid Execution - Boolean Custom Values", "[SimplnxCore][MultiThresholdObjectsFilter]")
{
  UnitTest::LoadPlugins();

  MultiThresholdObjectsFilter filter;
  DataStructure dataStructure = CreateTestDataStructure();
  Arguments args;

  int32 code;

  SECTION("Custom True Value")
  {
    code = MultiThresholdObjectsFilter::ErrorCodes::CustomTrueWithBoolean;
    args.insertOrAssign(MultiThresholdObjectsFilter::k_UseCustomTrueValue, std::make_any<bool>(true));
  }

  SECTION("Custom False Value")
  {
    code = MultiThresholdObjectsFilter::ErrorCodes::CustomFalseWithBoolean;
    args.insertOrAssign(MultiThresholdObjectsFilter::k_UseCustomFalseValue, std::make_any<bool>(true));
  }

  ArrayThresholdSet thresholdSet;
  auto threshold = std::make_shared<ArrayThreshold>();
  threshold->setArrayPath(k_TestArrayIntPath);
  threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
  threshold->setComparisonValue(15);
  thresholdSet.setArrayThresholds({threshold});

  args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::boolean));

  // Preflight the filter
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == code);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// DataType checks

template <typename T>
void checkMaskValues(const DataStructure& dataStructure, const DataPath& thresholdArrayPath)
{
  auto* thresholdArrayPtr = dataStructure.getDataAs<DataArray<T>>(thresholdArrayPath);
  REQUIRE(thresholdArrayPtr != nullptr);

  auto& thresholdStore = thresholdArrayPtr->getDataStoreRef();

  // Tuples below k_MaskTypeFirstTrueTuple are false and the rest are true. The split is taken from the
  // fixture constants so that changing k_TupleCount cannot silently make one of the two branches unreachable.
  for(usize i = 0; i < k_TupleCount; i++)
  {
    if(i < k_MaskTypeFirstTrueTuple)
    {
      REQUIRE(thresholdStore[i] == static_cast<T>(0));
    }
    else
    {
      REQUIRE(thresholdStore[i] == static_cast<T>(1));
    }
  }
}

template <typename T>
void runMaskTypeFilter(MultiThresholdObjectsFilter& filter, Arguments& args, DataStructure& dataStructure)
{
  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  checkMaskValues<T>(dataStructure, k_ThresholdArrayPath);
}

TEST_CASE("SimplnxCore::MultiThresholdObjects: Valid Execution, Mask DataType", "[SimplnxCore][MultiThresholdObjectsFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateTestDataStructure();

  // Shared filter setup
  MultiThresholdObjectsFilter filter;
  Arguments args;

  ArrayThresholdSet thresholdSet;
  auto threshold = std::make_shared<ArrayThreshold>();
  threshold->setArrayPath(k_TestArrayFloatPath);
  threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
  threshold->setComparisonValue(k_MaskTypeComparisonValue);
  thresholdSet.setArrayThresholds({threshold});

  args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));

  // Signed
  SECTION("Int8 Threshold")
  {
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::int8));

    runMaskTypeFilter<int8>(filter, args, dataStructure);
  }

  SECTION("Int16 Threshold")
  {
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::int16));

    runMaskTypeFilter<int16>(filter, args, dataStructure);
  }

  SECTION("Int32 Threshold")
  {
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::int32));

    runMaskTypeFilter<int32>(filter, args, dataStructure);
  }

  SECTION("Int64 Threshold")
  {
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::int64));

    runMaskTypeFilter<int64>(filter, args, dataStructure);
  }

  // Unsigned
  SECTION("UInt8 Threshold")
  {
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::uint8));

    runMaskTypeFilter<uint8>(filter, args, dataStructure);
  }

  SECTION("UInt16 Threshold")
  {
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::uint16));

    runMaskTypeFilter<uint16>(filter, args, dataStructure);
  }

  SECTION("UInt32 Threshold")
  {
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::uint32));

    runMaskTypeFilter<uint32>(filter, args, dataStructure);
  }

  SECTION("UInt64 Threshold")
  {
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::uint64));

    runMaskTypeFilter<uint64>(filter, args, dataStructure);
  }

  // Floating Point
  SECTION("Float32 Threshold")
  {
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::float32));

    runMaskTypeFilter<float32>(filter, args, dataStructure);
  }

  SECTION("Float64 Threshold")
  {
    args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::float64));

    runMaskTypeFilter<float64>(filter, args, dataStructure);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

void TestMaskOutputForInputType(Int8AbstractDataStore& mask, float64 comparisonValue)
{
  usize count = mask.size();
  for(usize i = 0; i < count; i++)
  {
    int8 targetValue = (i < comparisonValue) ? 1 : 0;
    REQUIRE(static_cast<bool>(mask[i]) == targetValue);
  }
}
void TestMaskOutputForBoolInputType(Int8AbstractDataStore& mask, float64 comparisonValue)
{
  usize count = mask.size();
  for(usize i = 0; i < count; i++)
  {
    int8 targetValue = (static_cast<bool>(i) < comparisonValue) ? 1 : 0;
    REQUIRE(static_cast<bool>(mask[i]) == targetValue);
  }
}

TEST_CASE("SimplnxCore::MultiThresholdObjects: Valid Execution, Input Array DataType", "[SimplnxCore][MultiThresholdObjectsFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateTestDataStructure2();

  float64 comparisonValue = 3.0;
  DataPath matrixPath({k_ImageGeometry, k_CellData});
  bool isBoolInput = false;

  // Shared filter setup
  MultiThresholdObjectsFilter filter;
  Arguments args;

  ArrayThresholdSet thresholdSet;
  auto threshold = std::make_shared<ArrayThreshold>();
  threshold->setComparisonType(ArrayThreshold::ComparisonType::LessThan);
  threshold->setComparisonValue(comparisonValue);
  thresholdSet.setArrayThresholds({threshold});

  // Signed
  SECTION("Int8")
  {
    threshold->setArrayPath(matrixPath.createChildPath("int8"));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  }
  SECTION("Int16")
  {
    threshold->setArrayPath(matrixPath.createChildPath("int16"));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  }
  SECTION("Int32")
  {
    threshold->setArrayPath(matrixPath.createChildPath("int32"));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  }
  SECTION("Int64")
  {
    threshold->setArrayPath(matrixPath.createChildPath("int64"));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  }
  // Unsigned
  SECTION("UInt8")
  {
    threshold->setArrayPath(matrixPath.createChildPath("uint8"));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  }
  SECTION("UInt16")
  {
    threshold->setArrayPath(matrixPath.createChildPath("uint16"));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  }
  SECTION("UInt32")
  {
    threshold->setArrayPath(matrixPath.createChildPath("uint32"));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  }
  SECTION("UInt64")
  {
    threshold->setArrayPath(matrixPath.createChildPath("uint64"));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  }
  // Floating Point
  SECTION("Float32")
  {
    threshold->setArrayPath(matrixPath.createChildPath("float32"));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  }
  SECTION("Float64")
  {
    threshold->setArrayPath(matrixPath.createChildPath("float64"));
    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  }
  // Bool
  SECTION("Boolean")
  {
    threshold->setArrayPath(matrixPath.createChildPath("bool"));
    comparisonValue = 0.9;
    threshold->setComparisonValue(comparisonValue);
    args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    isBoolInput = true;
  }

  args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::int8));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  auto* maskArray = dataStructure.getDataAs<Int8Array>(matrixPath.createChildPath(k_ThresholdArrayName));
  REQUIRE(maskArray != nullptr);
  auto& maskStore = maskArray->getDataStoreRef();
  // Bool input
  if(isBoolInput)
  {
    TestMaskOutputForBoolInputType(maskStore, comparisonValue);
  }
  else
  {
    TestMaskOutputForInputType(maskStore, comparisonValue);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::MultiThresholdObjectsFilter: SIMPL Backwards Compatibility", "[SimplnxCore][MultiThresholdObjectsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "MultiThresholdObjectsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "MultiThresholdObjectsFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<MultiThresholdObjectsFilter>::uuid);

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<std::string>(MultiThresholdObjectsFilter::k_CreatedDataName_Key) == "TestName");
    }
  }
}

TEMPLATE_TEST_CASE("SimplnxCore::MultiThresholdObjects: Valid Execution - Custom Values", "[SimplnxCore][MultiThresholdObjectsFilter]", int8, uint8, int16, uint16, int32, uint32, int64, uint64,
                   float32, float64)
{
  UnitTest::LoadPlugins();

  MultiThresholdObjectsFilter filter;
  DataStructure dataStructure = CreateTestDataStructure();
  Arguments args;

  float64 trueValue = 25;
  float64 falseValue = 10;
  const int32 comparisonValue = 3;

  ArrayThresholdSet thresholdSet;
  auto threshold = std::make_shared<ArrayThreshold>();
  threshold->setArrayPath(k_TestArrayIntPath);
  threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
  threshold->setComparisonValue(comparisonValue);
  thresholdSet.setArrayThresholds({threshold});

  args.insertOrAssign(MultiThresholdObjectsFilter::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_UseCustomTrueValue, std::make_any<bool>(true));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CustomTrueValue, std::make_any<float64>(trueValue));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_UseCustomFalseValue, std::make_any<bool>(true));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CustomFalseValue, std::make_any<float64>(falseValue));
  args.insertOrAssign(MultiThresholdObjectsFilter::k_CreatedMaskType_Key, std::make_any<DataType>(GetDataType<TestType>()));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CheckArraysInheritTupleDims(dataStructure);

  auto* thresholdArray = dataStructure.getDataAs<DataArray<TestType>>(k_ThresholdArrayPath);
  REQUIRE(thresholdArray != nullptr);
  auto& thresholdStore = thresholdArray->getDataStoreRef();

  // Use tuple count constant in case the underlying data size changes.
  for(usize i = 0; i < k_TupleCount; i++)
  {
    if(i <= comparisonValue)
    {
      REQUIRE(thresholdStore[i] == falseValue);
    }
    else
    {
      REQUIRE(thresholdStore[i] == trueValue);
    }
  }
}
