#include <catch2/catch.hpp>

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/FindAvgCAxesFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace complex;
using namespace complex::Constants;

namespace
{
const DataPath k_CellDataPath({k_SmallIN100, k_EbsdScanData});
const DataPath k_CellFeatureDataPath({k_SmallIN100, k_CellFeatureData});
const std::string k_AvgCAxesExemplar = "AvgCAxes";
const std::string k_AvgCAxesComputed = "NX_AvgCAxes";
} // namespace

TEST_CASE("OrientationAnalysis::FindAvgCAxesFilter: Valid Filter Execution", "[OrientationAnalysis][FindAvgCAxesFilter]")
{
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_find_avg_caxes.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindAvgCAxesFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(FindAvgCAxesFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_CellDataPath.createChildPath(k_Quats)));
  args.insertOrAssign(FindAvgCAxesFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_CellDataPath.createChildPath(k_FeatureIds)));
  args.insertOrAssign(FindAvgCAxesFilter::k_CellFeatureAttributeMatrix_Key, std::make_any<DataPath>(k_CellFeatureDataPath));
  args.insertOrAssign(FindAvgCAxesFilter::k_AvgCAxesArrayPath_Key, std::make_any<std::string>(k_AvgCAxesComputed));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareArrays<float32>(dataStructure, k_CellFeatureDataPath.createChildPath(k_AvgCAxesExemplar), k_CellFeatureDataPath.createChildPath(k_AvgCAxesComputed));
}
