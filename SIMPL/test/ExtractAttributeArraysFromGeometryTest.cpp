/**
 * This file is auto generated from the original Core/ExtractAttributeArraysFromGeometry
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
 * When you start working on this unit test remove "[ExtractAttributeArraysFromGeometry][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

#include "SIMPL/Filters/ExtractAttributeArraysFromGeometry.hpp"
#include "SIMPL/SIMPL_test_dirs.hpp"

using namespace complex;

TEST_CASE("Core::ExtractAttributeArraysFromGeometry: Instantiation and Parameter Check", "[Core][ExtractAttributeArraysFromGeometry][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ExtractAttributeArraysFromGeometry filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ExtractAttributeArraysFromGeometry::k_DataContainerName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ExtractAttributeArraysFromGeometry::k_XBoundsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ExtractAttributeArraysFromGeometry::k_YBoundsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ExtractAttributeArraysFromGeometry::k_ZBoundsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ExtractAttributeArraysFromGeometry::k_SharedVertexListArrayPath0_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ExtractAttributeArraysFromGeometry::k_SharedVertexListArrayPath1_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ExtractAttributeArraysFromGeometry::k_SharedEdgeListArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ExtractAttributeArraysFromGeometry::k_SharedVertexListArrayPath2_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ExtractAttributeArraysFromGeometry::k_SharedTriListArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ExtractAttributeArraysFromGeometry::k_SharedVertexListArrayPath3_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ExtractAttributeArraysFromGeometry::k_SharedQuadListArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ExtractAttributeArraysFromGeometry::k_SharedVertexListArrayPath4_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ExtractAttributeArraysFromGeometry::k_SharedTetListArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ExtractAttributeArraysFromGeometry::k_SharedVertexListArrayPath5_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ExtractAttributeArraysFromGeometry::k_SharedHexListArrayPath_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("Core::ExtractAttributeArraysFromGeometry: Valid filter execution")
//{
//
//}

// TEST_CASE("Core::ExtractAttributeArraysFromGeometry: InValid filter execution")
//{
//
//}
