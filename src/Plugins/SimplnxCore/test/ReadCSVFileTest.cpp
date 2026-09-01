#include "SimplnxCore/Filters/CreateDataGroupFilter.hpp"
#include "SimplnxCore/Filters/ReadCSVFileFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/ReadCSVFileParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/StringInterpretationUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <regex>
#include <system_error>

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

/**
 * @class ScopedBenchmarkDirectory
 * @brief Creates and removes the ReadCSV benchmark directory.
 */
class ScopedBenchmarkDirectory
{
public:
  explicit ScopedBenchmarkDirectory(const fs::path& parentPath)
  : m_Path(parentPath / "ReadCSVFileOocBenchmark")
  {
    std::error_code errorCode;
    fs::remove_all(m_Path, errorCode);
    REQUIRE_FALSE(errorCode);

    fs::create_directories(m_Path, errorCode);
    REQUIRE_FALSE(errorCode);
  }

  ~ScopedBenchmarkDirectory() noexcept
  {
    std::error_code errorCode;
    fs::remove_all(m_Path, errorCode);
  }

  ScopedBenchmarkDirectory(const ScopedBenchmarkDirectory&) = delete;
  ScopedBenchmarkDirectory(ScopedBenchmarkDirectory&&) = delete;
  ScopedBenchmarkDirectory& operator=(const ScopedBenchmarkDirectory&) = delete;
  ScopedBenchmarkDirectory& operator=(ScopedBenchmarkDirectory&&) = delete;

  const fs::path& path() const
  {
    return m_Path;
  }

private:
  fs::path m_Path;
};

/**
 * @brief Computes the deterministic signal used by the OOC benchmark.
 * @param tupleIndex Zero-based tuple index.
 * @return Signal value in the benchmark range.
 */
uint32 ExpectedBenchmarkSignal(usize tupleIndex)
{
  return static_cast<uint32>((tupleIndex * 37ULL + 11ULL) % 8000000ULL);
}
} // namespace

/**
 * @brief Writes a comma-separated test file with one header row.
 * @param inputFilePath Output file path.
 * @param colValues Row values to repeat for each header.
 * @param headers Column names.
 */
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
 * @brief Creates ReadCSVFileFilter arguments for one test case.
 * @param inputFilePath Input CSV path.
 * @param startImportRow First data row to import.
 * @param headerMode Header interpretation mode.
 * @param headersLine Header row index.
 * @param delimiters Delimiter characters.
 * @param customHeaders Replacement header names.
 * @param dataTypes Output array types.
 * @param skippedArrayMask True for columns to skip.
 * @param tupleDims Output tuple dimensions.
 * @param values Column values used by the test file.
 * @param newGroupName Output group name.
 * @return Configured filter arguments.
 */
Arguments createArguments(const std::string& inputFilePath, usize startImportRow, ReadCSVData::HeaderMode headerMode, usize headersLine, const std::vector<char>& delimiters,
                          const std::vector<std::string>& customHeaders, const std::vector<CSVType>& dataTypes, const std::vector<bool>& skippedArrayMask, const ShapeType& tupleDims,
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

// Run primitive type cases through the selected algorithm scope.
template <typename T>
void TestCase_TestPrimitives(UnitTest::AlgorithmTestScope& scope, nonstd::span<std::string> values)
{
  INFO(fmt::format("T = {}", DataTypeToString(GetDataType<T>())))
  INFO(fmt::format("Values = {}", values))

  std::string newGroupName = "New Group";

  std::string arrayName = "Array";
  DataPath arrayPath = DataPath({newGroupName, arrayName});

  ReadCSVFileFilter filter;
  DataStructure dataStructure;
  Arguments args =
      createArguments(k_TestInput.string(), 2, ReadCSVData::HeaderMode::LINE, 1, {','}, {arrayName}, {GetCSVType<T>()}, {false}, {static_cast<usize>(values.size())}, values, newGroupName);

  // Write the test input file.
  CreateTestDataFile(k_TestInput, values, {arrayName});

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Compare the generated arrays.
  const DataArray<T>* array = dataStructure.getDataAs<DataArray<T>>(arrayPath);
  REQUIRE(array != nullptr);
  scope.requireExpectedStore(*array);

  REQUIRE(values.size() == array->getSize());
  for(int i = 0; i < values.size(); i++)
  {
    Result<T> parseResult = StringInterpretationUtilities::Convert<T>(values[i]);
    SIMPLNX_RESULT_REQUIRE_VALID(parseResult);
    const auto& exemplaryValue = parseResult.value();
    const auto& testValue = array->at(i);
    REQUIRE(testValue == exemplaryValue);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

template <typename T>
void TestCase_TestPrimitives_Error(UnitTest::AlgorithmTestScope& scope, nonstd::span<std::string> values, int32 expectedErrorCode)
{
  INFO(fmt::format("T = {}", DataTypeToString(GetDataType<T>())))
  INFO(fmt::format("Values = {}", values))

  std::string newGroupName = "New Group";

  std::string arrayName = "Array";
  DataPath arrayPath = DataPath({newGroupName, arrayName});

  usize tupleCount = std::count_if(values.begin(), values.end(), [](const std::string& s) { return !s.empty(); });

  ReadCSVFileFilter filter;
  DataStructure dataStructure;
  Arguments args = createArguments(k_TestInput.string(), 2, ReadCSVData::HeaderMode::LINE, 1, {','}, {arrayName}, {GetCSVType<T>()}, {false}, {tupleCount}, values, newGroupName);

  // Write the test input file.
  fs::create_directories(k_TestInput.parent_path());
  CreateTestDataFile(k_TestInput, values, {arrayName});

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors()[0].code == expectedErrorCode);
}

void TestCase_TestImporterData_Error(const std::string& inputFilePath, usize startImportRow, ReadCSVData::HeaderMode headerMode, usize headersLine, const std::vector<char>& delimiters,
                                     const std::vector<std::string>& headers, const std::vector<CSVType>& dataTypes, const std::vector<bool>& skippedArrayMask, const ShapeType& tupleDims,
                                     nonstd::span<std::string> values, int32 expectedErrorCode)
{
  std::string newGroupName = "New Group";
  ReadCSVFileFilter filter;
  DataStructure dataStructure;
  Arguments args = createArguments(inputFilePath, startImportRow, headerMode, headersLine, delimiters, headers, dataTypes, skippedArrayMask, tupleDims, values, newGroupName);

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
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  // Create the parent output directory.
  fs::create_directories(k_TestInput.parent_path());

  std::vector<std::string> v = {std::to_string(std::numeric_limits<int8>::min()), std::to_string(std::numeric_limits<int8>::max())};
  TestCase_TestPrimitives<int8>(scope, v);

  v = {std::to_string(std::numeric_limits<int16>::min()), std::to_string(std::numeric_limits<int16>::max())};
  TestCase_TestPrimitives<int16>(scope, v);

  v = {std::to_string(std::numeric_limits<int32>::min()), std::to_string(std::numeric_limits<int32>::max())};
  TestCase_TestPrimitives<int32>(scope, v);

  v = {std::to_string(std::numeric_limits<int64>::min()), std::to_string(std::numeric_limits<int64>::max())};
  TestCase_TestPrimitives<int64>(scope, v);

  v = {std::to_string(std::numeric_limits<uint8>::min()), std::to_string(std::numeric_limits<uint8>::max())};
  TestCase_TestPrimitives<uint8>(scope, v);

  v = {std::to_string(std::numeric_limits<uint16>::min()), std::to_string(std::numeric_limits<uint16>::max())};
  TestCase_TestPrimitives<uint16>(scope, v);

  v = {std::to_string(std::numeric_limits<uint32>::min()), std::to_string(std::numeric_limits<uint32>::max())};
  TestCase_TestPrimitives<uint32>(scope, v);

  v = {std::to_string(std::numeric_limits<uint64>::min()), std::to_string(std::numeric_limits<uint64>::max())};
  TestCase_TestPrimitives<uint64>(scope, v);

  v = {std::to_string(std::numeric_limits<float32>::min()), std::to_string(std::numeric_limits<float32>::max())};
  TestCase_TestPrimitives<float32>(scope, v);

  v = {std::to_string(std::numeric_limits<float64>::min()), std::to_string(std::numeric_limits<float64>::max())};
  TestCase_TestPrimitives<float64>(scope, v);

  v = {std::to_string(std::numeric_limits<bool>::min()), std::to_string(std::numeric_limits<bool>::max())};
  TestCase_TestPrimitives<bool>(scope, v);
}

TEST_CASE("SimplnxCore::ReadCSVFileFilter (Case 2): Valid filter execution - Skipped Array")
{
  UnitTest::LoadPlugins();

  std::string newGroupName = "New Group";

  std::string arrayName = "Array";
  DataPath arrayPath = DataPath({newGroupName, arrayName});

  ReadCSVFileFilter filter;
  DataStructure dataStructure;
  std::vector<std::string> values = {"0"};
  Arguments args = createArguments(k_TestInput.string(), 2, ReadCSVData::HeaderMode::LINE, 1, {','}, {arrayName}, {CSVType::int8}, {true}, {static_cast<usize>(values.size())}, values, newGroupName);

  // Write the test input file.
  CreateTestDataFile(k_TestInput, values, {arrayName});

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Check that the array does not exist
  const IDataArray* array = dataStructure.getDataAs<IDataArray>(arrayPath);
  REQUIRE(array == nullptr);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ReadCSVFileFilter (Case 3): Invalid filter execution - Out of Bounds")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  // Create the parent output directory.
  fs::create_directories(k_TestInput.parent_path());

  // Int8 - Out of bounds
  std::vector<std::string> v = {"-129"};
  TestCase_TestPrimitives_Error<int8>(scope, v, k_OverflowErrorCode);

  v = {"128"};
  TestCase_TestPrimitives_Error<int8>(scope, v, k_OverflowErrorCode);

  // Int16 - Out of bounds
  v = {"-32769"};
  TestCase_TestPrimitives_Error<int16>(scope, v, k_OverflowErrorCode);

  v = {"32768"};
  TestCase_TestPrimitives_Error<int16>(scope, v, k_OverflowErrorCode);

  // Int32 - Out of bounds
  v = {"-2147483649"};
  TestCase_TestPrimitives_Error<int32>(scope, v, k_OverflowErrorCode);

  v = {"2147483648"};
  TestCase_TestPrimitives_Error<int32>(scope, v, k_OverflowErrorCode);

  // Int64 - Out of bounds
  v = {"-9223372036854775809"};
  TestCase_TestPrimitives_Error<int64>(scope, v, -10352);

  v = {"9223372036854775808"};
  TestCase_TestPrimitives_Error<int64>(scope, v, -10352);

  // UInt8 - Out of bounds
  v = {"-1"};
  TestCase_TestPrimitives_Error<uint8>(scope, v, -10350);

  v = {"256"};
  TestCase_TestPrimitives_Error<uint8>(scope, v, k_OverflowErrorCode);

  // UInt16 - Out of bounds
  v = {"-1"};
  TestCase_TestPrimitives_Error<uint16>(scope, v, -10350);

  v = {"65536"};
  TestCase_TestPrimitives_Error<uint16>(scope, v, k_OverflowErrorCode);

  // UInt32 - Out of bounds
  v = {"-1"};
  TestCase_TestPrimitives_Error<uint32>(scope, v, -10350);

  v = {"4294967296"};
  TestCase_TestPrimitives_Error<uint32>(scope, v, k_OverflowErrorCode);

  // UInt64 - Out of bounds
  v = {"-1"};
  TestCase_TestPrimitives_Error<uint64>(scope, v, -10350);

  v = {"18446744073709551616"};
  TestCase_TestPrimitives_Error<uint64>(scope, v, -10352);

  // Float32 - Out of bounds
  v = {"-3.5E38"};
  TestCase_TestPrimitives_Error<float32>(scope, v, -10352);

  v = {"3.5E38"};
  TestCase_TestPrimitives_Error<float32>(scope, v, -10352);

  // Float64 - Out of bounds
  v = {"-1.8E308"};
  TestCase_TestPrimitives_Error<float64>(scope, v, -10352);

  v = {"1.8E308"};
  TestCase_TestPrimitives_Error<float64>(scope, v, -10352);
}

TEST_CASE("SimplnxCore::ReadCSVFileFilter (Case 4): Invalid filter execution - Invalid arguments")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  // Create the parent output directory.
  fs::create_directories(k_TestInput.parent_path());

  std::vector<std::string> v = {" "};
  TestCase_TestPrimitives_Error<int8>(scope, v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<int16>(scope, v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<int32>(scope, v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<int64>(scope, v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<uint8>(scope, v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<uint16>(scope, v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<uint32>(scope, v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<uint64>(scope, v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<float32>(scope, v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<float64>(scope, v, k_InvalidArgumentErrorCode);

  v = {"a"};
  TestCase_TestPrimitives_Error<int8>(scope, v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<int16>(scope, v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<int32>(scope, v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<int64>(scope, v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<uint8>(scope, v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<uint16>(scope, v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<uint32>(scope, v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<uint64>(scope, v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<float32>(scope, v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<float64>(scope, v, k_InvalidArgumentErrorCode);

  v = {"&"};
  TestCase_TestPrimitives_Error<int8>(scope, v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<int16>(scope, v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<int32>(scope, v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<int64>(scope, v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<uint8>(scope, v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<uint16>(scope, v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<uint32>(scope, v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<uint64>(scope, v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<float32>(scope, v, k_InvalidArgumentErrorCode);
  TestCase_TestPrimitives_Error<float64>(scope, v, k_InvalidArgumentErrorCode);
}

TEST_CASE("SimplnxCore::ReadCSVFileFilter (Case 5): Invalid filter execution - Invalid ReadCSVData values")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  std::vector<std::string> v = {std::to_string(std::numeric_limits<int8>::min()), std::to_string(std::numeric_limits<int8>::max())};
  fs::create_directories(k_TestInput.parent_path());
  CreateTestDataFile(k_TestInput, v, {"Array"});
  ShapeType tupleDims = {static_cast<usize>(v.size())};

  // Empty input file path
  TestCase_TestImporterData_Error("", 2, ReadCSVData::HeaderMode::LINE, 1, {','}, {}, {CSVType::int8}, {false}, tupleDims, v, k_EmptyFile);

  // Input file does not exist
  fs::path tmp_file = fs::temp_directory_path() / "ThisFileDoesNotExist.txt";
  TestCase_TestImporterData_Error(tmp_file.string(), 2, ReadCSVData::HeaderMode::LINE, 1, {','}, {}, {CSVType::int8}, {false}, tupleDims, v, k_FileDoesNotExist);

  // Start Import Row Out-of-Range
  TestCase_TestImporterData_Error(k_TestInput.string(), 0, ReadCSVData::HeaderMode::LINE, 1, {','}, {}, {CSVType::int8}, {false}, tupleDims, v, k_StartImportRowOutOfRange);
  TestCase_TestImporterData_Error(k_TestInput.string(), 500, ReadCSVData::HeaderMode::LINE, 1, {','}, {}, {CSVType::int8}, {false}, tupleDims, v, k_StartImportRowOutOfRange);

  // Header Line Number Out-of-Range
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::LINE, 0, {','}, {}, {CSVType::int8}, {false}, tupleDims, v, k_HeaderLineOutOfRange);
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::LINE, 600, {','}, {}, {CSVType::int8}, {false}, tupleDims, v, k_HeaderLineOutOfRange);
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::LINE, 3, {','}, {}, {CSVType::int8}, {false}, tupleDims, v, k_HeaderLineOutOfRange);

  // Empty array headers
  tmp_file = fs::temp_directory_path() / "BlankLines.txt";
  v = {std::to_string(std::numeric_limits<int8>::min()), "", std::to_string(std::numeric_limits<int8>::max())};
  CreateTestDataFile(tmp_file, v, {"Array"});
  TestCase_TestImporterData_Error(tmp_file.string(), 4, ReadCSVData::HeaderMode::LINE, 3, {','}, {}, {CSVType::int8}, {false}, {static_cast<usize>(v.size())}, v, k_EmptyHeaders);
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, {}, {CSVType::int8}, {false}, {static_cast<usize>(v.size())}, v, k_EmptyHeaders);
  fs::remove(tmp_file);
  v = {std::to_string(std::numeric_limits<int8>::min()), std::to_string(std::numeric_limits<int8>::max())};

  // Incorrect Data Type Count
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::LINE, 1, {','}, {}, {}, {false}, tupleDims, v, k_IncorrectDataTypeCount);
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::LINE, 1, {','}, {}, {CSVType::int8, CSVType::int32}, {false}, tupleDims, v, k_IncorrectDataTypeCount);
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, {"Custom Array"}, {}, {false}, tupleDims, v, k_IncorrectDataTypeCount);
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, {"Custom Array"}, {CSVType::int8, CSVType::int32}, {false}, tupleDims, v,
                                  k_IncorrectDataTypeCount);

  // Incorrect Skipped Array Mask Count
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::LINE, 1, {','}, {}, {CSVType::int8}, {}, tupleDims, v, k_IncorrectMaskCount);
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::LINE, 1, {','}, {}, {CSVType::int8}, {false, false}, tupleDims, v, k_IncorrectMaskCount);
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, {"Custom Array"}, {CSVType::int8}, {}, tupleDims, v, k_IncorrectMaskCount);
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, {"Custom Array"}, {CSVType::int8}, {false, false}, tupleDims, v, k_IncorrectMaskCount);

  // Empty Header Names
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, {""}, {CSVType::int8}, {false}, tupleDims, v, k_EmptyNames);

  // Duplicate Header Names
  tmp_file = fs::temp_directory_path() / "DuplicateHeaders.txt";
  std::vector<std::string> duplicateHeaders = {"Custom Array", "Custom Array"};
  CreateTestDataFile(tmp_file, v, duplicateHeaders);
  TestCase_TestImporterData_Error(tmp_file.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, duplicateHeaders, {CSVType::int8, CSVType::int8}, {false, false}, tupleDims, v, k_DuplicateNames);
  fs::remove(tmp_file);

  // Illegal Header Names
  tmp_file = fs::temp_directory_path() / "IllegalHeaders.txt";

  std::vector<std::string> illegalHeaders = {"Illegal/Header"};
  CreateTestDataFile(tmp_file, v, illegalHeaders);
  scope.execute([&] { TestCase_TestImporterData_Error(tmp_file.string(), 2, ReadCSVData::HeaderMode::LINE, 1, {','}, {}, {CSVType::int8}, {false}, tupleDims, v, 0); });
  scope.execute([&] { TestCase_TestImporterData_Error(tmp_file.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, illegalHeaders, {CSVType::int8}, {false}, tupleDims, v, 0); });

  illegalHeaders = {"Illegal\\Header"};
  CreateTestDataFile(tmp_file, v, illegalHeaders);
  scope.execute([&] { TestCase_TestImporterData_Error(tmp_file.string(), 2, ReadCSVData::HeaderMode::LINE, 1, {','}, {}, {CSVType::int8}, {false}, tupleDims, v, 0); });
  scope.execute([&] { TestCase_TestImporterData_Error(tmp_file.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, illegalHeaders, {CSVType::int8}, {false}, tupleDims, v, 0); });

  illegalHeaders = {"Illegal&Header"};
  CreateTestDataFile(tmp_file, v, illegalHeaders);
  scope.execute([&] { TestCase_TestImporterData_Error(tmp_file.string(), 2, ReadCSVData::HeaderMode::LINE, 1, {','}, {}, {CSVType::int8}, {false}, tupleDims, v, 0); });
  scope.execute([&] { TestCase_TestImporterData_Error(tmp_file.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, illegalHeaders, {CSVType::int8}, {false}, tupleDims, v, 0); });

  illegalHeaders = {"Illegal:Header"};
  CreateTestDataFile(tmp_file, v, illegalHeaders);
  scope.execute([&] { TestCase_TestImporterData_Error(tmp_file.string(), 2, ReadCSVData::HeaderMode::LINE, 1, {','}, {}, {CSVType::int8}, {false}, tupleDims, v, 0); });
  scope.execute([&] { TestCase_TestImporterData_Error(tmp_file.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, illegalHeaders, {CSVType::int8}, {false}, tupleDims, v, 0); });

  fs::remove(tmp_file);

  // Incorrect Tuple Dimensions
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, {"Custom Array"}, {CSVType::int8}, {false}, {0}, v, k_IncorrectTuples);
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, {"Custom Array"}, {CSVType::int8}, {false}, {30}, v, k_IncorrectTuples);
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, {"Custom Array"}, {CSVType::int8}, {false}, {30, 2}, v, k_IncorrectTuples);
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, {"Custom Array"}, {CSVType::int8}, {false}, {30, 5, 7}, v, k_IncorrectTuples);

  // Inconsistent Columns
  TestCase_TestImporterData_Error(k_TestInput.string(), 2, ReadCSVData::HeaderMode::CUSTOM, 1, {','}, {"Custom Array", "Custom Array2"}, {CSVType::int8, CSVType::int8}, {false, false}, tupleDims, v,
                                  k_InconsistentCols);
}

TEST_CASE("SimplnxCore::ReadCSVFileFilter (Case 6): Invalid filter execution - Blank Lines")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  // Create the parent output directory.
  fs::create_directories(k_TestInput.parent_path());

  // The first row is blank.
  std::vector<std::string> v = {"", std::to_string(std::numeric_limits<int8>::min()), std::to_string(std::numeric_limits<int8>::max())};
  TestCase_TestPrimitives_Error<int8>(scope, v, k_BlankLineErrorCode);
}

TEST_CASE("SimplnxCore::ReadCSVFileFilter (Case 7): Valid filter execution - String Data")
{
  const std::vector<std::string> arrayNames = {"Name", "City"};
  const std::string groupName = "Group 1";

  UnitTest::LoadPlugins();
  fs::create_directories(k_TestInput.parent_path());

  std::vector<std::string> v = {"Alice", "Bob", "Charlie"};
  CreateTestDataFile(k_TestInput, v, arrayNames);

  Arguments args =
      createArguments(k_TestInput.string(), 2, ReadCSVData::HeaderMode::LINE, 1, {','}, arrayNames, {CSVType::string, CSVType::string}, {false, false}, {static_cast<usize>(v.size())}, v, groupName);

  ReadCSVFileFilter filter;
  DataStructure dataStructure;

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Verify both string arrays were created and contain the expected values
  for(const auto& name : arrayNames)
  {
    const StringArray* array = dataStructure.getDataAs<StringArray>(DataPath({groupName, name}));
    REQUIRE(array != nullptr);
    REQUIRE(array->getSize() == v.size());
    for(usize i = 0; i < v.size(); ++i)
    {
      REQUIRE(array->at(i) == v[i]);
    }
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ReadCSVFileFilter (Case 8): Valid filter execution - Mixed quoted strings and integers")
{
  UnitTest::LoadPlugins();
  fs::create_directories(k_TestInput.parent_path());

  // Create CSV with single and double quoted strings and integer strings
  std::ofstream file(k_TestInput);
  REQUIRE(file.is_open());
  file << "SQ,DQ,Num\n";
  std::vector<std::tuple<std::string, std::string, std::string>> rows = {{"'Alice'", "\"Alice\"", "1"}, {"'Bob'", "\"Bob\"", "2"}, {"'Charlie'", "\"Charlie\"", "3"}};
  for(size_t i = 0; i < rows.size(); ++i)
  {
    file << std::get<0>(rows[i]) << "," << std::get<1>(rows[i]) << "," << std::get<2>(rows[i]);
    if(i < rows.size() - 1)
    {
      file << "\n";
    }
  }
  file.close();

  // Set up filter arguments
  std::vector<std::string> dummy = {"1", "2", "3"};
  Arguments args = createArguments(k_TestInput.string(), 2, ReadCSVData::HeaderMode::LINE, 1, {','}, {"SQ", "DQ", "Num"}, {CSVType::string, CSVType::string, CSVType::string}, {false, false, false},
                                   {static_cast<usize>(dummy.size())}, dummy, "New Group");

  ReadCSVFileFilter filter;
  DataStructure dataStructure;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const std::regex re(R"(^['"]+|['"]+$)"); // Remove quotes and double quotes

  // Verify single quoted column
  const StringArray* sqArray = dataStructure.getDataAs<StringArray>(DataPath({"New Group", "SQ"}));
  REQUIRE(sqArray != nullptr);
  REQUIRE(sqArray->getSize() == rows.size());
  for(usize i = 0; i < sqArray->getSize(); ++i)
  {
    auto str = std::regex_replace(std::get<0>(rows[i]), re, "");
    REQUIRE(sqArray->at(i) == str);
  }

  // Verify double quoted column
  const StringArray* dqArray = dataStructure.getDataAs<StringArray>(DataPath({"New Group", "DQ"}));
  REQUIRE(dqArray != nullptr);
  REQUIRE(dqArray->getSize() == rows.size());
  for(usize i = 0; i < dqArray->getSize(); ++i)
  {
    auto str = std::regex_replace(std::get<1>(rows[i]), re, "");
    REQUIRE(sqArray->at(i) == str);
  }

  // Verify integer column
  const StringArray* numArray = dataStructure.getDataAs<StringArray>(DataPath({"New Group", "Num"}));
  REQUIRE(numArray != nullptr);
  REQUIRE(numArray->getSize() == rows.size());
  for(usize i = 0; i < numArray->getSize(); ++i)
  {
    REQUIRE(numArray->at(i) == std::get<2>(rows[i]));
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ReadCSVFileFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ReadCSVFileFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ReadCSVFileFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ReadCSVFileFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ReadCSVFileFilter>::uuid);

      const Arguments args = pipelineFilter->getArguments();
      // Fixture has Wizard_AutomaticAM=1, so the CreationFilter branch fires and sets k_CreatedDataGroup_Key.
      CHECK(args.value<DataPath>(ReadCSVFileFilter::k_CreatedDataGroup_Key) == DataPath({"DataContainer", "CellData"}));
    }
  }
}
