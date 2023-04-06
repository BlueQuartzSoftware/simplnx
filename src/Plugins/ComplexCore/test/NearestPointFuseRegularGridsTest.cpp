#include <catch2/catch.hpp>

#include "complex/Parameters/DataGroupSelectionParameter.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/NearestPointFuseRegularGridsFilter.hpp"

using namespace complex;

TEST_CASE("ComplexCore::NearestPointFuseRegularGridsFilter: Valid Filter Execution", "[ComplexCore][NearestPointFuseRegularGridsFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  NearestPointFuseRegularGridsFilter filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_UseFill_Key, std::make_any<bool>(true));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_FillValue_Key, std::make_any<float64>(9.8));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_SamplingGeometryPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_SamplingCellAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_ReferenceGeometryPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_ReferenceCellAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("ComplexCore::NearestPointFuseRegularGridsFilter: InValid Filter Execution")
//{
//
// }
