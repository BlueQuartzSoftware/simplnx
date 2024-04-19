#include "SimplnxCore/Filters/MultiThresholdObjects.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
const std::string k_TestArrayFloatName = "TestArrayFloat";
const std::string k_TestArrayIntName = "TestArrayInt";
const std::string k_ThresholdArrayName = "ThresholdArray";

const DataPath k_ImageCellDataName({k_ImageGeometry, k_CellData});
const DataPath k_TestArrayFloatPath = k_ImageCellDataName.createChildPath(k_TestArrayFloatName);
const DataPath k_TestArrayIntPath = k_ImageCellDataName.createChildPath(k_TestArrayIntName);
const DataPath k_ThresholdArrayPath = k_ImageCellDataName.createChildPath(k_ThresholdArrayName);

const DataPath k_MultiComponentArrayPath = k_ImageCellDataName.createChildPath("MultiComponentArray");
const DataPath k_MismatchingTuplesArrayPath({"MismatchingTuplesArray"});

DataStructure CreateTestDataStructure()
{
  DataStructure dataStructure;
  // Create two test arrays, a float array and a int array
  // Set up geometry for tuples, a cuboid with dimensions 20, 10, 1
  ImageGeom* image = ImageGeom::Create(dataStructure, k_ImageGeometry);
  std::vector<usize> dims = {20, 1, 1};
  image->setDimensions(dims);

  std::vector<usize> tDims = {20};
  std::vector<usize> cDims = {1};
  float fnum = 0.0f;
  int inum = 0;
  AttributeMatrix* am = AttributeMatrix::Create(dataStructure, k_CellData, tDims, image->getId());
  Float32Array* data = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, k_TestArrayFloatName, tDims, cDims, am->getId());
  Int32Array* data1 = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, k_TestArrayIntName, tDims, cDims, am->getId());

  Float32Array* invalid1 = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, k_MultiComponentArrayPath.getTargetName(), tDims, std::vector<usize>{3}, am->getId());
  invalid1->fill(1.0);
  Float32Array* invalid2 = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, k_MismatchingTuplesArrayPath.getTargetName(), std::vector<usize>{10}, cDims);
  invalid2->fill(2.0);

  // Fill the float array with {.01,.02,.03,.04,.05,.06,.07,.08,.09,.10,.11,.12,.13,.14,.15.,16,.17,.18,.19,.20}
  // Fill the int array with { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19 }
  for(usize i = 0; i < 20; i++)
  {
    fnum += 0.01f;
    (*data)[i] = fnum;  // float array
    (*data1)[i] = inum; // int array
    ++inum;
  }
  return dataStructure;
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

TEST_CASE("SimplnxCore::MultiThresholdObjects: Valid Execution", "[SimplnxCore][MultiThresholdObjects]")
{
  DataStructure dataStructure = CreateTestDataStructure();

  SECTION("Float Array Threshold")
  {
    MultiThresholdObjects filter;
    Arguments args;

    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_TestArrayFloatPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjects::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::boolean));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    auto* thresholdArray = dataStructure.getDataAs<BoolArray>(k_ThresholdArrayPath);
    REQUIRE(thresholdArray != nullptr);

    // For the comparison value of 0.1, the threshold array elements 0 to 9 should be false and 10 through 19 should be true
    for(usize i = 0; i < 20; i++)
    {
      if(i < 10)
      {
        REQUIRE((*thresholdArray)[i] == false);
      }
      else
      {
        REQUIRE((*thresholdArray)[i] == true);
      }
    }
  }

  SECTION("Int Array Threshold")
  {
    MultiThresholdObjects filter;
    Arguments args;

    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_TestArrayIntPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(15);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjects::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::boolean));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    auto* thresholdArray = dataStructure.getDataAs<BoolArray>(k_ThresholdArrayPath);
    REQUIRE(thresholdArray != nullptr);

    // For the comparison value of 0.1, the threshold array elements 0 to 9 should be false and 10 through 19 should be true
    for(usize i = 0; i < 20; i++)
    {
      if(i <= 15)
      {
        REQUIRE((*thresholdArray)[i] == false);
      }
      else
      {
        REQUIRE((*thresholdArray)[i] == true);
      }
    }
  }
}

TEMPLATE_TEST_CASE("SimplnxCore::MultiThresholdObjects: Valid Execution - Custom Values", "[SimplnxCore][MultiThresholdObjects]", int8, uint8, int16, uint16, int32, uint32, int64, uint64, float32,
                   float64)
{
  MultiThresholdObjects filter;
  DataStructure dataStructure = CreateTestDataStructure();
  Arguments args;

  float64 trueValue = 25;
  float64 falseValue = 10;

  ArrayThresholdSet thresholdSet;
  auto threshold = std::make_shared<ArrayThreshold>();
  threshold->setArrayPath(k_TestArrayIntPath);
  threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
  threshold->setComparisonValue(15);
  thresholdSet.setArrayThresholds({threshold});

  args.insertOrAssign(MultiThresholdObjects::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  args.insertOrAssign(MultiThresholdObjects::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
  args.insertOrAssign(MultiThresholdObjects::k_UseCustomTrueValue, std::make_any<bool>(true));
  args.insertOrAssign(MultiThresholdObjects::k_CustomTrueValue, std::make_any<float64>(trueValue));
  args.insertOrAssign(MultiThresholdObjects::k_UseCustomFalseValue, std::make_any<bool>(true));
  args.insertOrAssign(MultiThresholdObjects::k_CustomFalseValue, std::make_any<float64>(falseValue));
  args.insertOrAssign(MultiThresholdObjects::k_CreatedMaskType_Key, std::make_any<DataType>(GetDataType<TestType>()));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  auto* thresholdArray = dataStructure.getDataAs<DataArray<TestType>>(k_ThresholdArrayPath);
  REQUIRE(thresholdArray != nullptr);

  // For the comparison value of 0.1, the threshold array elements 0 to 9 should be false and 10 through 19 should be true
  for(usize i = 0; i < 20; i++)
  {
    if(i <= 15)
    {
      REQUIRE((*thresholdArray)[i] == falseValue);
    }
    else
    {
      REQUIRE((*thresholdArray)[i] == trueValue);
    }
  }
}

TEST_CASE("SimplnxCore::MultiThresholdObjects: Invalid Execution", "[SimplnxCore][MultiThresholdObjects]")
{
  MultiThresholdObjects filter;
  DataStructure dataStructure = CreateTestDataStructure();
  Arguments args;

  SECTION("Empty ArrayThresholdSet")
  {
    ArrayThresholdSet thresholdSet;

    args.insertOrAssign(MultiThresholdObjects::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
  }
  SECTION("Empty ArrayThreshold DataPath")
  {
    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjects::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
  }
  SECTION("MultiComponents in Threshold Array")
  {
    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_MultiComponentArrayPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjects::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
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

    args.insertOrAssign(MultiThresholdObjects::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
}

TEMPLATE_TEST_CASE("SimplnxCore::MultiThresholdObjects: Invalid Execution - Out of Bounds Custom Values", "[SimplnxCore][MultiThresholdObjects]", int8, uint8, int16, uint16, int32, uint32, int64,
                   uint64, float32)
{
  MultiThresholdObjects filter;
  DataStructure dataStructure = CreateTestDataStructure();
  Arguments args;

  float64 trueValue;
  float64 falseValue;
  int32 code;

  SECTION("True Value < Minimum")
  {
    trueValue = GetOutOfBoundsMinimumValue<TestType>();
    falseValue = 1;
    code = MultiThresholdObjects::ErrorCodes::CustomTrueOutOfBounds;
  }

  SECTION("False Value < Minimum")
  {
    trueValue = 1;
    falseValue = GetOutOfBoundsMinimumValue<TestType>();
    code = MultiThresholdObjects::ErrorCodes::CustomFalseOutOfBounds;
  }

  SECTION("True Value > Maximum")
  {
    trueValue = GetOutOfBoundsMaximumValue<TestType>();
    falseValue = 1;
    code = MultiThresholdObjects::ErrorCodes::CustomTrueOutOfBounds;
  }

  SECTION("False Value > Maximum")
  {
    trueValue = 1;
    falseValue = GetOutOfBoundsMaximumValue<TestType>();
    code = MultiThresholdObjects::ErrorCodes::CustomFalseOutOfBounds;
  }

  ArrayThresholdSet thresholdSet;
  auto threshold = std::make_shared<ArrayThreshold>();
  threshold->setArrayPath(k_TestArrayIntPath);
  threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
  threshold->setComparisonValue(15);
  thresholdSet.setArrayThresholds({threshold});

  args.insertOrAssign(MultiThresholdObjects::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  args.insertOrAssign(MultiThresholdObjects::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
  args.insertOrAssign(MultiThresholdObjects::k_UseCustomTrueValue, std::make_any<bool>(true));
  args.insertOrAssign(MultiThresholdObjects::k_CustomTrueValue, std::make_any<float64>(trueValue));
  args.insertOrAssign(MultiThresholdObjects::k_UseCustomFalseValue, std::make_any<bool>(true));
  args.insertOrAssign(MultiThresholdObjects::k_CustomFalseValue, std::make_any<float64>(falseValue));
  args.insertOrAssign(MultiThresholdObjects::k_CreatedMaskType_Key, std::make_any<DataType>(GetDataType<TestType>()));

  // Preflight the filter
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == code);
}

TEST_CASE("SimplnxCore::MultiThresholdObjects: Invalid Execution - Boolean Custom Values", "[SimplnxCore][MultiThresholdObjects]")
{
  MultiThresholdObjects filter;
  DataStructure dataStructure = CreateTestDataStructure();
  Arguments args;

  int32 code;

  SECTION("Custom True Value")
  {
    code = MultiThresholdObjects::ErrorCodes::CustomTrueWithBoolean;
    args.insertOrAssign(MultiThresholdObjects::k_UseCustomTrueValue, std::make_any<bool>(true));
  }

  SECTION("Custom False Value")
  {
    code = MultiThresholdObjects::ErrorCodes::CustomFalseWithBoolean;
    args.insertOrAssign(MultiThresholdObjects::k_UseCustomFalseValue, std::make_any<bool>(true));
  }

  ArrayThresholdSet thresholdSet;
  auto threshold = std::make_shared<ArrayThreshold>();
  threshold->setArrayPath(k_TestArrayIntPath);
  threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
  threshold->setComparisonValue(15);
  thresholdSet.setArrayThresholds({threshold});

  args.insertOrAssign(MultiThresholdObjects::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
  args.insertOrAssign(MultiThresholdObjects::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
  args.insertOrAssign(MultiThresholdObjects::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::boolean));

  // Preflight the filter
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == code);
}

template <typename T>
void checkMaskValues(const DataStructure& dataStructure, const DataPath& thresholdArrayPath)
{
  auto* thresholdArrayPtr = dataStructure.getDataAs<DataArray<T>>(thresholdArrayPath);
  REQUIRE(thresholdArrayPtr != nullptr);

  auto& thresholdArray = (*thresholdArrayPtr);

  // For the comparison value of 0.1, the threshold array elements 0 to 9 should be false and 10 through 19 should be true
  for(usize i = 0; i < 20; i++)
  {
    if(i < 10)
    {
      REQUIRE(thresholdArray[i] == static_cast<T>(0));
    }
    else
    {
      REQUIRE(thresholdArray[i] == static_cast<T>(1));
    }
  }
}

TEST_CASE("SimplnxCore::MultiThresholdObjects: Valid Execution, DataType", "[SimplnxCore][MultiThresholdObjects]")
{
  DataStructure dataStructure = CreateTestDataStructure();

  // Signed
  SECTION("Int8 Threshold")
  {
    MultiThresholdObjects filter;
    Arguments args;

    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_TestArrayFloatPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjects::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::int8));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    checkMaskValues<int8>(dataStructure, k_ThresholdArrayPath);
  }

  SECTION("Int16 Threshold")
  {
    MultiThresholdObjects filter;
    Arguments args;

    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_TestArrayFloatPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjects::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::int16));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    checkMaskValues<int16>(dataStructure, k_ThresholdArrayPath);
  }

  SECTION("Int32 Threshold")
  {
    MultiThresholdObjects filter;
    Arguments args;

    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_TestArrayFloatPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjects::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::int32));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    checkMaskValues<int32>(dataStructure, k_ThresholdArrayPath);
  }

  SECTION("Int64 Threshold")
  {
    MultiThresholdObjects filter;
    Arguments args;

    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_TestArrayFloatPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjects::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::int64));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    checkMaskValues<int64>(dataStructure, k_ThresholdArrayPath);
  }

  // Unsigned
  SECTION("UInt8 Threshold")
  {
    MultiThresholdObjects filter;
    Arguments args;

    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_TestArrayFloatPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjects::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::uint8));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    checkMaskValues<uint8>(dataStructure, k_ThresholdArrayPath);
  }

  SECTION("UInt16 Threshold")
  {
    MultiThresholdObjects filter;
    Arguments args;

    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_TestArrayFloatPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjects::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::uint16));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    checkMaskValues<uint16>(dataStructure, k_ThresholdArrayPath);
  }

  SECTION("UInt32 Threshold")
  {
    MultiThresholdObjects filter;
    Arguments args;

    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_TestArrayFloatPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjects::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::uint32));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    checkMaskValues<uint32>(dataStructure, k_ThresholdArrayPath);
  }

  SECTION("UInt64 Threshold")
  {
    MultiThresholdObjects filter;
    Arguments args;

    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_TestArrayFloatPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjects::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::uint64));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    checkMaskValues<uint64>(dataStructure, k_ThresholdArrayPath);
  }

  // Floating Point
  SECTION("Float32 Threshold")
  {
    MultiThresholdObjects filter;
    Arguments args;

    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_TestArrayFloatPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjects::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::float32));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    checkMaskValues<float32>(dataStructure, k_ThresholdArrayPath);
  }

  SECTION("Float64 Threshold")
  {
    MultiThresholdObjects filter;
    Arguments args;

    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_TestArrayFloatPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjects::k_ArrayThresholdsObject_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedDataName_Key, std::make_any<std::string>(k_ThresholdArrayName));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedMaskType_Key, std::make_any<DataType>(DataType::float64));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    checkMaskValues<float64>(dataStructure, k_ThresholdArrayPath);
  }
}
