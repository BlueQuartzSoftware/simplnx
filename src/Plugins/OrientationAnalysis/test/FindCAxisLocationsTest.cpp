#include <catch2/catch.hpp>

#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/FindCAxisLocationsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
const std::string k_CAxisLocationsExemplar = "CAxisLocations";
const std::string k_CAxisLocationsComputed = "NX_CAxisLocations";
} // namespace

TEST_CASE("OrientationAnalysis::FindCAxisLocationsFilter: Valid Filter Execution", "[OrientationAnalysis][FindCAxisLocationsFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_caxis_data.tar.gz", "6_6_caxis_data");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_caxis_data/6_6_find_caxis_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindCAxisLocationsFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(FindCAxisLocationsFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsArrayPath));
  args.insertOrAssign(FindCAxisLocationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));
  args.insertOrAssign(FindCAxisLocationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));
  args.insertOrAssign(FindCAxisLocationsFilter::k_CAxisLocationsArrayName_Key, std::make_any<std::string>(k_CAxisLocationsComputed));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, k_CellAttributeMatrix.createChildPath(k_CAxisLocationsExemplar), k_CellAttributeMatrix.createChildPath(k_CAxisLocationsComputed),
                                                UnitTest::EPSILON, false);
}

TEST_CASE("OrientationAnalysis::FindCAxisLocationsFilter: InValid Filter Execution")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_caxis_data.tar.gz", "6_6_caxis_data");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_caxis_data/6_6_find_caxis_data.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  auto& crystalStructs = dataStructure.getDataRefAs<UInt32Array>(k_CrystalStructuresArrayPath);
  crystalStructs[1] = 1;

  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindCAxisLocationsFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(FindCAxisLocationsFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_QuatsArrayPath));
  args.insertOrAssign(FindCAxisLocationsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesArrayPath));
  args.insertOrAssign(FindCAxisLocationsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresArrayPath));
  args.insertOrAssign(FindCAxisLocationsFilter::k_CAxisLocationsArrayName_Key, std::make_any<std::string>(k_CAxisLocationsComputed));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
}
