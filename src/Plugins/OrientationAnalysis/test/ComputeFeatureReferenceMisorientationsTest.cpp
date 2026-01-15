#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/ComputeFeatureReferenceMisorientationsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
const std::string k_GBEuclideanDistancesArrayName("GBManhattanDistances");

const std::string k_CellAvgQuatsMisorientationArrayName("AvgQuats Misorientation");
const std::string k_ComputedCellAvgQuatsMisorientationArrayName("AvgQuats Misorientation Computed");

const std::string k_FeatureAverageMisorientationArrayName("Average Quats Misorientation");
const std::string k_ComputedFeatureAverageMisorientationArrayName("Average Quats Misorientation Computed");

const std::string k_ComputedCellEuclideanDistancesArrayName("Euclidean Misorientation Computed");
const std::string k_CellEuclideanDistancesArrayName("Euclidean Misorientation");

const std::string k_ComputedFeatureEuclideanCentersArrayName("Average Euclidean Misorientation Computed");
const std::string k_FeatureEuclideanCentersArrayName("Average Euclidean Misorientation Computed");

const std::string k_ComputedEuclideanCentersArrayName("Euclidean Centers Computed");
const std::string k_EuclideanCentersArrayName("Euclidean Centers Computed");

} // namespace

/**
 *  @brief This version of the test uses the Average Orientation for each grain as the `Reference Orientation` to use
 */
TEST_CASE("OrientationAnalysis::ComputeFeatureReferenceMisorientationsFilter_AverageMisorientation", "[OrientationAnalysis][ComputeFeatureReferenceMisorientationsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "compute_feature_reference_misorientation.tar.gz",
                                                              "compute_feature_reference_misorientation");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/compute_feature_reference_misorientation/compute_feature_reference_misorientation.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({nx::core::Constants::k_DataContainer});
  DataPath cellDataPath = smallIn100Group.createChildPath(nx::core::Constants::k_CellData);

  DataPath cellFeatureDataPath({k_DataContainer, k_CellFeatureData});
  DataPath avgQuatsDataPath = cellFeatureDataPath.createChildPath(k_AvgQuats);
  DataPath featurePhasesDataPath = cellFeatureDataPath.createChildPath(k_Phases);

  // Instantiate the filter and an Arguments Object
  {
    ComputeFeatureReferenceMisorientationsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_ReferenceOrientation_Key, std::make_any<ChoicesParameter::ValueType>(0));

    args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath));
    args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));
    args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsArrayPath));

    args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(avgQuatsDataPath));

    args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));

    // output Cell data
    args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_CellMisorientationsArrayName_Key,
                        std::make_any<DataObjectNameParameter::ValueType>(k_ComputedCellAvgQuatsMisorientationArrayName));
    // output feature data
    args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_FeatureAvgMisorientationsArrayName_Key,
                        std::make_any<DataObjectNameParameter::ValueType>(k_ComputedFeatureAverageMisorientationArrayName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_feature_reference_misorientations_0.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  // Compare the Output Cell Data
  {
    const DataPath k_GeneratedDataPath({k_DataContainer, k_CellData, k_ComputedCellAvgQuatsMisorientationArrayName});
    const DataPath k_ExemplarArrayPath({k_DataContainer, k_CellData, k_CellAvgQuatsMisorientationArrayName});

    UnitTest::CompareArrays<float>(dataStructure, k_ExemplarArrayPath, k_GeneratedDataPath);
  }

  // Compare the Output Feature Data
  {
    const DataPath k_GeneratedDataPath({k_DataContainer, k_CellFeatureData, k_ComputedFeatureAverageMisorientationArrayName});
    const DataPath k_ExemplarArrayPath({k_DataContainer, k_CellFeatureData, k_FeatureAverageMisorientationArrayName});

    UnitTest::CompareArrays<float>(dataStructure, k_ExemplarArrayPath, k_GeneratedDataPath);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

/**
 *  @brief This version of the test uses the Average Orientation for each grain as the `Reference Orientation` to use
 */
TEST_CASE("OrientationAnalysis::ComputeFeatureReferenceMisorientationsFilter_EuclideanDistance", "[OrientationAnalysis][ComputeFeatureReferenceMisorientationsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "compute_feature_reference_misorientation.tar.gz",
                                                              "compute_feature_reference_misorientation");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/compute_feature_reference_misorientation/compute_feature_reference_misorientation.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath dataContainerPath({nx::core::Constants::k_DataContainer});
  DataPath cellDataPath = dataContainerPath.createChildPath(nx::core::Constants::k_CellData);

  DataPath cellFeatureDataPath({k_DataContainer, k_CellFeatureData});
  DataPath avgQuatsDataPath = cellFeatureDataPath.createChildPath(k_AvgQuats);
  DataPath cellGbEuclideanPath = cellDataPath.createChildPath(k_GBEuclideanDistancesArrayName);

  // Instantiate the filter and an Arguments Object
  {
    ComputeFeatureReferenceMisorientationsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_ReferenceOrientation_Key, std::make_any<ChoicesParameter::ValueType>(1));

    args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath));
    args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));
    args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsArrayPath));
    args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_GBEuclideanDistancesArrayPath_Key, std::make_any<DataPath>(cellGbEuclideanPath));

    args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(cellFeatureDataPath));

    args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));

    // output cell data
    args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_CellMisorientationsArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_ComputedCellEuclideanDistancesArrayName));

    args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_FeatureAvgMisorientationsArrayName_Key,
                        std::make_any<DataObjectNameParameter::ValueType>(k_ComputedFeatureEuclideanCentersArrayName));
    args.insertOrAssign(ComputeFeatureReferenceMisorientationsFilter::k_FeatureEuclideanCenterArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_ComputedEuclideanCentersArrayName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // #ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/find_feature_reference_misorientations_1.dream3d", unit_test::k_BinaryTestOutputDir)));
  // #endif

  // Compare the Output Cell Data
  {
    const DataPath k_GeneratedDataPath({k_DataContainer, k_CellData, k_ComputedCellEuclideanDistancesArrayName});
    const DataPath k_ExemplarArrayPath({k_DataContainer, k_CellData, k_CellEuclideanDistancesArrayName});

    UnitTest::CompareArrays<float>(dataStructure, k_ExemplarArrayPath, k_GeneratedDataPath);
  }

  // Compare the Output Feature Data
  {
    const DataPath k_GeneratedDataPath({k_DataContainer, k_CellFeatureData, k_ComputedFeatureEuclideanCentersArrayName});
    const DataPath k_ExemplarArrayPath({k_DataContainer, k_CellFeatureData, k_FeatureEuclideanCentersArrayName});

    UnitTest::CompareArrays<float>(dataStructure, k_ExemplarArrayPath, k_GeneratedDataPath);
  }

  // Compare the Output Feature Data
  {
    const DataPath k_GeneratedDataPath({k_DataContainer, k_CellFeatureData, k_ComputedEuclideanCentersArrayName});
    const DataPath k_ExemplarArrayPath({k_DataContainer, k_CellFeatureData, k_EuclideanCentersArrayName});

    UnitTest::CompareArrays<float>(dataStructure, k_ExemplarArrayPath, k_GeneratedDataPath);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
