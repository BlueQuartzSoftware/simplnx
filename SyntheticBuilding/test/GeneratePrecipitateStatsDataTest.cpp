/**
 * This file is auto generated from the original SyntheticBuilding/GeneratePrecipitateStatsData
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
 * When you start working on this unit test remove "[GeneratePrecipitateStatsData][.][UNIMPLEMENTED]"
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
#include "complex/Parameters/VectorParameter.hpp"

#include "SyntheticBuilding/Filters/GeneratePrecipitateStatsData.hpp"
#include "SyntheticBuilding/SyntheticBuilding_test_dirs.hpp"

using namespace complex;

TEST_CASE("SyntheticBuilding::GeneratePrecipitateStatsData: Basic Instantiation and Parameter Check", "[GeneratePrecipitateStatsData][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  GeneratePrecipitateStatsData filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insert(GeneratePrecipitateStatsData::k_PhaseName_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insert(GeneratePrecipitateStatsData::k_CrystalSymmetry_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insert(GeneratePrecipitateStatsData::k_MicroPresetModel_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insert(GeneratePrecipitateStatsData::k_PhaseFraction_Key, std::make_any<float64>(2.3456789));
  args.insert(GeneratePrecipitateStatsData::k_Mu_Key, std::make_any<float64>(2.3456789));
  args.insert(GeneratePrecipitateStatsData::k_Sigma_Key, std::make_any<float64>(2.3456789));
  args.insert(GeneratePrecipitateStatsData::k_MinCutOff_Key, std::make_any<float64>(2.3456789));
  args.insert(GeneratePrecipitateStatsData::k_MaxCutOff_Key, std::make_any<float64>(2.3456789));
  args.insert(GeneratePrecipitateStatsData::k_BinStepSize_Key, std::make_any<float64>(2.3456789));
  /*[x]*/ args.insert(GeneratePrecipitateStatsData::k_OdfData_Key, std::make_any<<<<NOT_IMPLEMENTED>>>>({}));
  /*[x]*/ args.insert(GeneratePrecipitateStatsData::k_MdfData_Key, std::make_any<<<<NOT_IMPLEMENTED>>>>({}));
  /*[x]*/ args.insert(GeneratePrecipitateStatsData::k_AxisOdfData_Key, std::make_any<<<<NOT_IMPLEMENTED>>>>({}));
  args.insert(GeneratePrecipitateStatsData::k_RdfMinMaxDistance_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(2)));
  args.insert(GeneratePrecipitateStatsData::k_RdfNumBins_Key, std::make_any<int32>(1234356));
  args.insert(GeneratePrecipitateStatsData::k_RdfBoxSize_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3)));
  args.insert(GeneratePrecipitateStatsData::k_CreateEnsembleAttributeMatrix_Key, std::make_any<bool>(false));
  args.insert(GeneratePrecipitateStatsData::k_DataContainerName_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(GeneratePrecipitateStatsData::k_CellEnsembleAttributeMatrixName_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(GeneratePrecipitateStatsData::k_AppendToExistingAttributeMatrix_Key, std::make_any<bool>(false));
  args.insert(GeneratePrecipitateStatsData::k_SelectedEnsembleAttributeMatrix_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("SyntheticBuilding::GeneratePrecipitateStatsData: Valid filter execution")
//{
//
//}

// TEST_CASE("SyntheticBuilding::GeneratePrecipitateStatsData: InValid filter execution")
//{
//
//}
