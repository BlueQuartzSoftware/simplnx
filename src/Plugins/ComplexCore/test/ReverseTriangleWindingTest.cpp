#include <catch2/catch.hpp>

#include "complex/Parameters/DataGroupSelectionParameter.hpp"

#include "ComplexCore/Filters/ReverseTriangleWindingFilter.hpp"
#include "ComplexCore/ComplexCore_test_dirs.hpp"

using namespace complex;

TEST_CASE("ComplexCore::ReverseTriangleWindingFilter: Valid Filter Execution","[ComplexCore][ReverseTriangleWindingFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ReverseTriangleWindingFilter filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ReverseTriangleWindingFilter::k_TriGeomPath_Key, std::make_any<DataPath>(DataPath{}));


  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}
