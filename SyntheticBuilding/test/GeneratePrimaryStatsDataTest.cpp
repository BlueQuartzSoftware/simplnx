/**
 * This file is auto generated from the original SyntheticBuilding/GeneratePrimaryStatsData
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
 * When you start working on this unit test remove "[GeneratePrimaryStatsData][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/DynamicTableFilterParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include "SyntheticBuilding/Filters/GeneratePrimaryStatsData.hpp"
#include "SyntheticBuilding/SyntheticBuilding_test_dirs.hpp"

using namespace complex;

TEST_CASE("SyntheticBuilding::GeneratePrimaryStatsData: Basic Instantiation and Parameter Check", "[GeneratePrimaryStatsData][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  GeneratePrimaryStatsData filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insert(GeneratePrimaryStatsData::k_PhaseName_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insert(GeneratePrimaryStatsData::k_CrystalSymmetry_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insert(GeneratePrimaryStatsData::k_MicroPresetModel_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insert(GeneratePrimaryStatsData::k_PhaseFraction_Key, std::make_any<float64>(2.3456789));
  args.insert(GeneratePrimaryStatsData::k_Mu_Key, std::make_any<float64>(2.3456789));
  args.insert(GeneratePrimaryStatsData::k_Sigma_Key, std::make_any<float64>(2.3456789));
  args.insert(GeneratePrimaryStatsData::k_MinCutOff_Key, std::make_any<float64>(2.3456789));
  args.insert(GeneratePrimaryStatsData::k_MaxCutOff_Key, std::make_any<float64>(2.3456789));
  args.insert(GeneratePrimaryStatsData::k_BinStepSize_Key, std::make_any<float64>(2.3456789));
  /*[x]*/ args.insert(GeneratePrimaryStatsData::k_OdfData_Key, std::make_any<<<<NOT_IMPLEMENTED>>>>({}));
  /*[x]*/ args.insert(GeneratePrimaryStatsData::k_MdfData_Key, std::make_any<<<<NOT_IMPLEMENTED>>>>({}));
  /*[x]*/ args.insert(GeneratePrimaryStatsData::k_AxisOdfData_Key, std::make_any<<<<NOT_IMPLEMENTED>>>>({}));
  args.insert(GeneratePrimaryStatsData::k_CreateEnsembleAttributeMatrix_Key, std::make_any<bool>(false));
  args.insert(GeneratePrimaryStatsData::k_DataContainerName_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(GeneratePrimaryStatsData::k_CellEnsembleAttributeMatrixName_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(GeneratePrimaryStatsData::k_AppendToExistingAttributeMatrix_Key, std::make_any<bool>(false));
  args.insert(GeneratePrimaryStatsData::k_SelectedEnsembleAttributeMatrix_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("SyntheticBuilding::GeneratePrimaryStatsData: Valid filter execution")
//{
//
//}

// TEST_CASE("SyntheticBuilding::GeneratePrimaryStatsData: InValid filter execution")
//{
//
//}
