#include <catch2/catch.hpp>

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArrayThresholdsParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/FindArrayStatisticsFilter.hpp"
#include "ComplexCore/Filters/MultiThresholdObjects.hpp"

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
  DataPath statsGroup({Constants::k_SmallIN100, "Statistics"});
  DataPath maskPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "Mask"});
  DataPath confIndexPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ConfidenceIndex});
  DataStructure dataGraph = UnitTest::CreateDataStructure();
  dataGraph.makePath(statsGroup);

  {
    MultiThresholdObjects thresholdsFilter;
    Arguments thresholdArgs;

    ArrayThresholdSet arrayThresholdset;
    ArrayThresholdSet::CollectionType thresholds;

    std::shared_ptr<ArrayThreshold> ciThreshold = std::make_shared<ArrayThreshold>();
    ciThreshold->setArrayPath(confIndexPath);
    ciThreshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
    ciThreshold->setComparisonValue(0.1);
    thresholds.push_back(ciThreshold);

    arrayThresholdset.setArrayThresholds(thresholds);

    constexpr StringLiteral k_ArrayThresholds_Key = "array_thresholds";
    constexpr StringLiteral k_CreatedDataPath_Key = "created_data_path";
    thresholdArgs.insertOrAssign(k_ArrayThresholds_Key, std::make_any<ArrayThresholdsParameter::ValueType>(arrayThresholdset));
    thresholdArgs.insertOrAssign(k_CreatedDataPath_Key, std::make_any<DataPath>(maskPath));

    // Preflight the filter and check result
    auto preflightResult = thresholdsFilter.preflight(dataGraph, thresholdArgs);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = thresholdsFilter.execute(dataGraph, thresholdArgs);
    REQUIRE(executeResult.result.valid());
  }

  FindArrayStatisticsFilter filter;
  Arguments args;
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
  args.insertOrAssign(FindArrayStatisticsFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(confIndexPath));
  // args.insertOrAssign(FindArrayStatisticsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{Constants::k_FeatureIds}));
  args.insertOrAssign(FindArrayStatisticsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskPath));
  args.insertOrAssign(FindArrayStatisticsFilter::k_DestinationAttributeMatrix_Key, std::make_any<DataPath>(statsGroup));
  args.insertOrAssign(FindArrayStatisticsFilter::k_HistogramArrayName_Key, std::make_any<std::string>("Histogram"));
  args.insertOrAssign(FindArrayStatisticsFilter::k_LengthArrayName_Key, std::make_any<std::string>("Length"));
  args.insertOrAssign(FindArrayStatisticsFilter::k_MinimumArrayName_Key, std::make_any<std::string>("Minimum"));
  args.insertOrAssign(FindArrayStatisticsFilter::k_MaximumArrayName_Key, std::make_any<std::string>("Maximum"));
  args.insertOrAssign(FindArrayStatisticsFilter::k_MeanArrayName_Key, std::make_any<std::string>("Mean"));
  args.insertOrAssign(FindArrayStatisticsFilter::k_MedianArrayName_Key, std::make_any<std::string>("Median"));
  args.insertOrAssign(FindArrayStatisticsFilter::k_StdDeviationArrayName_Key, std::make_any<std::string>("Standard Deviation"));
  args.insertOrAssign(FindArrayStatisticsFilter::k_SummationArrayName_Key, std::make_any<std::string>("Summation"));
  args.insertOrAssign(FindArrayStatisticsFilter::k_StandardizedArrayName_Key, std::make_any<std::string>("Standardization"));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.valid());
}
