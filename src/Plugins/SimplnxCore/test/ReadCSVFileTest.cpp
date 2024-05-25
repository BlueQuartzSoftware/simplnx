#include "SimplnxCore/Filters/CreateDataGroupFilter.hpp"
#include "SimplnxCore/Filters/ReadCSVFileFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/ReadCSVFileParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <catch2/catch.hpp>

#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const fs::path k_TestInput = fs::path(unit_test::k_BinaryDir.view()) / "ReadCSVFileTest" / "Input.txt";
constexpr int32 k_InvalidArgumentErrorCode = -10351;
constexpr int32 k_OverflowErrorCode = -10353;
constexpr int32 k_BlankLineErrorCode = -119;
constexpr int32 k_EmptyFile = -100;
constexpr int32 k_InconsistentCols = -104;
constexpr int32 k_DuplicateNames = -105;
constexpr int32 k_InvalidArrayType = -106;
constexpr int32 k_IllegalNames = -107;
constexpr int32 k_IncorrectDataTypeCount = -109;
constexpr int32 k_IncorrectMaskCount = -110;
constexpr int32 k_IncorrectTuples = -113;
constexpr int32 k_EmptyNames = -116;
constexpr int32 k_HeaderLineOutOfRange = -120;
constexpr int32 k_StartImportRowOutOfRange = -121;
constexpr int32 k_EmptyHeaders = -122;
constexpr int32 k_FileDoesNotExist = -300;
} // namespace

// -----------------------------------------------------------------------------
void CreateTestDataFile(const fs::path& inputFilePath, nonstd::span<std::string> colValues, const std::vector<std::string>& headers)
{
  if(fs::exists(inputFilePath))
  {
    fs::remove(inputFilePath);
  }

  std::ofstream file(inputFilePath);
  REQUIRE(file.is_open());

  for(int i = 0; i < headers.size(); i++)
  {
    file << headers[i];
    if(i < headers.size() - 1)
    {
      file << ",";
    }
  }

  file << "\n";

  usize rowCount = colValues.size();
  for(int i = 0; i < rowCount; i++)
  {
    for(int j = 0; j < headers.size(); j++)
    {
      file << colValues[i];
      if(j < headers.size() - 1)
      {
        file << ",";
      }
    }

    if(i < rowCount - 1)
    {
      file << "\n";
    }
  }
}

/**
 *
 * @param inputFilePath
 * @param startImportRow
 * @param headerMode
 * @param headersLine
 * @param delimiters
 * @param customHeaders
 * @param dataTypes
 * @param skippedArrayMask
 * @param tupleDims
 * @param values
 * @param newGroupName
 * @return
 */
Arguments createArguments(const std::string& inputFilePath, usize startImportRow, ReadCSVData::HeaderMode headerMode, usize headersLine, const std::vector<char>& delimiters,
                          const std::vector<std::string>& customHeaders, const std::vector<DataType>& dataTypes, const std::vector<bool>& skippedArrayMask, const std::vector<usize>& tupleDims,
                          nonstd::span<std::string> values, const std::string& newGroupName)
{
  Arguments args;

  ReadCSVData data;
  data.inputFilePath = inputFilePath;
  data.customHeaders = customHeaders;
  data.dataTypes = dataTypes;
  data.startImportRow = startImportRow;
  data.delimiters = delimiters;
  data.headersLine = headersLine;
  data.headerMode = headerMode;
  data.tupleDims = tupleDims;
  data.skippedArrayMask = skippedArrayMask;

  args.insertOrAssign(ReadCSVFileFilter::k_ReadCSVData_Key, std::make_any<ReadCSVData>(data));
  args.insertOrAssign(ReadCSVFileFilter::k_UseExistingGroup_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadCSVFileFilter::k_CreatedDataGroup_Key, std::make_any<DataPath>(DataPath({newGroupName})));

  return args;
}

// -----------------------------------------------------------------------------
template <typename T>
void TestCase_TestPrimitives(nonstd::span<std::string> values)
{
  INFO(fmt::format("T = {}", DataTypeToString(GetDataType<T>())))
  INFO(fmt::format("Values = {}", values))

  std::string newGroupName = "New Group";

  std::string arrayName = "Array";
  DataPath arrayPath = DataPath({newGroupName, arrayName});

  ReadCSVFileFilter filter;
  DataStructure dataStructure;
  Arguments args =
      createArguments(k_TestInput.string(), 2, ReadCSVData::HeaderMode::LINE, 1, {','}, {arrayName}, {GetDataType<T>()}, {false}, {static_cast<usize>(values.size())}, values, newGroupName);

  // Create the test input data file
  CreateTestDataFile(k_TestInput, values, {arrayName});

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Check the results
  const DataArray<T>* array = dataStructure.getDataAs<DataArray<T>>(arrayPath);
  REQUIRE(array != nullptr);

  REQUIRE(values.size() == array->getSize());
  for(int i = 0; i < values.size(); i++)
  {
    Result<T> parseResult = ConvertTo<T>::convert(values[i]);
    SIMPLNX_RESULT_REQUIRE_VALID(parseResult);
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

  std::string arrayName = "Array";
  DataPath arrayPath = DataPath({newGroupName, arrayName});

  usize tupleCount = std::count_if(values.begin(), values.end(), [](const std::string& s) { return !s.empty(); });

  ReadCSVFileFilter filter;
  DataStructure dataStructure;
  Arguments args = createArguments(k_TestInput.string(), 2, ReadCSVData::HeaderMode::LINE, 1, {','}, {arrayName}, {GetDataType<T>()}, {false}, {tupleCount}, values, newGroupName);

  // Create the test input data file
  fs::create_directories(k_TestInput.parent_path());
  CreateTestDataFile(k_TestInput, values, {arrayName});

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors()[0].code == expectedErrorCode);
}

// -----------------------------------------------------------------------------
void TestCase_TestImporterData_Error(const std::string& inputFilePath, usize startImportRow, ReadCSVData::HeaderMode headerMode, usize headersLine, const std::vector<char>& delimiters,
                                     const std::vector<std::string>& headers, const std::vector<DataType>& dataTypes, const std::vector<bool>& skippedArrayMask, const std::vector<usize>& tupleDims,
                                     nonstd::span<std::string> values, int32 expectedErrorCode)
{
  std::string newGroupName = "New Group";
  ReadCSVFileFilter filter;
  DataStructure dataStructure;
  Arguments args = createArguments(inputFilePath, startImportRow, headerMode, headersLine, delimiters, headers, dataTypes, skippedArrayMask, tupleDims, values, newGroupName);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  if(expectedErrorCode == 0)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors().size() == 1);
    REQUIRE(executeResult.result.errors()[0].code == expectedErrorCode);
  }
}

TEST_CASE("SimplnxCore::ReadCSVFileFilter (Case 1): Valid filter execution")
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

  v = {std::to_string(std::numeric_limits<bool>::min()), std::to_string(std::numeric_limits<bool>::max())};
  TestCase_TestPrimitives<bool>(v);
}

TEST_CASE("SimplnxCore::ReadCSVFileFilter (Case 2): Valid filter execution - Skipped Array")
{
  std::string newGroupName = "New Group";

  std::string arrayName = "Array";
  DataPath arrayPath = DataPath({newGroupName, arrayName});

  ReadCSVFileFilter filter;
  DataStructure dataStructure;
  std::vector<std::string> values = {"0"};
  Arguments args = createArguments(k_TestInput.string(), 2, ReadCSVData::HeaderMode::LINE, 1, {','}, {arrayName}, {DataType::int8}, {true}, {static_cast<usize>(values.size())}, values, newGroupName);

  // Create the test input data file
  CreateTestDataFile(k_TestInput, values, {arrayName});

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Check that the array does not exist
  const IDataArray* array = dataStructure.getDataAs<IDataArray>(arrayPath);
  REQUIRE(array == nullptr);
}

TEST_CASE("SimplnxCore::ReadCSVFileFilter (Case 3): Invalid filter execution - Out of Bounds")
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

TEST_CASE("SimplnxCore::ReadCSVFileFilter (Case 4): Invalid filter execution - Invalid arguments")
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

TEST_CASE("SimplnxCore::ReadCSVFileFilter (Case 5): Invalid filter execution - Invalid ReadCSVData values")
{
  std::vector<std::string> v = {std::to_string(std::numeric_limits<int8>::min()), std::to_string(std::numeric_limits<int8>::max())};
  fs::create_directories(k_TestInput.parent_path());
  CreateTestDataFile(k_TestInput, v, {"Array"});
  std::vector<usize> tupleDims = {static_cast<usize>(v.size())};

  // Empty input file path
  TestCase_TestImporterData_Error("", 2, ReadCSVData::HeaderMode::LINE, 1, {','}, {}, {DataType::int8}, {false}, tupleDims, v, k_EmptyFile);

  // Input file does not exist
  fs::path tmp_file = fs::temp_directory_path() / "ThisFileDoesNotExist.txt";
  TestCase_TestImporterData_Error(tmp_file.string(), 2, ReadCSVData::HeaderMode::LINE, 1, {','}, {}, {DataType::int8}, {false}, tupleDims, v, k_FileDoesNotExist);

  // Start Import Row Out-of-Range
  TestCase_TestImporterData_Error(k_TestInput.string(), 0, ReadCSVData::HeaderMode::LINE, 1, {','}, {}, {DataType::int8}, {false}, tupleDims, v, k_StartImportRowOutOfRange);
  TestCase_TestImporterData_Error(k_TestInput.string(), 500, ReadCSVData::HeaderMode::LINE, 1, {','}, {}, {DataType::int8}, {false}, tupleDims, v, k_StartImportRowOutOfRange);

  // Header Line Number Out-of-Range
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::LINE, 0, {','}, {}, {DataType::int8}, {false}, tupleDims, v, k_HeaderLineOutOfRange);
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::LINE, 600, {','}, {}, {DataType::int8}, {false}, tupleDims, v, k_HeaderLineOutOfRange);
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::LINE, 3, {','}, {}, {DataType::int8}, {false}, tupleDims, v, k_HeaderLineOutOfRange);

  // Empty array headers
  tmp_file = fs::temp_directory_path() / "BlankLines.txt";
  v = {std::to_string(std::numeric_limits<int8>::min()), "", std::to_string(std::numeric_limits<int8>::max())};
  CreateTestDataFile(tmp_file, v, {"Array"});
  TestCase_TestImporterData_Error(tmp_file.string(), 4, ReadCSVData::HeaderMode::LINE, 3, {','}, {}, {DataType::int8}, {false}, {static_cast<usize>(v.size())}, v, k_EmptyHeaders);
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, {}, {DataType::int8}, {false}, {static_cast<usize>(v.size())}, v, k_EmptyHeaders);
  fs::remove(tmp_file);
  v = {std::to_string(std::numeric_limits<int8>::min()), std::to_string(std::numeric_limits<int8>::max())};

  // Incorrect Data Type Count
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::LINE, 1, {','}, {}, {}, {false}, tupleDims, v, k_IncorrectDataTypeCount);
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::LINE, 1, {','}, {}, {DataType::int8, DataType::int32}, {false}, tupleDims, v, k_IncorrectDataTypeCount);
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, {"Custom Array"}, {}, {false}, tupleDims, v, k_IncorrectDataTypeCount);
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, {"Custom Array"}, {DataType::int8, DataType::int32}, {false}, tupleDims, v,
                                  k_IncorrectDataTypeCount);

  // Incorrect Skipped Array Mask Count
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::LINE, 1, {','}, {}, {DataType::int8}, {}, tupleDims, v, k_IncorrectMaskCount);
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::LINE, 1, {','}, {}, {DataType::int8}, {false, false}, tupleDims, v, k_IncorrectMaskCount);
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, {"Custom Array"}, {DataType::int8}, {}, tupleDims, v, k_IncorrectMaskCount);
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, {"Custom Array"}, {DataType::int8}, {false, false}, tupleDims, v, k_IncorrectMaskCount);

  // Empty Header Names
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, {""}, {DataType::int8}, {false}, tupleDims, v, k_EmptyNames);

  // Duplicate Header Names
  tmp_file = fs::temp_directory_path() / "DuplicateHeaders.txt";
  std::vector<std::string> duplicateHeaders = {"Custom Array", "Custom Array"};
  CreateTestDataFile(tmp_file, v, duplicateHeaders);
  TestCase_TestImporterData_Error(tmp_file.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, duplicateHeaders, {DataType::int8, DataType::int8}, {false, false}, tupleDims, v, k_DuplicateNames);
  fs::remove(tmp_file);

  // Illegal Header Names
  tmp_file = fs::temp_directory_path() / "IllegalHeaders.txt";

  std::vector<std::string> illegalHeaders = {"Illegal/Header"};
  CreateTestDataFile(tmp_file, v, illegalHeaders);
  TestCase_TestImporterData_Error(tmp_file.string(), 2, ReadCSVData::HeaderMode::LINE, 1, {','}, {}, {DataType::int8}, {false}, tupleDims, v, 0);
  TestCase_TestImporterData_Error(tmp_file.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, illegalHeaders, {DataType::int8}, {false}, tupleDims, v, 0);

  illegalHeaders = {"Illegal\\Header"};
  CreateTestDataFile(tmp_file, v, illegalHeaders);
  TestCase_TestImporterData_Error(tmp_file.string(), 2, ReadCSVData::HeaderMode::LINE, 1, {','}, {}, {DataType::int8}, {false}, tupleDims, v, 0);
  TestCase_TestImporterData_Error(tmp_file.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, illegalHeaders, {DataType::int8}, {false}, tupleDims, v, 0);

  illegalHeaders = {"Illegal&Header"};
  CreateTestDataFile(tmp_file, v, illegalHeaders);
  TestCase_TestImporterData_Error(tmp_file.string(), 2, ReadCSVData::HeaderMode::LINE, 1, {','}, {}, {DataType::int8}, {false}, tupleDims, v, 0);
  TestCase_TestImporterData_Error(tmp_file.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, illegalHeaders, {DataType::int8}, {false}, tupleDims, v, 0);

  illegalHeaders = {"Illegal:Header"};
  CreateTestDataFile(tmp_file, v, illegalHeaders);
  TestCase_TestImporterData_Error(tmp_file.string(), 2, ReadCSVData::HeaderMode::LINE, 1, {','}, {}, {DataType::int8}, {false}, tupleDims, v, 0);
  TestCase_TestImporterData_Error(tmp_file.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, illegalHeaders, {DataType::int8}, {false}, tupleDims, v, 0);

  fs::remove(tmp_file);

  // Incorrect Tuple Dimensions
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, {"Custom Array"}, {DataType::int8}, {false}, {0}, v, k_IncorrectTuples);
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, {"Custom Array"}, {DataType::int8}, {false}, {30}, v, k_IncorrectTuples);
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, {"Custom Array"}, {DataType::int8}, {false}, {30, 2}, v, k_IncorrectTuples);
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, {"Custom Array"}, {DataType::int8}, {false}, {30, 5, 7}, v, k_IncorrectTuples);

  // Inconsistent Columns
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, {"Custom Array", "Custom Array2"}, {DataType::int8, DataType::int8}, {false, false}, tupleDims, v,
                                  k_InconsistentCols);
}

TEST_CASE("SimplnxCore::ReadCSVFileFilter (Case 6): Invalid filter execution - Blank Lines")
{
  // Create the parent directory path
  fs::create_directories(k_TestInput.parent_path());

  // First line blank tests
  std::vector<std::string> v = {"", std::to_string(std::numeric_limits<int8>::min()), std::to_string(std::numeric_limits<int8>::max())};
  TestCase_TestPrimitives_Error<int8>(v, k_BlankLineErrorCode);

  v = {"", std::to_string(std::numeric_limits<int16>::min()), std::to_string(std::numeric_limits<int16>::max())};
  TestCase_TestPrimitives_Error<int16>(v, k_BlankLineErrorCode);

  v = {"", std::to_string(std::numeric_limits<int32>::min()), std::to_string(std::numeric_limits<int32>::max())};
  TestCase_TestPrimitives_Error<int32>(v, k_BlankLineErrorCode);

  v = {"", std::to_string(std::numeric_limits<int64>::min()), std::to_string(std::numeric_limits<int64>::max())};
  TestCase_TestPrimitives_Error<int64>(v, k_BlankLineErrorCode);

  v = {"", std::to_string(std::numeric_limits<uint8>::min()), std::to_string(std::numeric_limits<uint8>::max())};
  TestCase_TestPrimitives_Error<uint8>(v, k_BlankLineErrorCode);

  v = {"", std::to_string(std::numeric_limits<uint16>::min()), std::to_string(std::numeric_limits<uint16>::max())};
  TestCase_TestPrimitives_Error<uint16>(v, k_BlankLineErrorCode);

  v = {"", std::to_string(std::numeric_limits<uint32>::min()), std::to_string(std::numeric_limits<uint32>::max())};
  TestCase_TestPrimitives_Error<uint32>(v, k_BlankLineErrorCode);

  v = {"", std::to_string(std::numeric_limits<uint64>::min()), std::to_string(std::numeric_limits<uint64>::max())};
  TestCase_TestPrimitives_Error<uint64>(v, k_BlankLineErrorCode);

  v = {"", std::to_string(std::numeric_limits<float32>::min()), std::to_string(std::numeric_limits<float32>::max())};
  TestCase_TestPrimitives_Error<float32>(v, k_BlankLineErrorCode);

  v = {"", std::to_string(std::numeric_limits<float64>::min()), std::to_string(std::numeric_limits<float64>::max())};
  TestCase_TestPrimitives_Error<float64>(v, k_BlankLineErrorCode);

  // Middle line blank tests
  v = {std::to_string(std::numeric_limits<int8>::min()), "", std::to_string(std::numeric_limits<int8>::max())};
  TestCase_TestPrimitives_Error<int8>(v, k_BlankLineErrorCode);

  v = {std::to_string(std::numeric_limits<int16>::min()), "", std::to_string(std::numeric_limits<int16>::max())};
  TestCase_TestPrimitives_Error<int16>(v, k_BlankLineErrorCode);

  v = {std::to_string(std::numeric_limits<int32>::min()), "", std::to_string(std::numeric_limits<int32>::max())};
  TestCase_TestPrimitives_Error<int32>(v, k_BlankLineErrorCode);

  v = {std::to_string(std::numeric_limits<int64>::min()), "", std::to_string(std::numeric_limits<int64>::max())};
  TestCase_TestPrimitives_Error<int64>(v, k_BlankLineErrorCode);

  v = {std::to_string(std::numeric_limits<uint8>::min()), "", std::to_string(std::numeric_limits<uint8>::max())};
  TestCase_TestPrimitives_Error<uint8>(v, k_BlankLineErrorCode);

  v = {std::to_string(std::numeric_limits<uint16>::min()), "", std::to_string(std::numeric_limits<uint16>::max())};
  TestCase_TestPrimitives_Error<uint16>(v, k_BlankLineErrorCode);

  v = {std::to_string(std::numeric_limits<uint32>::min()), "", std::to_string(std::numeric_limits<uint32>::max())};
  TestCase_TestPrimitives_Error<uint32>(v, k_BlankLineErrorCode);

  v = {std::to_string(std::numeric_limits<uint64>::min()), "", std::to_string(std::numeric_limits<uint64>::max())};
  TestCase_TestPrimitives_Error<uint64>(v, k_BlankLineErrorCode);

  v = {std::to_string(std::numeric_limits<float32>::min()), "", std::to_string(std::numeric_limits<float32>::max())};
  TestCase_TestPrimitives_Error<float32>(v, k_BlankLineErrorCode);

  v = {std::to_string(std::numeric_limits<float64>::min()), "", std::to_string(std::numeric_limits<float64>::max())};
  TestCase_TestPrimitives_Error<float64>(v, k_BlankLineErrorCode);

  v = {std::to_string(std::numeric_limits<bool>::min()), "", std::to_string(std::numeric_limits<bool>::max())};
  TestCase_TestPrimitives_Error<bool>(v, k_BlankLineErrorCode);

  // Blank lines at the end of the file are not counted in the line count
}
