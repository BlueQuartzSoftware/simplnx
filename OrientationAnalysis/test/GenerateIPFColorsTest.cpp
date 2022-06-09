/*
# Test Plan #

Input Files:
DREAM3D_Data/Data/ASCII_Data/EulerAngles.csv
DREAM3D_Data/Data/ASCII_Data/Phases.csv

Output DataArrays:
IPFColors (3 component UInt8 Array)

Comparison Files:
DREAM3D_Data/Data/ASCII_Data/IPFColor.csv

You will need to create a UInt32 DataArray with 2 values in it: [ 999, 1 ]. This will
be the input 'k_CrystalStructuresArrayPath_Key' path and data.


Compare the data sets. The values should be exactly the same.

*/

#include <catch2/catch.hpp>

#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include "OrientationAnalysis/Filters/GenerateIPFColorsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace complex;

TEST_CASE("OrientationAnalysis::GenerateIPFColors: Instantiation and Parameter Check", "[OrientationAnalysis][GenerateIPFColors][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  GenerateIPFColorsFilter filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(GenerateIPFColorsFilter::k_ReferenceDir_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3)));
  args.insertOrAssign(GenerateIPFColorsFilter::k_UseGoodVoxels_Key, std::make_any<bool>(false));
  args.insertOrAssign(GenerateIPFColorsFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(GenerateIPFColorsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(GenerateIPFColorsFilter::k_GoodVoxelsPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(GenerateIPFColorsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(GenerateIPFColorsFilter::k_CellIPFColorsArrayName_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("OrientationAnalysis::GenerateIPFColors: Valid filter execution")
//{
//
//}

// TEST_CASE("OrientationAnalysis::GenerateIPFColors: InValid filter execution")
//{
//
//}
