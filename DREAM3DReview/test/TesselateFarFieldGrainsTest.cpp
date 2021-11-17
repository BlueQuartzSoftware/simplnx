/**
 * This file is auto generated from the original DREAM3DReview/TesselateFarFieldGrains
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
 * When you start working on this unit test remove "[TesselateFarFieldGrains][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/GeneratedFileListParameter.hpp"

#include "DREAM3DReview/DREAM3DReview_test_dirs.hpp"
#include "DREAM3DReview/Filters/TesselateFarFieldGrains.hpp"

using namespace complex;

TEST_CASE("DREAM3DReview::TesselateFarFieldGrains: Instantiation and Parameter Check", "[DREAM3DReview][TesselateFarFieldGrains][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  TesselateFarFieldGrains filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insert(TesselateFarFieldGrains::k_FeatureInputFileListInfo_Key, std::make_any<GeneratedFileListParameter::ValueType>(GeneratedFileListParameter::ValueType{}));
  args.insert(TesselateFarFieldGrains::k_OutputCellAttributeMatrixName_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(TesselateFarFieldGrains::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(TesselateFarFieldGrains::k_FeatureIdsArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(TesselateFarFieldGrains::k_CellPhasesArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(TesselateFarFieldGrains::k_OutputCellFeatureAttributeMatrixName_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(TesselateFarFieldGrains::k_FeaturePhasesArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(TesselateFarFieldGrains::k_FeatureEulerAnglesArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(TesselateFarFieldGrains::k_ElasticStrainsArrayName_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(TesselateFarFieldGrains::k_OutputCellEnsembleAttributeMatrixName_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(TesselateFarFieldGrains::k_CrystalStructuresArrayName_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("DREAM3DReview::TesselateFarFieldGrains: Valid filter execution")
//{
//
//}

// TEST_CASE("DREAM3DReview::TesselateFarFieldGrains: InValid filter execution")
//{
//
//}
