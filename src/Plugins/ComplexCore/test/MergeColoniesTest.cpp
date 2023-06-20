/**
 * This file is auto generated from the original ComplexCore/MergeColoniesFilter
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
 * When you start working on this unit test remove "[MergeColoniesFilter][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */


#include <catch2/catch.hpp>

#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"

#include "ComplexCore/Filters/MergeColoniesFilter.hpp"
#include "ComplexCore/ComplexCore_test_dirs.hpp"

using namespace complex;

TEST_CASE("ComplexCore::MergeColoniesFilter: Valid Filter Execution","[ComplexCore][MergeColoniesFilter][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  MergeColoniesFilter filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(MergeColoniesFilter::k_UseNonContiguousNeighbors_Key, std::make_any<bool>(false));
  args.insertOrAssign(MergeColoniesFilter::k_NonContiguousNeighborListArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(MergeColoniesFilter::k_ContiguousNeighborListArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(MergeColoniesFilter::k_AxisTolerance_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(MergeColoniesFilter::k_AngleTolerance_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(MergeColoniesFilter::k_IdentifyGlobAlpha_Key, std::make_any<bool>(false));
  args.insertOrAssign(MergeColoniesFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(MergeColoniesFilter::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(MergeColoniesFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(MergeColoniesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(MergeColoniesFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(MergeColoniesFilter::k_CellParentIdsArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(MergeColoniesFilter::k_GlobAlphaArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(MergeColoniesFilter::k_NewCellFeatureAttributeMatrixName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(MergeColoniesFilter::k_FeatureParentIdsArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(MergeColoniesFilter::k_ActiveArrayName_Key, std::make_any<DataPath>(DataPath{}));


  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

//TEST_CASE("ComplexCore::MergeColoniesFilter: InValid Filter Execution")
//{
//
//}
