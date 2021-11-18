/**
 * This file is auto generated from the original SimulationIO/CreateAbaqusFile
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
 * When you start working on this unit test remove "[CreateAbaqusFile][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DynamicTableFilterParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "SimulationIO/Filters/CreateAbaqusFile.hpp"
#include "SimulationIO/SimulationIO_test_dirs.hpp"

using namespace complex;

TEST_CASE("SimulationIO::CreateAbaqusFile: Instantiation and Parameter Check", "[SimulationIO][CreateAbaqusFile][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  CreateAbaqusFile filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(CreateAbaqusFile::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path("/Path/To/Output/Directory/To/Read")));
  args.insertOrAssign(CreateAbaqusFile::k_OutputFilePrefix_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insertOrAssign(CreateAbaqusFile::k_JobName_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insertOrAssign(CreateAbaqusFile::k_NumDepvar_Key, std::make_any<int32>(1234356));
  args.insertOrAssign(CreateAbaqusFile::k_NumUserOutVar_Key, std::make_any<int32>(1234356));
  /*[x]*/ args.insertOrAssign(CreateAbaqusFile::k_MatConst_Key, std::make_any<<<<NOT_IMPLEMENTED>>>>({}));
  args.insertOrAssign(CreateAbaqusFile::k_AbqFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateAbaqusFile::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateAbaqusFile::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("SimulationIO::CreateAbaqusFile: Valid filter execution")
//{
//
//}

// TEST_CASE("SimulationIO::CreateAbaqusFile: InValid filter execution")
//{
//
//}
