#include "SimplnxCore/Filters/ReadTextDataArrayFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "SimplnxCore/Filters/ReadCSVFileFilter.hpp"
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
using namespace nx::core::Constants;

namespace
{
const std::vector<int> inputIntVector({1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
const std::vector<int> inputHexVector({0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A});
const std::vector<int> inputOctVector({001, 002, 003, 004, 005, 006, 007});
const std::vector<double> inputDoubleVector({1.5, 2.2, 3.65, 4.34, 5.76, 6.534, 7.0, 8.342, 9.8723, 10.89});
const std::vector<std::string> inputCharErrorVector({"sdstrg", "2.2", "3.65", "&(^$#", "5.76", "sjtyr", "7.0", "8.342", "&*^#F", "youikjhgf"});
const std::vector<double> inputScientificNotation({0.15e1, 2.2e0, 3.65, 43.4e-1, 0.576e1, 653.4e-2, 7.0, 8.342, 9.8723, 10.89});

const std::vector<double> outputDoubleVector({1.5, 2.2, 3.65, 4.34, 5.76, 6.534, 7.0, 8.342, 9.8723, 10.89});
const std::vector<double> outputIntAsDoubleVector({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0});

const std::string k_DataArrayName = "Array1";

const usize xDim = 10;
const usize yDim = 5;
const usize zDim = 1;
} // namespace

template <typename T>
void writeInvalidFile(const std::string& filePath, std::vector<T> values, char delimiter)
{
  std::ofstream data(filePath, std::ios::out /*| std::ios::trunc*/);
  // std::ofstream data(filePath, std::ios::binary);
  if(data.is_open())
  {
    // data.write(reinterpret_cast<const char*>(values.data()), values.size() * sizeof(T));
    for(size_t row = 0; row < values.size(); row++)
    {
      data << values[row];
      if(row + 1 < values.size())
      {
        data << delimiter;
      }
    }
    data.close();
  }
}

void writeFile(const std::string& filePath, char delimiter)
{
  std::ofstream outfile(filePath.c_str(), std::ios_base::binary);
  int32 index = 0;
  for(usize y = 0; y < yDim; y++)
  {
    for(usize x = 0; x < xDim; x++)
    {
      outfile << index++;
      // if(x < xDim - 1)
      {
        outfile << delimiter;
      }
    }
    outfile << std::endl;
  }
}

template <typename T>
void RunInvalidTest()
{
  const std::string inputFilePath = fmt::format("{}/TestFile1.txt", unit_test::k_BinaryTestOutputDir);
  std::vector<usize> tupleDims = {10};
  char delimiter = '\t';
  const DataPath createdArrayPath({k_GroupAName, k_DataArrayName});

  // Test Using Unexpected Input - Alphabetical Characters, Special Characters, etc.
  {
    writeInvalidFile<std::string>(inputFilePath, inputCharErrorVector, delimiter);

    ReadTextDataArrayFilter filter;
    DataStructure dataStructure;
    AttributeMatrix* am = AttributeMatrix::Create(dataStructure, k_GroupAName, tupleDims);
    Arguments args;
    args.insertOrAssign(ReadTextDataArrayFilter::k_InputFileKey, std::make_any<fs::path>(fs::path(inputFilePath)));
    args.insertOrAssign(ReadTextDataArrayFilter::k_ScalarTypeKey, std::make_any<NumericType>(GetNumericType<T>()));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NCompKey, std::make_any<uint64>(1));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NSkipLinesKey, std::make_any<uint64>(0));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DelimiterChoiceKey, std::make_any<uint64>(4));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DataArrayKey, std::make_any<DataPath>(createdArrayPath));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DataFormat_Key, std::make_any<std::string>(""));
    args.insertOrAssign(ReadTextDataArrayFilter::k_AdvancedOptions_Key, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
  }

  // RemoveTestFiles();

  // Max Overflow Test
  {
    std::string maxValue = StringUtilities::number(std::numeric_limits<T>().max());
    maxValue = maxValue + '0';
    std::vector<std::string> inputMaxVector({maxValue});
    writeInvalidFile<std::string>(inputFilePath, inputMaxVector, delimiter);

    tupleDims = {1};

    ReadTextDataArrayFilter filter;
    DataStructure dataStructure;
    AttributeMatrix* am = AttributeMatrix::Create(dataStructure, k_GroupAName, tupleDims);
    Arguments args;
    args.insertOrAssign(ReadTextDataArrayFilter::k_InputFileKey, std::make_any<fs::path>(fs::path(inputFilePath)));
    args.insertOrAssign(ReadTextDataArrayFilter::k_ScalarTypeKey, std::make_any<NumericType>(GetNumericType<T>()));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NCompKey, std::make_any<uint64>(1));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NSkipLinesKey, std::make_any<uint64>(0));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DelimiterChoiceKey, std::make_any<uint64>(4));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DataArrayKey, std::make_any<DataPath>(createdArrayPath));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DataFormat_Key, std::make_any<std::string>(""));
    args.insertOrAssign(ReadTextDataArrayFilter::k_AdvancedOptions_Key, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
  }

  // Min Overflow Test
  {
    // We must hard-code these three types because there is no way to store them otherwise...
    std::string minValue;
    if constexpr(std::is_same_v<T, float32>)
    {
      minValue = "-3.4e39";
    }
    else if constexpr(std::is_same_v<T, float64>)
    {
      minValue = "-1.7e309";
    }
    else if constexpr(std::is_same_v<T, int64>)
    {
      minValue = "-9223372036854775809";
    }
    else
    {
      int64_t store = std::numeric_limits<T>().min();
      store = store - 1;
      minValue = StringUtilities::number(store);
    }

    std::vector<std::string> inputMinVector({minValue});
    writeInvalidFile<std::string>(inputFilePath, inputMinVector, delimiter);

    tupleDims = {1};

    ReadTextDataArrayFilter filter;
    DataStructure dataStructure;
    AttributeMatrix* am = AttributeMatrix::Create(dataStructure, k_GroupAName, tupleDims);
    Arguments args;
    args.insertOrAssign(ReadTextDataArrayFilter::k_InputFileKey, std::make_any<fs::path>(fs::path(inputFilePath)));
    args.insertOrAssign(ReadTextDataArrayFilter::k_ScalarTypeKey, std::make_any<NumericType>(GetNumericType<T>()));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NCompKey, std::make_any<uint64>(1));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NSkipLinesKey, std::make_any<uint64>(0));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DelimiterChoiceKey, std::make_any<uint64>(4));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DataArrayKey, std::make_any<DataPath>(createdArrayPath));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DataFormat_Key, std::make_any<std::string>(""));
    args.insertOrAssign(ReadTextDataArrayFilter::k_AdvancedOptions_Key, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
  }
}

template <typename T>
void RunTest(char sep, int delimiter)
{
  const std::string inputFilePath = fmt::format("{}/TestFile1.txt", unit_test::k_BinaryTestOutputDir);
  writeFile(inputFilePath, sep);

  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "ImageDataContainer");
  const std::vector<usize> tDims = {xDim, yDim, zDim};
  imageGeom->setDimensions(tDims);
  AttributeMatrix* attrMat = AttributeMatrix::Create(dataStructure, "AttributeMatrix", std::vector<usize>{zDim, yDim, xDim}, imageGeom->getId());

  DataPath createdAttributeArrayPath({"ImageDataContainer", "AttributeMatrix", "ImportedData"});
  NumericType scalarType = GetNumericType<T>();
  int numberOfComponents = 1;
  int skipHeaderLines = 0;

  ReadTextDataArrayFilter filter;
  Arguments args;
  args.insertOrAssign(ReadTextDataArrayFilter::k_InputFileKey, std::make_any<fs::path>(fs::path(inputFilePath)));
  args.insertOrAssign(ReadTextDataArrayFilter::k_ScalarTypeKey, std::make_any<NumericType>(scalarType));
  args.insertOrAssign(ReadTextDataArrayFilter::k_NCompKey, std::make_any<uint64>(numberOfComponents));
  args.insertOrAssign(ReadTextDataArrayFilter::k_NSkipLinesKey, std::make_any<uint64>(skipHeaderLines));
  args.insertOrAssign(ReadTextDataArrayFilter::k_DelimiterChoiceKey, std::make_any<uint64>(delimiter));
  args.insertOrAssign(ReadTextDataArrayFilter::k_DataArrayKey, std::make_any<DataPath>(createdAttributeArrayPath));
  args.insertOrAssign(ReadTextDataArrayFilter::k_DataFormat_Key, std::make_any<std::string>(""));
  args.insertOrAssign(ReadTextDataArrayFilter::k_AdvancedOptions_Key, std::make_any<bool>(false));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  REQUIRE(dataStructure.getDataAs<DataArray<T>>(createdAttributeArrayPath) != nullptr);
  auto& createdArray = dataStructure.getDataRefAs<DataArray<T>>(createdAttributeArrayPath);
  usize index = 0;
  for(usize y = 0; y < yDim; y++)
  {
    for(usize x = 0; x < xDim; x++)
    {
      T value = static_cast<T>(index);
      REQUIRE(value == createdArray[index]);
      index++;
    }
  }
}

TEST_CASE("SimplnxCore::ReadTextDataArrayFilter: Valid filter execution", "[SimplnxCore][ReadTextDataArrayFilter]")
{
  RunTest<int8_t>(',', 0);
  RunTest<uint8_t>(',', 0);
  RunTest<int16_t>(',', 0);
  RunTest<uint16_t>(',', 0);
  RunTest<int32_t>(',', 0);
  RunTest<uint32_t>(',', 0);
  RunTest<int64_t>(',', 0);
  RunTest<uint64_t>(',', 0);
  RunTest<float>(',', 0);
  RunTest<double>(',', 0);

  RunTest<int8_t>(';', 1);
  RunTest<uint8_t>(';', 1);
  RunTest<int16_t>(';', 1);
  RunTest<uint16_t>(';', 1);
  RunTest<int32_t>(';', 1);
  RunTest<uint32_t>(';', 1);
  RunTest<int64_t>(';', 1);
  RunTest<uint64_t>(';', 1);
  RunTest<float>(';', 1);
  RunTest<double>(';', 1);

  RunTest<int8_t>(' ', 2);
  RunTest<uint8_t>(' ', 2);
  RunTest<int16_t>(' ', 2);
  RunTest<uint16_t>(' ', 2);
  RunTest<int32_t>(' ', 2);
  RunTest<uint32_t>(' ', 2);
  RunTest<int64_t>(' ', 2);
  RunTest<uint64_t>(' ', 2);
  RunTest<float>(' ', 2);
  RunTest<double>(' ', 2);

  RunTest<int8_t>(':', 3);
  RunTest<uint8_t>(':', 3);
  RunTest<int16_t>(':', 3);
  RunTest<uint16_t>(':', 3);
  RunTest<int32_t>(':', 3);
  RunTest<uint32_t>(':', 3);
  RunTest<int64_t>(':', 3);
  RunTest<uint64_t>(':', 3);
  RunTest<float>(':', 3);
  RunTest<double>(':', 3);

  RunTest<int8_t>('\t', 4);
  RunTest<uint8_t>('\t', 4);
  RunTest<int16_t>('\t', 4);
  RunTest<uint16_t>('\t', 4);
  RunTest<int32_t>('\t', 4);
  RunTest<uint32_t>('\t', 4);
  RunTest<int64_t>('\t', 4);
  RunTest<uint64_t>('\t', 4);
  RunTest<float>('\t', 4);
  RunTest<double>('\t', 4);
}

TEST_CASE("SimplnxCore::ReadTextDataArrayFilter: Invalid filter execution", "[SimplnxCore][ReadTextDataArrayFilter]")
{
  // Reading alphabetical/special characters, and min/max overflow
  RunInvalidTest<int8>();
  RunInvalidTest<int16>();
  RunInvalidTest<int32>();
  RunInvalidTest<int64>();
  RunInvalidTest<uint8>();
  RunInvalidTest<uint16>();
  RunInvalidTest<uint32>();
  RunInvalidTest<uint64>();
  RunInvalidTest<float32>();
  RunInvalidTest<float64>();

  const std::string inputFilePath = fmt::format("{}/TestFile1.txt", unit_test::k_BinaryTestOutputDir);
  const DataPath createdArrayPath({k_GroupAName, k_DataArrayName});

  // DataGroup parent but no tuple dimensions set
  {
    ReadTextDataArrayFilter filter;
    DataStructure dataStructure;
    DataGroup* dataGroup = DataGroup::Create(dataStructure, k_GroupAName);
    Arguments args;
    args.insertOrAssign(ReadTextDataArrayFilter::k_InputFileKey, std::make_any<fs::path>(fs::path(inputFilePath)));
    args.insertOrAssign(ReadTextDataArrayFilter::k_ScalarTypeKey, std::make_any<NumericType>(NumericType::int32));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NCompKey, std::make_any<uint64>(1));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NSkipLinesKey, std::make_any<uint64>(0));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DelimiterChoiceKey, std::make_any<uint64>(4));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DataArrayKey, std::make_any<DataPath>(createdArrayPath));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DataFormat_Key, std::make_any<std::string>(""));
    args.insertOrAssign(ReadTextDataArrayFilter::k_AdvancedOptions_Key, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
  }

  // DataGroup parent with tuple dimension of 0
  {
    ReadTextDataArrayFilter filter;
    DataStructure dataStructure;
    DataGroup* dataGroup = DataGroup::Create(dataStructure, k_GroupAName);
    Arguments args;
    args.insertOrAssign(ReadTextDataArrayFilter::k_InputFileKey, std::make_any<fs::path>(fs::path(inputFilePath)));
    args.insertOrAssign(ReadTextDataArrayFilter::k_ScalarTypeKey, std::make_any<NumericType>(NumericType::int32));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NCompKey, std::make_any<uint64>(1));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NSkipLinesKey, std::make_any<uint64>(0));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DelimiterChoiceKey, std::make_any<uint64>(4));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DataArrayKey, std::make_any<DataPath>(createdArrayPath));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DataFormat_Key, std::make_any<std::string>(""));
    args.insertOrAssign(ReadTextDataArrayFilter::k_AdvancedOptions_Key, std::make_any<bool>(false));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NTuplesKey, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{0}}));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
  }
}
