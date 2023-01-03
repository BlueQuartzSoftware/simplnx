#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/FindVertexToTriangleDistancesFilter.hpp"

using namespace complex;

TEST_CASE("ComplexCore::FindVertexToTriangleDistancesFilter: Instantiation and Parameter Check", "[ComplexCore][FindVertexToTriangleDistancesFilter][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindVertexToTriangleDistancesFilter filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(FindVertexToTriangleDistancesFilter::k_VertexDataContainer_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(FindVertexToTriangleDistancesFilter::k_TriangleDataContainer_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(FindVertexToTriangleDistancesFilter::k_TriangleNormalsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(FindVertexToTriangleDistancesFilter::k_DistancesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(FindVertexToTriangleDistancesFilter::k_ClosestTriangleIdArrayPath_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("ComplexCore::FindVertexToTriangleDistancesFilter: Valid filter execution")
//{
//
// }

// TEST_CASE("ComplexCore::FindVertexToTriangleDistancesFilter: InValid filter execution")
//{
//
// }
