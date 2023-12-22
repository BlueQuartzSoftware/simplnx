#include <catch2/catch.hpp>

#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/CAxisSegmentFeaturesFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
const std::string k_ComputedFeatureIds = "NX_FeatureIds";
const std::string k_ComputedCellFeatureData = "NX_CellFeatureData";
} // namespace

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Valid Filter Execution", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_caxis_data.tar.gz", "6_6_caxis_data");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_caxis_data/6_6_caxis_segment_features.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // CAxis Segment Features Filter
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    CAxisSegmentFeaturesFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsArrayPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_MaskArrayPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_ComputedFeatureIds));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>(k_ComputedCellFeatureData));
    args.insertOrAssign(CAxisSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_ActiveName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // Loop and compare each array from the 'Exemplar Data / CellData' to the 'Data Container / CellData' group
  {
    const auto& generatedFeatureIds = dataStructure.getDataRefAs<Int32Array>(k_CellAttributeMatrix.createChildPath(k_ComputedFeatureIds));
    const auto& exemplarFeatureIds = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsArrayPath);
    UnitTest::CompareDataArrays<int32>(generatedFeatureIds, exemplarFeatureIds);

    const auto& generatedActive = dataStructure.getDataRefAs<UInt8Array>(k_DataContainerPath.createChildPath(k_ComputedCellFeatureData).createChildPath(k_ActiveName));
    const auto& exemplarActive = dataStructure.getDataRefAs<BoolArray>(k_CellFeatureDataPath.createChildPath(k_ActiveName));
    REQUIRE(generatedActive.size() == exemplarActive.size());
  }
}

TEST_CASE("OrientationAnalysis::CAxisSegmentFeaturesFilter: Invalid Filter Execution", "[OrientationAnalysis][CAxisSegmentFeaturesFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_caxis_data.tar.gz", "6_6_caxis_data");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_caxis_data/6_6_find_caxis_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  auto& crystalStructs = dataStructure.getDataRefAs<UInt32Array>(k_CrystalStructuresArrayPath);
  crystalStructs[1] = 1;

  // Instantiate the filter, a DataStructure object and an Arguments Object
  CAxisSegmentFeaturesFilter filter;
  Arguments args;

  // Invalid crystal structure type : should fail in execute
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_MisorientationTolerance_Key, std::make_any<float32>(5.0f));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_RandomizeFeatureIds_Key, std::make_any<bool>(false));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsArrayPath));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_FeatureIdsArrayName_Key, std::make_any<std::string>(k_ComputedFeatureIds));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_CellFeatureAttributeMatrixName_Key, std::make_any<std::string>(k_ComputedCellFeatureData));
  args.insertOrAssign(CAxisSegmentFeaturesFilter::k_ActiveArrayName_Key, std::make_any<std::string>(k_ActiveName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
}
