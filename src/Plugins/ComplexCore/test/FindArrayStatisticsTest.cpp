#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/FindArrayStatisticsFilter.hpp"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace complex;
using namespace complex::Constants;

namespace
{
template <typename T>
bool VectorContains(const std::vector<T>& vector, T value)
{
  return (std::find(vector.begin(), vector.end(), value) != vector.end());
}
} // namespace

TEST_CASE("ComplexCore::FindArrayStatisticsFilter: Test Algorithm", "[ComplexCore][FindArrayStatisticsFilter]")
{
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

  const std::string histogram = "Histogram";
  const std::string mostPopulatedBin = "Most Populated Bin";
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
    args.insertOrAssign(FindArrayStatisticsFilter::k_FindMode_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_FindStdDeviation_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_FindSummation_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_FindUniqueValues_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_ComputeByIndex_Key, std::make_any<bool>(false));
    args.insertOrAssign(FindArrayStatisticsFilter::k_StandardizeData_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(inputArrayPath));
    args.insertOrAssign(FindArrayStatisticsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>());
    args.insertOrAssign(FindArrayStatisticsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"TestData", "Mask"})));
    args.insertOrAssign(FindArrayStatisticsFilter::k_DestinationAttributeMatrix_Key, std::make_any<DataPath>(statsDataPath));
    args.insertOrAssign(FindArrayStatisticsFilter::k_HistogramArrayName_Key, std::make_any<std::string>(histogram));
    args.insertOrAssign(FindArrayStatisticsFilter::k_MostPopulatedBinArrayName_Key, std::make_any<std::string>(mostPopulatedBin));
    args.insertOrAssign(FindArrayStatisticsFilter::k_LengthArrayName_Key, std::make_any<std::string>(length));
    args.insertOrAssign(FindArrayStatisticsFilter::k_MinimumArrayName_Key, std::make_any<std::string>(min));
    args.insertOrAssign(FindArrayStatisticsFilter::k_MaximumArrayName_Key, std::make_any<std::string>(max));
    args.insertOrAssign(FindArrayStatisticsFilter::k_MeanArrayName_Key, std::make_any<std::string>(mean));
    args.insertOrAssign(FindArrayStatisticsFilter::k_MedianArrayName_Key, std::make_any<std::string>(median));
    args.insertOrAssign(FindArrayStatisticsFilter::k_ModeArrayName_Key, std::make_any<std::string>(mode));
    args.insertOrAssign(FindArrayStatisticsFilter::k_StdDeviationArrayName_Key, std::make_any<std::string>(std));
    args.insertOrAssign(FindArrayStatisticsFilter::k_SummationArrayName_Key, std::make_any<std::string>(sum));
    args.insertOrAssign(FindArrayStatisticsFilter::k_StandardizedArrayName_Key, std::make_any<std::string>(standardization));
    args.insertOrAssign(FindArrayStatisticsFilter::k_NumUniqueValues_Key, std::make_any<std::string>(numUniqueValues));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  // Check resulting values
  {
    auto* attrMatrix = dataStructure.getDataAs<AttributeMatrix>(statsDataPath);
    REQUIRE(attrMatrix != nullptr);
    REQUIRE(attrMatrix->getShape().size() == 1);
    REQUIRE(attrMatrix->getShape()[0] == 1);
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
    auto* standardizeArray = dataStructure.getDataAs<Float32Array>(inputArrayPath.getParent().createChildPath(standardization));
    REQUIRE(standardizeArray != nullptr);
    REQUIRE(standardizeArray->getNumberOfTuples() == 11);
    auto* histArray = dataStructure.getDataAs<UInt64Array>(statsDataPath.createChildPath(histogram));
    REQUIRE(histArray != nullptr);
    auto* mostPopulatedBinArray = dataStructure.getDataAs<UInt64Array>(statsDataPath.createChildPath(mostPopulatedBin));
    REQUIRE(mostPopulatedBinArray != nullptr);
    auto* numUniqueValuesArray = dataStructure.getDataAs<Int32Array>(statsDataPath.createChildPath(numUniqueValues));
    REQUIRE(numUniqueValuesArray != nullptr);

    auto lengthVal = (*lengthArray)[0];
    auto minVal = (*minArray)[0];
    auto maxVal = (*maxArray)[0];
    auto meanVal = (*meanArray)[0];
    auto medianVal = (*medianArray)[0];
    auto modeVals = (*modeArray).getListReference(0);
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
    REQUIRE((*histArray)[0] == 4);
    REQUIRE((*histArray)[1] == 2);
    REQUIRE((*histArray)[2] == 2);
    REQUIRE((*histArray)[3] == 0);
    REQUIRE((*histArray)[4] == 1);
    REQUIRE((*mostPopulatedBinArray)[0] == 0);
    REQUIRE((*mostPopulatedBinArray)[1] == 4);
  }
}

TEST_CASE("ComplexCore::FindArrayStatisticsFilter: Test Algorithm By Index", "[ComplexCore][FindArrayStatisticsFilter]")
{
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

  const std::string histogram = "Histogram";
  const std::string mostPopulatedBin = "Most Populated Bin";
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
    args.insertOrAssign(FindArrayStatisticsFilter::k_FindMode_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_FindStdDeviation_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_FindSummation_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_FindUniqueValues_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_ComputeByIndex_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_StandardizeData_Key, std::make_any<bool>(true));
    args.insertOrAssign(FindArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(inputArrayPath));
    args.insertOrAssign(FindArrayStatisticsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"TestData", "FeatureIds"})));
    args.insertOrAssign(FindArrayStatisticsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"TestData", "Mask"})));
    args.insertOrAssign(FindArrayStatisticsFilter::k_DestinationAttributeMatrix_Key, std::make_any<DataPath>(statsDataPath));
    args.insertOrAssign(FindArrayStatisticsFilter::k_HistogramArrayName_Key, std::make_any<std::string>(histogram));
    args.insertOrAssign(FindArrayStatisticsFilter::k_MostPopulatedBinArrayName_Key, std::make_any<std::string>(mostPopulatedBin));
    args.insertOrAssign(FindArrayStatisticsFilter::k_LengthArrayName_Key, std::make_any<std::string>(length));
    args.insertOrAssign(FindArrayStatisticsFilter::k_MinimumArrayName_Key, std::make_any<std::string>(min));
    args.insertOrAssign(FindArrayStatisticsFilter::k_MaximumArrayName_Key, std::make_any<std::string>(max));
    args.insertOrAssign(FindArrayStatisticsFilter::k_MeanArrayName_Key, std::make_any<std::string>(mean));
    args.insertOrAssign(FindArrayStatisticsFilter::k_MedianArrayName_Key, std::make_any<std::string>(median));
    args.insertOrAssign(FindArrayStatisticsFilter::k_ModeArrayName_Key, std::make_any<std::string>(mode));
    args.insertOrAssign(FindArrayStatisticsFilter::k_StdDeviationArrayName_Key, std::make_any<std::string>(std));
    args.insertOrAssign(FindArrayStatisticsFilter::k_SummationArrayName_Key, std::make_any<std::string>(sum));
    args.insertOrAssign(FindArrayStatisticsFilter::k_StandardizedArrayName_Key, std::make_any<std::string>(standardization));
    args.insertOrAssign(FindArrayStatisticsFilter::k_NumUniqueValues_Key, std::make_any<std::string>(numUniqueValues));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  // Check resulting values
  {
    auto* attrMatrix = dataStructure.getDataAs<AttributeMatrix>(statsDataPath);
    REQUIRE(attrMatrix != nullptr);
    REQUIRE(attrMatrix->getShape().size() == 1);
    REQUIRE(attrMatrix->getShape()[0] == 3);
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
    auto* standardizeArray = dataStructure.getDataAs<Float32Array>(inputArrayPath.getParent().createChildPath(standardization));
    REQUIRE(standardizeArray != nullptr);
    REQUIRE(standardizeArray->getNumberOfTuples() == 12);
    auto* histArray = dataStructure.getDataAs<UInt64Array>(statsDataPath.createChildPath(histogram));
    REQUIRE(histArray != nullptr);
    REQUIRE(histArray->getNumberOfTuples() == 3);
    REQUIRE(histArray->getNumberOfComponents() == 5);
    auto* mostPopulatedBinArray = dataStructure.getDataAs<UInt64Array>(statsDataPath.createChildPath(mostPopulatedBin));
    REQUIRE(mostPopulatedBinArray != nullptr);
    REQUIRE(mostPopulatedBinArray->getNumberOfTuples() == 3);
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
    auto modes0 = (*modeArray).getListReference(0);
    auto modes1 = (*modeArray).getListReference(1);
    auto modes2 = (*modeArray).getListReference(2);
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

    const auto& standardizeDataStore = standardizeArray->getIDataStoreRefAs<AbstractDataStore<float32>>();
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
    auto hist1_1 = (*histArray)[0];
    auto hist1_2 = (*histArray)[1];
    auto hist1_3 = (*histArray)[2];
    auto hist1_4 = (*histArray)[3];
    auto hist1_5 = (*histArray)[4];
    auto hist2_1 = (*histArray)[5];
    auto hist2_2 = (*histArray)[6];
    auto hist2_3 = (*histArray)[7];
    auto hist2_4 = (*histArray)[8];
    auto hist2_5 = (*histArray)[9];
    auto hist3_1 = (*histArray)[10];
    auto hist3_2 = (*histArray)[11];
    auto hist3_3 = (*histArray)[12];
    auto hist3_4 = (*histArray)[13];
    auto hist3_5 = (*histArray)[14];

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
    REQUIRE(hist1_1 == 1);
    REQUIRE(hist1_2 == 0);
    REQUIRE(hist1_3 == 0);
    REQUIRE(hist1_4 == 0);
    REQUIRE(hist1_5 == 1);
    REQUIRE(hist2_1 == 1);
    REQUIRE(hist2_2 == 0);
    REQUIRE(hist2_3 == 0);
    REQUIRE(hist2_4 == 1);
    REQUIRE(hist2_5 == 2);
    REQUIRE(hist3_1 == 2);
    REQUIRE(hist3_2 == 0);
    REQUIRE(hist3_3 == 0);
    REQUIRE(hist3_4 == 0);
    REQUIRE(hist3_5 == 2);
    REQUIRE((*mostPopulatedBinArray)[0] == 0);
    REQUIRE((*mostPopulatedBinArray)[1] == 1);
    REQUIRE((*mostPopulatedBinArray)[2] == 4);
    REQUIRE((*mostPopulatedBinArray)[3] == 2);
    REQUIRE((*mostPopulatedBinArray)[4] == 0);
    REQUIRE((*mostPopulatedBinArray)[5] == 2);
  }
}
