#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/GenerateVectorColorsFilter.hpp"

using namespace complex;

TEST_CASE("ComplexCore::GenerateVectorColorsFilter: Valid Filter Execution", "[ComplexCore][GenerateVectorColorsFilter][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  GenerateVectorColorsFilter filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(GenerateVectorColorsFilter::k_UseGoodVoxels_Key, std::make_any<bool>(false));
  args.insertOrAssign(GenerateVectorColorsFilter::k_VectorsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(GenerateVectorColorsFilter::k_GoodVoxelsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(GenerateVectorColorsFilter::k_CellVectorColorsArrayName_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}
