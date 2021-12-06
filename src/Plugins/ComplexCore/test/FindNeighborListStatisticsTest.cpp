#include <iostream>
#include <string>

#include <catch2/catch.hpp>

#include "complex/Utilities/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/FindNeighborListStatistics.hpp"

using namespace complex;
using namespace complex::Constants;

TEST_CASE("FindNeighborListStatistics: Instantiate Filter", "[FindNeighborListStatistics]")
{
  FindNeighborListStatistics filter;
  DataStructure dataGraph;
  Arguments args;

  DataPath inputArrayPath;
  DataPath lengthOutputPath;
  DataPath minimumOutputPath;
  DataPath maximumOutputPath;
  DataPath meanOutputPath;
  DataPath medianOutputPath;
  DataPath stdDevOutputPath;
  DataPath summationOutputPath;

  args.insertOrAssign(FindNeighborListStatistics::k_FindLength_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindNeighborListStatistics::k_FindMinimum_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindNeighborListStatistics::k_FindMaximum_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindNeighborListStatistics::k_FindMean_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindNeighborListStatistics::k_FindMedian_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindNeighborListStatistics::k_FindStandardDeviation_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindNeighborListStatistics::k_FindSummation_Key, std::make_any<bool>(false));

  args.insertOrAssign(FindNeighborListStatistics::k_InputArray_Key, std::make_any<DataPath>(inputArrayPath));
  args.insertOrAssign(FindNeighborListStatistics::k_Length_Key, std::make_any<DataPath>(lengthOutputPath));
  args.insertOrAssign(FindNeighborListStatistics::k_Minimum_Key, std::make_any<DataPath>(minimumOutputPath));
  args.insertOrAssign(FindNeighborListStatistics::k_Maximum_Key, std::make_any<DataPath>(maximumOutputPath));
  args.insertOrAssign(FindNeighborListStatistics::k_Mean_Key, std::make_any<DataPath>(meanOutputPath));
  args.insertOrAssign(FindNeighborListStatistics::k_Median_Key, std::make_any<DataPath>(medianOutputPath));
  args.insertOrAssign(FindNeighborListStatistics::k_StandardDeviation_Key, std::make_any<DataPath>(stdDevOutputPath));
  args.insertOrAssign(FindNeighborListStatistics::k_Summation_Key, std::make_any<DataPath>(summationOutputPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.invalid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.invalid());
}

TEST_CASE("FindNeighborListStatistics: Missing Paths", "[FindNeighborListStatistics]")
{
  FindNeighborListStatistics filter;
  DataStructure dataGraph;
  Arguments args;

  DataPath inputArrayPath;
  DataPath lengthOutputPath;
  DataPath minimumOutputPath;
  DataPath maximumOutputPath;
  DataPath meanOutputPath;
  DataPath medianOutputPath;
  DataPath stdDevOutputPath;
  DataPath summationOutputPath;

  args.insertOrAssign(FindNeighborListStatistics::k_FindLength_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindNeighborListStatistics::k_FindMinimum_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindNeighborListStatistics::k_FindMaximum_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindNeighborListStatistics::k_FindMean_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindNeighborListStatistics::k_FindMedian_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindNeighborListStatistics::k_FindStandardDeviation_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindNeighborListStatistics::k_FindSummation_Key, std::make_any<bool>(true));

  args.insertOrAssign(FindNeighborListStatistics::k_InputArray_Key, std::make_any<DataPath>(inputArrayPath));
  args.insertOrAssign(FindNeighborListStatistics::k_Length_Key, std::make_any<DataPath>(lengthOutputPath));
  args.insertOrAssign(FindNeighborListStatistics::k_Minimum_Key, std::make_any<DataPath>(minimumOutputPath));
  args.insertOrAssign(FindNeighborListStatistics::k_Maximum_Key, std::make_any<DataPath>(maximumOutputPath));
  args.insertOrAssign(FindNeighborListStatistics::k_Mean_Key, std::make_any<DataPath>(meanOutputPath));
  args.insertOrAssign(FindNeighborListStatistics::k_Median_Key, std::make_any<DataPath>(medianOutputPath));
  args.insertOrAssign(FindNeighborListStatistics::k_StandardDeviation_Key, std::make_any<DataPath>(stdDevOutputPath));
  args.insertOrAssign(FindNeighborListStatistics::k_Summation_Key, std::make_any<DataPath>(summationOutputPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.invalid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.invalid());
}

TEST_CASE("FindNeighborListStatistics: Test Algorithm", "[FindNeighborListStatistics]")
{
  DataPath statsGroup({Constants::k_SmallIN100, "Statistics"});

  FindNeighborListStatistics filter;
  DataStructure dataGraph = UnitTest::CreateDataStructure();
  dataGraph.makePath(statsGroup);
  Arguments args;

  DataPath inputArrayPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "Neighbor List"});
  DataPath lengthOutputPath = statsGroup.createChildPath("Length");
  DataPath minimumOutputPath = statsGroup.createChildPath("Minimum");
  DataPath maximumOutputPath = statsGroup.createChildPath("Maximum");
  DataPath meanOutputPath = statsGroup.createChildPath("Mean");
  DataPath medianOutputPath = statsGroup.createChildPath("Median");
  DataPath stdDevOutputPath = statsGroup.createChildPath("Standard Deviation");
  DataPath summationOutputPath = statsGroup.createChildPath("Summation");

  args.insertOrAssign(FindNeighborListStatistics::k_FindLength_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindNeighborListStatistics::k_FindMinimum_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindNeighborListStatistics::k_FindMaximum_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindNeighborListStatistics::k_FindMean_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindNeighborListStatistics::k_FindMedian_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindNeighborListStatistics::k_FindStandardDeviation_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindNeighborListStatistics::k_FindSummation_Key, std::make_any<bool>(true));

  args.insertOrAssign(FindNeighborListStatistics::k_InputArray_Key, std::make_any<DataPath>(inputArrayPath));
  args.insertOrAssign(FindNeighborListStatistics::k_Length_Key, std::make_any<DataPath>(lengthOutputPath));
  args.insertOrAssign(FindNeighborListStatistics::k_Minimum_Key, std::make_any<DataPath>(minimumOutputPath));
  args.insertOrAssign(FindNeighborListStatistics::k_Maximum_Key, std::make_any<DataPath>(maximumOutputPath));
  args.insertOrAssign(FindNeighborListStatistics::k_Mean_Key, std::make_any<DataPath>(meanOutputPath));
  args.insertOrAssign(FindNeighborListStatistics::k_Median_Key, std::make_any<DataPath>(medianOutputPath));
  args.insertOrAssign(FindNeighborListStatistics::k_StandardDeviation_Key, std::make_any<DataPath>(stdDevOutputPath));
  args.insertOrAssign(FindNeighborListStatistics::k_Summation_Key, std::make_any<DataPath>(summationOutputPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.valid());
}
