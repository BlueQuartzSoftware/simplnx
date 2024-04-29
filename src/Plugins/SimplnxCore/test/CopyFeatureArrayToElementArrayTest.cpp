#include "SimplnxCore/Filters/CopyFeatureArrayToElementArrayFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

namespace
{
const std::string k_CellFeatureIdsArrayName("FeatureIds");
const std::string k_FeatureTemperatureName("Feature Temperature");
const std::string k_FeatureDataArrayName("Feature Data Array");
const std::string k_CellTempArraySuffix("_ToCell");
const DataPath k_CellTempArrayPath({k_FeatureTemperatureName + k_CellTempArraySuffix});
const DataPath k_CellFeatureArrayPath({k_FeatureDataArrayName + k_CellTempArraySuffix});
} // namespace

TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: Parameter Check", "[Core][CopyFeatureArrayToElementArrayFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  CopyFeatureArrayToElementArrayFilter filter;
  DataStructure dataStructure;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_SelectedFeatureArrayPath_Key, std::make_any<std::vector<DataPath>>(std::vector<DataPath>{}));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CreatedArraySuffix_Key, std::make_any<StringParameter::ValueType>(""));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  for(const Error& err : preflightResult.outputActions.errors())
  {
    REQUIRE(err.code == nx::core::FilterParameter::Constants::k_Validate_Empty_Value);
  }

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
  REQUIRE(executeResult.result.errors().size() == 1);
  for(const Error& err : executeResult.result.errors())
  {
    REQUIRE(err.code == nx::core::FilterParameter::Constants::k_Validate_Empty_Value);
  }
}

using ListOfTypes = std::tuple<int8, uint8, int16, uint16, int32, uint32, int64, uint64, float32, float64>;
TEMPLATE_LIST_TEST_CASE("SimplnxCore::CopyFeatureArrayToElementArrayFilter: Valid filter execution", "[Core][CopyFeatureArrayToElementArrayFilter]", ListOfTypes)
{
  DataStructure dataStructure;

  // Create Cell FeatureIds array
  Int32Array* cellFeatureIdsPtr = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_CellFeatureIdsArrayName, {{10, 3}}, {1});
  REQUIRE(cellFeatureIdsPtr != nullptr);
  Int32Array& cellFeatureIds = *cellFeatureIdsPtr;

  for(usize y = 0; y < 3; y++)
  {
    for(usize x = 0; x < 10; x++)
    {
      usize index = (10 * y) + x;
      cellFeatureIds[index] = static_cast<int32>(y);
    }
  }

  // Create a feature data array with 3 values
  DataArray<TestType>* avgTempValuePtr = DataArray<TestType>::template CreateWithStore<DataStore<TestType>>(dataStructure, k_FeatureTemperatureName, {3}, {1});
  REQUIRE(avgTempValuePtr != nullptr);
  DataArray<TestType>& avgTempValue = *avgTempValuePtr;
  DataArray<TestType>* featureDataPtr = DataArray<TestType>::template CreateWithStore<DataStore<TestType>>(dataStructure, k_FeatureDataArrayName, {3}, {1});
  REQUIRE(featureDataPtr != nullptr);
  DataArray<TestType>& featureDataValue = *featureDataPtr;

  for(int i = 0; i < 3; i++)
  {
    avgTempValue[i] = static_cast<TestType>(0);
  }

  // Create filter and set arguments
  CopyFeatureArrayToElementArrayFilter filter;
  Arguments args;

  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_SelectedFeatureArrayPath_Key,
                      std::make_any<std::vector<DataPath>>(std::vector<DataPath>{DataPath({k_FeatureTemperatureName}), DataPath({k_FeatureDataArrayName})}));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({k_CellFeatureIdsArrayName})));
  args.insertOrAssign(CopyFeatureArrayToElementArrayFilter::k_CreatedArraySuffix_Key, std::make_any<StringParameter::ValueType>(k_CellTempArraySuffix));

  // Preflight the filter
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Check the filter results
  auto& createdElementTempArray = dataStructure.getDataRefAs<DataArray<TestType>>(k_CellTempArrayPath);
  auto& createdElementFeatureArray = dataStructure.getDataRefAs<DataArray<TestType>>(k_CellFeatureArrayPath);
  REQUIRE(createdElementTempArray.getNumberOfTuples() == createdElementFeatureArray.getNumberOfTuples());
  for(usize i = 0; i < createdElementTempArray.getNumberOfTuples(); i++)
  {
    int32 featureId = cellFeatureIds[i];
    TestType value1 = createdElementTempArray[i];
    TestType value2 = createdElementFeatureArray[i];
    TestType featureValue1 = avgTempValue[featureId];
    TestType featureValue2 = featureDataValue[featureId];
    REQUIRE(value1 == featureValue1);
    REQUIRE(value2 == featureValue2);
  }
}
