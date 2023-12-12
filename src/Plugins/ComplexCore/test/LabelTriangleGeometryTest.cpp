#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/LabelTriangleGeometryFilter.hpp"

using namespace complex;

TEST_CASE("ComplexCore::LabelTriangleGeometryFilter: Valid Filter Execution", "[ComplexCore][LabelTriangleGeometryFilter][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  LabelTriangleGeometryFilter filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(LabelTriangleGeometryFilter::k_TriangleGeomPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(LabelTriangleGeometryFilter::k_CreatedRegionIdsPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(LabelTriangleGeometryFilter::k_TriangleAttributeMatrixName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(LabelTriangleGeometryFilter::k_NumTrianglesName_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("ComplexCore::LabelTriangleGeometryFilter: InValid Filter Execution")
//{
//
// }
