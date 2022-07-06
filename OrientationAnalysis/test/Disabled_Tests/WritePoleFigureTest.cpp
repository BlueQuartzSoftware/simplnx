/**
 * This file is auto generated from the original OrientationAnalysis/WritePoleFigure
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
 * When you start working on this unit test remove "[WritePoleFigure][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "OrientationAnalysis/Filters/WritePoleFigure.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace complex;

TEST_CASE("OrientationAnalysis::WritePoleFigure: Instantiation and Parameter Check", "[OrientationAnalysis][WritePoleFigure][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  WritePoleFigure filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(WritePoleFigure::k_Title_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insertOrAssign(WritePoleFigure::k_LambertSize_Key, std::make_any<int32>(1234356));
  args.insertOrAssign(WritePoleFigure::k_NumColors_Key, std::make_any<int32>(1234356));
  args.insertOrAssign(WritePoleFigure::k_ImageFormat_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(WritePoleFigure::k_ImageLayout_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(WritePoleFigure::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path("/Path/To/Output/Directory/To/Read")));
  args.insertOrAssign(WritePoleFigure::k_ImagePrefix_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insertOrAssign(WritePoleFigure::k_ImageSize_Key, std::make_any<int32>(1234356));
  args.insertOrAssign(WritePoleFigure::k_UseGoodVoxels_Key, std::make_any<bool>(false));
  args.insertOrAssign(WritePoleFigure::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(WritePoleFigure::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(WritePoleFigure::k_GoodVoxelsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(WritePoleFigure::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(WritePoleFigure::k_MaterialNameArrayPath_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("OrientationAnalysis::WritePoleFigure: Valid filter execution")
//{
//
//}

// TEST_CASE("OrientationAnalysis::WritePoleFigure: InValid filter execution")
//{
//
//}
