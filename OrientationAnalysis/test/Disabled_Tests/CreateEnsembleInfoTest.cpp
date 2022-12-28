/**
 * This file is auto generated from the original OrientationAnalysis/CreateEnsembleInfo
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
 * When you start working on this unit test remove "[CreateEnsembleInfo][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/EnsembleInfoFilterParameter.hpp"

#include "OrientationAnalysis/Filters/CreateEnsembleInfo.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace complex;

TEST_CASE("OrientationAnalysis::CreateEnsembleInfo: Instantiation and Parameter Check", "[OrientationAnalysis][CreateEnsembleInfo][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  CreateEnsembleInfo filter;
  DataStructure dataStructure;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(CreateEnsembleInfo::k_DataContainerName_Key, std::make_any<DataPath>(DataPath{}));
  /*[x]*/ args.insertOrAssign(CreateEnsembleInfo::k_Ensemble_Key, std::make_any<<<<NOT_IMPLEMENTED>>>>({}));
  args.insertOrAssign(CreateEnsembleInfo::k_CellEnsembleAttributeMatrixName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateEnsembleInfo::k_CrystalStructuresArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateEnsembleInfo::k_PhaseTypesArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(CreateEnsembleInfo::k_PhaseNamesArrayName_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("OrientationAnalysis::CreateEnsembleInfo: Valid filter execution")
//{
//
//}

// TEST_CASE("OrientationAnalysis::CreateEnsembleInfo: InValid filter execution")
//{
//
//}
