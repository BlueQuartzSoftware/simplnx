/**
 * This file is auto generated from the original SyntheticBuilding/InsertPrecipitatePhases
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
 * When you start working on this unit test remove "[InsertPrecipitatePhases][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "SyntheticBuilding/Filters/InsertPrecipitatePhases.hpp"
#include "SyntheticBuilding/SyntheticBuilding_test_dirs.hpp"

using namespace complex;

TEST_CASE("SyntheticBuilding::InsertPrecipitatePhases: Instantiation and Parameter Check", "[SyntheticBuilding][InsertPrecipitatePhases][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  InsertPrecipitatePhases filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(InsertPrecipitatePhases::k_PeriodicBoundaries_Key, std::make_any<bool>(false));
  args.insertOrAssign(InsertPrecipitatePhases::k_MatchRDF_Key, std::make_any<bool>(false));
  args.insertOrAssign(InsertPrecipitatePhases::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(InsertPrecipitatePhases::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(InsertPrecipitatePhases::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(InsertPrecipitatePhases::k_BoundaryCellsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(InsertPrecipitatePhases::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(InsertPrecipitatePhases::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(InsertPrecipitatePhases::k_InputStatsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(InsertPrecipitatePhases::k_InputPhaseTypesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(InsertPrecipitatePhases::k_InputShapeTypesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(InsertPrecipitatePhases::k_NumFeaturesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(InsertPrecipitatePhases::k_PrecipInputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path("/Path/To/Input/File/To/Read.data")));
  args.insertOrAssign(InsertPrecipitatePhases::k_NewAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(InsertPrecipitatePhases::k_SelectedAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("SyntheticBuilding::InsertPrecipitatePhases: Valid filter execution")
//{
//
//}

// TEST_CASE("SyntheticBuilding::InsertPrecipitatePhases: InValid filter execution")
//{
//
//}
