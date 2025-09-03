/**
 * This file is auto generated from the original OrientationAnalysis/EMsoftSO3SamplerFilter
 * runtime information. These are the steps that need to be taken to utilize this
 * unit test in the proper way.
 *
 * 1: Validate each of the default parameters that gets created.
 * 2: Inspect the actual filter to determine if the filter in its default state
 * would pass or fail BOTH the preflight() and execute() methods
 * 3: UPDATE the ```REQUIRE(result.result.valid());``` code to have the proper
 *
 * 4: Add additional unit tests to actually test each code path within the filter
 *
 * There are some example Catch2 ```TEST_CASE``` sections for your inspiration.
 *
 * NOTE the format of the ```TEST_CASE``` macro. Please stick to this format to
 * allow easier parsing of the unit tests.
 *
 * When you start working on this unit test remove "[EMsoftSO3SamplerFilter][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include "OrientationAnalysis/Filters/EMsoftSO3SamplerFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace nx::core;

TEST_CASE("OrientationAnalysis::EMsoftSO3SamplerFilter: Valid Filter Execution", "[OrientationAnalysis][EMsoftSO3SamplerFilter][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  EMsoftSO3SamplerFilter filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(EMsoftSO3SamplerFilter::k_CrystalStructure_Index, std::make_any<int32>(1));
  args.insertOrAssign(EMsoftSO3SamplerFilter::k_OffsetGrid_Key, std::make_any<bool>(false));
  args.insertOrAssign(EMsoftSO3SamplerFilter::k_Mode1Misorientation_Key, std::make_any<float64>(2.3456789));
  args.insertOrAssign(EMsoftSO3SamplerFilter::k_Mode1EulerAngle_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3)));
  args.insertOrAssign(EMsoftSO3SamplerFilter::k_Mode2Misorientation_Key, std::make_any<float64>(2.3456789));
  args.insertOrAssign(EMsoftSO3SamplerFilter::k_Mode2EulerAngle_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3)));
  args.insertOrAssign(EMsoftSO3SamplerFilter::k_NumberSamples_Key, std::make_any<int32>(1234356));
  args.insertOrAssign(EMsoftSO3SamplerFilter::k_EulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("OrientationAnalysis::EMsoftSO3SamplerFilter: InValid Filter Execution")
//{
//
// }
