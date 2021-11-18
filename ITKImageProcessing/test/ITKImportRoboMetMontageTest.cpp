/**
 * This file is auto generated from the original ITKImageProcessing/ITKImportRoboMetMontage
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
 * When you start working on this unit test remove "[ITKImportRoboMetMontage][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "ITKImageProcessing/Filters/ITKImportRoboMetMontage.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"

using namespace complex;

TEST_CASE("ITKImageProcessing::ITKImportRoboMetMontage: Instantiation and Parameter Check", "[ITKImageProcessing][ITKImportRoboMetMontage][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ITKImportRoboMetMontage filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ITKImportRoboMetMontage::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path("/Path/To/Input/File/To/Read.data")));
  args.insertOrAssign(ITKImportRoboMetMontage::k_MontageName_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insertOrAssign(ITKImportRoboMetMontage::k_ColumnMontageLimits_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>(2)));
  args.insertOrAssign(ITKImportRoboMetMontage::k_RowMontageLimits_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>(2)));
  args.insertOrAssign(ITKImportRoboMetMontage::k_LengthUnit_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(ITKImportRoboMetMontage::k_SliceNumber_Key, std::make_any<int32>(1234356));
  args.insertOrAssign(ITKImportRoboMetMontage::k_ImageFilePrefix_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insertOrAssign(ITKImportRoboMetMontage::k_ImageFileExtension_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insertOrAssign(ITKImportRoboMetMontage::k_ChangeOrigin_Key, std::make_any<bool>(false));
  args.insertOrAssign(ITKImportRoboMetMontage::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3)));
  args.insertOrAssign(ITKImportRoboMetMontage::k_ChangeSpacing_Key, std::make_any<bool>(false));
  args.insertOrAssign(ITKImportRoboMetMontage::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3)));
  args.insertOrAssign(ITKImportRoboMetMontage::k_ConvertToGrayScale_Key, std::make_any<bool>(false));
  args.insertOrAssign(ITKImportRoboMetMontage::k_ColorWeights_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3)));
  args.insertOrAssign(ITKImportRoboMetMontage::k_DataContainerPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ITKImportRoboMetMontage::k_CellAttributeMatrixName_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insertOrAssign(ITKImportRoboMetMontage::k_ImageDataArrayName_Key, std::make_any<StringParameter::ValueType>("SomeString"));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("ITKImageProcessing::ITKImportRoboMetMontage: Valid filter execution")
//{
//
//}

// TEST_CASE("ITKImageProcessing::ITKImportRoboMetMontage: InValid filter execution")
//{
//
//}
