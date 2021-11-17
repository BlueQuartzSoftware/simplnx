/**
 * This file is auto generated from the original EMMPM/EMMPMFilter
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
 * When you start working on this unit test remove "[EMMPMFilter][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/EMMPMFilterParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include "EMMPM/EMMPM_test_dirs.hpp"
#include "EMMPM/Filters/EMMPMFilter.hpp"

using namespace complex;

TEST_CASE("EMMPM::EMMPMFilter: Instantiation and Parameter Check", "[EMMPM][EMMPMFilter][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  EMMPMFilter filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  /*[x]*/ args.insert(EMMPMFilter::k_NumClasses_Key, std::make_any<<<<NOT_IMPLEMENTED>>>>({}));
  args.insert(EMMPMFilter::k_UseOneBasedValues_Key, std::make_any<bool>(false));
  args.insert(EMMPMFilter::k_UseGradientPenalty_Key, std::make_any<bool>(false));
  args.insert(EMMPMFilter::k_GradientBetaE_Key, std::make_any<float64>(2.3456789));
  args.insert(EMMPMFilter::k_UseCurvaturePenalty_Key, std::make_any<bool>(false));
  args.insert(EMMPMFilter::k_CurvatureBetaC_Key, std::make_any<float64>(2.3456789));
  args.insert(EMMPMFilter::k_CurvatureRMax_Key, std::make_any<float64>(2.3456789));
  args.insert(EMMPMFilter::k_CurvatureEMLoopDelay_Key, std::make_any<int32>(1234356));
  args.insert(EMMPMFilter::k_InputDataArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(EMMPMFilter::k_OutputDataArrayPath_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("EMMPM::EMMPMFilter: Valid filter execution")
//{
//
//}

// TEST_CASE("EMMPM::EMMPMFilter: InValid filter execution")
//{
//
//}
