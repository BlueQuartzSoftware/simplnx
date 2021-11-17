/**
 * This file is auto generated from the original ITKImageProcessing/CalcDewarpParameters
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
 * When you start working on this unit test remove "[CalcDewarpParameters][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/EbsdWarpPolynomialFilterParameter.hpp"
#include "complex/Parameters/MontageStructureSelectionFilterParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include "ITKImageProcessing/Filters/CalcDewarpParameters.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"

using namespace complex;

TEST_CASE("ITKImageProcessing::CalcDewarpParameters: Instantiation and Parameter Check", "[ITKImageProcessing][CalcDewarpParameters][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  CalcDewarpParameters filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  /*[x]*/ args.insert(CalcDewarpParameters::k_MontageName_Key, std::make_any<<<<NOT_IMPLEMENTED>>>>({}));
  args.insert(CalcDewarpParameters::k_MaxIterations_Key, std::make_any<int32>(1234356));
  args.insert(CalcDewarpParameters::k_Delta_Key, std::make_any<int32>(1234356));
  args.insert(CalcDewarpParameters::k_FractionalTolerance_Key, std::make_any<float64>(2.3456789));
  args.insert(CalcDewarpParameters::k_SpecifyInitialSimplex_Key, std::make_any<bool>(false));
  /*[x]*/ args.insert(CalcDewarpParameters::k_XFactors_Key, std::make_any<<<<NOT_IMPLEMENTED>>>>({}));
  /*[x]*/ args.insert(CalcDewarpParameters::k_YFactors_Key, std::make_any<<<<NOT_IMPLEMENTED>>>>({}));
  args.insert(CalcDewarpParameters::k_AttributeMatrixName_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insert(CalcDewarpParameters::k_IPFColorsArrayName_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insert(CalcDewarpParameters::k_TransformDCName_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insert(CalcDewarpParameters::k_TransformMatrixName_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insert(CalcDewarpParameters::k_TransformArrayName_Key, std::make_any<StringParameter::ValueType>("SomeString"));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("ITKImageProcessing::CalcDewarpParameters: Valid filter execution")
//{
//
//}

// TEST_CASE("ITKImageProcessing::CalcDewarpParameters: InValid filter execution")
//{
//
//}
