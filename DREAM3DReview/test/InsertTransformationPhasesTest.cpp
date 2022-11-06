/**
 * This file is auto generated from the original DREAM3DReview/InsertTransformationPhases
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
 * When you start working on this unit test remove "[InsertTransformationPhases][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include "DREAM3DReview/DREAM3DReview_test_dirs.hpp"
#include "DREAM3DReview/Filters/InsertTransformationPhases.hpp"

using namespace complex;

TEST_CASE("DREAM3DReview::InsertTransformationPhases: Instantiation and Parameter Check", "[DREAM3DReview][InsertTransformationPhases][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  InsertTransformationPhases filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(InsertTransformationPhases::k_ParentPhase_Key, std::make_any<int32>(1234356));
  args.insertOrAssign(InsertTransformationPhases::k_TransCrystalStruct_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(InsertTransformationPhases::k_TransformationPhaseMisorientation_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(InsertTransformationPhases::k_DefineHabitPlane_Key, std::make_any<bool>(false));
  args.insertOrAssign(InsertTransformationPhases::k_TransformationPhaseHabitPlane_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3)));
  args.insertOrAssign(InsertTransformationPhases::k_UseAllVariants_Key, std::make_any<bool>(false));
  args.insertOrAssign(InsertTransformationPhases::k_CoherentFrac_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(InsertTransformationPhases::k_TransformationPhaseThickness_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(InsertTransformationPhases::k_NumTransformationPhasesPerFeature_Key, std::make_any<int32>(1234356));
  args.insertOrAssign(InsertTransformationPhases::k_PeninsulaFrac_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(InsertTransformationPhases::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(InsertTransformationPhases::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(InsertTransformationPhases::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(InsertTransformationPhases::k_CellFeatureAttributeMatrixName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(InsertTransformationPhases::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(InsertTransformationPhases::k_AvgQuatsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(InsertTransformationPhases::k_CentroidsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(InsertTransformationPhases::k_EquivalentDiametersArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(InsertTransformationPhases::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(InsertTransformationPhases::k_StatsGenCellEnsembleAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(InsertTransformationPhases::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(InsertTransformationPhases::k_PhaseTypesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(InsertTransformationPhases::k_ShapeTypesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(InsertTransformationPhases::k_NumFeaturesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(InsertTransformationPhases::k_FeatureParentIdsArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(InsertTransformationPhases::k_NumFeaturesPerParentArrayPath_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("DREAM3DReview::InsertTransformationPhases: Valid filter execution")
//{
//
//}

// TEST_CASE("DREAM3DReview::InsertTransformationPhases: InValid filter execution")
//{
//
//}
