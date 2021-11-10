/**
 * This file is auto generated from the original ITKImageProcessing/ImportAxioVisionV4Montage
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
 * When you start working on this unit test remove "[ImportAxioVisionV4Montage][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "ITKImageProcessing/Filters/ImportAxioVisionV4Montage.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"

using namespace complex;

TEST_CASE("ITKImageProcessing::ImportAxioVisionV4Montage: Basic Instantiation and Parameter Check", "[ImportAxioVisionV4Montage][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ImportAxioVisionV4Montage filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insert(ImportAxioVisionV4Montage::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path("/Path/To/Input/File/To/Read.data")));
  args.insert(ImportAxioVisionV4Montage::k_MontageName_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insert(ImportAxioVisionV4Montage::k_ColumnMontageLimits_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>(2)));
  args.insert(ImportAxioVisionV4Montage::k_RowMontageLimits_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>(2)));
  args.insert(ImportAxioVisionV4Montage::k_ImportAllMetaData_Key, std::make_any<bool>(false));
  args.insert(ImportAxioVisionV4Montage::k_ChangeOrigin_Key, std::make_any<bool>(false));
  args.insert(ImportAxioVisionV4Montage::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3)));
  args.insert(ImportAxioVisionV4Montage::k_ChangeSpacing_Key, std::make_any<bool>(false));
  args.insert(ImportAxioVisionV4Montage::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3)));
  args.insert(ImportAxioVisionV4Montage::k_ConvertToGrayScale_Key, std::make_any<bool>(false));
  args.insert(ImportAxioVisionV4Montage::k_ColorWeights_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3)));
  args.insert(ImportAxioVisionV4Montage::k_DataContainerPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(ImportAxioVisionV4Montage::k_CellAttributeMatrixName_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insert(ImportAxioVisionV4Montage::k_ImageDataArrayName_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insert(ImportAxioVisionV4Montage::k_MetaDataAttributeMatrixName_Key, std::make_any<StringParameter::ValueType>("SomeString"));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("ITKImageProcessing::ImportAxioVisionV4Montage: Valid filter execution")
//{
//
//}

// TEST_CASE("ITKImageProcessing::ImportAxioVisionV4Montage: InValid filter execution")
//{
//
//}
