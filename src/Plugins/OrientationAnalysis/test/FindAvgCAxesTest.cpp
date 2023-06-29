#include <catch2/catch.hpp>

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/FindAvgCAxesFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace complex;
using namespace complex::Constants;

namespace
{
const std::string k_AvgCAxesExemplar = "AvgCAxes";
const std::string k_AvgCAxesComputed = "NX_AvgCAxes";
} // namespace

TEST_CASE("OrientationAnalysis::FindAvgCAxesFilter: Valid Filter Execution", "[OrientationAnalysis][FindAvgCAxesFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_caxis_data.tar.gz", "6_6_caxis_data",
                                                             complex::unit_test::k_BinaryTestOutputDir);

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_caxis_data/6_6_find_caxis_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindAvgCAxesFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(FindAvgCAxesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsArrayPath));
  args.insertOrAssign(FindAvgCAxesFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath));
  args.insertOrAssign(FindAvgCAxesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));
  args.insertOrAssign(FindAvgCAxesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));
  args.insertOrAssign(FindAvgCAxesFilter::k_CellFeatureAttributeMatrix_Key, std::make_any<DataPath>(k_CellFeatureDataPath));
  args.insertOrAssign(FindAvgCAxesFilter::k_AvgCAxesArrayPath_Key, std::make_any<std::string>(k_AvgCAxesComputed));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, k_CellFeatureDataPath.createChildPath(k_AvgCAxesExemplar), k_CellFeatureDataPath.createChildPath(k_AvgCAxesComputed), UnitTest::EPSILON,
                                                false);
}

TEST_CASE("OrientationAnalysis::FindAvgCAxesFilter: Invalid Filter Execution", "[OrientationAnalysis][FindAvgCAxesFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_caxis_data.tar.gz", "6_6_caxis_data",
                                                             complex::unit_test::k_BinaryTestOutputDir);

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_caxis_data/6_6_find_caxis_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  auto& crystalStructs = dataStructure.getDataRefAs<UInt32Array>(k_CrystalStructuresArrayPath);
  crystalStructs[1] = 1;

  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindAvgCAxesFilter filter;
  Arguments args;

  // Invalid crystal structure type : should fail in execute
  args.insertOrAssign(FindAvgCAxesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsArrayPath));
  args.insertOrAssign(FindAvgCAxesFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsArrayPath));
  args.insertOrAssign(FindAvgCAxesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));
  args.insertOrAssign(FindAvgCAxesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));
  args.insertOrAssign(FindAvgCAxesFilter::k_CellFeatureAttributeMatrix_Key, std::make_any<DataPath>(k_CellFeatureDataPath));
  args.insertOrAssign(FindAvgCAxesFilter::k_AvgCAxesArrayPath_Key, std::make_any<std::string>(k_AvgCAxesComputed));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(executeResult.result)
}
