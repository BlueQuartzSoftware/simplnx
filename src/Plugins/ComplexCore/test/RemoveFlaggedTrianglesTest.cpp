#include <catch2/catch.hpp>

#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/RemoveFlaggedTrianglesFilter.hpp"

using namespace complex;

TEST_CASE("ComplexCore::RemoveFlaggedTrianglesFilter: Valid Filter Execution", "[ComplexCore][RemoveFlaggedTrianglesFilter][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  RemoveFlaggedTrianglesFilter filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(RemoveFlaggedTrianglesFilter::k_InputGeometry_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(RemoveFlaggedTrianglesFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(RemoveFlaggedTrianglesFilter::k_OutputGeometry_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("ComplexCore::RemoveFlaggedTrianglesFilter: InValid Filter Execution")
//{
//
//}
