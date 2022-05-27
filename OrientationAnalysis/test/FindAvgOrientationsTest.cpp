

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
