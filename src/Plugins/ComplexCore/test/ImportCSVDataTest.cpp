#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/CreateDataGroup.hpp"
#include "ComplexCore/Filters/ImportCSVDataFilter.hpp"

#include "complex/Common/TypesUtility.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/ImportCSVDataParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include <catch2/catch.hpp>

#include <fstream>

namespace fs = std::filesystem;
using namespace complex;

namespace
{
const fs::path k_TestInput = fs::path(unit_test::k_BinaryDir.view()) / "ImportCSVDataTest" / "Input.txt";
constexpr int32 k_InvalidArgumentErrorCode = -100;
constexpr int32 k_OverflowErrorCode = -101;
} // namespace

// -----------------------------------------------------------------------------
void CreateTestDataFile(nonstd::span<std::string> colValues, const std::string& header)
{
  std::ofstream file(k_TestInput);
  REQUIRE(file.is_open());

  file << header << "\n";

  usize rowCount = colValues.size();
  for(int i = 0; i < rowCount; i++)
  {
    file << colValues[i] << "\n";
  }
}

// -----------------------------------------------------------------------------
DataStructure createDataStructure(const std::string& dummyGroupName)
{
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

  return dataStructure;
}

// -----------------------------------------------------------------------------
Arguments createArguments(const std::string& arrayName, DataType dataType, nonstd::span<std::string> values, const std::string& newGroupName, const std::string& dummyGroupName)
{
  Arguments args;

  CSVWizardData data;
  data.inputFilePath = k_TestInput.string();
  data.customHeaders = {arrayName};
  data.dataTypes = {dataType};
  data.startImportRow = 2;
  data.commaAsDelimiter = true;
  data.headersLine = 1;
  data.tupleDims = {static_cast<size_t>(values.size())};

  args.insertOrAssign(ImportCSVDataFilter::k_WizardData_Key, std::make_any<CSVWizardData>(data));
  args.insertOrAssign(ImportCSVDataFilter::k_UseExistingGroup_Key, std::make_any<bool>(false));
  args.insertOrAssign(ImportCSVDataFilter::k_CreatedDataGroup_Key, std::make_any<DataPath>(DataPath({newGroupName})));
  args.insertOrAssign(ImportCSVDataFilter::k_SelectedDataGroup_Key, std::make_any<DataPath>(DataPath({dummyGroupName})));

  return args;
}

// -----------------------------------------------------------------------------
template <typename T>
void TestCase_TestPrimitives(nonstd::span<std::string> values)
{
  INFO(fmt::format("T = {}", DataTypeToString(GetDataType<T>())))
  INFO(fmt::format("Values = {}", values))

  std::string newGroupName = "New Group";
  std::string dummyGroupName = "Dummy Group";

  std::string arrayName = "Array";
  DataPath arrayPath = DataPath({newGroupName, arrayName});

  ImportCSVDataFilter filter;
  DataStructure dataStructure = createDataStructure(dummyGroupName);
  Arguments args = createArguments(arrayName, GetDataType<T>(), values, newGroupName, dummyGroupName);

  // Create the test input data file
  CreateTestDataFile(values, arrayName);

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  // Check the results
  const DataArray<T>* array = dataStructure.getDataAs<DataArray<T>>(arrayPath);
  REQUIRE(array != nullptr);

  REQUIRE(values.size() == array->getSize());
  for(int i = 0; i < values.size(); i++)
  {
    Result<T> parseResult = ConvertTo<T>::convert(values[i]);
    COMPLEX_RESULT_REQUIRE_VALID(parseResult);
    const auto& exemplaryValue = parseResult.value();
    const auto& testValue = array->at(i);
    REQUIRE(testValue == exemplaryValue);
  }
}

// -----------------------------------------------------------------------------
template <typename T>
void TestCase_TestPrimitives_Error(nonstd::span<std::string> values, int32 expectedErrorCode)
{
  INFO(fmt::format("T = {}", DataTypeToString(GetDataType<T>())))
  INFO(fmt::format("Values = {}", values))

  std::string newGroupName = "New Group";
  std::string dummyGroupName = "Dummy Group";

  std::string arrayName = "Array";
  DataPath arrayPath = DataPath({newGroupName, arrayName});

  ImportCSVDataFilter filter;
  DataStructure dataStructure = createDataStructure(dummyGroupName);
  Arguments args = createArguments(arrayName, GetDataType<T>(), values, newGroupName, dummyGroupName);

  // Create the test input data file
  fs::create_directories(k_TestInput.parent_path());
  CreateTestDataFile(values, arrayName);

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors()[0].code == expectedErrorCode);
}

TEST_CASE("ComplexCore::ImportCSVDataFilter (Case 1): Valid filter execution")
{
  // Create the parent directory path
  fs::create_directories(k_TestInput.parent_path());

  std::vector<std::string> v = {std::to_string(std::numeric_limits<int8>::min()), std::to_string(std::numeric_limits<int8>::max())};
  TestCase_TestPrimitives<int8>(v);

  v = {std::to_string(std::numeric_limits<int16>::min()), std::to_string(std::numeric_limits<int16>::max())};
  TestCase_TestPrimitives<int16>(v);

  v = {std::to_string(std::numeric_limits<int32>::min()), std::to_string(std::numeric_limits<int32>::max())};
  TestCase_TestPrimitives<int32>(v);

  v = {std::to_string(std::numeric_limits<int64>::min()), std::to_string(std::numeric_limits<int64>::max())};
  TestCase_TestPrimitives<int64>(v);

  v = {std::to_string(std::numeric_limits<uint8>::min()), std::to_string(std::numeric_limits<uint8>::max())};
  TestCase_TestPrimitives<uint8>(v);

  v = {std::to_string(std::numeric_limits<uint16>::min()), std::to_string(std::numeric_limits<uint16>::max())};
  TestCase_TestPrimitives<uint16>(v);

  v = {std::to_string(std::numeric_limits<uint32>::min()), std::to_string(std::numeric_limits<uint32>::max())};
  TestCase_TestPrimitives<uint32>(v);

  v = {std::to_string(std::numeric_limits<uint64>::min()), std::to_string(std::numeric_limits<uint64>::max())};
  TestCase_TestPrimitives<uint64>(v);

  v = {std::to_string(std::numeric_limits<float32>::min()), std::to_string(std::numeric_limits<float32>::max())};
  TestCase_TestPrimitives<float32>(v);

  v = {std::to_string(std::numeric_limits<float64>::min()), std::to_string(std::numeric_limits<float64>::max())};
  TestCase_TestPrimitives<float64>(v);
}

TEST_CASE("ComplexCore::ImportCSVDataFilter (Case 2): Valid filter execution - Skipped Array")
{
  std::string newGroupName = "New Group";
  std::string dummyGroupName = "Dummy Group";

  std::string arrayName = "Array";
  DataPath arrayPath = DataPath({newGroupName, arrayName});

  ImportCSVDataFilter filter;
  DataStructure dataStructure = createDataStructure(dummyGroupName);
  std::vector<std::string> values = {"0"};
  Arguments args = createArguments(arrayName, {}, values, newGroupName, dummyGroupName);

  // Create the test input data file
  CreateTestDataFile(values, arrayName);

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  // Check that the array does not exist
  const IDataArray* array = dataStructure.getDataAs<IDataArray>(arrayPath);
  REQUIRE(array == nullptr);
}

TEST_CASE("ComplexCore::ImportCSVDataFilter (Case 3): Invalid filter execution - Out of Bounds")
{
  // Create the parent directory path
  fs::create_directories(k_TestInput.parent_path());

  // Int8 - Out of bounds
  std::vector<std::string> v = {"-129"};
  TestCase_TestPrimitives_Error<int8>(v, k_OverflowErrorCode);

  v = {"128"};
  TestCase_TestPrimitives_Error<int8>(v, k_OverflowErrorCode);

  // Int16 - Out of bounds
  v = {"-32769"};
  TestCase_TestPrimitives_Error<int16>(v, k_OverflowErrorCode);

  v = {"32768"};
  TestCase_TestPrimitives_Error<int16>(v, k_OverflowErrorCode);

  // Int32 - Out of bounds
  v = {"-2147483649"};
  TestCase_TestPrimitives_Error<int32>(v, k_OverflowErrorCode);

  v = {"2147483648"};
  TestCase_TestPrimitives_Error<int32>(v, k_OverflowErrorCode);

  // Int64 - Out of bounds
  v = {"-9223372036854775809"};
  TestCase_TestPrimitives_Error<int64>(v, k_OverflowErrorCode);

  v = {"9223372036854775808"};
  TestCase_TestPrimitives_Error<int64>(v, k_OverflowErrorCode);

  // UInt8 - Out of bounds
  v = {"-1"};
  TestCase_TestPrimitives_Error<uint8>(v, k_OverflowErrorCode);

  v = {"256"};
  TestCase_TestPrimitives_Error<uint8>(v, k_OverflowErrorCode);

  // UInt16 - Out of bounds
  v = {"-1"};
  TestCase_TestPrimitives_Error<uint16>(v, k_OverflowErrorCode);

  v = {"65536"};
  TestCase_TestPrimitives_Error<uint16>(v, k_OverflowErrorCode);

  // UInt32 - Out of bounds
  v = {"-1"};
  TestCase_TestPrimitives_Error<uint32>(v, k_OverflowErrorCode);

  v = {"4294967296"};
  TestCase_TestPrimitives_Error<uint32>(v, k_OverflowErrorCode);

  // UInt64 - Out of bounds
  v = {"-1"};
  TestCase_TestPrimitives_Error<uint64>(v, k_OverflowErrorCode);

  v = {"18446744073709551616"};
  TestCase_TestPrimitives_Error<uint64>(v, k_OverflowErrorCode);

  // Float32 - Out of bounds
  v = {"-3.5E38"};
  TestCase_TestPrimitives_Error<float32>(v, k_OverflowErrorCode);

  v = {"3.5E38"};
  TestCase_TestPrimitives_Error<float32>(v, k_OverflowErrorCode);

  // Float64 - Out of bounds
  v = {"-1.8E308"};
  TestCase_TestPrimitives_Error<float64>(v, k_OverflowErrorCode);

  v = {"1.8E308"};
  TestCase_TestPrimitives_Error<float64>(v, k_OverflowErrorCode);
}

TEST_CASE("ComplexCore::ImportCSVDataFilter (Case 4): Invalid filter execution - Invalid arguments")
{
  // Create the parent directory path
  fs::create_directories(k_TestInput.parent_path());

  std::vector<std::string> v = {" "};
  TestCase_TestPrimitives_Error<int8>(v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<int16>(v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<int32>(v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<int64>(v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<uint8>(v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<uint16>(v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<uint32>(v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<uint64>(v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<float32>(v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<float64>(v, k_InvalidArgumentErrorCode);

  v = {"a"};
  TestCase_TestPrimitives_Error<int8>(v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<int16>(v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<int32>(v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<int64>(v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<uint8>(v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<uint16>(v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<uint32>(v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<uint64>(v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<float32>(v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<float64>(v, k_InvalidArgumentErrorCode);

  v = {"&"};
  TestCase_TestPrimitives_Error<int8>(v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<int16>(v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<int32>(v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<int64>(v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<uint8>(v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<uint16>(v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<uint32>(v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<uint64>(v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<float32>(v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<float64>(v, k_InvalidArgumentErrorCode);
}
