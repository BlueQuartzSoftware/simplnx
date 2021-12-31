/**
 * This file is auto generated from the original Core/RawBinaryReaderFilter
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
 * When you start working on this unit test remove "[RawBinaryReaderFilter][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include <fstream>

#include "complex/DataStructure/DataArray.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"

#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;

#include "ComplexCore/Filters/RawBinaryReaderFilter.hpp"
#include "complex/unit_test/complex_test_dirs.hpp"

using namespace complex;

TEST_CASE("Core::RawBinaryReaderFilter: Valid filter execution")
{
  std::string inputFile = (fs::temp_directory_path() /= "raw_binary_reader_data.bin").string();
  std::string arrayName = "Imported Array";
  DataPath createdAttributeArrayPathValue = DataPath(std::vector<std::string>{arrayName});

  // Write binary file
  std::array<char, 10> buffer = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  std::fstream myFile(inputFile, std::ios::out | std::ios::binary);
  myFile.write(buffer.data(), buffer.size());
  myFile.close();

  // Instantiate the filter, a DataStructure object and an Arguments Object
  RawBinaryReaderFilter filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(RawBinaryReaderFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputFile)));
  args.insertOrAssign(RawBinaryReaderFilter::k_ScalarType_Key, std::make_any<NumericType>(NumericType::uint8));
  args.insertOrAssign(RawBinaryReaderFilter::k_NumberOfComponents_Key, std::make_any<int32>(1));
  args.insertOrAssign(RawBinaryReaderFilter::k_Endian_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(RawBinaryReaderFilter::k_SkipHeaderBytes_Key, std::make_any<uint64>(0));
  args.insertOrAssign(RawBinaryReaderFilter::k_CreatedAttributeArrayPath_Key, std::make_any<DataPath>(createdAttributeArrayPathValue));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());

  // Check the results
  auto* resultArray = ds.getDataAs<DataArray<uint8>>(createdAttributeArrayPathValue);
  for(int i = 0; i < 10; i++)
  {
    uint8 resultVal = resultArray->at(i);
    REQUIRE(resultVal == i);
  }
}

TEST_CASE("Core::RawBinaryReaderFilter: Invalid filter execution")
{
  std::string arrayName = "Imported Array";
  DataPath createdAttributeArrayPathValue = DataPath(std::vector<std::string>{arrayName});

  // Instantiate the filter, a DataStructure object and an Arguments Object
  RawBinaryReaderFilter filter;
  DataStructure ds;
  Arguments args;

  // Test empty input file
  std::string inputFile;

  args.insertOrAssign(RawBinaryReaderFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputFile)));
  args.insertOrAssign(RawBinaryReaderFilter::k_ScalarType_Key, std::make_any<NumericType>(NumericType::uint8));
  args.insertOrAssign(RawBinaryReaderFilter::k_NumberOfComponents_Key, std::make_any<int32>(1));
  args.insertOrAssign(RawBinaryReaderFilter::k_Endian_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(RawBinaryReaderFilter::k_SkipHeaderBytes_Key, std::make_any<uint64>(0));
  args.insertOrAssign(RawBinaryReaderFilter::k_CreatedAttributeArrayPath_Key, std::make_any<DataPath>(createdAttributeArrayPathValue));

  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.invalid());

  // Test non-existent input file
  inputFile = "/tmp/this_does_not_exist.bin";
  args.insertOrAssign(RawBinaryReaderFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputFile)));
  preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.invalid());

  // Update InputFile to the correct path and write the file so it exists
  inputFile = (fs::temp_directory_path() /= "raw_binary_reader_data.bin").string();
  args.insertOrAssign(RawBinaryReaderFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputFile)));

  std::array<char, 10> buffer = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  std::fstream myFile(inputFile, std::ios::out | std::ios::binary);
  myFile.write(buffer.data(), 10);

  // Test empty input file
  preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.invalid());
  myFile.close();

  // Test zero components
  args.insertOrAssign(RawBinaryReaderFilter::k_NumberOfComponents_Key, std::make_any<int32>(0));
  preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.invalid());
  args.insertOrAssign(RawBinaryReaderFilter::k_NumberOfComponents_Key, std::make_any<int32>(1));

  // Test incorrect scalar type
  args.insertOrAssign(RawBinaryReaderFilter::k_ScalarType_Key, std::make_any<NumericType>(NumericType::uint32));
  preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.invalid());
  args.insertOrAssign(RawBinaryReaderFilter::k_ScalarType_Key, std::make_any<NumericType>(NumericType::uint8));

  // Test incorrect components
  args.insertOrAssign(RawBinaryReaderFilter::k_NumberOfComponents_Key, std::make_any<int32>(3));
  preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.invalid());
  args.insertOrAssign(RawBinaryReaderFilter::k_NumberOfComponents_Key, std::make_any<int32>(1));

  //  // Execute the filter and check the result
  //  auto executeResult = filter.execute(ds, args);
  //  REQUIRE(executeResult.result.valid());
}
