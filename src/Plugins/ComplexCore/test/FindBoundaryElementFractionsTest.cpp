#include <catch2/catch.hpp>

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/FindBoundaryElementFractionsFilter.hpp"

using namespace complex;

namespace
{
const std::string k_BCFName = "BoundaryCellFractions";

const DataPath k_ExemplarBCFPath = DataPath{};
const DataPath k_GeneratedBCFPath = DataPath{};
} // namespace

TEST_CASE("ComplexCore::FindBoundaryElementFractionsFilter: Valid Filter Execution", "[ComplexCore][FindBoundaryElementFractionsFilter]")
{
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    FindBoundaryElementFractionsFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(FindBoundaryElementFractionsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(FindBoundaryElementFractionsFilter::k_BoundaryCellsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(FindBoundaryElementFractionsFilter::k_FeatureDataAMPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(FindBoundaryElementFractionsFilter::k_BoundaryCellFractionsArrayName_Key, std::make_any<std::string>(::k_BCFName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  UnitTest::CompareArrays<float32>(dataStructure, k_ExemplarBCFPath, k_GeneratedBCFPath);
}
