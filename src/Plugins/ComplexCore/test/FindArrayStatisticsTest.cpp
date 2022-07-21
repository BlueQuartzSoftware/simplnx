#include <catch2/catch.hpp>

#include <filesystem>
namespace fs = std::filesystem;

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArrayThresholdsParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/Dream3dImportParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/FindArrayStatisticsFilter.hpp"
#include "ComplexCore/Filters/ImportDREAM3DFilter.hpp"

using namespace complex;
using namespace complex::Constants;

TEST_CASE("FindArrayStatisticsFilter: Instantiate Filter", "[ComplexCore][FindArrayStatisticsFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindArrayStatisticsFilter filter;
  DataStructure ds;
  Arguments args;

  DataPath inputArrayPath;
  DataPath destPath;
  DataPath maskArrayPath;
  DataPath featureIdsArrayPath;

  // Create default Parameters for the filter.
  args.insertOrAssign(FindArrayStatisticsFilter::k_FindHistogram_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindArrayStatisticsFilter::k_MinRange_Key, std::make_any<float64>(2.3456789));
  args.insertOrAssign(FindArrayStatisticsFilter::k_MaxRange_Key, std::make_any<float64>(2.3456789));
  args.insertOrAssign(FindArrayStatisticsFilter::k_UseFullRange_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindArrayStatisticsFilter::k_NumBins_Key, std::make_any<int32>(1234356));
  args.insertOrAssign(FindArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindArrayStatisticsFilter::k_FindMin_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindArrayStatisticsFilter::k_FindMax_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindArrayStatisticsFilter::k_FindMean_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindArrayStatisticsFilter::k_FindMedian_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindArrayStatisticsFilter::k_FindStdDeviation_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindArrayStatisticsFilter::k_FindSummation_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindArrayStatisticsFilter::k_UseMask_Key, std::make_any<bool>(false));
  // args.insertOrAssign(FindArrayStatisticsFilter::k_ComputeByIndex_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindArrayStatisticsFilter::k_StandardizeData_Key, std::make_any<bool>(false));
  args.insertOrAssign(FindArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(inputArrayPath));
  // args.insertOrAssign(FindArrayStatisticsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsArrayPath));
  args.insertOrAssign(FindArrayStatisticsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskArrayPath));
  args.insertOrAssign(FindArrayStatisticsFilter::k_DestinationAttributeMatrix_Key, std::make_any<DataPath>(destPath));
  args.insertOrAssign(FindArrayStatisticsFilter::k_HistogramArrayName_Key, std::make_any<std::string>(""));
  args.insertOrAssign(FindArrayStatisticsFilter::k_LengthArrayName_Key, std::make_any<std::string>(""));
  args.insertOrAssign(FindArrayStatisticsFilter::k_MinimumArrayName_Key, std::make_any<std::string>(""));
  args.insertOrAssign(FindArrayStatisticsFilter::k_MaximumArrayName_Key, std::make_any<std::string>(""));
  args.insertOrAssign(FindArrayStatisticsFilter::k_MeanArrayName_Key, std::make_any<std::string>(""));
  args.insertOrAssign(FindArrayStatisticsFilter::k_MedianArrayName_Key, std::make_any<std::string>(""));
  args.insertOrAssign(FindArrayStatisticsFilter::k_StdDeviationArrayName_Key, std::make_any<std::string>(""));
  args.insertOrAssign(FindArrayStatisticsFilter::k_SummationArrayName_Key, std::make_any<std::string>(""));
  args.insertOrAssign(FindArrayStatisticsFilter::k_StandardizedArrayName_Key, std::make_any<std::string>(""));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.invalid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.invalid());
}

TEST_CASE("FindArrayStatisticsFilter: Test Algorithm", "[ComplexCore][FindArrayStatisticsFilter]")
{
  DataStructure dataStructure;
  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, "TestData");
  DataGroup* statsGroup = DataGroup::Create(dataStructure, "Statistics", topLevelGroup->getId());
  DataPath statsDataPath({"TestData", "Statistics"});
  Int32Array* testInputArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "InputArray", {10}, {1}, topLevelGroup->getId());
  auto& testInputDataStore = testInputArray->getDataStoreRef();
  testInputDataStore[0] = 1;
  testInputDataStore[1] = 20;
  testInputDataStore[2] = 13;
  testInputDataStore[3] = 45;
  testInputDataStore[4] = 5;
  testInputDataStore[5] = 16;
  testInputDataStore[6] = 73;
  testInputDataStore[7] = 22;
  testInputDataStore[8] = 9;
  testInputDataStore[9] = 10;
  BoolArray* maskArray = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {10}, {1}, topLevelGroup->getId());
  auto& maskDataStore = maskArray->getDataStoreRef();
  maskDataStore[0] = true;
  maskDataStore[1] = true;
  maskDataStore[2] = false;
  maskDataStore[3] = true;
  maskDataStore[4] = true;
  maskDataStore[5] = true;
  maskDataStore[6] = false;
  maskDataStore[7] = true;
  maskDataStore[8] = true;
  maskDataStore[9] = true;

  const std::string histogram = "Histogram";
  const std::string length = "Length";
  const std::string min = "Minimum";
  const std::string max = "Maximum";
  const std::string mean = "Mean";
  const std::string median = "Median";
  const std::string std = "Standard Deviation";
  const std::string sum = "Summation";
  const std::string standardization = "Standardization";

  // Execute the Find Array Statistics Filter
  {
    FindArrayStatisticsFilter filter;
    Arguments args;
    args.insertOrAssign(FindArrayStatisticsFilter::k_FindHistogram_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_MinRange_Key, std::make_any<float64>(0));
    args.insertOrAssign(FindArrayStatisticsFilter::k_MaxRange_Key, std::make_any<float64>(100));
    args.insertOrAssign(FindArrayStatisticsFilter::k_UseFullRange_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_NumBins_Key, std::make_any<int32>(5));
    args.insertOrAssign(FindArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_FindMin_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_FindMax_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_FindMean_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_FindMedian_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_FindStdDeviation_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_FindSummation_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_UseMask_Key, std::make_any<bool>(true));
    // args.insertOrAssign(FindArrayStatisticsFilter::k_ComputeByIndex_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_StandardizeData_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(DataPath({"TestData", "InputArray"})));
    // args.insertOrAssign(FindArrayStatisticsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>());
    args.insertOrAssign(FindArrayStatisticsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"TestData", "Mask"})));
    args.insertOrAssign(FindArrayStatisticsFilter::k_DestinationAttributeMatrix_Key, std::make_any<DataPath>(statsDataPath));
    args.insertOrAssign(FindArrayStatisticsFilter::k_HistogramArrayName_Key, std::make_any<std::string>(histogram));
    args.insertOrAssign(FindArrayStatisticsFilter::k_LengthArrayName_Key, std::make_any<std::string>(length));
    args.insertOrAssign(FindArrayStatisticsFilter::k_MinimumArrayName_Key, std::make_any<std::string>(min));
    args.insertOrAssign(FindArrayStatisticsFilter::k_MaximumArrayName_Key, std::make_any<std::string>(max));
    args.insertOrAssign(FindArrayStatisticsFilter::k_MeanArrayName_Key, std::make_any<std::string>(mean));
    args.insertOrAssign(FindArrayStatisticsFilter::k_MedianArrayName_Key, std::make_any<std::string>(median));
    args.insertOrAssign(FindArrayStatisticsFilter::k_StdDeviationArrayName_Key, std::make_any<std::string>(std));
    args.insertOrAssign(FindArrayStatisticsFilter::k_SummationArrayName_Key, std::make_any<std::string>(sum));
    args.insertOrAssign(FindArrayStatisticsFilter::k_StandardizedArrayName_Key, std::make_any<std::string>(standardization));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  // Check resulting values
  {
    auto* lengthArray = dataStructure.getDataAs<UInt64Array>(statsDataPath.createChildPath(length));
    REQUIRE(lengthArray != nullptr);
    auto* minArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(min));
    REQUIRE(minArray != nullptr);
    auto* maxArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(max));
    REQUIRE(maxArray != nullptr);
    auto* meanArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(mean));
    REQUIRE(meanArray != nullptr);
    auto* medianArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(median));
    REQUIRE(medianArray != nullptr);
    auto* stdArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(std));
    REQUIRE(stdArray != nullptr);
    auto* sumArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(sum));
    REQUIRE(sumArray != nullptr);
    auto* standardizeArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(standardization));
    REQUIRE(standardizeArray != nullptr);
    auto* histArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(histogram));
    REQUIRE(histArray != nullptr);

    auto lengthVal = (*lengthArray)[0];
    auto minVal = (*minArray)[0];
    auto maxVal = (*maxArray)[0];
    auto meanVal = (*meanArray)[0];
    auto meadianVal = (*medianArray)[0];
    auto stdVal = (*stdArray)[0];
    stdVal = std::ceil(stdVal * 100.0f) / 100.0f; // round value to 2 decimal places
    auto sumVal = (*sumArray)[0];

    REQUIRE(lengthVal == 8);
    REQUIRE(minVal == 1);
    REQUIRE(maxVal == 45);
    REQUIRE(meanVal == 16.0f);
    REQUIRE(meadianVal == 13.0f);
    REQUIRE(stdVal == 12.87f);
    REQUIRE(sumVal == 128.0f);

    const auto& standardizeDataStore = standardizeArray->getIDataStoreRefAs<AbstractDataStore<float32>>();
    auto stand0 = std::ceil(standardizeDataStore[0] * 100.0f) / 100.0f;
    auto stand1 = std::ceil(standardizeDataStore[1] * 100.0f) / 100.0f;
    auto stand2 = standardizeDataStore[2]; //
    auto stand3 = std::ceil(standardizeDataStore[3] * 100.0f) / 100.0f;
    auto stand4 = std::ceil(standardizeDataStore[4] * 100.0f) / 100.0f;
    auto stand5 = std::ceil(standardizeDataStore[5] * 100.0f) / 100.0f;
    auto stand6 = standardizeDataStore[6]; //
    auto stand7 = std::ceil(standardizeDataStore[7] * 100.0f) / 100.0f;
    auto stand8 = std::ceil(standardizeDataStore[8] * 100.0f) / 100.0f;
    auto stand9 = std::ceil(standardizeDataStore[9] * 100.0f) / 100.0f;
    REQUIRE(stand0 == -1.16f);
    REQUIRE(stand1 == 0.32f);
    REQUIRE(stand2 == 0.0f);
    REQUIRE(stand3 == 2.26f);
    REQUIRE(stand4 == -0.85f);
    REQUIRE(stand5 == 0.0f);
    REQUIRE(stand6 == 0.0f);
    REQUIRE(stand7 == .47f);
    REQUIRE(stand8 == -.54f);
    REQUIRE(stand9 == -.46f);
    REQUIRE((*histArray)[0] == 3.0f);
    REQUIRE((*histArray)[1] == 2.0f);
    REQUIRE((*histArray)[2] == 2.0f);
    REQUIRE((*histArray)[3] == 0.0f);
    REQUIRE((*histArray)[4] == 1.0f);
  }
}