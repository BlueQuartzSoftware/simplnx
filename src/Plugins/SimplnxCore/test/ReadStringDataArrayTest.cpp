#include "SimplnxCore/Filters/ReadTextDataArrayFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "SimplnxCore/Filters/ReadStringDataArrayFilter.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
const std::vector<std::string> k_InputNames = {"comma", "semi_colon", "space", "colon", "tab", "new_line"};
} // namespace

TEST_CASE("SimplnxCore::ReadStringDataArrayFilter: Valid filter execution", "[SimplnxCore][ReadStringDataArrayFilter]")
{

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "7_ReadStringArray.tar.gz", "7_ReadStringArray");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/7_ReadStringArray/7_read_string_array.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  {
    const usize k_InputIndex = 0;
    const DataPath k_CreatedDataArrayPath({fmt::format("computed_{}", k_InputNames[k_InputIndex])});
    const DataPath k_ExemplarDataArrayPath({fmt::format("exemplar_{}", k_InputNames[k_InputIndex])});

    auto inputFilePath = fs::path(fmt::format("{}/7_ReadStringArray/{}", unit_test::k_TestFilesDir, fmt::format("random_words_{}.csv", k_InputNames[k_InputIndex])));
    // Instantiate the filter and an Arguments Object
    Arguments args;

    ReadStringDataArrayFilter filter;
    args.insertOrAssign(ReadStringDataArrayFilter::k_InputFile_Key, std::make_any<fs::path>(fs::path(inputFilePath)));
    args.insertOrAssign(ReadStringDataArrayFilter::k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{100}}));
    args.insertOrAssign(ReadStringDataArrayFilter::k_NSkipLines_Key, std::make_any<uint64>(1));
    args.insertOrAssign(ReadStringDataArrayFilter::k_DelimiterChoice_Key, std::make_any<uint64>(k_InputIndex));
    args.insertOrAssign(ReadStringDataArrayFilter::k_DataArrayPath_Key, std::make_any<DataPath>(k_CreatedDataArrayPath));
    args.insertOrAssign(ReadStringDataArrayFilter::k_DataFormat_Key, std::make_any<std::string>(""));
    args.insertOrAssign(ReadStringDataArrayFilter::k_SetTupleDimensions, std::make_any<bool>(true));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    nx::core::UnitTest::CompareStringArrays(dataStructure, k_ExemplarDataArrayPath, k_CreatedDataArrayPath);
  }

  {
    const usize k_InputIndex = 1;
    const DataPath k_CreatedDataArrayPath({fmt::format("computed_{}", k_InputNames[k_InputIndex])});
    const DataPath k_ExemplarDataArrayPath({fmt::format("exemplar_{}", k_InputNames[k_InputIndex])});
    auto inputFilePath = fs::path(fmt::format("{}/7_ReadStringArray/{}", unit_test::k_TestFilesDir, fmt::format("random_words_{}.csv", k_InputNames[k_InputIndex])));
    // Instantiate the filter and an Arguments Object
    Arguments args;

    ReadStringDataArrayFilter filter;
    args.insertOrAssign(ReadStringDataArrayFilter::k_InputFile_Key, std::make_any<fs::path>(fs::path(inputFilePath)));
    args.insertOrAssign(ReadStringDataArrayFilter::k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{100}}));
    args.insertOrAssign(ReadStringDataArrayFilter::k_NSkipLines_Key, std::make_any<uint64>(1));
    args.insertOrAssign(ReadStringDataArrayFilter::k_DelimiterChoice_Key, std::make_any<uint64>(k_InputIndex));
    args.insertOrAssign(ReadStringDataArrayFilter::k_DataArrayPath_Key, std::make_any<DataPath>(k_CreatedDataArrayPath));
    args.insertOrAssign(ReadStringDataArrayFilter::k_DataFormat_Key, std::make_any<std::string>(""));
    args.insertOrAssign(ReadStringDataArrayFilter::k_SetTupleDimensions, std::make_any<bool>(true));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    nx::core::UnitTest::CompareStringArrays(dataStructure, k_ExemplarDataArrayPath, k_CreatedDataArrayPath);
  }

  {
    const usize k_InputIndex = 3;
    const DataPath k_CreatedDataArrayPath({fmt::format("computed_{}", k_InputNames[k_InputIndex])});
    const DataPath k_ExemplarDataArrayPath({fmt::format("exemplar_{}", k_InputNames[k_InputIndex])});
    auto inputFilePath = fs::path(fmt::format("{}/7_ReadStringArray/{}", unit_test::k_TestFilesDir, fmt::format("random_words_{}.csv", k_InputNames[k_InputIndex])));
    // Instantiate the filter and an Arguments Object
    Arguments args;

    ReadStringDataArrayFilter filter;
    args.insertOrAssign(ReadStringDataArrayFilter::k_InputFile_Key, std::make_any<fs::path>(fs::path(inputFilePath)));
    args.insertOrAssign(ReadStringDataArrayFilter::k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{100}}));
    args.insertOrAssign(ReadStringDataArrayFilter::k_NSkipLines_Key, std::make_any<uint64>(1));
    args.insertOrAssign(ReadStringDataArrayFilter::k_DelimiterChoice_Key, std::make_any<uint64>(k_InputIndex));
    args.insertOrAssign(ReadStringDataArrayFilter::k_DataArrayPath_Key, std::make_any<DataPath>(k_CreatedDataArrayPath));
    args.insertOrAssign(ReadStringDataArrayFilter::k_DataFormat_Key, std::make_any<std::string>(""));
    args.insertOrAssign(ReadStringDataArrayFilter::k_SetTupleDimensions, std::make_any<bool>(true));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    nx::core::UnitTest::CompareStringArrays(dataStructure, k_ExemplarDataArrayPath, k_CreatedDataArrayPath);
  }

  {
    const usize k_InputIndex = 4;
    const DataPath k_CreatedDataArrayPath({fmt::format("computed_{}", k_InputNames[k_InputIndex])});
    const DataPath k_ExemplarDataArrayPath({fmt::format("exemplar_{}", k_InputNames[k_InputIndex])});
    auto inputFilePath = fs::path(fmt::format("{}/7_ReadStringArray/{}", unit_test::k_TestFilesDir, fmt::format("random_words_{}.csv", k_InputNames[k_InputIndex])));
    // Instantiate the filter and an Arguments Object
    Arguments args;

    ReadStringDataArrayFilter filter;
    args.insertOrAssign(ReadStringDataArrayFilter::k_InputFile_Key, std::make_any<fs::path>(fs::path(inputFilePath)));
    args.insertOrAssign(ReadStringDataArrayFilter::k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{100}}));
    args.insertOrAssign(ReadStringDataArrayFilter::k_NSkipLines_Key, std::make_any<uint64>(1));
    args.insertOrAssign(ReadStringDataArrayFilter::k_DelimiterChoice_Key, std::make_any<uint64>(k_InputIndex));
    args.insertOrAssign(ReadStringDataArrayFilter::k_DataArrayPath_Key, std::make_any<DataPath>(k_CreatedDataArrayPath));
    args.insertOrAssign(ReadStringDataArrayFilter::k_DataFormat_Key, std::make_any<std::string>(""));
    args.insertOrAssign(ReadStringDataArrayFilter::k_SetTupleDimensions, std::make_any<bool>(true));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    nx::core::UnitTest::CompareStringArrays(dataStructure, k_ExemplarDataArrayPath, k_CreatedDataArrayPath);
  }

  {
    const usize k_InputIndex = 5;
    const DataPath k_CreatedDataArrayPath({fmt::format("computed_{}", k_InputNames[k_InputIndex])});
    const DataPath k_ExemplarDataArrayPath({fmt::format("exemplar_{}", k_InputNames[k_InputIndex])});
    auto inputFilePath = fs::path(fmt::format("{}/7_ReadStringArray/{}", unit_test::k_TestFilesDir, fmt::format("random_words_{}.csv", k_InputNames[k_InputIndex])));
    // Instantiate the filter and an Arguments Object
    Arguments args;

    ReadStringDataArrayFilter filter;
    args.insertOrAssign(ReadStringDataArrayFilter::k_InputFile_Key, std::make_any<fs::path>(fs::path(inputFilePath)));
    args.insertOrAssign(ReadStringDataArrayFilter::k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{100}}));
    args.insertOrAssign(ReadStringDataArrayFilter::k_NSkipLines_Key, std::make_any<uint64>(1));
    args.insertOrAssign(ReadStringDataArrayFilter::k_DelimiterChoice_Key, std::make_any<uint64>(k_InputIndex));
    args.insertOrAssign(ReadStringDataArrayFilter::k_DataArrayPath_Key, std::make_any<DataPath>(k_CreatedDataArrayPath));
    args.insertOrAssign(ReadStringDataArrayFilter::k_DataFormat_Key, std::make_any<std::string>(""));
    args.insertOrAssign(ReadStringDataArrayFilter::k_SetTupleDimensions, std::make_any<bool>(true));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    nx::core::UnitTest::CompareStringArrays(dataStructure, k_ExemplarDataArrayPath, k_CreatedDataArrayPath);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ReadStringDataArrayFilter: Invalid filter execution", "[SimplnxCore][ReadStringDataArrayFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "7_ReadStringArray.tar.gz", "7_ReadStringArray");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/7_ReadStringArray/7_read_string_array.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = LoadDataStructure(exemplarFilePath);

  {
    const usize k_InputIndex = 5;
    const DataPath k_CreatedDataArrayPath({fmt::format("computed_{}", k_InputNames[k_InputIndex])});
    const DataPath k_ExemplarDataArrayPath({fmt::format("exemplar_{}", k_InputNames[k_InputIndex])});
    auto inputFilePath = fs::path(fmt::format("{}/7_ReadStringArray/{}", unit_test::k_TestFilesDir, fmt::format("random_words_{}.csv", k_InputNames[k_InputIndex])));
    // Instantiate the filter and an Arguments Object
    Arguments args;

    ReadStringDataArrayFilter filter;
    args.insertOrAssign(ReadStringDataArrayFilter::k_InputFile_Key, std::make_any<fs::path>(fs::path(inputFilePath)));
    args.insertOrAssign(ReadStringDataArrayFilter::k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{0}));
    args.insertOrAssign(ReadStringDataArrayFilter::k_NSkipLines_Key, std::make_any<uint64>(1));
    args.insertOrAssign(ReadStringDataArrayFilter::k_DelimiterChoice_Key, std::make_any<uint64>(k_InputIndex));
    args.insertOrAssign(ReadStringDataArrayFilter::k_DataArrayPath_Key, std::make_any<DataPath>(k_CreatedDataArrayPath));
    args.insertOrAssign(ReadStringDataArrayFilter::k_DataFormat_Key, std::make_any<std::string>(""));
    args.insertOrAssign(ReadStringDataArrayFilter::k_SetTupleDimensions, std::make_any<bool>(true));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
