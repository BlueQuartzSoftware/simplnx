#include <catch2/catch.hpp>

#include "ComplexCore/Filters/CopyFeatureArrayToElementArray.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"

using namespace complex;

namespace
{
const int k_EmptyArrayPath = -201;

const std::string k_CellFeatureIdsArrayName("FeatureIds");
const std::string k_CellTempArrayName("Cell Temperature");
const std::string k_FeatureDataArrayName("Feature Temperature");

template <typename T>
void TestCopyFeatureArrayToElementArrayForType()
{
  DataStructure ds;

  // Create Cell FeatureIds array
  Int32Array* cellFeatureIdsPtr = Int32Array::CreateWithStore<DataStore<int32>>(ds, k_CellFeatureIdsArrayName, {{10, 3}}, {1});
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
  DataArray<T>* avgTempValuePtr = DataArray<T>::template CreateWithStore<DataStore<T>>(ds, k_FeatureDataArrayName, {3}, {1});
  REQUIRE(avgTempValuePtr != nullptr);
  DataArray<T>& avgTempValue = *avgTempValuePtr;

  for(int i = 0; i < 3; i++)
  {
    avgTempValue[i] = static_cast<T>(0);
  }

  // Create filter and set arguments
  CopyFeatureArrayToElementArray filter;
  Arguments args;

  args.insertOrAssign(CopyFeatureArrayToElementArray::k_SelectedFeatureArrayPath_Key, std::make_any<DataPath>(DataPath({k_FeatureDataArrayName})));
  args.insertOrAssign(CopyFeatureArrayToElementArray::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({k_CellFeatureIdsArrayName})));
  args.insertOrAssign(CopyFeatureArrayToElementArray::k_CreatedArrayName_Key, std::make_any<DataPath>(DataPath({k_CellTempArrayName})));

  // Preflight the filter
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());

  // Check the filter results
  DataArray<T>& createdElementArray = ds.getDataRefAs<DataArray<T>>(DataPath({k_CellTempArrayName}));
  for(usize i = 0; i < createdElementArray.getNumberOfTuples(); i++)
  {
    int32 featureId = cellFeatureIds[i];
    T value = createdElementArray[i];
    T featureValue = avgTempValue[featureId];
    REQUIRE(value == featureValue);
  }
}
} // namespace

TEST_CASE("Core::CopyFeatureArrayToElementArray: Instantiation and Parameter Check", "[Core][CopyFeatureArrayToElementArray]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  CopyFeatureArrayToElementArray filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(CopyFeatureArrayToElementArray::k_SelectedFeatureArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CopyFeatureArrayToElementArray::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CopyFeatureArrayToElementArray::k_CreatedArrayName_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.invalid());
  REQUIRE(preflightResult.outputActions.errors().size() == 3);
  for(const Error& err : preflightResult.outputActions.errors())
  {
    REQUIRE(err.code == k_EmptyArrayPath);
  }

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.invalid());
  REQUIRE(executeResult.result.errors().size() == 3);
  for(const Error& err : executeResult.result.errors())
  {
    REQUIRE(err.code == k_EmptyArrayPath);
  }
}

TEST_CASE("Core::CopyFeatureArrayToElementArray: Valid filter execution", "[Core][CopyFeatureArrayToElementArray]")
{
  TestCopyFeatureArrayToElementArrayForType<uint8>();
  TestCopyFeatureArrayToElementArrayForType<int8>();
  TestCopyFeatureArrayToElementArrayForType<uint16>();
  TestCopyFeatureArrayToElementArrayForType<int16>();
  TestCopyFeatureArrayToElementArrayForType<uint32>();
  TestCopyFeatureArrayToElementArrayForType<int32>();
  TestCopyFeatureArrayToElementArrayForType<uint64>();
  TestCopyFeatureArrayToElementArrayForType<int64>();
  TestCopyFeatureArrayToElementArrayForType<float32>();
  TestCopyFeatureArrayToElementArrayForType<float64>();
}
