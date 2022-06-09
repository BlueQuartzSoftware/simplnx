/**

# Test Plan #

Read the following data from DREAM3D_Data:

Input Data:
DREAM3D_Data/Data/ASCII_Data/EulerAngles.csv
Float32, 3 component. 480000 tuples

use 30 Degrees about the <1, 1, 1> axis

Run the filter which does the rotation *in place*.

Read the reference data set:

DREAM3D_Data/Data/ASCII_Data/EulersRotated.csv
Float32, 3 component. 480000 tuples

Compare the 2 data sets. Due to going back and forth between ASCII and Binary you will
probably have to compare using a tolerance of about .0001. Look at the 'ConvertOrientationsTest' at the bottom for an example
 of doing that.

*/

#include <catch2/catch.hpp>

#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include "OrientationAnalysis/Filters/RotateEulerRefFrameFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace complex;

TEST_CASE("OrientationAnalysis::RotateEulerRefFrame: Instantiation and Parameter Check", "[OrientationAnalysis][RotateEulerRefFrame][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  RotateEulerRefFrameFilter filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(RotateEulerRefFrameFilter::k_RotationAngle_Key, std::make_any<float32>(30));
  args.insertOrAssign(RotateEulerRefFrameFilter::k_RotationAxis_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1.0F, 1.0F, 1.0F}));
  args.insertOrAssign(RotateEulerRefFrameFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath{}));

  std::string inputFile = fmt::format("{}/Data/ASCII_Data/EulerAngles.csv", unit_test::k_DREAM3DDataDir.view());
  std::string comparisonDataFile = fmt::format("{}/Data/ASCII_Data/EulersRotated.csv", unit_test::k_DREAM3DDataDir.view());

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("OrientationAnalysis::RotateEulerRefFrame: Valid filter execution")
//{
//
//}

// TEST_CASE("OrientationAnalysis::RotateEulerRefFrame: InValid filter execution")
//{
//
//}
