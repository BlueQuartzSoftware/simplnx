/**
 * This file is auto generated from the original DREAM3DReview/FindNeighborListStatistics
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
 * When you start working on this unit test remove "[FindNeighborListStatistics][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

#include "DREAM3DReview/DREAM3DReview_test_dirs.hpp"
#include "DREAM3DReview/Filters/FindNeighborListStatistics.hpp"

using namespace complex;

TEST_CASE("DREAM3DReview::FindNeighborListStatistics: Basic Instantiation and Parameter Check", "[FindNeighborListStatistics][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindNeighborListStatistics filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insert(FindNeighborListStatistics::k_FindLength_Key, std::make_any<bool>(false));
  args.insert(FindNeighborListStatistics::k_FindMin_Key, std::make_any<bool>(false));
  args.insert(FindNeighborListStatistics::k_FindMax_Key, std::make_any<bool>(false));
  args.insert(FindNeighborListStatistics::k_FindMean_Key, std::make_any<bool>(false));
  args.insert(FindNeighborListStatistics::k_FindMedian_Key, std::make_any<bool>(false));
  args.insert(FindNeighborListStatistics::k_FindStdDeviation_Key, std::make_any<bool>(false));
  args.insert(FindNeighborListStatistics::k_FindSummation_Key, std::make_any<bool>(false));
  args.insert(FindNeighborListStatistics::k_SelectedArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(FindNeighborListStatistics::k_DestinationAttributeMatrix_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(FindNeighborListStatistics::k_LengthArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(FindNeighborListStatistics::k_MinimumArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(FindNeighborListStatistics::k_MaximumArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(FindNeighborListStatistics::k_MeanArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(FindNeighborListStatistics::k_MedianArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(FindNeighborListStatistics::k_StdDeviationArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(FindNeighborListStatistics::k_SummationArrayName_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("DREAM3DReview::FindNeighborListStatistics: Valid filter execution")
//{
//
//}

// TEST_CASE("DREAM3DReview::FindNeighborListStatistics: InValid filter execution")
//{
//
//}
