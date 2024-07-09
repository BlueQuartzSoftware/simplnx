#include <catch2/catch.hpp>

#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/ComputeCAxisLocationsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
const std::string k_CAxisLocationsExemplar = "CAxisLocations";
const std::string k_CAxisLocationsComputed = "NX_CAxisLocations";
} // namespace

TEST_CASE("OrientationAnalysis::ComputeCAxisLocationsFilter: Valid Filter Execution", "[OrientationAnalysis][ComputeCAxisLocationsFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "caxis_data.tar.gz", "caxis_data");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/caxis_data/7_0_find_caxis_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeCAxisLocationsFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsArrayPath));
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_CAxisLocationsArrayName_Key, std::make_any<std::string>(k_CAxisLocationsComputed));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, k_CellAttributeMatrix.createChildPath(k_CAxisLocationsExemplar), k_CellAttributeMatrix.createChildPath(k_CAxisLocationsComputed),
                                                UnitTest::EPSILON, false);
}

TEST_CASE("OrientationAnalysis::ComputeCAxisLocationsFilter: InValid Filter Execution")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "caxis_data.tar.gz", "caxis_data");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/caxis_data/7_0_find_caxis_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  auto& crystalStructs = dataStructure.getDataRefAs<UInt32Array>(k_CrystalStructuresArrayPath);
  crystalStructs[1] = 1;

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeCAxisLocationsFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsArrayPath));
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));
  args.insertOrAssign(ComputeCAxisLocationsFilter::k_CAxisLocationsArrayName_Key, std::make_any<std::string>(k_CAxisLocationsComputed));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
}
