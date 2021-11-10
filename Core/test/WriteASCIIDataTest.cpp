/**
 * This file is auto generated from the original Core/WriteASCIIData
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
 * When you start working on this unit test remove "[WriteASCIIData][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "Core/Core_test_dirs.hpp"
#include "Core/Filters/WriteASCIIData.hpp"

using namespace complex;

TEST_CASE("Core::WriteASCIIData: Basic Instantiation and Parameter Check", "[WriteASCIIData][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  WriteASCIIData filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insert(WriteASCIIData::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path("/Path/To/Output/Directory/To/Read")));
  args.insert(WriteASCIIData::k_FileExtension_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insert(WriteASCIIData::k_MaxValPerLine_Key, std::make_any<int32>(1234356));
  args.insert(WriteASCIIData::k_OutputFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path("/Path/To/Output/File/To/Write.data")));
  args.insert(WriteASCIIData::k_Delimiter_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insert(WriteASCIIData::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("Core::WriteASCIIData: Valid filter execution")
//{
//
//}

// TEST_CASE("Core::WriteASCIIData: InValid filter execution")
//{
//
//}
