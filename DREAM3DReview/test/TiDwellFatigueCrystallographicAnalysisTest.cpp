/**
 * This file is auto generated from the original DREAM3DReview/TiDwellFatigueCrystallographicAnalysis
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
 * When you start working on this unit test remove "[TiDwellFatigueCrystallographicAnalysis][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include "DREAM3DReview/DREAM3DReview_test_dirs.hpp"
#include "DREAM3DReview/Filters/TiDwellFatigueCrystallographicAnalysis.hpp"

using namespace complex;

TEST_CASE("DREAM3DReview::TiDwellFatigueCrystallographicAnalysis: Instantiation and Parameter Check", "[DREAM3DReview][TiDwellFatigueCrystallographicAnalysis][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  TiDwellFatigueCrystallographicAnalysis filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_AlphaGlobPhasePresent_Key, std::make_any<bool>(false));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_AlphaGlobPhase_Key, std::make_any<int32>(1234356));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_MTRPhase_Key, std::make_any<int32>(1234356));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_LatticeParameterA_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_LatticeParameterC_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_StressAxis_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3)));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_SubsurfaceDistance_Key, std::make_any<int32>(1234356));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_ConsiderationFraction_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_DoNotAssumeInitiatorPresence_Key, std::make_any<bool>(false));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_InitiatorLowerThreshold_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_InitiatorUpperThreshold_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_HardFeatureLowerThreshold_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_HardFeatureUpperThreshold_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_SoftFeatureLowerThreshold_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_SoftFeatureUpperThreshold_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_DataContainerName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_NeighborListArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_CentroidsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_CellParentIdsArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_NewCellFeatureAttributeMatrixName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_SelectedFeaturesArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_InitiatorsArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_HardFeaturesArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_SoftFeaturesArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_HardSoftGroupsArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(TiDwellFatigueCrystallographicAnalysis::k_FeatureParentIdsArrayName_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("DREAM3DReview::TiDwellFatigueCrystallographicAnalysis: Valid filter execution")
//{
//
//}

// TEST_CASE("DREAM3DReview::TiDwellFatigueCrystallographicAnalysis: InValid filter execution")
//{
//
//}
