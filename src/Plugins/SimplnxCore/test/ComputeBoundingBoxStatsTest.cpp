#include "SimplnxCore/Filters/ComputeBoundingBoxStatsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
constexpr StringLiteral k_Length = "Length";
constexpr StringLiteral k_Min = "Minimum";
constexpr StringLiteral k_Max = "Maximum";
constexpr StringLiteral k_Mean = "Mean";
constexpr StringLiteral k_Median = "Median";
constexpr StringLiteral k_Mode = "Mode";
constexpr StringLiteral k_StdDev = "Standard Deviation";
constexpr StringLiteral k_Sum = "Summation";
constexpr StringLiteral k_NumUniqueValues = "NumUniqueValues";
constexpr StringLiteral k_BoundsHasData = "BoundsHasData";

constexpr StringLiteral k_GeomName = "ImageGeom";
const DataPath k_GeomPath({k_GeomName});
constexpr StringLiteral k_CellAMName = "Cell Data";
const DataPath k_CellAMPath = k_GeomPath.createChildPath(k_CellAMName);
const StringLiteral k_InputArrayName = "Input Data";
const DataPath k_InputArrayPath = k_CellAMPath.createChildPath(k_InputArrayName);
constexpr StringLiteral k_FeatureAMName = "Feature Data";
const DataPath k_FeatureAMPath = k_GeomPath.createChildPath(k_FeatureAMName);
constexpr StringLiteral k_UnifiedBoundsName = "Unified Bounds";
const DataPath k_UnifiedBoundsPath = k_FeatureAMPath.createChildPath(k_UnifiedBoundsName);

DataStructure InitializeImageDataStructure()
{
  DataStructure dataStructure;

  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_GeomName);
  constexpr size_t dimsIn[3] = {5, 5, 1};
  imageGeom->setDimensions(dimsIn);
  imageGeom->setOrigin({0, 0, 0});
  imageGeom->setSpacing({1, 1, 1});
  std::vector<size_t> dims(3, 0);
  dims[0] = 1;
  dims[1] = 5;
  dims[2] = 5;

  AttributeMatrix* cellAm = AttributeMatrix::Create(dataStructure, k_CellAMName, dims, imageGeom->getId());

  Int32Array* inputArray = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, k_InputArrayName, dims, std::vector<usize>{1}, cellAm->getId());
  (*inputArray)[0] = 1;
  (*inputArray)[1] = 2;
  (*inputArray)[2] = 3;
  (*inputArray)[3] = 4;
  (*inputArray)[4] = 5;
  (*inputArray)[5] = 6;
  (*inputArray)[6] = 7;
  (*inputArray)[7] = 8;
  (*inputArray)[8] = 9;
  (*inputArray)[9] = 10;
  (*inputArray)[10] = 11;
  (*inputArray)[11] = 12;
  (*inputArray)[12] = 13;
  (*inputArray)[13] = 14;
  (*inputArray)[14] = 15;
  (*inputArray)[15] = 16;
  (*inputArray)[16] = 17;
  (*inputArray)[17] = 18;
  (*inputArray)[18] = 19;
  (*inputArray)[19] = 20;
  (*inputArray)[20] = 21;
  (*inputArray)[21] = 22;
  (*inputArray)[22] = 23;
  (*inputArray)[23] = 24;
  (*inputArray)[24] = 25;

  dims.resize(1);
  dims[0] = 2;
  AttributeMatrix* featureAm = AttributeMatrix::Create(dataStructure, k_FeatureAMName, dims, imageGeom->getId());
  Float32Array* unifiedBoundsArray = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, k_UnifiedBoundsName, dims, std::vector<usize>{6}, featureAm->getId());

  return dataStructure;
}

template <bool UseFreqV, bool UseModeV>
void FillArgs(Arguments& args)
{
  args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_CalculateLength_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_CalculateMin_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_CalculateMax_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_CalculateMean_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_CalculateStandardDeviation_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_CalculateSummation_Key, std::make_any<bool>(true));
  if constexpr(UseFreqV)
  {
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_CalculateUniqueValues_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_CalculateMedian_Key, std::make_any<bool>(true));
  }
  if constexpr(UseModeV)
  {
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_CalculateMode_Key, std::make_any<bool>(true));
  }

  args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_CreateNewAM_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_OutputAMPath_Key, std::make_any<DataPath>(k_FeatureAMPath));

  args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_GeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
  args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_InputArrayPath_Key, std::make_any<DataPath>(k_InputArrayPath));
  args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_UnifiedBoundsPath_Key, std::make_any<DataPath>(k_UnifiedBoundsPath));

  args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_BoundsHasDataName_Key, std::make_any<std::string>(k_BoundsHasData));
  args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_LengthName_Key, std::make_any<std::string>(k_Length));
  args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_MinName_Key, std::make_any<std::string>(k_Min));
  args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_MaxName_Key, std::make_any<std::string>(k_Max));
  args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_MeanName_Key, std::make_any<std::string>(k_Mean));
  args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_StdDevName_Key, std::make_any<std::string>(k_StdDev));
  args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_SummationName_Key, std::make_any<std::string>(k_Sum));
  if constexpr(UseFreqV)
  {
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_NumUniqueValuesName_Key, std::make_any<std::string>(k_NumUniqueValues));
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_MedianName_Key, std::make_any<std::string>(k_Median));
  }
  if constexpr(UseModeV)
  {
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_ModeName_Key, std::make_any<std::string>(k_Mode));
  }
}

const std::function<void(Arguments&)> k_FillBaseArgs = [](Arguments& args) { FillArgs<false, false>(args); };
const std::function<void(Arguments&)> k_FillAllExceptModeArgs = [](Arguments& args) { FillArgs<true, false>(args); };
const std::function<void(Arguments&)> k_FillAllArgs = [](Arguments& args) { FillArgs<true, true>(args); };
} // namespace

TEST_CASE("SimplnxCore::ComputeBoundingBoxStatsFilter: Test All Stats - Two Overlapping Bounds", "[SimplnxCore][ComputeBoundingBoxStatsFilter]")
{
  DataStructure dataStructure = ::InitializeImageDataStructure();

  // Setup Unified Bounds Array
  {
    auto& unifiedBounds = dataStructure.getDataRefAs<Float32Array>(k_UnifiedBoundsPath);
    unifiedBounds[0] = 0.0f;
    unifiedBounds[1] = 0.0f;
    unifiedBounds[2] = 0.0f;
    unifiedBounds[3] = 3.0f;
    unifiedBounds[4] = 3.0f;
    unifiedBounds[5] = 1.0f;
    unifiedBounds[6] = 2.0f;
    unifiedBounds[7] = 2.0f;
    unifiedBounds[8] = 0.0f;
    unifiedBounds[9] = 5.0f;
    unifiedBounds[10] = 5.0f;
    unifiedBounds[11] = 1.0f;
  }

  {
    ComputeBoundingBoxStatsFilter filter;

    Arguments args;
    k_FillAllArgs(args);

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Check resulting values
  {
    auto* lengthArray = dataStructure.getDataAs<UInt64Array>(k_FeatureAMPath.createChildPath(k_Length));
    REQUIRE(lengthArray != nullptr);
    auto* minArray = dataStructure.getDataAs<Int32Array>(k_FeatureAMPath.createChildPath(k_Min));
    REQUIRE(minArray != nullptr);
    auto* maxArray = dataStructure.getDataAs<Int32Array>(k_FeatureAMPath.createChildPath(k_Max));
    REQUIRE(maxArray != nullptr);
    auto* meanArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_Mean));
    REQUIRE(meanArray != nullptr);
    auto* medianArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_Median));
    REQUIRE(medianArray != nullptr);
    auto* modeArray = dataStructure.getDataAs<NeighborList<int32>>(k_FeatureAMPath.createChildPath(k_Mode));
    REQUIRE(modeArray != nullptr);
    auto* stdArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_StdDev));
    REQUIRE(stdArray != nullptr);
    auto* sumArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_Sum));
    REQUIRE(sumArray != nullptr);
    auto* numUniqueValuesArray = dataStructure.getDataAs<Int32Array>(k_FeatureAMPath.createChildPath(k_NumUniqueValues));
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
    REQUIRE(maxVal == 13);
    REQUIRE(modeVals.size() == 9);
    REQUIRE(modeVals[0] == 1);
    REQUIRE(std::fabs(sumVal - 63.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(meanVal - 7.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal - 7.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stdVal - 4.17f) < UnitTest::EPSILON);
    REQUIRE(numUnique == 9);

    lengthVal = (*lengthArray)[1];
    minVal = (*minArray)[1];
    maxVal = (*maxArray)[1];
    meanVal = (*meanArray)[1];
    medianVal = (*medianArray)[1];
    modeVals = (*modeArray).getList(1);
    stdVal = (*stdArray)[1];
    stdVal = std::ceil(stdVal * 100.0f) / 100.0f; // round value to 2 decimal places
    sumVal = (*sumArray)[1];
    numUnique = (*numUniqueValuesArray)[1];

    REQUIRE(lengthVal == 9);
    REQUIRE(minVal == 13);
    REQUIRE(maxVal == 25);
    REQUIRE(modeVals.size() == 9);
    REQUIRE(modeVals[0] == 13);
    REQUIRE(std::fabs(sumVal - 171.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(meanVal - 19.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal - 19.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stdVal - 4.17f) < UnitTest::EPSILON);
    REQUIRE(numUnique == 9);
  }
}

TEST_CASE("SimplnxCore::ComputeBoundingBoxStatsFilter: Test All Stats Except Mode - Two Overlapping Bounds", "[SimplnxCore][ComputeBoundingBoxStatsFilter]")
{
  DataStructure dataStructure = ::InitializeImageDataStructure();

  // Setup Unified Bounds Array
  {
    auto& unifiedBounds = dataStructure.getDataRefAs<Float32Array>(k_UnifiedBoundsPath);
    unifiedBounds[0] = 0.0f;
    unifiedBounds[1] = 0.0f;
    unifiedBounds[2] = 0.0f;
    unifiedBounds[3] = 3.0f;
    unifiedBounds[4] = 3.0f;
    unifiedBounds[5] = 1.0f;
    unifiedBounds[6] = 2.0f;
    unifiedBounds[7] = 2.0f;
    unifiedBounds[8] = 0.0f;
    unifiedBounds[9] = 5.0f;
    unifiedBounds[10] = 5.0f;
    unifiedBounds[11] = 1.0f;
  }

  {
    ComputeBoundingBoxStatsFilter filter;

    Arguments args;
    k_FillAllExceptModeArgs(args);

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Check resulting values
  {
    auto* lengthArray = dataStructure.getDataAs<UInt64Array>(k_FeatureAMPath.createChildPath(k_Length));
    REQUIRE(lengthArray != nullptr);
    auto* minArray = dataStructure.getDataAs<Int32Array>(k_FeatureAMPath.createChildPath(k_Min));
    REQUIRE(minArray != nullptr);
    auto* maxArray = dataStructure.getDataAs<Int32Array>(k_FeatureAMPath.createChildPath(k_Max));
    REQUIRE(maxArray != nullptr);
    auto* meanArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_Mean));
    REQUIRE(meanArray != nullptr);
    auto* medianArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_Median));
    REQUIRE(medianArray != nullptr);
    auto* modeArray = dataStructure.getDataAs<NeighborList<int32>>(k_FeatureAMPath.createChildPath(k_Mode));
    REQUIRE(modeArray == nullptr);
    auto* stdArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_StdDev));
    REQUIRE(stdArray != nullptr);
    auto* sumArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_Sum));
    REQUIRE(sumArray != nullptr);
    auto* numUniqueValuesArray = dataStructure.getDataAs<Int32Array>(k_FeatureAMPath.createChildPath(k_NumUniqueValues));
    REQUIRE(numUniqueValuesArray != nullptr);

    auto lengthVal = (*lengthArray)[0];
    auto minVal = (*minArray)[0];
    auto maxVal = (*maxArray)[0];
    auto meanVal = (*meanArray)[0];
    auto medianVal = (*medianArray)[0];
    auto stdVal = (*stdArray)[0];
    stdVal = std::ceil(stdVal * 100.0f) / 100.0f; // round value to 2 decimal places
    auto sumVal = (*sumArray)[0];
    auto numUnique = (*numUniqueValuesArray)[0];

    REQUIRE(lengthVal == 9);
    REQUIRE(minVal == 1);
    REQUIRE(maxVal == 13);
    REQUIRE(std::fabs(sumVal - 63.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(meanVal - 7.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal - 7.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stdVal - 4.17f) < UnitTest::EPSILON);
    REQUIRE(numUnique == 9);

    lengthVal = (*lengthArray)[1];
    minVal = (*minArray)[1];
    maxVal = (*maxArray)[1];
    meanVal = (*meanArray)[1];
    medianVal = (*medianArray)[1];
    stdVal = (*stdArray)[1];
    stdVal = std::ceil(stdVal * 100.0f) / 100.0f; // round value to 2 decimal places
    sumVal = (*sumArray)[1];
    numUnique = (*numUniqueValuesArray)[1];

    REQUIRE(lengthVal == 9);
    REQUIRE(minVal == 13);
    REQUIRE(maxVal == 25);
    REQUIRE(std::fabs(sumVal - 171.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(meanVal - 19.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal - 19.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stdVal - 4.17f) < UnitTest::EPSILON);
    REQUIRE(numUnique == 9);
  }
}

TEST_CASE("SimplnxCore::ComputeBoundingBoxStatsFilter: Test Base Stats - Two Overlapping Bounds", "[SimplnxCore][ComputeBoundingBoxStatsFilter]")
{
  DataStructure dataStructure = ::InitializeImageDataStructure();

  // Setup Unified Bounds Array
  {
    auto& unifiedBounds = dataStructure.getDataRefAs<Float32Array>(k_UnifiedBoundsPath);
    unifiedBounds[0] = 0.0f;
    unifiedBounds[1] = 0.0f;
    unifiedBounds[2] = 0.0f;
    unifiedBounds[3] = 3.0f;
    unifiedBounds[4] = 3.0f;
    unifiedBounds[5] = 1.0f;
    unifiedBounds[6] = 2.0f;
    unifiedBounds[7] = 2.0f;
    unifiedBounds[8] = 0.0f;
    unifiedBounds[9] = 5.0f;
    unifiedBounds[10] = 5.0f;
    unifiedBounds[11] = 1.0f;
  }

  {
    ComputeBoundingBoxStatsFilter filter;

    Arguments args;
    k_FillBaseArgs(args);

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Check resulting values
  {
    auto* lengthArray = dataStructure.getDataAs<UInt64Array>(k_FeatureAMPath.createChildPath(k_Length));
    REQUIRE(lengthArray != nullptr);
    auto* minArray = dataStructure.getDataAs<Int32Array>(k_FeatureAMPath.createChildPath(k_Min));
    REQUIRE(minArray != nullptr);
    auto* maxArray = dataStructure.getDataAs<Int32Array>(k_FeatureAMPath.createChildPath(k_Max));
    REQUIRE(maxArray != nullptr);
    auto* meanArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_Mean));
    REQUIRE(meanArray != nullptr);
    auto* medianArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_Median));
    REQUIRE(medianArray == nullptr);
    auto* modeArray = dataStructure.getDataAs<NeighborList<int32>>(k_FeatureAMPath.createChildPath(k_Mode));
    REQUIRE(modeArray == nullptr);
    auto* stdArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_StdDev));
    REQUIRE(stdArray != nullptr);
    auto* sumArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_Sum));
    REQUIRE(sumArray != nullptr);
    auto* numUniqueValuesArray = dataStructure.getDataAs<Int32Array>(k_FeatureAMPath.createChildPath(k_NumUniqueValues));
    REQUIRE(numUniqueValuesArray == nullptr);

    auto lengthVal = (*lengthArray)[0];
    auto minVal = (*minArray)[0];
    auto maxVal = (*maxArray)[0];
    auto meanVal = (*meanArray)[0];
    auto stdVal = (*stdArray)[0];
    stdVal = std::ceil(stdVal * 100.0f) / 100.0f; // round value to 2 decimal places
    auto sumVal = (*sumArray)[0];

    REQUIRE(lengthVal == 9);
    REQUIRE(minVal == 1);
    REQUIRE(maxVal == 13);
    REQUIRE(std::fabs(sumVal - 63.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(meanVal - 7.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stdVal - 4.17f) < UnitTest::EPSILON);

    lengthVal = (*lengthArray)[1];
    minVal = (*minArray)[1];
    maxVal = (*maxArray)[1];
    meanVal = (*meanArray)[1];
    stdVal = (*stdArray)[1];
    stdVal = std::ceil(stdVal * 100.0f) / 100.0f; // round value to 2 decimal places
    sumVal = (*sumArray)[1];

    REQUIRE(lengthVal == 9);
    REQUIRE(minVal == 13);
    REQUIRE(maxVal == 25);
    REQUIRE(std::fabs(sumVal - 171.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(meanVal - 19.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stdVal - 4.17f) < UnitTest::EPSILON);
  }
}

TEST_CASE("SimplnxCore::ComputeBoundingBoxStatsFilter: Test All Stats - Two Isolated Bounds - Box Bounds Checking", "[SimplnxCore][ComputeBoundingBoxStatsFilter]")
{
  DataStructure dataStructure = ::InitializeImageDataStructure();

  // Setup Unified Bounds Array
  {
    auto& unifiedBounds = dataStructure.getDataRefAs<Float32Array>(k_UnifiedBoundsPath);
    unifiedBounds[0] = -1.0f;
    unifiedBounds[1] = -1.0f;
    unifiedBounds[2] = 0.0f;
    unifiedBounds[3] = 2.0f;
    unifiedBounds[4] = 2.0f;
    unifiedBounds[5] = 1.0f;
    unifiedBounds[6] = 3.0f;
    unifiedBounds[7] = 3.0f;
    unifiedBounds[8] = 0.0f;
    unifiedBounds[9] = 7.0f;
    unifiedBounds[10] = 7.0f;
    unifiedBounds[11] = 1.0f;
  }

  {
    ComputeBoundingBoxStatsFilter filter;

    Arguments args;
    k_FillAllArgs(args);

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Check resulting values
  {
    auto* lengthArray = dataStructure.getDataAs<UInt64Array>(k_FeatureAMPath.createChildPath(k_Length));
    REQUIRE(lengthArray != nullptr);
    auto* minArray = dataStructure.getDataAs<Int32Array>(k_FeatureAMPath.createChildPath(k_Min));
    REQUIRE(minArray != nullptr);
    auto* maxArray = dataStructure.getDataAs<Int32Array>(k_FeatureAMPath.createChildPath(k_Max));
    REQUIRE(maxArray != nullptr);
    auto* meanArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_Mean));
    REQUIRE(meanArray != nullptr);
    auto* medianArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_Median));
    REQUIRE(medianArray != nullptr);
    auto* modeArray = dataStructure.getDataAs<NeighborList<int32>>(k_FeatureAMPath.createChildPath(k_Mode));
    REQUIRE(modeArray != nullptr);
    auto* stdArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_StdDev));
    REQUIRE(stdArray != nullptr);
    auto* sumArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_Sum));
    REQUIRE(sumArray != nullptr);
    auto* numUniqueValuesArray = dataStructure.getDataAs<Int32Array>(k_FeatureAMPath.createChildPath(k_NumUniqueValues));
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

    REQUIRE(lengthVal == 4);
    REQUIRE(minVal == 1);
    REQUIRE(maxVal == 7);
    REQUIRE(modeVals.size() == 4);
    REQUIRE(modeVals[0] == 1);
    REQUIRE(std::fabs(sumVal - 16.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(meanVal - 4.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal - 4.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stdVal - 2.55f) < UnitTest::EPSILON);
    REQUIRE(numUnique == 4);

    lengthVal = (*lengthArray)[1];
    minVal = (*minArray)[1];
    maxVal = (*maxArray)[1];
    meanVal = (*meanArray)[1];
    medianVal = (*medianArray)[1];
    modeVals = (*modeArray).getList(1);
    stdVal = (*stdArray)[1];
    stdVal = std::ceil(stdVal * 100.0f) / 100.0f; // round value to 2 decimal places
    sumVal = (*sumArray)[1];
    numUnique = (*numUniqueValuesArray)[1];

    REQUIRE(lengthVal == 4);
    REQUIRE(minVal == 19);
    REQUIRE(maxVal == 25);
    REQUIRE(modeVals.size() == 4);
    REQUIRE(modeVals[0] == 19);
    REQUIRE(std::fabs(sumVal - 88.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(meanVal - 22.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal - 22.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stdVal - 2.55f) < UnitTest::EPSILON);
    REQUIRE(numUnique == 4);
  }
}

TEST_CASE("SimplnxCore::ComputeBoundingBoxStatsFilter: Test All Stats Except Mode - Two Isolated Bounds - Box Bounds Checking", "[SimplnxCore][ComputeBoundingBoxStatsFilter]")
{
  DataStructure dataStructure = ::InitializeImageDataStructure();

  // Setup Unified Bounds Array
  {
    auto& unifiedBounds = dataStructure.getDataRefAs<Float32Array>(k_UnifiedBoundsPath);
    unifiedBounds[0] = -1.0f;
    unifiedBounds[1] = -1.0f;
    unifiedBounds[2] = 0.0f;
    unifiedBounds[3] = 2.0f;
    unifiedBounds[4] = 2.0f;
    unifiedBounds[5] = 1.0f;
    unifiedBounds[6] = 3.0f;
    unifiedBounds[7] = 3.0f;
    unifiedBounds[8] = 0.0f;
    unifiedBounds[9] = 7.0f;
    unifiedBounds[10] = 7.0f;
    unifiedBounds[11] = 1.0f;
  }

  {
    ComputeBoundingBoxStatsFilter filter;

    Arguments args;
    k_FillAllExceptModeArgs(args);

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Check resulting values
  {
    auto* lengthArray = dataStructure.getDataAs<UInt64Array>(k_FeatureAMPath.createChildPath(k_Length));
    REQUIRE(lengthArray != nullptr);
    auto* minArray = dataStructure.getDataAs<Int32Array>(k_FeatureAMPath.createChildPath(k_Min));
    REQUIRE(minArray != nullptr);
    auto* maxArray = dataStructure.getDataAs<Int32Array>(k_FeatureAMPath.createChildPath(k_Max));
    REQUIRE(maxArray != nullptr);
    auto* meanArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_Mean));
    REQUIRE(meanArray != nullptr);
    auto* medianArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_Median));
    REQUIRE(medianArray != nullptr);
    auto* modeArray = dataStructure.getDataAs<NeighborList<int32>>(k_FeatureAMPath.createChildPath(k_Mode));
    REQUIRE(modeArray == nullptr);
    auto* stdArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_StdDev));
    REQUIRE(stdArray != nullptr);
    auto* sumArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_Sum));
    REQUIRE(sumArray != nullptr);
    auto* numUniqueValuesArray = dataStructure.getDataAs<Int32Array>(k_FeatureAMPath.createChildPath(k_NumUniqueValues));
    REQUIRE(numUniqueValuesArray != nullptr);

    auto lengthVal = (*lengthArray)[0];
    auto minVal = (*minArray)[0];
    auto maxVal = (*maxArray)[0];
    auto meanVal = (*meanArray)[0];
    auto medianVal = (*medianArray)[0];
    auto stdVal = (*stdArray)[0];
    stdVal = std::ceil(stdVal * 100.0f) / 100.0f; // round value to 2 decimal places
    auto sumVal = (*sumArray)[0];
    auto numUnique = (*numUniqueValuesArray)[0];

    REQUIRE(lengthVal == 4);
    REQUIRE(minVal == 1);
    REQUIRE(maxVal == 7);
    REQUIRE(std::fabs(sumVal - 16.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(meanVal - 4.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal - 4.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stdVal - 2.55f) < UnitTest::EPSILON);
    REQUIRE(numUnique == 4);

    lengthVal = (*lengthArray)[1];
    minVal = (*minArray)[1];
    maxVal = (*maxArray)[1];
    meanVal = (*meanArray)[1];
    medianVal = (*medianArray)[1];
    stdVal = (*stdArray)[1];
    stdVal = std::ceil(stdVal * 100.0f) / 100.0f; // round value to 2 decimal places
    sumVal = (*sumArray)[1];
    numUnique = (*numUniqueValuesArray)[1];

    REQUIRE(lengthVal == 4);
    REQUIRE(minVal == 19);
    REQUIRE(maxVal == 25);
    REQUIRE(std::fabs(sumVal - 88.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(meanVal - 22.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal - 22.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stdVal - 2.55f) < UnitTest::EPSILON);
    REQUIRE(numUnique == 4);
  }
}

TEST_CASE("SimplnxCore::ComputeBoundingBoxStatsFilter: Test Base Stats - Two Isolated Bounds - Box Bounds Checking", "[SimplnxCore][ComputeBoundingBoxStatsFilter]")
{
  DataStructure dataStructure = ::InitializeImageDataStructure();

  // Setup Unified Bounds Array
  {
    auto& unifiedBounds = dataStructure.getDataRefAs<Float32Array>(k_UnifiedBoundsPath);
    unifiedBounds[0] = -1.0f;
    unifiedBounds[1] = -1.0f;
    unifiedBounds[2] = 0.0f;
    unifiedBounds[3] = 2.0f;
    unifiedBounds[4] = 2.0f;
    unifiedBounds[5] = 1.0f;
    unifiedBounds[6] = 3.0f;
    unifiedBounds[7] = 3.0f;
    unifiedBounds[8] = 0.0f;
    unifiedBounds[9] = 7.0f;
    unifiedBounds[10] = 7.0f;
    unifiedBounds[11] = 1.0f;
  }

  {
    ComputeBoundingBoxStatsFilter filter;

    Arguments args;
    k_FillBaseArgs(args);

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Check resulting values
  {
    auto* lengthArray = dataStructure.getDataAs<UInt64Array>(k_FeatureAMPath.createChildPath(k_Length));
    REQUIRE(lengthArray != nullptr);
    auto* minArray = dataStructure.getDataAs<Int32Array>(k_FeatureAMPath.createChildPath(k_Min));
    REQUIRE(minArray != nullptr);
    auto* maxArray = dataStructure.getDataAs<Int32Array>(k_FeatureAMPath.createChildPath(k_Max));
    REQUIRE(maxArray != nullptr);
    auto* meanArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_Mean));
    REQUIRE(meanArray != nullptr);
    auto* medianArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_Median));
    REQUIRE(medianArray == nullptr);
    auto* modeArray = dataStructure.getDataAs<NeighborList<int32>>(k_FeatureAMPath.createChildPath(k_Mode));
    REQUIRE(modeArray == nullptr);
    auto* stdArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_StdDev));
    REQUIRE(stdArray != nullptr);
    auto* sumArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_Sum));
    REQUIRE(sumArray != nullptr);
    auto* numUniqueValuesArray = dataStructure.getDataAs<Int32Array>(k_FeatureAMPath.createChildPath(k_NumUniqueValues));
    REQUIRE(numUniqueValuesArray == nullptr);

    auto lengthVal = (*lengthArray)[0];
    auto minVal = (*minArray)[0];
    auto maxVal = (*maxArray)[0];
    auto meanVal = (*meanArray)[0];
    auto stdVal = (*stdArray)[0];
    stdVal = std::ceil(stdVal * 100.0f) / 100.0f; // round value to 2 decimal places
    auto sumVal = (*sumArray)[0];

    REQUIRE(lengthVal == 4);
    REQUIRE(minVal == 1);
    REQUIRE(maxVal == 7);
    REQUIRE(std::fabs(sumVal - 16.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(meanVal - 4.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stdVal - 2.55f) < UnitTest::EPSILON);

    lengthVal = (*lengthArray)[1];
    minVal = (*minArray)[1];
    maxVal = (*maxArray)[1];
    meanVal = (*meanArray)[1];
    stdVal = (*stdArray)[1];
    stdVal = std::ceil(stdVal * 100.0f) / 100.0f; // round value to 2 decimal places
    sumVal = (*sumArray)[1];

    REQUIRE(lengthVal == 4);
    REQUIRE(minVal == 19);
    REQUIRE(maxVal == 25);
    REQUIRE(std::fabs(sumVal - 88.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(meanVal - 22.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stdVal - 2.55f) < UnitTest::EPSILON);
  }
}

TEST_CASE("SimplnxCore::ComputeBoundingBoxStatsFilter: Test All Stats - 1 Empty Bound - 1 In Bound", "[SimplnxCore][ComputeBoundingBoxStatsFilter]")
{
  DataStructure dataStructure = ::InitializeImageDataStructure();

  // Setup Unified Bounds Array
  {
    auto& unifiedBounds = dataStructure.getDataRefAs<Float32Array>(k_UnifiedBoundsPath);
    unifiedBounds[0] = 0.0f;
    unifiedBounds[1] = 0.0f;
    unifiedBounds[2] = 0.0f;
    unifiedBounds[3] = 3.0f;
    unifiedBounds[4] = 3.0f;
    unifiedBounds[5] = 1.0f;
    unifiedBounds[6] = -40.0f;
    unifiedBounds[7] = -40.0f;
    unifiedBounds[8] = -40.0f;
    unifiedBounds[9] = -1.0f;
    unifiedBounds[10] = -1.0f;
    unifiedBounds[11] = -1.0f;
  }

  {
    ComputeBoundingBoxStatsFilter filter;

    Arguments args;
    k_FillAllArgs(args);

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Check resulting values
  {
    auto* boundsHasDataArray = dataStructure.getDataAs<BoolArray>(k_FeatureAMPath.createChildPath(k_BoundsHasData));
    REQUIRE(boundsHasDataArray != nullptr);
    auto* lengthArray = dataStructure.getDataAs<UInt64Array>(k_FeatureAMPath.createChildPath(k_Length));
    REQUIRE(lengthArray != nullptr);
    auto* minArray = dataStructure.getDataAs<Int32Array>(k_FeatureAMPath.createChildPath(k_Min));
    REQUIRE(minArray != nullptr);
    auto* maxArray = dataStructure.getDataAs<Int32Array>(k_FeatureAMPath.createChildPath(k_Max));
    REQUIRE(maxArray != nullptr);
    auto* meanArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_Mean));
    REQUIRE(meanArray != nullptr);
    auto* medianArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_Median));
    REQUIRE(medianArray != nullptr);
    auto* modeArray = dataStructure.getDataAs<NeighborList<int32>>(k_FeatureAMPath.createChildPath(k_Mode));
    REQUIRE(modeArray != nullptr);
    auto* stdArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_StdDev));
    REQUIRE(stdArray != nullptr);
    auto* sumArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_Sum));
    REQUIRE(sumArray != nullptr);
    auto* numUniqueValuesArray = dataStructure.getDataAs<Int32Array>(k_FeatureAMPath.createChildPath(k_NumUniqueValues));
    REQUIRE(numUniqueValuesArray != nullptr);

    auto boundHasDataVal = (*boundsHasDataArray)[0];
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

    REQUIRE(boundHasDataVal);
    REQUIRE(lengthVal == 9);
    REQUIRE(minVal == 1);
    REQUIRE(maxVal == 13);
    REQUIRE(modeVals.size() == 9);
    REQUIRE(modeVals[0] == 1);
    REQUIRE(std::fabs(sumVal - 63.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(meanVal - 7.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal - 7.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stdVal - 4.17f) < UnitTest::EPSILON);
    REQUIRE(numUnique == 9);

    boundHasDataVal = (*boundsHasDataArray)[1];
    lengthVal = (*lengthArray)[1];

    REQUIRE(!boundHasDataVal);
    REQUIRE(lengthVal == 0);
  }
}

TEST_CASE("SimplnxCore::ComputeBoundingBoxStatsFilter: Test All Except Mode Stats - 1 Empty Bound - 1 In Bound", "[SimplnxCore][ComputeBoundingBoxStatsFilter]")
{
  DataStructure dataStructure = ::InitializeImageDataStructure();

  // Setup Unified Bounds Array
  {
    auto& unifiedBounds = dataStructure.getDataRefAs<Float32Array>(k_UnifiedBoundsPath);
    unifiedBounds[0] = 0.0f;
    unifiedBounds[1] = 0.0f;
    unifiedBounds[2] = 0.0f;
    unifiedBounds[3] = 3.0f;
    unifiedBounds[4] = 3.0f;
    unifiedBounds[5] = 1.0f;
    unifiedBounds[6] = -40.0f;
    unifiedBounds[7] = -40.0f;
    unifiedBounds[8] = -40.0f;
    unifiedBounds[9] = -1.0f;
    unifiedBounds[10] = -1.0f;
    unifiedBounds[11] = -1.0f;
  }

  {
    ComputeBoundingBoxStatsFilter filter;

    Arguments args;
    k_FillAllExceptModeArgs(args);

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Check resulting values
  {
    auto* boundsHasDataArray = dataStructure.getDataAs<BoolArray>(k_FeatureAMPath.createChildPath(k_BoundsHasData));
    REQUIRE(boundsHasDataArray != nullptr);
    auto* lengthArray = dataStructure.getDataAs<UInt64Array>(k_FeatureAMPath.createChildPath(k_Length));
    REQUIRE(lengthArray != nullptr);
    auto* minArray = dataStructure.getDataAs<Int32Array>(k_FeatureAMPath.createChildPath(k_Min));
    REQUIRE(minArray != nullptr);
    auto* maxArray = dataStructure.getDataAs<Int32Array>(k_FeatureAMPath.createChildPath(k_Max));
    REQUIRE(maxArray != nullptr);
    auto* meanArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_Mean));
    REQUIRE(meanArray != nullptr);
    auto* medianArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_Median));
    REQUIRE(medianArray != nullptr);
    auto* modeArray = dataStructure.getDataAs<NeighborList<int32>>(k_FeatureAMPath.createChildPath(k_Mode));
    REQUIRE(modeArray == nullptr);
    auto* stdArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_StdDev));
    REQUIRE(stdArray != nullptr);
    auto* sumArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_Sum));
    REQUIRE(sumArray != nullptr);
    auto* numUniqueValuesArray = dataStructure.getDataAs<Int32Array>(k_FeatureAMPath.createChildPath(k_NumUniqueValues));
    REQUIRE(numUniqueValuesArray != nullptr);

    auto boundHasDataVal = (*boundsHasDataArray)[0];
    auto lengthVal = (*lengthArray)[0];
    auto minVal = (*minArray)[0];
    auto maxVal = (*maxArray)[0];
    auto meanVal = (*meanArray)[0];
    auto medianVal = (*medianArray)[0];
    auto stdVal = (*stdArray)[0];
    stdVal = std::ceil(stdVal * 100.0f) / 100.0f; // round value to 2 decimal places
    auto sumVal = (*sumArray)[0];
    auto numUnique = (*numUniqueValuesArray)[0];

    REQUIRE(boundHasDataVal);
    REQUIRE(lengthVal == 9);
    REQUIRE(minVal == 1);
    REQUIRE(maxVal == 13);
    REQUIRE(std::fabs(sumVal - 63.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(meanVal - 7.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(medianVal - 7.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stdVal - 4.17f) < UnitTest::EPSILON);
    REQUIRE(numUnique == 9);

    boundHasDataVal = (*boundsHasDataArray)[1];
    lengthVal = (*lengthArray)[1];

    REQUIRE(!boundHasDataVal);
    REQUIRE(lengthVal == 0);
  }
}

TEST_CASE("SimplnxCore::ComputeBoundingBoxStatsFilter: Test Base Stats - 1 Empty Bound - 1 In Bound", "[SimplnxCore][ComputeBoundingBoxStatsFilter]")
{
  DataStructure dataStructure = ::InitializeImageDataStructure();

  // Setup Unified Bounds Array
  {
    auto& unifiedBounds = dataStructure.getDataRefAs<Float32Array>(k_UnifiedBoundsPath);
    unifiedBounds[0] = 0.0f;
    unifiedBounds[1] = 0.0f;
    unifiedBounds[2] = 0.0f;
    unifiedBounds[3] = 3.0f;
    unifiedBounds[4] = 3.0f;
    unifiedBounds[5] = 1.0f;
    unifiedBounds[6] = -40.0f;
    unifiedBounds[7] = -40.0f;
    unifiedBounds[8] = -40.0f;
    unifiedBounds[9] = -1.0f;
    unifiedBounds[10] = -1.0f;
    unifiedBounds[11] = -1.0f;
  }

  {
    ComputeBoundingBoxStatsFilter filter;

    Arguments args;
    k_FillBaseArgs(args);

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Check resulting values
  {
    auto* boundsHasDataArray = dataStructure.getDataAs<BoolArray>(k_FeatureAMPath.createChildPath(k_BoundsHasData));
    REQUIRE(boundsHasDataArray != nullptr);
    auto* lengthArray = dataStructure.getDataAs<UInt64Array>(k_FeatureAMPath.createChildPath(k_Length));
    REQUIRE(lengthArray != nullptr);
    auto* minArray = dataStructure.getDataAs<Int32Array>(k_FeatureAMPath.createChildPath(k_Min));
    REQUIRE(minArray != nullptr);
    auto* maxArray = dataStructure.getDataAs<Int32Array>(k_FeatureAMPath.createChildPath(k_Max));
    REQUIRE(maxArray != nullptr);
    auto* meanArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_Mean));
    REQUIRE(meanArray != nullptr);
    auto* medianArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_Median));
    REQUIRE(medianArray == nullptr);
    auto* modeArray = dataStructure.getDataAs<NeighborList<int32>>(k_FeatureAMPath.createChildPath(k_Mode));
    REQUIRE(modeArray == nullptr);
    auto* stdArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_StdDev));
    REQUIRE(stdArray != nullptr);
    auto* sumArray = dataStructure.getDataAs<Float32Array>(k_FeatureAMPath.createChildPath(k_Sum));
    REQUIRE(sumArray != nullptr);
    auto* numUniqueValuesArray = dataStructure.getDataAs<Int32Array>(k_FeatureAMPath.createChildPath(k_NumUniqueValues));
    REQUIRE(numUniqueValuesArray == nullptr);

    auto boundHasDataVal = (*boundsHasDataArray)[0];
    auto lengthVal = (*lengthArray)[0];
    auto minVal = (*minArray)[0];
    auto maxVal = (*maxArray)[0];
    auto meanVal = (*meanArray)[0];
    auto stdVal = (*stdArray)[0];
    stdVal = std::ceil(stdVal * 100.0f) / 100.0f; // round value to 2 decimal places
    auto sumVal = (*sumArray)[0];

    REQUIRE(boundHasDataVal);
    REQUIRE(lengthVal == 9);
    REQUIRE(minVal == 1);
    REQUIRE(maxVal == 13);
    REQUIRE(std::fabs(sumVal - 63.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(meanVal - 7.0f) < UnitTest::EPSILON);
    REQUIRE(std::fabs(stdVal - 4.17f) < UnitTest::EPSILON);

    boundHasDataVal = (*boundsHasDataArray)[1];
    lengthVal = (*lengthArray)[1];

    REQUIRE(!boundHasDataVal);
    REQUIRE(lengthVal == 0);
  }
}

TEST_CASE("SimplnxCore::ComputeBoundingBoxStatsFilter: Attribute Matrix Handling Checks - Create")
{
  DataStructure dataStructure = ::InitializeImageDataStructure();

  constexpr StringLiteral k_NewAMName = "newAM";
  {
    ComputeBoundingBoxStatsFilter filter;

    Arguments args;
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_CalculateLength_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_CalculateMin_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_CalculateMax_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_CalculateMean_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_CalculateStandardDeviation_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_CalculateSummation_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_CalculateUniqueValues_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_CalculateMedian_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_CalculateMode_Key, std::make_any<bool>(true));

    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_CreateNewAM_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_NewAMName_Key, std::make_any<std::string>(k_NewAMName));

    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_GeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_InputArrayPath_Key, std::make_any<DataPath>(k_InputArrayPath));
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_UnifiedBoundsPath_Key, std::make_any<DataPath>(k_UnifiedBoundsPath));

    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_BoundsHasDataName_Key, std::make_any<std::string>(k_BoundsHasData));
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_LengthName_Key, std::make_any<std::string>(k_Length));
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_MinName_Key, std::make_any<std::string>(k_Min));
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_MaxName_Key, std::make_any<std::string>(k_Max));
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_MeanName_Key, std::make_any<std::string>(k_Mean));
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_StdDevName_Key, std::make_any<std::string>(k_StdDev));
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_SummationName_Key, std::make_any<std::string>(k_Sum));
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_NumUniqueValuesName_Key, std::make_any<std::string>(k_NumUniqueValues));
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_MedianName_Key, std::make_any<std::string>(k_Median));
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_ModeName_Key, std::make_any<std::string>(k_Mode));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }
  const DataPath newAMPath = k_GeomPath.createChildPath(k_NewAMName);
  auto* amPtr = dataStructure.getDataAs<AttributeMatrix>(newAMPath);
  REQUIRE(amPtr != nullptr);
  REQUIRE(amPtr->getNumTuples() == 2);

  auto* lengthArray = dataStructure.getDataAs<UInt64Array>(newAMPath.createChildPath(k_Length));
  REQUIRE(lengthArray != nullptr);
  auto* minArray = dataStructure.getDataAs<Int32Array>(newAMPath.createChildPath(k_Min));
  REQUIRE(minArray != nullptr);
  auto* maxArray = dataStructure.getDataAs<Int32Array>(newAMPath.createChildPath(k_Max));
  REQUIRE(maxArray != nullptr);
  auto* meanArray = dataStructure.getDataAs<Float32Array>(newAMPath.createChildPath(k_Mean));
  REQUIRE(meanArray != nullptr);
  auto* medianArray = dataStructure.getDataAs<Float32Array>(newAMPath.createChildPath(k_Median));
  REQUIRE(medianArray != nullptr);
  auto* modeArray = dataStructure.getDataAs<NeighborList<int32>>(newAMPath.createChildPath(k_Mode));
  REQUIRE(modeArray != nullptr);
  auto* stdArray = dataStructure.getDataAs<Float32Array>(newAMPath.createChildPath(k_StdDev));
  REQUIRE(stdArray != nullptr);
  auto* sumArray = dataStructure.getDataAs<Float32Array>(newAMPath.createChildPath(k_Sum));
  REQUIRE(sumArray != nullptr);
  auto* numUniqueValuesArray = dataStructure.getDataAs<Int32Array>(newAMPath.createChildPath(k_NumUniqueValues));
  REQUIRE(numUniqueValuesArray != nullptr);
}

TEST_CASE("SimplnxCore::ComputeBoundingBoxStatsFilter: Attribute Matrix Handling Checks - Invalid Existing")
{
  DataStructure dataStructure = ::InitializeImageDataStructure();

  constexpr StringLiteral k_NewAmName = "newAM";

  AttributeMatrix* cellAm = AttributeMatrix::Create(dataStructure, k_NewAmName, AttributeMatrix::ShapeType(1, 1));

  {
    ComputeBoundingBoxStatsFilter filter;

    Arguments args;
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_CalculateLength_Key, std::make_any<bool>(true));

    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_CreateNewAM_Key, std::make_any<bool>(false));
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_OutputAMPath_Key, std::make_any<DataPath>(DataPath{{k_NewAmName}}));

    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_GeometryPath_Key, std::make_any<DataPath>(k_GeomPath));
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_InputArrayPath_Key, std::make_any<DataPath>(k_InputArrayPath));
    args.insertOrAssign(ComputeBoundingBoxStatsFilter::k_UnifiedBoundsPath_Key, std::make_any<DataPath>(k_UnifiedBoundsPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }
}
