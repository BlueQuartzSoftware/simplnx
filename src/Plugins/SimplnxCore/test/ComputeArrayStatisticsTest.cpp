#include "SimplnxCore/Filters/ComputeArrayStatisticsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

using namespace nx::core;
using namespace nx::core::Constants;
namespace fs = std::filesystem;

namespace
{
template <typename T>
bool VectorContains(const std::vector<T>& vector, T value)
{
  return (std::find(vector.begin(), vector.end(), value) != vector.end());
}
} // namespace

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Test Algorithm", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, "TestData");
  DataPath statsDataPath({"TestData", "Statistics"});
  DataPath inputArrayPath({"TestData", "InputArray"});
  Int32Array* testInputArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "InputArray", {11}, {1}, topLevelGroup->getId());
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
  testInputDataStore[10] = 1;
  BoolArray* maskArray = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {11}, {1}, topLevelGroup->getId());
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
  maskDataStore[10] = true;

  const std::string length = "Length";
  const std::string min = "Minimum";
  const std::string max = "Maximum";
  const std::string mean = "Mean";
  const std::string median = "Median";
  const std::string mode = "Mode";
  const std::string std = "Standard Deviation";
  const std::string sum = "Summation";
  const std::string standardization = "Standardization";
  const std::string numUniqueValues = "NumUniqueValues";

  // Execute the Find Array Statistics Filter
  {
    ComputeArrayStatisticsFilter filter;
    Arguments args;
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMin_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMax_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMean_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMedian_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMode_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindStdDeviation_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindSummation_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindUniqueValues_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_ComputeByIndex_Key, std::make_any<bool>(false));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StandardizeData_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(inputArrayPath));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>());
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"TestData", "Mask"})));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(statsDataPath));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_LengthArrayName_Key, std::make_any<std::string>(length));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MinimumArrayName_Key, std::make_any<std::string>(min));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaximumArrayName_Key, std::make_any<std::string>(max));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MeanArrayName_Key, std::make_any<std::string>(mean));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MedianArrayName_Key, std::make_any<std::string>(median));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_ModeArrayName_Key, std::make_any<std::string>(mode));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StdDeviationArrayName_Key, std::make_any<std::string>(std));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_SummationArrayName_Key, std::make_any<std::string>(sum));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StandardizedArrayName_Key, std::make_any<std::string>(standardization));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_NumUniqueValuesName_Key, std::make_any<std::string>(numUniqueValues));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Check resulting values
  {
    auto* amPtr = dataStructure.getDataAs<AttributeMatrix>(statsDataPath);
    REQUIRE(amPtr != nullptr);
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
    auto* modeArray = dataStructure.getDataAs<NeighborList<int32>>(statsDataPath.createChildPath(mode));
    REQUIRE(modeArray != nullptr);
    auto* stdArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(std));
    REQUIRE(stdArray != nullptr);
    auto* sumArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(sum));
    REQUIRE(sumArray != nullptr);
    auto* standardizeArray = dataStructure.getDataAs<Float32Array>(inputArrayPath.replaceName(standardization));
    REQUIRE(standardizeArray != nullptr);
    REQUIRE(standardizeArray->getNumberOfTuples() == 11);
    auto* numUniqueValuesArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(numUniqueValues));
    REQUIRE(numUniqueValuesArray != nullptr);

    auto lengthVal = (*lengthArray)[0];
    auto minVal = (*minArray)[0];
    auto maxVal = (*maxArray)[0];
    auto meanVal = (*meanArray)[0];
    auto medianVal = (*medianArray)[0];
    auto modeVals = (*modeArray).getList(0);
    auto stdVal = (*stdArray)[0];
    stdVal = std::ceil(stdVal * 100.0f) / 100.0f; // round value to 2 decimal places
    auto sumVal = (*sumArray)[0];
    auto numUnique = (*numUniqueValuesArray)[0];

    REQUIRE(lengthVal == 9);
    REQUIRE(minVal == 1);
    REQUIRE(maxVal == 45);
    REQUIRE(modeVals.size() == 1);
    REQUIRE(modeVals[0] == 1);
    REQUIRE(std::fabs(meanVal - 14.3333f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal - 10.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stdVal - 13.02f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(sumVal - 129.0f) < UnitTest::EPSILON);
    REQUIRE(numUnique == 8);

    const auto& standardizeDataStore = standardizeArray->getDataStoreRef();
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
    REQUIRE(std::fabs(stand0 - -1.01999f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand1 - 0.43999f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand2 - 0.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand3 - 2.35999f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand4 - -0.70999f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand5 - 0.12999f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand6 - 0.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand7 - .58999f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand8 - -.4f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand9 - -.33f) < UnitTest::EPSILON);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Test Algorithm By Index", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, "TestData");
  DataPath statsDataPath({"TestData", "Statistics"});
  DataPath inputArrayPath({"TestData", "InputArray"});
  Int32Array* testInputArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "InputArray", {12}, {1}, topLevelGroup->getId());
  auto& testInputDataStore = testInputArray->getDataStoreRef();
  testInputDataStore[0] = 1;
  testInputDataStore[1] = 20;
  testInputDataStore[2] = 13;
  testInputDataStore[3] = 20;
  testInputDataStore[4] = 5;
  testInputDataStore[5] = 16;
  testInputDataStore[6] = 73;
  testInputDataStore[7] = 22;
  testInputDataStore[8] = 22;
  testInputDataStore[9] = 10;
  testInputDataStore[10] = 10;
  testInputDataStore[11] = 22;
  BoolArray* maskArray = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {12}, {1}, topLevelGroup->getId());
  auto& maskDataStore = maskArray->getDataStoreRef();
  maskDataStore[0] = true;
  maskDataStore[1] = true;
  maskDataStore[2] = false;
  maskDataStore[3] = true;
  maskDataStore[4] = true;
  maskDataStore[5] = true;
  maskDataStore[6] = true;
  maskDataStore[7] = false;
  maskDataStore[8] = true;
  maskDataStore[9] = true;
  maskDataStore[10] = true;
  maskDataStore[11] = true;
  Int32Array* testFeatIdsArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {12}, {1}, topLevelGroup->getId());
  auto& testFeatIdsDataStore = testFeatIdsArray->getDataStoreRef();
  testFeatIdsDataStore[0] = 0;
  testFeatIdsDataStore[1] = 1;
  testFeatIdsDataStore[2] = 1;
  testFeatIdsDataStore[3] = 1;
  testFeatIdsDataStore[4] = 1;
  testFeatIdsDataStore[5] = 1;
  testFeatIdsDataStore[6] = 0;
  testFeatIdsDataStore[7] = 2;
  testFeatIdsDataStore[8] = 2;
  testFeatIdsDataStore[9] = 2;
  testFeatIdsDataStore[10] = 2;
  testFeatIdsDataStore[11] = 2;

  const std::string length = "Length";
  const std::string min = "Minimum";
  const std::string max = "Maximum";
  const std::string mean = "Mean";
  const std::string median = "Median";
  const std::string mode = "Mode";
  const std::string std = "Standard Deviation";
  const std::string sum = "Summation";
  const std::string standardization = "Standardization";
  const std::string numUniqueValues = "NumUniqueValues";

  // Execute the Find Array Statistics Filter
  {
    ComputeArrayStatisticsFilter filter;
    Arguments args;
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMin_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMax_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMean_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMedian_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMode_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindStdDeviation_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindSummation_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindUniqueValues_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_ComputeByIndex_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StandardizeData_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(inputArrayPath));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"TestData", "FeatureIds"})));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"TestData", "Mask"})));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(statsDataPath));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_LengthArrayName_Key, std::make_any<std::string>(length));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MinimumArrayName_Key, std::make_any<std::string>(min));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaximumArrayName_Key, std::make_any<std::string>(max));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MeanArrayName_Key, std::make_any<std::string>(mean));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MedianArrayName_Key, std::make_any<std::string>(median));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_ModeArrayName_Key, std::make_any<std::string>(mode));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StdDeviationArrayName_Key, std::make_any<std::string>(std));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_SummationArrayName_Key, std::make_any<std::string>(sum));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StandardizedArrayName_Key, std::make_any<std::string>(standardization));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_NumUniqueValuesName_Key, std::make_any<std::string>(numUniqueValues));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Check resulting values
  {
    auto* amPtr = dataStructure.getDataAs<AttributeMatrix>(statsDataPath);
    REQUIRE(amPtr != nullptr);
    auto* lengthArray = dataStructure.getDataAs<UInt64Array>(statsDataPath.createChildPath(length));
    REQUIRE(lengthArray != nullptr);
    REQUIRE(lengthArray->getNumberOfTuples() == 3);
    auto* minArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(min));
    REQUIRE(minArray != nullptr);
    REQUIRE(minArray->getNumberOfTuples() == 3);
    auto* maxArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(max));
    REQUIRE(maxArray != nullptr);
    REQUIRE(maxArray->getNumberOfTuples() == 3);
    auto* meanArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(mean));
    REQUIRE(meanArray != nullptr);
    REQUIRE(meanArray->getNumberOfTuples() == 3);
    auto* medianArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(median));
    REQUIRE(medianArray != nullptr);
    REQUIRE(medianArray->getNumberOfTuples() == 3);
    auto* modeArray = dataStructure.getDataAs<NeighborList<int32>>(statsDataPath.createChildPath(mode));
    REQUIRE(modeArray != nullptr);
    REQUIRE(modeArray->getNumberOfLists() == 3);
    auto* stdArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(std));
    REQUIRE(stdArray != nullptr);
    REQUIRE(stdArray->getNumberOfTuples() == 3);
    auto* sumArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(sum));
    REQUIRE(sumArray != nullptr);
    REQUIRE(sumArray->getNumberOfTuples() == 3);
    auto* standardizeArray = dataStructure.getDataAs<Float32Array>(inputArrayPath.replaceName(standardization));
    REQUIRE(standardizeArray != nullptr);
    REQUIRE(standardizeArray->getNumberOfTuples() == 12);
    auto* numUniqueValuesArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(numUniqueValues));
    REQUIRE(numUniqueValuesArray != nullptr);
    REQUIRE(numUniqueValuesArray->getNumberOfTuples() == 3);

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
    auto medianVal1 = (*medianArray)[0];
    auto medianVal2 = (*medianArray)[1];
    auto medianVal3 = (*medianArray)[2];
    auto modes0 = (*modeArray).getList(0);
    auto modes1 = (*modeArray).getList(1);
    auto modes2 = (*modeArray).getList(2);
    auto stdVal1 = (*stdArray)[0];
    auto stdVal2 = (*stdArray)[1];
    auto stdVal3 = (*stdArray)[2];
    auto sumVal1 = (*sumArray)[0];
    auto sumVal2 = (*sumArray)[1];
    auto sumVal3 = (*sumArray)[2];
    auto numUnique1 = (*numUniqueValuesArray)[0];
    auto numUnique2 = (*numUniqueValuesArray)[1];
    auto numUnique3 = (*numUniqueValuesArray)[2];

    REQUIRE(lengthVal1 == 2);
    REQUIRE(minVal1 == 1);
    REQUIRE(maxVal1 == 73);
    REQUIRE(std::fabs(meanVal1 - 37.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal1 - 37.0f) < UnitTest::EPSILON);
    REQUIRE(modes0.size() == 2);
    REQUIRE(VectorContains(modes0, 1) == true);
    REQUIRE(VectorContains(modes0, 73) == true);
    REQUIRE(std::fabs(stdVal1 - 36.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(sumVal1 - 74.0f) < UnitTest::EPSILON);
    REQUIRE(numUnique1 == 2);
    REQUIRE(lengthVal2 == 4);
    REQUIRE(minVal2 == 5);
    REQUIRE(maxVal2 == 20);
    REQUIRE(std::fabs(meanVal2 - 15.25f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal2 - 18.0f) < UnitTest::EPSILON);
    REQUIRE(modes1.size() == 1);
    REQUIRE(VectorContains(modes1, 20) == true);
    REQUIRE(std::fabs(stdVal2 - 6.139014) < UnitTest::EPSILON);
    REQUIRE(std::fabs(sumVal2 - 61.0f) < UnitTest::EPSILON);
    REQUIRE(numUnique2 == 3);
    REQUIRE(lengthVal3 == 4);
    REQUIRE(minVal3 == 10);
    REQUIRE(maxVal3 == 22);
    REQUIRE(std::fabs(meanVal3 - 16.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal3 - 16.0f) < UnitTest::EPSILON);
    REQUIRE(modes2.size() == 2);
    REQUIRE(VectorContains(modes2, 10) == true);
    REQUIRE(VectorContains(modes2, 22) == true);
    REQUIRE(std::fabs(stdVal3 - 6.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(sumVal3 - 64.0f) < UnitTest::EPSILON);
    REQUIRE(numUnique3 == 2);

    const auto& standardizeDataStore = standardizeArray->getDataStoreRef();
    auto stand0 = standardizeDataStore[0];
    auto stand1 = standardizeDataStore[1];
    auto stand2 = standardizeDataStore[2];
    auto stand3 = standardizeDataStore[3];
    auto stand4 = standardizeDataStore[4];
    auto stand5 = standardizeDataStore[5];
    auto stand6 = standardizeDataStore[6];
    auto stand7 = standardizeDataStore[7];
    auto stand8 = standardizeDataStore[8];
    auto stand9 = standardizeDataStore[9];

    REQUIRE(std::fabs(stand0 - -1.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand1 - 0.773739f) < UnitTest::EPSILON);
    REQUIRE(stand2 == 0.0f);
    REQUIRE(std::fabs(stand3 - 0.773739f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand4 - -1.669649f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand5 - 0.122169f) < UnitTest::EPSILON);
    REQUIRE(stand6 == 1.0f);
    REQUIRE(std::fabs(stand7 - 0.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand8 - 1.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand9 - -1.0f) < UnitTest::EPSILON);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Test Algorithm By Index - Ignore 0", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, "TestData");
  DataPath statsDataPath({"TestData", "Statistics"});
  DataPath inputArrayPath({"TestData", "InputArray"});
  Int32Array* testInputArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "InputArray", {12}, {1}, topLevelGroup->getId());
  auto& testInputDataStore = testInputArray->getDataStoreRef();
  testInputDataStore[0] = 1;
  testInputDataStore[1] = 20;
  testInputDataStore[2] = 13;
  testInputDataStore[3] = 20;
  testInputDataStore[4] = 5;
  testInputDataStore[5] = 16;
  testInputDataStore[6] = 73;
  testInputDataStore[7] = 22;
  testInputDataStore[8] = 22;
  testInputDataStore[9] = 10;
  testInputDataStore[10] = 10;
  testInputDataStore[11] = 22;
  BoolArray* maskArray = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {12}, {1}, topLevelGroup->getId());
  auto& maskDataStore = maskArray->getDataStoreRef();
  maskDataStore[0] = true;
  maskDataStore[1] = true;
  maskDataStore[2] = false;
  maskDataStore[3] = true;
  maskDataStore[4] = true;
  maskDataStore[5] = true;
  maskDataStore[6] = true;
  maskDataStore[7] = false;
  maskDataStore[8] = true;
  maskDataStore[9] = true;
  maskDataStore[10] = true;
  maskDataStore[11] = true;
  Int32Array* testFeatIdsArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {12}, {1}, topLevelGroup->getId());
  auto& testFeatIdsDataStore = testFeatIdsArray->getDataStoreRef();
  testFeatIdsDataStore[0] = 0;
  testFeatIdsDataStore[1] = 1;
  testFeatIdsDataStore[2] = 1;
  testFeatIdsDataStore[3] = 1;
  testFeatIdsDataStore[4] = 1;
  testFeatIdsDataStore[5] = 1;
  testFeatIdsDataStore[6] = 0;
  testFeatIdsDataStore[7] = 2;
  testFeatIdsDataStore[8] = 2;
  testFeatIdsDataStore[9] = 2;
  testFeatIdsDataStore[10] = 2;
  testFeatIdsDataStore[11] = 2;

  const std::string length = "Length";
  const std::string min = "Minimum";
  const std::string max = "Maximum";
  const std::string mean = "Mean";
  const std::string median = "Median";
  const std::string mode = "Mode";
  const std::string std = "Standard Deviation";
  const std::string sum = "Summation";
  const std::string standardization = "Standardization";
  const std::string numUniqueValues = "NumUniqueValues";
  const std::string featureIdMapping = "FeatureIdMapping";

  // Execute the Find Array Statistics Filter
  {
    ComputeArrayStatisticsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeArrayStatisticsFilter::k_RangeType_Key, std::make_any<ChoicesParameter::ValueType>(1ULL)); // Ignore feature 0
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FeatureIdsIndexingName_Key, std::make_any<std::string>(featureIdMapping));

    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMin_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMax_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMean_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMedian_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMode_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindStdDeviation_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindSummation_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindUniqueValues_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_ComputeByIndex_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StandardizeData_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(inputArrayPath));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"TestData", "FeatureIds"})));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"TestData", "Mask"})));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(statsDataPath));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_LengthArrayName_Key, std::make_any<std::string>(length));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MinimumArrayName_Key, std::make_any<std::string>(min));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaximumArrayName_Key, std::make_any<std::string>(max));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MeanArrayName_Key, std::make_any<std::string>(mean));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MedianArrayName_Key, std::make_any<std::string>(median));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_ModeArrayName_Key, std::make_any<std::string>(mode));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StdDeviationArrayName_Key, std::make_any<std::string>(std));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_SummationArrayName_Key, std::make_any<std::string>(sum));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StandardizedArrayName_Key, std::make_any<std::string>(standardization));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_NumUniqueValuesName_Key, std::make_any<std::string>(numUniqueValues));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Check resulting values
  {
    auto* amPtr = dataStructure.getDataAs<AttributeMatrix>(statsDataPath);
    REQUIRE(amPtr != nullptr);
    auto* lengthArray = dataStructure.getDataAs<UInt64Array>(statsDataPath.createChildPath(length));
    REQUIRE(lengthArray != nullptr);
    REQUIRE(lengthArray->getNumberOfTuples() == 2);
    auto* minArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(min));
    REQUIRE(minArray != nullptr);
    REQUIRE(minArray->getNumberOfTuples() == 2);
    auto* maxArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(max));
    REQUIRE(maxArray != nullptr);
    REQUIRE(maxArray->getNumberOfTuples() == 2);
    auto* meanArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(mean));
    REQUIRE(meanArray != nullptr);
    REQUIRE(meanArray->getNumberOfTuples() == 2);
    auto* medianArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(median));
    REQUIRE(medianArray != nullptr);
    REQUIRE(medianArray->getNumberOfTuples() == 2);
    auto* modeArray = dataStructure.getDataAs<NeighborList<int32>>(statsDataPath.createChildPath(mode));
    REQUIRE(modeArray != nullptr);
    REQUIRE(modeArray->getNumberOfLists() == 2);
    auto* stdArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(std));
    REQUIRE(stdArray != nullptr);
    REQUIRE(stdArray->getNumberOfTuples() == 2);
    auto* sumArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(sum));
    REQUIRE(sumArray != nullptr);
    REQUIRE(sumArray->getNumberOfTuples() == 2);
    auto* standardizeArray = dataStructure.getDataAs<Float32Array>(inputArrayPath.replaceName(standardization));
    REQUIRE(standardizeArray != nullptr);
    REQUIRE(standardizeArray->getNumberOfTuples() == 12);
    auto* numUniqueValuesArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(numUniqueValues));
    REQUIRE(numUniqueValuesArray != nullptr);
    REQUIRE(numUniqueValuesArray->getNumberOfTuples() == 2);
    auto* featureIdMappingArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(featureIdMapping));
    REQUIRE(featureIdMappingArray != nullptr);
    REQUIRE(featureIdMappingArray->getNumberOfTuples() == 2);

    auto lengthVal1 = (*lengthArray)[0];
    auto lengthVal2 = (*lengthArray)[1];
    auto minVal1 = (*minArray)[0];
    auto minVal2 = (*minArray)[1];
    auto maxVal1 = (*maxArray)[0];
    auto maxVal2 = (*maxArray)[1];
    auto meanVal1 = (*meanArray)[0];
    auto meanVal2 = (*meanArray)[1];
    auto medianVal1 = (*medianArray)[0];
    auto medianVal2 = (*medianArray)[1];
    auto modes1 = (*modeArray).getList(0);
    auto modes2 = (*modeArray).getList(1);
    auto stdVal1 = (*stdArray)[0];
    auto stdVal2 = (*stdArray)[1];
    auto sumVal1 = (*sumArray)[0];
    auto sumVal2 = (*sumArray)[1];
    auto numUnique1 = (*numUniqueValuesArray)[0];
    auto numUnique2 = (*numUniqueValuesArray)[1];
    auto mapping1 = (*featureIdMappingArray)[0];
    auto mapping2 = (*featureIdMappingArray)[1];

    REQUIRE(mapping1 == 1);
    REQUIRE(lengthVal1 == 4);
    REQUIRE(minVal1 == 5);
    REQUIRE(maxVal1 == 20);
    REQUIRE(std::fabs(meanVal1 - 15.25f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal1 - 18.0f) < UnitTest::EPSILON);
    REQUIRE(modes1.size() == 1);
    REQUIRE(VectorContains(modes1, 20) == true);
    REQUIRE(std::fabs(stdVal1 - 6.139014) < UnitTest::EPSILON);
    REQUIRE(std::fabs(sumVal1 - 61.0f) < UnitTest::EPSILON);
    REQUIRE(numUnique1 == 3);
    REQUIRE(mapping2 == 2);
    REQUIRE(lengthVal2 == 4);
    REQUIRE(minVal2 == 10);
    REQUIRE(maxVal2 == 22);
    REQUIRE(std::fabs(meanVal2 - 16.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal2 - 16.0f) < UnitTest::EPSILON);
    REQUIRE(modes2.size() == 2);
    REQUIRE(VectorContains(modes2, 10) == true);
    REQUIRE(VectorContains(modes2, 22) == true);
    REQUIRE(std::fabs(stdVal2 - 6.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(sumVal2 - 64.0f) < UnitTest::EPSILON);
    REQUIRE(numUnique2 == 2);

    const auto& standardizeDataStore = standardizeArray->getDataStoreRef();
    auto stand0 = standardizeDataStore[0];
    auto stand1 = standardizeDataStore[1];
    auto stand2 = standardizeDataStore[2];
    auto stand3 = standardizeDataStore[3];
    auto stand4 = standardizeDataStore[4];
    auto stand5 = standardizeDataStore[5];
    auto stand6 = standardizeDataStore[6];
    auto stand7 = standardizeDataStore[7];
    auto stand8 = standardizeDataStore[8];
    auto stand9 = standardizeDataStore[9];

    REQUIRE(std::fabs(stand0 - 0.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand1 - 0.773739f) < UnitTest::EPSILON);
    REQUIRE(stand2 == 0.0f);
    REQUIRE(std::fabs(stand3 - 0.773739f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand4 - -1.669649f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand5 - 0.122169f) < UnitTest::EPSILON);
    REQUIRE(stand6 == 0.0f);
    REQUIRE(std::fabs(stand7 - 0.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand8 - 1.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand9 - -1.0f) < UnitTest::EPSILON);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Test Algorithm By Index - Shrink to Fit", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, "TestData");
  DataPath statsDataPath({"TestData", "Statistics"});
  DataPath inputArrayPath({"TestData", "InputArray"});
  Int32Array* testInputArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "InputArray", {12}, {1}, topLevelGroup->getId());
  auto& testInputDataStore = testInputArray->getDataStoreRef();
  testInputDataStore[0] = 1;
  testInputDataStore[1] = 20;
  testInputDataStore[2] = 13;
  testInputDataStore[3] = 20;
  testInputDataStore[4] = 5;
  testInputDataStore[5] = 16;
  testInputDataStore[6] = 73;
  testInputDataStore[7] = 22;
  testInputDataStore[8] = 22;
  testInputDataStore[9] = 10;
  testInputDataStore[10] = 10;
  testInputDataStore[11] = 22;
  BoolArray* maskArray = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {12}, {1}, topLevelGroup->getId());
  auto& maskDataStore = maskArray->getDataStoreRef();
  maskDataStore[0] = true;
  maskDataStore[1] = true;
  maskDataStore[2] = false;
  maskDataStore[3] = true;
  maskDataStore[4] = true;
  maskDataStore[5] = true;
  maskDataStore[6] = true;
  maskDataStore[7] = false;
  maskDataStore[8] = true;
  maskDataStore[9] = true;
  maskDataStore[10] = true;
  maskDataStore[11] = true;
  Int32Array* testFeatIdsArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {12}, {1}, topLevelGroup->getId());
  auto& testFeatIdsDataStore = testFeatIdsArray->getDataStoreRef();
  testFeatIdsDataStore[0] = 0;
  testFeatIdsDataStore[1] = 1;
  testFeatIdsDataStore[2] = 1;
  testFeatIdsDataStore[3] = 1;
  testFeatIdsDataStore[4] = 1;
  testFeatIdsDataStore[5] = 1;
  testFeatIdsDataStore[6] = 0;
  testFeatIdsDataStore[7] = 2;
  testFeatIdsDataStore[8] = 2;
  testFeatIdsDataStore[9] = 2;
  testFeatIdsDataStore[10] = 2;
  testFeatIdsDataStore[11] = 2;

  const std::string length = "Length";
  const std::string min = "Minimum";
  const std::string max = "Maximum";
  const std::string mean = "Mean";
  const std::string median = "Median";
  const std::string mode = "Mode";
  const std::string std = "Standard Deviation";
  const std::string sum = "Summation";
  const std::string standardization = "Standardization";
  const std::string numUniqueValues = "NumUniqueValues";
  const std::string featureIdMapping = "FeatureIdMapping";

  // Execute the Find Array Statistics Filter
  {
    ComputeArrayStatisticsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeArrayStatisticsFilter::k_RangeType_Key, std::make_any<ChoicesParameter::ValueType>(2ULL)); // Shrink To Fit
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FeatureIdsIndexingName_Key, std::make_any<std::string>(featureIdMapping));

    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMin_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMax_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMean_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMedian_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMode_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindStdDeviation_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindSummation_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindUniqueValues_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_ComputeByIndex_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StandardizeData_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(inputArrayPath));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"TestData", "FeatureIds"})));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"TestData", "Mask"})));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(statsDataPath));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_LengthArrayName_Key, std::make_any<std::string>(length));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MinimumArrayName_Key, std::make_any<std::string>(min));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaximumArrayName_Key, std::make_any<std::string>(max));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MeanArrayName_Key, std::make_any<std::string>(mean));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MedianArrayName_Key, std::make_any<std::string>(median));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_ModeArrayName_Key, std::make_any<std::string>(mode));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StdDeviationArrayName_Key, std::make_any<std::string>(std));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_SummationArrayName_Key, std::make_any<std::string>(sum));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StandardizedArrayName_Key, std::make_any<std::string>(standardization));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_NumUniqueValuesName_Key, std::make_any<std::string>(numUniqueValues));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Check resulting values
  {
    auto* amPtr = dataStructure.getDataAs<AttributeMatrix>(statsDataPath);
    REQUIRE(amPtr != nullptr);
    auto* lengthArray = dataStructure.getDataAs<UInt64Array>(statsDataPath.createChildPath(length));
    REQUIRE(lengthArray != nullptr);
    REQUIRE(lengthArray->getNumberOfTuples() == 3);
    auto* minArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(min));
    REQUIRE(minArray != nullptr);
    REQUIRE(minArray->getNumberOfTuples() == 3);
    auto* maxArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(max));
    REQUIRE(maxArray != nullptr);
    REQUIRE(maxArray->getNumberOfTuples() == 3);
    auto* meanArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(mean));
    REQUIRE(meanArray != nullptr);
    REQUIRE(meanArray->getNumberOfTuples() == 3);
    auto* medianArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(median));
    REQUIRE(medianArray != nullptr);
    REQUIRE(medianArray->getNumberOfTuples() == 3);
    auto* modeArray = dataStructure.getDataAs<NeighborList<int32>>(statsDataPath.createChildPath(mode));
    REQUIRE(modeArray != nullptr);
    REQUIRE(modeArray->getNumberOfLists() == 3);
    auto* stdArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(std));
    REQUIRE(stdArray != nullptr);
    REQUIRE(stdArray->getNumberOfTuples() == 3);
    auto* sumArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(sum));
    REQUIRE(sumArray != nullptr);
    REQUIRE(sumArray->getNumberOfTuples() == 3);
    auto* standardizeArray = dataStructure.getDataAs<Float32Array>(inputArrayPath.replaceName(standardization));
    REQUIRE(standardizeArray != nullptr);
    REQUIRE(standardizeArray->getNumberOfTuples() == 12);
    auto* numUniqueValuesArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(numUniqueValues));
    REQUIRE(numUniqueValuesArray != nullptr);
    REQUIRE(numUniqueValuesArray->getNumberOfTuples() == 3);
    auto* featureIdMappingArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(featureIdMapping));
    REQUIRE(featureIdMappingArray != nullptr);
    REQUIRE(featureIdMappingArray->getNumberOfTuples() == 3);

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
    auto medianVal1 = (*medianArray)[0];
    auto medianVal2 = (*medianArray)[1];
    auto medianVal3 = (*medianArray)[2];
    auto modes0 = (*modeArray).getList(0);
    auto modes1 = (*modeArray).getList(1);
    auto modes2 = (*modeArray).getList(2);
    auto stdVal1 = (*stdArray)[0];
    auto stdVal2 = (*stdArray)[1];
    auto stdVal3 = (*stdArray)[2];
    auto sumVal1 = (*sumArray)[0];
    auto sumVal2 = (*sumArray)[1];
    auto sumVal3 = (*sumArray)[2];
    auto numUnique1 = (*numUniqueValuesArray)[0];
    auto numUnique2 = (*numUniqueValuesArray)[1];
    auto numUnique3 = (*numUniqueValuesArray)[2];
    auto mapping1 = (*featureIdMappingArray)[0];
    auto mapping2 = (*featureIdMappingArray)[1];
    auto mapping3 = (*featureIdMappingArray)[2];

    REQUIRE(mapping1 == 0);
    REQUIRE(lengthVal1 == 2);
    REQUIRE(minVal1 == 1);
    REQUIRE(maxVal1 == 73);
    REQUIRE(std::fabs(meanVal1 - 37.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal1 - 37.0f) < UnitTest::EPSILON);
    REQUIRE(modes0.size() == 2);
    REQUIRE(VectorContains(modes0, 1) == true);
    REQUIRE(VectorContains(modes0, 73) == true);
    REQUIRE(std::fabs(stdVal1 - 36.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(sumVal1 - 74.0f) < UnitTest::EPSILON);
    REQUIRE(numUnique1 == 2);
    REQUIRE(mapping2 == 1);
    REQUIRE(lengthVal2 == 4);
    REQUIRE(minVal2 == 5);
    REQUIRE(maxVal2 == 20);
    REQUIRE(std::fabs(meanVal2 - 15.25f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal2 - 18.0f) < UnitTest::EPSILON);
    REQUIRE(modes1.size() == 1);
    REQUIRE(VectorContains(modes1, 20) == true);
    REQUIRE(std::fabs(stdVal2 - 6.139014) < UnitTest::EPSILON);
    REQUIRE(std::fabs(sumVal2 - 61.0f) < UnitTest::EPSILON);
    REQUIRE(numUnique2 == 3);
    REQUIRE(mapping3 == 2);
    REQUIRE(lengthVal3 == 4);
    REQUIRE(minVal3 == 10);
    REQUIRE(maxVal3 == 22);
    REQUIRE(std::fabs(meanVal3 - 16.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal3 - 16.0f) < UnitTest::EPSILON);
    REQUIRE(modes2.size() == 2);
    REQUIRE(VectorContains(modes2, 10) == true);
    REQUIRE(VectorContains(modes2, 22) == true);
    REQUIRE(std::fabs(stdVal3 - 6.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(sumVal3 - 64.0f) < UnitTest::EPSILON);
    REQUIRE(numUnique3 == 2);

    const auto& standardizeDataStore = standardizeArray->getDataStoreRef();
    auto stand0 = standardizeDataStore[0];
    auto stand1 = standardizeDataStore[1];
    auto stand2 = standardizeDataStore[2];
    auto stand3 = standardizeDataStore[3];
    auto stand4 = standardizeDataStore[4];
    auto stand5 = standardizeDataStore[5];
    auto stand6 = standardizeDataStore[6];
    auto stand7 = standardizeDataStore[7];
    auto stand8 = standardizeDataStore[8];
    auto stand9 = standardizeDataStore[9];

    REQUIRE(std::fabs(stand0 - -1.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand1 - 0.773739f) < UnitTest::EPSILON);
    REQUIRE(stand2 == 0.0f);
    REQUIRE(std::fabs(stand3 - 0.773739f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand4 - -1.669649f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand5 - 0.122169f) < UnitTest::EPSILON);
    REQUIRE(stand6 == 1.0f);
    REQUIRE(std::fabs(stand7 - 0.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand8 - 1.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand9 - -1.0f) < UnitTest::EPSILON);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Test Algorithm By Index - Padded Custom Range", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, "TestData");
  DataPath statsDataPath({"TestData", "Statistics"});
  DataPath inputArrayPath({"TestData", "InputArray"});
  Int32Array* testInputArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "InputArray", {12}, {1}, topLevelGroup->getId());
  auto& testInputDataStore = testInputArray->getDataStoreRef();
  testInputDataStore[0] = 1;
  testInputDataStore[1] = 20;
  testInputDataStore[2] = 13;
  testInputDataStore[3] = 20;
  testInputDataStore[4] = 5;
  testInputDataStore[5] = 16;
  testInputDataStore[6] = 73;
  testInputDataStore[7] = 22;
  testInputDataStore[8] = 22;
  testInputDataStore[9] = 10;
  testInputDataStore[10] = 10;
  testInputDataStore[11] = 22;
  BoolArray* maskArray = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {12}, {1}, topLevelGroup->getId());
  auto& maskDataStore = maskArray->getDataStoreRef();
  maskDataStore[0] = true;
  maskDataStore[1] = true;
  maskDataStore[2] = false;
  maskDataStore[3] = true;
  maskDataStore[4] = true;
  maskDataStore[5] = true;
  maskDataStore[6] = true;
  maskDataStore[7] = false;
  maskDataStore[8] = true;
  maskDataStore[9] = true;
  maskDataStore[10] = true;
  maskDataStore[11] = true;
  Int32Array* testFeatIdsArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {12}, {1}, topLevelGroup->getId());
  auto& testFeatIdsDataStore = testFeatIdsArray->getDataStoreRef();
  testFeatIdsDataStore[0] = 0;
  testFeatIdsDataStore[1] = 1;
  testFeatIdsDataStore[2] = 1;
  testFeatIdsDataStore[3] = 1;
  testFeatIdsDataStore[4] = 1;
  testFeatIdsDataStore[5] = 1;
  testFeatIdsDataStore[6] = 0;
  testFeatIdsDataStore[7] = 2;
  testFeatIdsDataStore[8] = 2;
  testFeatIdsDataStore[9] = 2;
  testFeatIdsDataStore[10] = 2;
  testFeatIdsDataStore[11] = 2;

  const std::string length = "Length";
  const std::string min = "Minimum";
  const std::string max = "Maximum";
  const std::string mean = "Mean";
  const std::string median = "Median";
  const std::string mode = "Mode";
  const std::string std = "Standard Deviation";
  const std::string sum = "Summation";
  const std::string standardization = "Standardization";
  const std::string numUniqueValues = "NumUniqueValues";
  const std::string featureIdMapping = "FeatureIdMapping";
  const std::string featureHasData = "FeatureHasData";

  // Execute the Find Array Statistics Filter
  {
    ComputeArrayStatisticsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeArrayStatisticsFilter::k_RangeType_Key, std::make_any<ChoicesParameter::ValueType>(3ULL)); // Padded Custom Range
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FeatureIdsIndexingName_Key, std::make_any<std::string>(featureIdMapping));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FeatureHasDataArrayName_Key, std::make_any<std::string>(featureHasData));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_Range_Key, std::make_any<VectorInt32Parameter::ValueType>({1, 4}));

    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMin_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMax_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMean_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMedian_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMode_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindStdDeviation_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindSummation_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindUniqueValues_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_ComputeByIndex_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StandardizeData_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(inputArrayPath));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"TestData", "FeatureIds"})));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"TestData", "Mask"})));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(statsDataPath));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_LengthArrayName_Key, std::make_any<std::string>(length));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MinimumArrayName_Key, std::make_any<std::string>(min));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaximumArrayName_Key, std::make_any<std::string>(max));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MeanArrayName_Key, std::make_any<std::string>(mean));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MedianArrayName_Key, std::make_any<std::string>(median));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_ModeArrayName_Key, std::make_any<std::string>(mode));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StdDeviationArrayName_Key, std::make_any<std::string>(std));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_SummationArrayName_Key, std::make_any<std::string>(sum));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StandardizedArrayName_Key, std::make_any<std::string>(standardization));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_NumUniqueValuesName_Key, std::make_any<std::string>(numUniqueValues));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Check resulting values
  {
    auto* amPtr = dataStructure.getDataAs<AttributeMatrix>(statsDataPath);
    REQUIRE(amPtr != nullptr);
    auto* lengthArray = dataStructure.getDataAs<UInt64Array>(statsDataPath.createChildPath(length));
    REQUIRE(lengthArray != nullptr);
    REQUIRE(lengthArray->getNumberOfTuples() == 4);
    auto* minArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(min));
    REQUIRE(minArray != nullptr);
    REQUIRE(minArray->getNumberOfTuples() == 4);
    auto* maxArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(max));
    REQUIRE(maxArray != nullptr);
    REQUIRE(maxArray->getNumberOfTuples() == 4);
    auto* meanArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(mean));
    REQUIRE(meanArray != nullptr);
    REQUIRE(meanArray->getNumberOfTuples() == 4);
    auto* medianArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(median));
    REQUIRE(medianArray != nullptr);
    REQUIRE(medianArray->getNumberOfTuples() == 4);
    auto* modeArray = dataStructure.getDataAs<NeighborList<int32>>(statsDataPath.createChildPath(mode));
    REQUIRE(modeArray != nullptr);
    REQUIRE(modeArray->getNumberOfLists() == 4);
    auto* stdArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(std));
    REQUIRE(stdArray != nullptr);
    REQUIRE(stdArray->getNumberOfTuples() == 4);
    auto* sumArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(sum));
    REQUIRE(sumArray != nullptr);
    REQUIRE(sumArray->getNumberOfTuples() == 4);
    auto* standardizeArray = dataStructure.getDataAs<Float32Array>(inputArrayPath.replaceName(standardization));
    REQUIRE(standardizeArray != nullptr);
    REQUIRE(standardizeArray->getNumberOfTuples() == 12);
    auto* numUniqueValuesArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(numUniqueValues));
    REQUIRE(numUniqueValuesArray != nullptr);
    REQUIRE(numUniqueValuesArray->getNumberOfTuples() == 4);
    auto* featureIdMappingArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(featureIdMapping));
    REQUIRE(featureIdMappingArray != nullptr);
    REQUIRE(featureIdMappingArray->getNumberOfTuples() == 4);
    auto* featureHasDataArray = dataStructure.getDataAs<BoolArray>(statsDataPath.createChildPath(featureHasData));
    REQUIRE(featureHasDataArray != nullptr);
    REQUIRE(featureHasDataArray->getNumberOfTuples() == 4);

    auto lengthVal1 = (*lengthArray)[0];
    auto lengthVal2 = (*lengthArray)[1];
    auto minVal1 = (*minArray)[0];
    auto minVal2 = (*minArray)[1];
    auto maxVal1 = (*maxArray)[0];
    auto maxVal2 = (*maxArray)[1];
    auto meanVal1 = (*meanArray)[0];
    auto meanVal2 = (*meanArray)[1];
    auto medianVal1 = (*medianArray)[0];
    auto medianVal2 = (*medianArray)[1];
    auto modes1 = (*modeArray).getList(0);
    auto modes2 = (*modeArray).getList(1);
    auto stdVal1 = (*stdArray)[0];
    auto stdVal2 = (*stdArray)[1];
    auto sumVal1 = (*sumArray)[0];
    auto sumVal2 = (*sumArray)[1];
    auto numUnique1 = (*numUniqueValuesArray)[0];
    auto numUnique2 = (*numUniqueValuesArray)[1];
    auto mapping1 = (*featureIdMappingArray)[0];
    auto mapping2 = (*featureIdMappingArray)[1];
    auto mapping3 = (*featureIdMappingArray)[2];
    auto mapping4 = (*featureIdMappingArray)[3];
    auto featureHasData1 = (*featureHasDataArray)[0];
    auto featureHasData2 = (*featureHasDataArray)[1];
    auto featureHasData3 = (*featureHasDataArray)[2];
    auto featureHasData4 = (*featureHasDataArray)[3];

    REQUIRE(mapping1 == 1);
    REQUIRE(lengthVal1 == 4);
    REQUIRE(minVal1 == 5);
    REQUIRE(maxVal1 == 20);
    REQUIRE(std::fabs(meanVal1 - 15.25f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal1 - 18.0f) < UnitTest::EPSILON);
    REQUIRE(modes1.size() == 1);
    REQUIRE(VectorContains(modes1, 20) == true);
    REQUIRE(std::fabs(stdVal1 - 6.139014) < UnitTest::EPSILON);
    REQUIRE(std::fabs(sumVal1 - 61.0f) < UnitTest::EPSILON);
    REQUIRE(numUnique1 == 3);
    REQUIRE(mapping2 == 2);
    REQUIRE(lengthVal2 == 4);
    REQUIRE(minVal2 == 10);
    REQUIRE(maxVal2 == 22);
    REQUIRE(std::fabs(meanVal2 - 16.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal2 - 16.0f) < UnitTest::EPSILON);
    REQUIRE(modes2.size() == 2);
    REQUIRE(VectorContains(modes2, 10) == true);
    REQUIRE(VectorContains(modes2, 22) == true);
    REQUIRE(std::fabs(stdVal2 - 6.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(sumVal2 - 64.0f) < UnitTest::EPSILON);
    REQUIRE(numUnique2 == 2);
    REQUIRE(mapping3 == 3);
    REQUIRE(mapping4 == 4);
    REQUIRE(featureHasData1);
    REQUIRE(featureHasData2);
    REQUIRE(!featureHasData3);
    REQUIRE(!featureHasData4);

    const auto& standardizeDataStore = standardizeArray->getDataStoreRef();
    auto stand0 = standardizeDataStore[0];
    auto stand1 = standardizeDataStore[1];
    auto stand2 = standardizeDataStore[2];
    auto stand3 = standardizeDataStore[3];
    auto stand4 = standardizeDataStore[4];
    auto stand5 = standardizeDataStore[5];
    auto stand6 = standardizeDataStore[6];
    auto stand7 = standardizeDataStore[7];
    auto stand8 = standardizeDataStore[8];
    auto stand9 = standardizeDataStore[9];

    REQUIRE(std::fabs(stand0 - 0.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand1 - 0.773739f) < UnitTest::EPSILON);
    REQUIRE(stand2 == 0.0f);
    REQUIRE(std::fabs(stand3 - 0.773739f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand4 - -1.669649f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand5 - 0.122169f) < UnitTest::EPSILON);
    REQUIRE(stand6 == 0.0f);
    REQUIRE(std::fabs(stand7 - 0.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand8 - 1.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand9 - -1.0f) < UnitTest::EPSILON);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: Test Algorithm By Index - Custom Range", "[SimplnxCore][ComputeArrayStatisticsFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure;
  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, "TestData");
  DataPath statsDataPath({"TestData", "Statistics"});
  DataPath inputArrayPath({"TestData", "InputArray"});
  Int32Array* testInputArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "InputArray", {12}, {1}, topLevelGroup->getId());
  auto& testInputDataStore = testInputArray->getDataStoreRef();
  testInputDataStore[0] = 1;
  testInputDataStore[1] = 20;
  testInputDataStore[2] = 13;
  testInputDataStore[3] = 20;
  testInputDataStore[4] = 5;
  testInputDataStore[5] = 16;
  testInputDataStore[6] = 73;
  testInputDataStore[7] = 22;
  testInputDataStore[8] = 22;
  testInputDataStore[9] = 10;
  testInputDataStore[10] = 10;
  testInputDataStore[11] = 22;
  BoolArray* maskArray = BoolArray::CreateWithStore<DataStore<bool>>(dataStructure, "Mask", {12}, {1}, topLevelGroup->getId());
  auto& maskDataStore = maskArray->getDataStoreRef();
  maskDataStore[0] = true;
  maskDataStore[1] = true;
  maskDataStore[2] = false;
  maskDataStore[3] = true;
  maskDataStore[4] = true;
  maskDataStore[5] = true;
  maskDataStore[6] = true;
  maskDataStore[7] = false;
  maskDataStore[8] = true;
  maskDataStore[9] = true;
  maskDataStore[10] = true;
  maskDataStore[11] = true;
  Int32Array* testFeatIdsArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, "FeatureIds", {12}, {1}, topLevelGroup->getId());
  auto& testFeatIdsDataStore = testFeatIdsArray->getDataStoreRef();
  testFeatIdsDataStore[0] = 0;
  testFeatIdsDataStore[1] = 1;
  testFeatIdsDataStore[2] = 1;
  testFeatIdsDataStore[3] = 1;
  testFeatIdsDataStore[4] = 1;
  testFeatIdsDataStore[5] = 1;
  testFeatIdsDataStore[6] = 0;
  testFeatIdsDataStore[7] = 2;
  testFeatIdsDataStore[8] = 2;
  testFeatIdsDataStore[9] = 2;
  testFeatIdsDataStore[10] = 2;
  testFeatIdsDataStore[11] = 2;

  const std::string length = "Length";
  const std::string min = "Minimum";
  const std::string max = "Maximum";
  const std::string mean = "Mean";
  const std::string median = "Median";
  const std::string mode = "Mode";
  const std::string std = "Standard Deviation";
  const std::string sum = "Summation";
  const std::string standardization = "Standardization";
  const std::string numUniqueValues = "NumUniqueValues";
  const std::string featureIdMapping = "FeatureIdMapping";

  // Execute the Find Array Statistics Filter
  {
    ComputeArrayStatisticsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeArrayStatisticsFilter::k_RangeType_Key, std::make_any<ChoicesParameter::ValueType>(4ULL)); // Custom Range
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FeatureIdsIndexingName_Key, std::make_any<std::string>(featureIdMapping));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_Range_Key, std::make_any<VectorInt32Parameter::ValueType>({2, 4}));

    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindLength_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMin_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMax_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMean_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMedian_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindMode_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindStdDeviation_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindSummation_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_FindUniqueValues_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_ComputeByIndex_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StandardizeData_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(inputArrayPath));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"TestData", "FeatureIds"})));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"TestData", "Mask"})));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key, std::make_any<DataPath>(statsDataPath));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_LengthArrayName_Key, std::make_any<std::string>(length));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MinimumArrayName_Key, std::make_any<std::string>(min));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MaximumArrayName_Key, std::make_any<std::string>(max));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MeanArrayName_Key, std::make_any<std::string>(mean));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_MedianArrayName_Key, std::make_any<std::string>(median));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_ModeArrayName_Key, std::make_any<std::string>(mode));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StdDeviationArrayName_Key, std::make_any<std::string>(std));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_SummationArrayName_Key, std::make_any<std::string>(sum));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_StandardizedArrayName_Key, std::make_any<std::string>(standardization));
    args.insertOrAssign(ComputeArrayStatisticsFilter::k_NumUniqueValuesName_Key, std::make_any<std::string>(numUniqueValues));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Check resulting values
  {
    auto* amPtr = dataStructure.getDataAs<AttributeMatrix>(statsDataPath);
    REQUIRE(amPtr != nullptr);
    auto* lengthArray = dataStructure.getDataAs<UInt64Array>(statsDataPath.createChildPath(length));
    REQUIRE(lengthArray != nullptr);
    REQUIRE(lengthArray->getNumberOfTuples() == 1);
    auto* minArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(min));
    REQUIRE(minArray != nullptr);
    REQUIRE(minArray->getNumberOfTuples() == 1);
    auto* maxArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(max));
    REQUIRE(maxArray != nullptr);
    REQUIRE(maxArray->getNumberOfTuples() == 1);
    auto* meanArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(mean));
    REQUIRE(meanArray != nullptr);
    REQUIRE(meanArray->getNumberOfTuples() == 1);
    auto* medianArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(median));
    REQUIRE(medianArray != nullptr);
    REQUIRE(medianArray->getNumberOfTuples() == 1);
    auto* modeArray = dataStructure.getDataAs<NeighborList<int32>>(statsDataPath.createChildPath(mode));
    REQUIRE(modeArray != nullptr);
    REQUIRE(modeArray->getNumberOfLists() == 1);
    auto* stdArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(std));
    REQUIRE(stdArray != nullptr);
    REQUIRE(stdArray->getNumberOfTuples() == 1);
    auto* sumArray = dataStructure.getDataAs<Float32Array>(statsDataPath.createChildPath(sum));
    REQUIRE(sumArray != nullptr);
    REQUIRE(sumArray->getNumberOfTuples() == 1);
    auto* standardizeArray = dataStructure.getDataAs<Float32Array>(inputArrayPath.replaceName(standardization));
    REQUIRE(standardizeArray != nullptr);
    REQUIRE(standardizeArray->getNumberOfTuples() == 12);
    auto* numUniqueValuesArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(numUniqueValues));
    REQUIRE(numUniqueValuesArray != nullptr);
    REQUIRE(numUniqueValuesArray->getNumberOfTuples() == 1);
    auto* featureIdMappingArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(featureIdMapping));
    REQUIRE(featureIdMappingArray != nullptr);
    REQUIRE(featureIdMappingArray->getNumberOfTuples() == 1);

    auto lengthVal1 = (*lengthArray)[0];
    auto minVal1 = (*minArray)[0];
    auto maxVal1 = (*maxArray)[0];
    auto meanVal1 = (*meanArray)[0];
    auto medianVal1 = (*medianArray)[0];
    auto modes1 = (*modeArray).getList(0);
    auto stdVal1 = (*stdArray)[0];
    auto sumVal1 = (*sumArray)[0];
    auto numUnique1 = (*numUniqueValuesArray)[0];
    auto mapping1 = (*featureIdMappingArray)[0];

    REQUIRE(mapping1 == 2);
    REQUIRE(lengthVal1 == 4);
    REQUIRE(minVal1 == 10);
    REQUIRE(maxVal1 == 22);
    REQUIRE(std::fabs(meanVal1 - 16.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal1 - 16.0f) < UnitTest::EPSILON);
    REQUIRE(modes1.size() == 2);
    REQUIRE(VectorContains(modes1, 10) == true);
    REQUIRE(VectorContains(modes1, 22) == true);
    REQUIRE(std::fabs(stdVal1 - 6.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(sumVal1 - 64.0f) < UnitTest::EPSILON);
    REQUIRE(numUnique1 == 2);

    const auto& standardizeDataStore = standardizeArray->getDataStoreRef();
    auto stand0 = standardizeDataStore[0];
    auto stand1 = standardizeDataStore[1];
    auto stand2 = standardizeDataStore[2];
    auto stand3 = standardizeDataStore[3];
    auto stand4 = standardizeDataStore[4];
    auto stand5 = standardizeDataStore[5];
    auto stand6 = standardizeDataStore[6];
    auto stand7 = standardizeDataStore[7];
    auto stand8 = standardizeDataStore[8];
    auto stand9 = standardizeDataStore[9];

    REQUIRE(stand0 == 0.0f);
    REQUIRE(stand1 == 0.0f);
    REQUIRE(stand2 == 0.0f);
    REQUIRE(stand3 == 0.0f);
    REQUIRE(stand4 == 0.0f);
    REQUIRE(stand5 == 0.0f);
    REQUIRE(stand6 == 0.0f);
    REQUIRE(stand7 == 0.0f);
    REQUIRE(std::fabs(stand8 - 1.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stand9 - -1.0f) < UnitTest::EPSILON);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeArrayStatisticsFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeArrayStatisticsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeArrayStatisticsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeArrayStatisticsFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<ComputeArrayStatisticsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<bool>(ComputeArrayStatisticsFilter::k_FindLength_Key) == true);
      CHECK(args.value<bool>(ComputeArrayStatisticsFilter::k_FindMin_Key) == true);
      CHECK(args.value<bool>(ComputeArrayStatisticsFilter::k_FindMax_Key) == true);
      CHECK(args.value<bool>(ComputeArrayStatisticsFilter::k_FindMean_Key) == true);
      CHECK(args.value<bool>(ComputeArrayStatisticsFilter::k_FindMedian_Key) == true);
      CHECK(args.value<bool>(ComputeArrayStatisticsFilter::k_FindStdDeviation_Key) == true);
      CHECK(args.value<bool>(ComputeArrayStatisticsFilter::k_FindSummation_Key) == true);
      CHECK(args.value<bool>(ComputeArrayStatisticsFilter::k_UseMask_Key) == true);
      CHECK(args.value<bool>(ComputeArrayStatisticsFilter::k_ComputeByIndex_Key) == true);
      CHECK(args.value<bool>(ComputeArrayStatisticsFilter::k_StandardizeData_Key) == true);
      CHECK(args.value<DataPath>(ComputeArrayStatisticsFilter::k_SelectedArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeArrayStatisticsFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeArrayStatisticsFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeArrayStatisticsFilter::k_DestinationAttributeMatrixPath_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<std::string>(ComputeArrayStatisticsFilter::k_LengthArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeArrayStatisticsFilter::k_MinimumArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeArrayStatisticsFilter::k_MaximumArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeArrayStatisticsFilter::k_MeanArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeArrayStatisticsFilter::k_MedianArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeArrayStatisticsFilter::k_StdDeviationArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeArrayStatisticsFilter::k_SummationArrayName_Key) == "TestName");
      CHECK(args.value<std::string>(ComputeArrayStatisticsFilter::k_StandardizedArrayName_Key) == "TestName");
    }
  }
}
