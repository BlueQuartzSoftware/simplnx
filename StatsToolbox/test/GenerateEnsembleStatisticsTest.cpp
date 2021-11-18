/**
 * This file is auto generated from the original StatsToolbox/GenerateEnsembleStatistics
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
 * When you start working on this unit test remove "[GenerateEnsembleStatistics][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/PhaseTypeSelectionFilterParameter.hpp"

#include "StatsToolbox/Filters/GenerateEnsembleStatistics.hpp"
#include "StatsToolbox/StatsToolbox_test_dirs.hpp"

using namespace complex;

TEST_CASE("StatsToolbox::GenerateEnsembleStatistics: Instantiation and Parameter Check", "[StatsToolbox][GenerateEnsembleStatistics][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  GenerateEnsembleStatistics filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  /*[x]*/ args.insertOrAssign(GenerateEnsembleStatistics::k_PhaseTypeArray_Key, std::make_any<<<<NOT_IMPLEMENTED>>>>({}));
  args.insertOrAssign(GenerateEnsembleStatistics::k_SizeCorrelationResolution_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(GenerateEnsembleStatistics::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(GenerateEnsembleStatistics::k_NeighborListArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(GenerateEnsembleStatistics::k_CalculateMorphologicalStats_Key, std::make_any<bool>(false));
  args.insertOrAssign(GenerateEnsembleStatistics::k_SizeDistributionFitType_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(GenerateEnsembleStatistics::k_BiasedFeaturesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(GenerateEnsembleStatistics::k_EquivalentDiametersArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(GenerateEnsembleStatistics::k_AspectRatioDistributionFitType_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(GenerateEnsembleStatistics::k_AspectRatiosArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(GenerateEnsembleStatistics::k_Omega3DistributionFitType_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(GenerateEnsembleStatistics::k_Omega3sArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(GenerateEnsembleStatistics::k_NeighborhoodDistributionFitType_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(GenerateEnsembleStatistics::k_NeighborhoodsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(GenerateEnsembleStatistics::k_AxisEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(GenerateEnsembleStatistics::k_CalculateCrystallographicStats_Key, std::make_any<bool>(false));
  args.insertOrAssign(GenerateEnsembleStatistics::k_SurfaceFeaturesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(GenerateEnsembleStatistics::k_VolumesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(GenerateEnsembleStatistics::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(GenerateEnsembleStatistics::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(GenerateEnsembleStatistics::k_SharedSurfaceAreaListArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(GenerateEnsembleStatistics::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(GenerateEnsembleStatistics::k_PhaseTypesArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(GenerateEnsembleStatistics::k_StatisticsArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(GenerateEnsembleStatistics::k_IncludeRadialDistFunc_Key, std::make_any<bool>(false));
  args.insertOrAssign(GenerateEnsembleStatistics::k_RDFArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(GenerateEnsembleStatistics::k_MaxMinRDFArrayPath_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("StatsToolbox::GenerateEnsembleStatistics: Valid filter execution")
//{
//
//}

// TEST_CASE("StatsToolbox::GenerateEnsembleStatistics: InValid filter execution")
//{
//
//}
