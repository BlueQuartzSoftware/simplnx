/**
 * This file is auto generated from the original Core/ImportCSVData
 * runtime information. These are the steps that need to be taken to utilize this
 * unit test in the proper way.
 *
 * 1: Validate each of the default parameters that gets created.
 * 2: Inspect the actual filter to determine if the filter in its default state
 * would pass or fail BOTH the preflight() and execute() methods
 * 3: UPDATE the ```REQUIRE(result.result.valid());``` code to have the proper
 *
 * 4: Add additional unit tests to actually test each code path within the filter
 *
 * There are some example Catch2 ```TEST_CASE``` sections for your inspiration.
 *
 * NOTE the format of the ```TEST_CASE``` macro. Please stick to this format to
 * allow easier parsing of the unit tests.
 *
 * When you start working on this unit test remove "[ImportCSVData][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include <fstream>

#include "complex/DataStructure/DataArray.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/ImportCSVDataParameter.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/CreateDataGroup.hpp"
#include "ComplexCore/Filters/ImportCSVDataFilter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/StringUtilities.hpp"

namespace fs = std::filesystem;
using namespace complex;

namespace
{
const fs::path k_TestInput = fs::path(unit_test::k_BinaryDir.view()) / "ImportCSVDataTest" / "Input.txt";
}

// -----------------------------------------------------------------------------
template <typename A, typename B, typename C>
void CreateTestDataFile(const nonstd::span<A>& col1Values, const nonstd::span<B>& col2Values, const nonstd::span<C>& col3Values, const std::vector<std::string>& headers)
{
  REQUIRE(col1Values.size() == col2Values.size());
  REQUIRE(col2Values.size() == col3Values.size());
  REQUIRE(headers.size() == 3);

  std::ofstream file(k_TestInput.string(), std::ios::out);
  REQUIRE(file.is_open());

  file << StringUtilities::join(headers, ',') << "\n";

  usize rowCount = col1Values.size();
  for(int i = 0; i < rowCount; i++)
  {
    file << std::to_string(col1Values[i]) << ", " << std::to_string(col2Values[i]) << ", " << std::to_string(col3Values[i]) << "\n";
  }

  file.close();
}

TEST_CASE("Core::ImportCSVData: Valid filter execution")
{
  std::string newGroupName = "New Group";
  std::string dummyGroupName = "Dummy Group";

  std::string array1Name = "Array 1";
  DataPath array1Path = DataPath({newGroupName, array1Name});
  std::string array2Name = "Array 2";
  DataPath array2Path = DataPath({newGroupName, array2Name});
  std::string array3Name = "Array 3";
  DataPath array3Path = DataPath({newGroupName, array3Name});

  // Instantiate the filter, a DataStructure object and an Arguments Object
  DataStructure dataStructure;

  // Create a dummy group so that Existing Group parameter doesn't error out.  This should be removed once
  // disabled linked parameters are no longer automatically validated!
  {
    CreateDataGroup filter;
    Arguments args;

    args.insertOrAssign(CreateDataGroup::k_DataObjectPath, std::make_any<DataPath>(DataPath({dummyGroupName})));

    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  ImportCSVDataFilter filter;
  Arguments args;

  auto col1Values = std::vector<int8>{3, 12, 4, 18, 31, 62, 127, -128};
  auto col2Values = std::vector<uint64>{7, 0, 13000, 95, 745, 356, 12000, 7034};
  auto col3Values = std::vector<float32>{3.23, 9.57, 134.677, -25.954, 0.001, 89.12, -2.67, 92.2};

  // Create the test input data file
  fs::create_directories(k_TestInput.parent_path());
  CreateTestDataFile<int8, uint64, float32>(col1Values, col2Values, col3Values, {array1Name, array2Name, array3Name});

  CSVWizardData data;
  data.inputFilePath = k_TestInput.string();
  data.dataHeaders = {array1Name, array2Name, array3Name};
  data.dataTypes = {DataType::int8, DataType::uint64, DataType::float32};
  data.beginIndex = 2;
  data.commaAsDelimiter = true;
  data.delimiters = {','};
  data.headerLine = 1;
  data.numberOfLines = 9;

  args.insertOrAssign(ImportCSVDataFilter::k_WizardData_Key, std::make_any<CSVWizardData>(data));
  args.insertOrAssign(ImportCSVDataFilter::k_TupleDims_Key, std::make_any<DynamicTableData>(DynamicTableData{DynamicTableData::TableDataType{{8}}, {""}, {""}}));
  args.insertOrAssign(ImportCSVDataFilter::k_UseExistingGroup_Key, std::make_any<bool>(false));
  args.insertOrAssign(ImportCSVDataFilter::k_CreatedDataGroup_Key, std::make_any<DataPath>(DataPath({newGroupName})));
  args.insertOrAssign(ImportCSVDataFilter::k_SelectedDataGroup_Key, std::make_any<DataPath>(DataPath({dummyGroupName})));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  // Check the results
  const Int8Array* array1 = dataStructure.getDataAs<Int8Array>(array1Path);
  REQUIRE(array1 != nullptr);
  const UInt64Array* array2 = dataStructure.getDataAs<UInt64Array>(array2Path);
  REQUIRE(array2 != nullptr);
  const Float32Array* array3 = dataStructure.getDataAs<Float32Array>(array3Path);
  REQUIRE(array3 != nullptr);

  REQUIRE(col1Values.size() == array1->getSize());
  for(int i = 0; i < col1Values.size(); i++)
  {
    const auto& exemplaryValue = col1Values[i];
    const auto& testValue = array1->at(i);
    REQUIRE(testValue == exemplaryValue);
  }

  REQUIRE(col2Values.size() == array2->getSize());
  for(int i = 0; i < col2Values.size(); i++)
  {
    const auto& exemplaryValue = col2Values[i];
    const auto& testValue = array2->at(i);
    REQUIRE(testValue == exemplaryValue);
  }

  REQUIRE(col3Values.size() == array3->getSize());
  for(int i = 0; i < col3Values.size(); i++)
  {
    const auto& exemplaryValue = col3Values[i];
    const auto& testValue = array3->at(i);
    REQUIRE(testValue == Approx(exemplaryValue));
  }
}
