/**
 * This file is auto generated from the original DREAM3DReview/ExportCLIFile
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
 * When you start working on this unit test remove "[ExportCLIFile][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "DREAM3DReview/DREAM3DReview_test_dirs.hpp"
#include "DREAM3DReview/Filters/ExportCLIFile.hpp"

using namespace complex;

TEST_CASE("DREAM3DReview::ExportCLIFile: Basic Instantiation and Parameter Check", "[ExportCLIFile][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ExportCLIFile filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insert(ExportCLIFile::k_UnitsScaleFactor_Key, std::make_any<float64>(2.3456789));
  args.insert(ExportCLIFile::k_Precision_Key, std::make_any<int32>(1234356));
  args.insert(ExportCLIFile::k_OutputDirectory_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path("/Path/To/Output/Directory/To/Read")));
  args.insert(ExportCLIFile::k_OutputFilePrefix_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insert(ExportCLIFile::k_SplitByGroup_Key, std::make_any<bool>(false));
  args.insert(ExportCLIFile::k_EdgeGeometry_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(ExportCLIFile::k_LayerIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(ExportCLIFile::k_GroupIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("DREAM3DReview::ExportCLIFile: Valid filter execution")
//{
//
//}

// TEST_CASE("DREAM3DReview::ExportCLIFile: InValid filter execution")
//{
//
//}
