#include <catch2/catch.hpp>

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/FindBoundaryElementFractionsFilter.hpp"

using namespace complex;

TEST_CASE("ComplexCore::FindBoundaryElementFractionsFilter: Valid Filter Execution", "[ComplexCore][FindBoundaryElementFractionsFilter][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindBoundaryElementFractionsFilter filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(FindBoundaryElementFractionsFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(FindBoundaryElementFractionsFilter::k_BoundaryCellsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(FindBoundaryElementFractionsFilter::k_BoundaryCellFractionsArrayName_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}
