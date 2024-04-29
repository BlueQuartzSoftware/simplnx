#include "SimplnxCore/Filters/FindNeighborListStatisticsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <string>

using namespace nx::core;
using namespace nx::core::Constants;

TEST_CASE("SimplnxCore::FindNeighborListStatisticsFilter: Test Algorithm", "[FindNeighborListStatisticsFilter]")
{
  DataStructure dataStructure;
  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, "TestData");
  DataPath statsDataPath({"TestData"});
  DataPath inputArrayPath({"TestData", "Neighbor List"});
  std::string length = "Length";
  std::string minimum = "Minimum";
  std::string maximum = "Maximum";
  std::string mean = "Mean";
  std::string median = "Median";
  std::string stdDev = "Standard Deviation";
  std::string summation = "Summation";
  DataPath lengthOutputPath = statsDataPath.createChildPath(length);
  DataPath minimumOutputPath = statsDataPath.createChildPath(minimum);
  DataPath maximumOutputPath = statsDataPath.createChildPath(maximum);
  DataPath meanOutputPath = statsDataPath.createChildPath(mean);
  DataPath medianOutputPath = statsDataPath.createChildPath(median);
  DataPath stdDevOutputPath = statsDataPath.createChildPath(stdDev);
  DataPath summationOutputPath = statsDataPath.createChildPath(summation);

  usize numTuples = 3;
  auto* neighborList = NeighborList<float32>::Create(dataStructure, "Neighbor List", numTuples, topLevelGroup->getId());
  neighborList->resizeTotalElements(numTuples);
  std::vector<float32> list1 = {117, 875, 1035, 3905, 4214};
  std::vector<float32> list2 = {750, 1905, 1912, 2015, 2586, 3180, 3592, 4041, 4772};
  std::vector<float32> list3 = {309, 775, 2625, 2818, 3061, 3751, 4235, 4817};
  neighborList->setList(0, std::make_shared<std::vector<float32>>(list1));
  neighborList->setList(1, std::make_shared<std::vector<float32>>(list2));
  neighborList->setList(2, std::make_shared<std::vector<float32>>(list3));

  FindNeighborListStatisticsFilter filter;
  Arguments args;

  args.insertOrAssign(FindNeighborListStatisticsFilter::k_FindLength_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindNeighborListStatisticsFilter::k_FindMinimum_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindNeighborListStatisticsFilter::k_FindMaximum_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindNeighborListStatisticsFilter::k_FindMean_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindNeighborListStatisticsFilter::k_FindMedian_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindNeighborListStatisticsFilter::k_FindStandardDeviation_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindNeighborListStatisticsFilter::k_FindSummation_Key, std::make_any<bool>(true));

  args.insertOrAssign(FindNeighborListStatisticsFilter::k_InputNeighborListPath_Key, std::make_any<DataPath>(inputArrayPath));
  args.insertOrAssign(FindNeighborListStatisticsFilter::k_LengthName_Key, std::make_any<std::string>(length));
  args.insertOrAssign(FindNeighborListStatisticsFilter::k_MinimumName_Key, std::make_any<std::string>(minimum));
  args.insertOrAssign(FindNeighborListStatisticsFilter::k_MaximumName_Key, std::make_any<std::string>(maximum));
  args.insertOrAssign(FindNeighborListStatisticsFilter::k_MeanName_Key, std::make_any<std::string>(mean));
  args.insertOrAssign(FindNeighborListStatisticsFilter::k_MedianName_Key, std::make_any<std::string>(median));
  args.insertOrAssign(FindNeighborListStatisticsFilter::k_StandardDeviationName_Key, std::make_any<std::string>(stdDev));
  args.insertOrAssign(FindNeighborListStatisticsFilter::k_SummationName_Key, std::make_any<std::string>(summation));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());

  // Check resulting values
  auto* lengthArray = dataStructure.getDataAs<UInt64Array>(lengthOutputPath);
  REQUIRE(lengthArray != nullptr);
  auto* minArray = dataStructure.getDataAs<Float32Array>(minimumOutputPath);
  REQUIRE(minArray != nullptr);
  auto* maxArray = dataStructure.getDataAs<Float32Array>(maximumOutputPath);
  REQUIRE(maxArray != nullptr);
  auto* meanArray = dataStructure.getDataAs<Float32Array>(meanOutputPath);
  REQUIRE(meanArray != nullptr);
  auto* medianArray = dataStructure.getDataAs<Float32Array>(medianOutputPath);
  REQUIRE(medianArray != nullptr);
  auto* stdArray = dataStructure.getDataAs<Float32Array>(stdDevOutputPath);
  REQUIRE(stdArray != nullptr);
  auto* sumArray = dataStructure.getDataAs<Float32Array>(summationOutputPath);
  REQUIRE(sumArray != nullptr);

  auto lengthVal1 = (*lengthArray)[0];
  auto lengthVal2 = (*lengthArray)[1];
  auto lengthVal3 = (*lengthArray)[2];

  auto minVal1 = (*minArray)[0];
  auto minVal2 = (*minArray)[1];
  auto minVal3 = (*minArray)[2];

  auto maxVal1 = (*maxArray)[0];
  auto maxVal2 = (*maxArray)[1];
  auto maxVal3 = (*maxArray)[2];

  auto meanVal1 = (*meanArray)[0];
  auto meanVal2 = (*meanArray)[1];
  auto meanVal3 = (*meanArray)[2];

  auto meadianVal1 = (*medianArray)[0];
  auto meadianVal2 = (*medianArray)[1];
  auto meadianVal3 = (*medianArray)[2];

  auto stdVal1 = (*stdArray)[0];
  auto stdVal2 = (*stdArray)[1];
  auto stdVal3 = (*stdArray)[2];

  auto sumVal1 = (*sumArray)[0];
  auto sumVal2 = (*sumArray)[1];
  auto sumVal3 = (*sumArray)[2];

  REQUIRE(lengthVal1 == 5);
  REQUIRE(lengthVal2 == 9);
  REQUIRE(lengthVal3 == 8);

  REQUIRE(minVal1 == 117.0f);
  REQUIRE(minVal2 == 750.0f);
  REQUIRE(minVal3 == 309.0f);

  REQUIRE(maxVal1 == 4214.0f);
  REQUIRE(maxVal2 == 4772.0f);
  REQUIRE(maxVal3 == 4817.0f);

  REQUIRE(meanVal1 == 2029.2f);
  REQUIRE(meanVal2 == 2750.33325f);
  REQUIRE(meanVal3 == 2798.875f);

  REQUIRE(meadianVal1 == 1035.0f);
  REQUIRE(meadianVal2 == 2586.0f);
  REQUIRE(meadianVal3 == 2939.5f);

  REQUIRE(stdVal1 == 1689.32507f);
  REQUIRE(stdVal2 == 1184.73706f);
  REQUIRE(stdVal3 == 1476.3418f);

  REQUIRE(sumVal1 == 10146.0f);
  REQUIRE(sumVal2 == 24753.0f);
  REQUIRE(sumVal3 == 22391.0f);
}

TEST_CASE("SimplnxCore::FindNeighborListStatisticsFilter: Invalid Input Array", "[FindNeighborListStatisticsFilter]")
{
  DataStructure dataStructure;
  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, "TestData");
  DataPath statsDataPath({"TestData"});
  DataPath inputArrayPath({"TestData", "Data Array"});
  std::string length = "Length";
  std::string minimum = "Minimum";
  std::string maximum = "Maximum";
  std::string mean = "Mean";
  std::string median = "Median";
  std::string stdDev = "Standard Deviation";
  std::string summation = "Summation";
  DataPath lengthOutputPath = statsDataPath.createChildPath(length);
  DataPath minimumOutputPath = statsDataPath.createChildPath(minimum);
  DataPath maximumOutputPath = statsDataPath.createChildPath(maximum);
  DataPath meanOutputPath = statsDataPath.createChildPath(mean);
  DataPath medianOutputPath = statsDataPath.createChildPath(median);
  DataPath stdDevOutputPath = statsDataPath.createChildPath(stdDev);
  DataPath summationOutputPath = statsDataPath.createChildPath(summation);

  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{0}, std::vector<usize>{3}, 0.0f);
  auto* dataArray = DataArray<float32>::Create(dataStructure, "Data Array", std::move(dataStore), topLevelGroup->getId());
  dataArray->fill(10.0f);

  FindNeighborListStatisticsFilter filter;
  Arguments args;

  args.insertOrAssign(FindNeighborListStatisticsFilter::k_FindLength_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindNeighborListStatisticsFilter::k_FindMinimum_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindNeighborListStatisticsFilter::k_FindMaximum_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindNeighborListStatisticsFilter::k_FindMean_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindNeighborListStatisticsFilter::k_FindMedian_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindNeighborListStatisticsFilter::k_FindStandardDeviation_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindNeighborListStatisticsFilter::k_FindSummation_Key, std::make_any<bool>(true));

  args.insertOrAssign(FindNeighborListStatisticsFilter::k_InputNeighborListPath_Key, std::make_any<DataPath>(inputArrayPath));
  args.insertOrAssign(FindNeighborListStatisticsFilter::k_LengthName_Key, std::make_any<std::string>(length));
  args.insertOrAssign(FindNeighborListStatisticsFilter::k_MinimumName_Key, std::make_any<std::string>(minimum));
  args.insertOrAssign(FindNeighborListStatisticsFilter::k_MaximumName_Key, std::make_any<std::string>(maximum));
  args.insertOrAssign(FindNeighborListStatisticsFilter::k_MeanName_Key, std::make_any<std::string>(mean));
  args.insertOrAssign(FindNeighborListStatisticsFilter::k_MedianName_Key, std::make_any<std::string>(median));
  args.insertOrAssign(FindNeighborListStatisticsFilter::k_StandardDeviationName_Key, std::make_any<std::string>(stdDev));
  args.insertOrAssign(FindNeighborListStatisticsFilter::k_SummationName_Key, std::make_any<std::string>(summation));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.invalid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.invalid());
}
