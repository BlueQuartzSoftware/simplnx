#include <catch2/catch.hpp>

#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/ComputeFeatureNeighborCAxisMisalignmentsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace nx::core;
using namespace nx::core::Constants;

namespace compute_feature_neighbor_caxis_misalignments::constants
{
const DataPath k_GeometryPath = DataPath({"6_5_simplnx_test_file_25x50_Hex"});
const DataPath k_CellFeatureDataPath = k_GeometryPath.createChildPath("CellFeatureData");
const DataPath k_CellEnsembleDataPath = k_GeometryPath.createChildPath("CellEnsembleData");

const DataPath k_NeighborListPath = k_CellFeatureDataPath.createChildPath("NeighborList");
const DataPath k_AvgQuatsPath = k_CellFeatureDataPath.createChildPath("AvgQuats");
const DataPath k_PhasesPath = k_CellFeatureDataPath.createChildPath("Phases");

const DataPath k_CrystalStructuresArrayPath = k_CellEnsembleDataPath.createChildPath("CrystalStructures");

const std::string k_ComputedCAxisMisalignmentList = "CAxisMisalignmentList";
const std::string k_ComputedAvgCAxisMisalignment = "AvgCAxisMisalignments";

const std::string k_ExemplarCAxisMisalignmentList = "CAxisMisalignmentList (7_5)";
const std::string k_ExemplarAvgCAxisMisalignment = "AvgCAxisMisalignments (7_5)";

} // namespace compute_feature_neighbor_caxis_misalignments::constants

TEST_CASE("OrientationAnalysis::ComputeFeatureNeighborCAxisMisalignmentsFilter: Valid Filter Execution", "[OrientationAnalysis][ComputeFeatureNeighborCAxisMisalignmentsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "compute_feature_neighbor_caxis_misalignments.tar.gz",
                                                              "compute_feature_neighbor_caxis_misalignments");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/compute_feature_neighbor_caxis_misalignments/7_5_simplnx_test_file_25x50_Hex.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeFeatureNeighborCAxisMisalignmentsFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_FindAvgMisals_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_NeighborListArrayPath_Key,
                      std::make_any<DataPath>(compute_feature_neighbor_caxis_misalignments::constants::k_NeighborListPath));
  args.insertOrAssign(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(compute_feature_neighbor_caxis_misalignments::constants::k_AvgQuatsPath));
  args.insertOrAssign(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(compute_feature_neighbor_caxis_misalignments::constants::k_PhasesPath));
  args.insertOrAssign(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_CrystalStructuresArrayPath_Key,
                      std::make_any<DataPath>(compute_feature_neighbor_caxis_misalignments::constants::k_CrystalStructuresArrayPath));
  args.insertOrAssign(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_CAxisMisalignmentListArrayName_Key,
                      std::make_any<std::string>(compute_feature_neighbor_caxis_misalignments::constants::k_ComputedCAxisMisalignmentList));
  args.insertOrAssign(ComputeFeatureNeighborCAxisMisalignmentsFilter::k_AvgCAxisMisalignmentsArrayName_Key,
                      std::make_any<std::string>(compute_feature_neighbor_caxis_misalignments::constants::k_ComputedAvgCAxisMisalignment));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  UnitTest::WriteTestDataStructure(dataStructure, fmt::format("{}/compute_feature_neighbor_caxis_misalignments.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  UnitTest::CompareNeighborListFloatArraysWithNans<float32>(
      dataStructure,
      compute_feature_neighbor_caxis_misalignments::constants::k_CellFeatureDataPath.createChildPath(compute_feature_neighbor_caxis_misalignments::constants::k_ComputedCAxisMisalignmentList),
      compute_feature_neighbor_caxis_misalignments::constants::k_CellFeatureDataPath.createChildPath(compute_feature_neighbor_caxis_misalignments::constants::k_ExemplarCAxisMisalignmentList),
      UnitTest::EPSILON, true);
  UnitTest::CompareFloatArraysWithNans<float32>(
      dataStructure,
      compute_feature_neighbor_caxis_misalignments::constants::k_CellFeatureDataPath.createChildPath(compute_feature_neighbor_caxis_misalignments::constants::k_ComputedAvgCAxisMisalignment),
      compute_feature_neighbor_caxis_misalignments::constants::k_CellFeatureDataPath.createChildPath(compute_feature_neighbor_caxis_misalignments::constants::k_ExemplarAvgCAxisMisalignment),
      UnitTest::EPSILON, true);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
