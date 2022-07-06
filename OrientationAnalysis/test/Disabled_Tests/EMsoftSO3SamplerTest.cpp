/**
 * This file is auto generated from the original OrientationAnalysis/EMsoftSO3Sampler
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
 * When you start working on this unit test remove "[EMsoftSO3Sampler][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include "OrientationAnalysis/Filters/EMsoftSO3Sampler.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace complex;

TEST_CASE("OrientationAnalysis::EMsoftSO3Sampler: Instantiation and Parameter Check", "[OrientationAnalysis][EMsoftSO3Sampler][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  EMsoftSO3Sampler filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(EMsoftSO3Sampler::k_PointGroup_Key, std::make_any<int32>(1234356));
  args.insertOrAssign(EMsoftSO3Sampler::k_OffsetGrid_Key, std::make_any<bool>(false));
  args.insertOrAssign(EMsoftSO3Sampler::k_MisOr_Key, std::make_any<float64>(2.3456789));
  args.insertOrAssign(EMsoftSO3Sampler::k_RefOr_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3)));
  args.insertOrAssign(EMsoftSO3Sampler::k_MisOrFull_Key, std::make_any<float64>(2.3456789));
  args.insertOrAssign(EMsoftSO3Sampler::k_RefOrFull_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3)));
  args.insertOrAssign(EMsoftSO3Sampler::k_Numsp_Key, std::make_any<int32>(1234356));
  args.insertOrAssign(EMsoftSO3Sampler::k_EulerAnglesArrayName_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("OrientationAnalysis::EMsoftSO3Sampler: Valid filter execution")
//{
//
//}

// TEST_CASE("OrientationAnalysis::EMsoftSO3Sampler: InValid filter execution")
//{
//
//}
