/**
 * This file is auto generated from the original OrientationAnalysis/ImportEbsdMontage
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
 * When you start working on this unit test remove "[ImportEbsdMontage][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/EbsdMontageImportFilterParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include "OrientationAnalysis/Filters/ImportEbsdMontage.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace complex;

TEST_CASE("OrientationAnalysis::ImportEbsdMontage: Instantiation and Parameter Check", "[OrientationAnalysis][ImportEbsdMontage][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ImportEbsdMontage filter;
  DataStructure dataStructure;
  Arguments args;

  // Create default Parameters for the filter.
  /*[x]*/ args.insertOrAssign(ImportEbsdMontage::k_InputFileListInfo_Key, std::make_any<<<<NOT_IMPLEMENTED>>>>({}));
  args.insertOrAssign(ImportEbsdMontage::k_MontageName_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insertOrAssign(ImportEbsdMontage::k_CellAttributeMatrixName_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insertOrAssign(ImportEbsdMontage::k_CellEnsembleAttributeMatrixName_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insertOrAssign(ImportEbsdMontage::k_ScanOverlapPixel_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>(2)));
  args.insertOrAssign(ImportEbsdMontage::k_ScanOverlapPercent_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(2)));
  args.insertOrAssign(ImportEbsdMontage::k_GenerateIPFColorMap_Key, std::make_any<bool>(false));
  args.insertOrAssign(ImportEbsdMontage::k_CellIPFColorsArrayName_Key, std::make_any<StringParameter::ValueType>("SomeString"));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("OrientationAnalysis::ImportEbsdMontage: Valid filter execution")
//{
//
//}

// TEST_CASE("OrientationAnalysis::ImportEbsdMontage: InValid filter execution")
//{
//
//}
