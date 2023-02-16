#include <catch2/catch.hpp>

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/FindBoundaryCellsFilter.hpp"

using namespace complex;

namespace
{
const std::string k_ExemplarDataContainer = "SyntheticVolumeDataContainer";
const std::string k_ComputedBoundaryCellsName = "ComputedBoundaryCells";
const DataPath k_GeometryPath({k_ExemplarDataContainer});
const DataPath k_FeatureIdsPath({k_ExemplarDataContainer, Constants::k_CellData, Constants::k_FeatureIds});
const DataPath k_ExemplarBoundaryCellsPath({k_ExemplarDataContainer, Constants::k_CellData, "BoundaryCellsWithBoundary"});
const DataPath k_ComputedBoundaryCellsPath({k_ExemplarDataContainer, Constants::k_CellData, k_ComputedBoundaryCellsName});
} // namespace

TEST_CASE("ComplexCore::FindBoundaryCellsFilter: Valid filter execution", "[FindBoundaryCellsFilter]")
{
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_FindBoundaryCellsExemplar.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindBoundaryCellsFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(FindBoundaryCellsFilter::k_IgnoreFeatureZero_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindBoundaryCellsFilter::k_IncludeVolumeBoundary_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindBoundaryCellsFilter::k_GeometryPath_Key, std::make_any<DataPath>(k_GeometryPath));
  args.insertOrAssign(FindBoundaryCellsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(FindBoundaryCellsFilter::k_BoundaryCellsArrayName_Key, std::make_any<std::string>(k_ComputedBoundaryCellsName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareArrays<int8>(dataStructure, k_ExemplarBoundaryCellsPath, k_ComputedBoundaryCellsPath);
}
