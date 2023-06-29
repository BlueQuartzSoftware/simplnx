#include <catch2/catch.hpp>

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/FindFeatureNeighborCAxisMisalignmentsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace complex;
using namespace complex::Constants;

namespace
{
const DataPath k_NeighborListPath = k_CellFeatureDataPath.createChildPath("NeighborList");
const std::string k_CAxisMisalignmentListNameExemplar = "CAxisMisalignmentList";
const std::string k_AvgCAxisMisalignmentsNameExemplar = "AvgCAxisMisalignments";
const std::string k_CAxisMisalignmentListNameComputed = "NX_CAxisMisalignmentList";
const std::string k_AvgCAxisMisalignmentsNameComputed = "NX_AvgCAxisMisalignments";
} // namespace

TEST_CASE("OrientationAnalysis::FindFeatureNeighborCAxisMisalignmentsFilter: Valid Filter Execution", "[OrientationAnalysis][FindFeatureNeighborCAxisMisalignmentsFilter]")
{
  const std::string kDataInputArchive = "6_6_caxis_data.tar.gz";
  const std::string kExpectedOutputTopLevel = "6_6_caxis_data";
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, kDataInputArchive, kExpectedOutputTopLevel,
                                                             complex::unit_test::k_BinaryTestOutputDir);

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_caxis_data/6_6_find_caxis_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindFeatureNeighborCAxisMisalignmentsFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(FindFeatureNeighborCAxisMisalignmentsFilter::k_FindAvgMisals_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindFeatureNeighborCAxisMisalignmentsFilter::k_NeighborListArrayPath_Key, std::make_any<DataPath>(k_NeighborListPath));
  args.insertOrAssign(FindFeatureNeighborCAxisMisalignmentsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataPath.createChildPath(k_AvgQuats)));
  args.insertOrAssign(FindFeatureNeighborCAxisMisalignmentsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataPath.createChildPath(k_Phases)));
  args.insertOrAssign(FindFeatureNeighborCAxisMisalignmentsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));
  args.insertOrAssign(FindFeatureNeighborCAxisMisalignmentsFilter::k_CAxisMisalignmentListArrayName_Key, std::make_any<std::string>(k_CAxisMisalignmentListNameComputed));
  args.insertOrAssign(FindFeatureNeighborCAxisMisalignmentsFilter::k_AvgCAxisMisalignmentsArrayName_Key, std::make_any<std::string>(k_AvgCAxisMisalignmentsNameComputed));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareNeighborListFloatArraysWithNans<float32>(dataStructure, k_CellFeatureDataPath.createChildPath(k_CAxisMisalignmentListNameExemplar),
                                                            k_CellFeatureDataPath.createChildPath(k_CAxisMisalignmentListNameComputed), UnitTest::EPSILON, true);
  UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, k_CellFeatureDataPath.createChildPath(k_AvgCAxisMisalignmentsNameExemplar),
                                                k_CellFeatureDataPath.createChildPath(k_AvgCAxisMisalignmentsNameComputed), UnitTest::EPSILON, true);
}

TEST_CASE("OrientationAnalysis::FindFeatureNeighborCAxisMisalignmentsFilter: InValid Filter Execution")
{
  const std::string kDataInputArchive = "6_6_caxis_data.tar.gz";
  const std::string kExpectedOutputTopLevel = "6_6_caxis_data";
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, kDataInputArchive, kExpectedOutputTopLevel,
                                                             complex::unit_test::k_BinaryTestOutputDir);

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_caxis_data/6_6_find_caxis_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindFeatureNeighborCAxisMisalignmentsFilter filter;
  Arguments args;

  args.insertOrAssign(FindFeatureNeighborCAxisMisalignmentsFilter::k_FindAvgMisals_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindFeatureNeighborCAxisMisalignmentsFilter::k_NeighborListArrayPath_Key, std::make_any<DataPath>(k_NeighborListPath));
  args.insertOrAssign(FindFeatureNeighborCAxisMisalignmentsFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataPath.createChildPath(k_AvgQuats)));
  args.insertOrAssign(FindFeatureNeighborCAxisMisalignmentsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));
  args.insertOrAssign(FindFeatureNeighborCAxisMisalignmentsFilter::k_CAxisMisalignmentListArrayName_Key, std::make_any<std::string>(k_CAxisMisalignmentListNameComputed));
  args.insertOrAssign(FindFeatureNeighborCAxisMisalignmentsFilter::k_AvgCAxisMisalignmentsArrayName_Key, std::make_any<std::string>(k_AvgCAxisMisalignmentsNameComputed));

  SECTION("Invalid Crystal Structure Type")
  {
    // Invalid crystal structure type : should fail in execute
    args.insertOrAssign(FindFeatureNeighborCAxisMisalignmentsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_CellFeatureDataPath.createChildPath(k_Phases)));

    auto& crystalStructs = dataStructure.getDataRefAs<UInt32Array>(k_CrystalStructuresArrayPath);
    crystalStructs[1] = 1;

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  }
  SECTION("Mismatching Input Array Tuples")
  {
    args.insertOrAssign(FindFeatureNeighborCAxisMisalignmentsFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_CellAttributeMatrix.createChildPath(k_Phases)));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  }

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(executeResult.result)
}
