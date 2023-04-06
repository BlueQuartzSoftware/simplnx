#include <catch2/catch.hpp>

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/CAxisSegmentFeaturesFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace complex;
using namespace complex::Constants;

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Valid Filter Execution", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  std::shared_ptr<UnitTest::make_shared_enabler> app = std::make_shared<UnitTest::make_shared_enabler>();
  app->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterList = Application::Instance()->getFilterList();

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_caxis_segment_features.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // MultiThreshold Objects Filter (From ComplexCore Plugins)
  SmallIn100::ExecuteMultiThresholdObjects(dataStructure, *filterList);

  // Convert Orientations Filter (From OrientationAnalysis Plugin)
  SmallIn100::ExecuteConvertOrientations(dataStructure, *filterList);

  // Align Sections Misorientation Filter (From OrientationAnalysis Plugin)
  SmallIn100::ExecuteAlignSectionsMisorientation(dataStructure, *filterList, fs::path(fmt::format("{}/AlignSectionsMisorientation_1.txt", unit_test::k_BinaryDir)));

  // Identify Sample Filter
  SmallIn100::ExecuteIdentifySample(dataStructure, *filterList);

  // Align Sections Feature Centroid Filter
  SmallIn100::ExecuteAlignSectionsFeatureCentroid(dataStructure, *filterList, fs::path(fmt::format("{}/AlignSectionsFeatureCentroid_1.txt", unit_test::k_BinaryDir)));

  // Bad Data Neighbor Orientation Check Filter
  SmallIn100::ExecuteBadDataNeighborOrientationCheck(dataStructure, *filterList);

  // Neighbor Orientation Correlation Filter
  SmallIn100::ExecuteNeighborOrientationCorrelation(dataStructure, *filterList);

  // CAxis Segment Features Filter
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    CAxisSegmentFeaturesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_UseGoodVoxels_Key, std::make_any<bool>(true));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsArrayPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_GoodVoxelsArrayPath_Key, std::make_any<DataPath>(k_MaskArrayPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_FeatureIds));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>(k_Grain_Data));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_ActiveName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Loop and compare each array from the 'Exemplar Data / CellData' to the 'Data Container / CellData' group
  {
    const auto& generatedFeatureIds = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsArrayPath);
    const auto& exemplarFeatureIds = exemplarDataStructure.getDataRefAs<Int32Array>(DataPath({k_SmallIN100, SmallIn100::k_SmallIN100ScanData, k_FeatureIds}));
    UnitTest::CompareDataArrays<int32>(generatedFeatureIds, exemplarFeatureIds);

    const auto& generatedActive = dataStructure.getDataRefAs<UInt8Array>(k_DataContainerPath.createChildPath(k_Grain_Data).createChildPath(k_ActiveName));
    const auto& exemplarActive = exemplarDataStructure.getDataRefAs<BoolArray>(DataPath({k_SmallIN100, k_CellFeatureData, k_ActiveName}));
    REQUIRE(generatedActive.size() == exemplarActive.size());
  }
}
