/**
 * This file is auto generated from the original EMMPM/MultiEmmpmFilter
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
 * When you start working on this unit test remove "[MultiEmmpmFilter][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/EMMPMFilterParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include "EMMPM/EMMPM_test_dirs.hpp"
#include "EMMPM/Filters/MultiEmmpmFilter.hpp"

using namespace complex;

TEST_CASE("EMMPM::MultiEmmpmFilter: Instantiation and Parameter Check", "[EMMPM][MultiEmmpmFilter][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  MultiEmmpmFilter filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  /*[x]*/ args.insert(MultiEmmpmFilter::k_NumClasses_Key, std::make_any<<<<NOT_IMPLEMENTED>>>>({}));
  args.insert(MultiEmmpmFilter::k_UseOneBasedValues_Key, std::make_any<bool>(false));
  args.insert(MultiEmmpmFilter::k_UseGradientPenalty_Key, std::make_any<bool>(false));
  args.insert(MultiEmmpmFilter::k_GradientBetaE_Key, std::make_any<float64>(2.3456789));
  args.insert(MultiEmmpmFilter::k_UseCurvaturePenalty_Key, std::make_any<bool>(false));
  args.insert(MultiEmmpmFilter::k_CurvatureBetaC_Key, std::make_any<float64>(2.3456789));
  args.insert(MultiEmmpmFilter::k_CurvatureRMax_Key, std::make_any<float64>(2.3456789));
  args.insert(MultiEmmpmFilter::k_CurvatureEMLoopDelay_Key, std::make_any<int32>(1234356));
  args.insert(MultiEmmpmFilter::k_InputDataArrayVector_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()}));
  args.insert(MultiEmmpmFilter::k_OutputAttributeMatrixName_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insert(MultiEmmpmFilter::k_UsePreviousMuSigma_Key, std::make_any<bool>(false));
  args.insert(MultiEmmpmFilter::k_OutputArrayPrefix_Key, std::make_any<StringParameter::ValueType>("SomeString"));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("EMMPM::MultiEmmpmFilter: Valid filter execution")
//{
//
//}

// TEST_CASE("EMMPM::MultiEmmpmFilter: InValid filter execution")
//{
//
//}
