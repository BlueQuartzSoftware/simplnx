#include "ComplexCore/Filters/MultiThresholdObjects.hpp"
#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace complex;
using namespace complex::Constants;

namespace
{
const std::string k_TestArrayFloatName = "TestArrayFloat";
const std::string k_TestArrayIntName = "TestArrayInt";
const std::string k_ThresholdArrayName = "ThresholdArray";

const DataPath k_ImageCellDataPath({k_ImageGeometry, k_CellData});
const DataPath k_TestArrayFloatPath = k_ImageCellDataPath.createChildPath(k_TestArrayFloatName);
const DataPath k_TestArrayIntPath = k_ImageCellDataPath.createChildPath(k_TestArrayIntName);
const DataPath k_ThresholdArrayPath = k_ImageCellDataPath.createChildPath(k_ThresholdArrayName);

const DataPath k_MultiComponentArrayPath = k_ImageCellDataPath.createChildPath("MultiComponentArray");
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
} // namespace

TEST_CASE("ComplexCore::MultiThresholdObjects: Valid Execution", "[ComplexCore][MultiThresholdObjects]")
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

    args.insertOrAssign(MultiThresholdObjects::k_ArrayThresholds_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedDataPath_Key, std::make_any<std::string>(k_ThresholdArrayName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

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

    args.insertOrAssign(MultiThresholdObjects::k_ArrayThresholds_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedDataPath_Key, std::make_any<std::string>(k_ThresholdArrayName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

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

TEST_CASE("ComplexCore::MultiThresholdObjects: Invalid Execution", "[ComplexCore][MultiThresholdObjects]")
{
  MultiThresholdObjects filter;
  DataStructure dataStructure = CreateTestDataStructure();
  Arguments args;

  SECTION("Empty ArrayThresholdSet")
  {
    ArrayThresholdSet thresholdSet;

    args.insertOrAssign(MultiThresholdObjects::k_ArrayThresholds_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedDataPath_Key, std::make_any<std::string>(k_ThresholdArrayName));
  }
  SECTION("Empty ArrayThreshold DataPath")
  {
    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjects::k_ArrayThresholds_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedDataPath_Key, std::make_any<std::string>(k_ThresholdArrayName));
  }
  SECTION("MultiComponents in Threshold Array")
  {
    ArrayThresholdSet thresholdSet;
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setArrayPath(k_MultiComponentArrayPath);
    threshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    threshold->setComparisonValue(0.1);
    thresholdSet.setArrayThresholds({threshold});

    args.insertOrAssign(MultiThresholdObjects::k_ArrayThresholds_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedDataPath_Key, std::make_any<std::string>(k_ThresholdArrayName));
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

    args.insertOrAssign(MultiThresholdObjects::k_ArrayThresholds_Key, std::make_any<ArrayThresholdSet>(thresholdSet));
    args.insertOrAssign(MultiThresholdObjects::k_CreatedDataPath_Key, std::make_any<std::string>(k_ThresholdArrayName));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(executeResult.result)
}
