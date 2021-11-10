/**
 * This file is auto generated from the original StatsToolbox/FindFeatureClustering
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
 * When you start working on this unit test remove "[FindFeatureClustering][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include "StatsToolbox/Filters/FindFeatureClustering.hpp"
#include "StatsToolbox/StatsToolbox_test_dirs.hpp"

using namespace complex;

TEST_CASE("StatsToolbox::FindFeatureClustering: Basic Instantiation and Parameter Check", "[FindFeatureClustering][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindFeatureClustering filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insert(FindFeatureClustering::k_NumberOfBins_Key, std::make_any<int32>(1234356));
  args.insert(FindFeatureClustering::k_PhaseNumber_Key, std::make_any<int32>(1234356));
  args.insert(FindFeatureClustering::k_RemoveBiasedFeatures_Key, std::make_any<bool>(false));
  args.insert(FindFeatureClustering::k_EquivalentDiametersArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(FindFeatureClustering::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(FindFeatureClustering::k_CentroidsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(FindFeatureClustering::k_BiasedFeaturesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(FindFeatureClustering::k_CellEnsembleAttributeMatrixName_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(FindFeatureClustering::k_ClusteringListArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(FindFeatureClustering::k_NewEnsembleArrayArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(FindFeatureClustering::k_MaxMinArrayName_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("StatsToolbox::FindFeatureClustering: Valid filter execution")
//{
//
//}

// TEST_CASE("StatsToolbox::FindFeatureClustering: InValid filter execution")
//{
//
//}
