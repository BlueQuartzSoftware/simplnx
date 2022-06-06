/*
 # Test Plan #

Input Files:
DREAM3D_Data/Data/ASCII_Data/FeatureIds.csv
DREAM3D_Data/Data/ASCII_Data/Quats.csv
DREAM3D_Data/Data/ASCII_Data/Phases.csv

Output DataArrays:
 AvgEulerAngles
 AvgQuats

Comparison Files:
DREAM3D_Data/Data/ASCII_Data/AvgEulerAngles.csv
DREAM3D_Data/Data/ASCII_Data/AvgQuats.csv

You will need to create a UInt32 DataArray with 2 values in it: [ 999, 1 ]. This will
 be the input 'k_CrystalStructuresArrayPath_Key' path and data.


Compare the data sets. Due to going back and forth between ASCII and Binary you will
probably have to compare using a tolerance of about .0001. Look at the 'ConvertOrientationsTest' at the bottom for an example
 of doing that.

*/

#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/FindAvgOrientationsFilter.hpp"

#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace complex;

TEST_CASE("OrientationAnalysis::FindAvgOrientations: Invalid preflight", "[OrientationAnalysis][FindAvgOrientations]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindAvgOrientationsFilter filter;
  DataStructure dataStructure;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(FindAvgOrientationsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(FindAvgOrientationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(FindAvgOrientationsFilter::k_CellQuatsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(FindAvgOrientationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(FindAvgOrientationsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(FindAvgOrientationsFilter::k_AvgEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("OrientationAnalysis::FindAvgOrientations: Valid filter execution")
//{
//
//}

// TEST_CASE("OrientationAnalysis::FindAvgOrientations: InValid filter execution")
//{
//
//}
