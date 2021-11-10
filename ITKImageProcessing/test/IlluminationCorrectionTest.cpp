/**
 * This file is auto generated from the original ITKImageProcessing/IlluminationCorrection
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
 * When you start working on this unit test remove "[IlluminationCorrection][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/MontageSelectionFilterParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "ITKImageProcessing/Filters/IlluminationCorrection.hpp"
#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"

using namespace complex;

TEST_CASE("ITKImageProcessing::IlluminationCorrection: Basic Instantiation and Parameter Check", "[IlluminationCorrection][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  IlluminationCorrection filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  /*[x]*/ args.insert(IlluminationCorrection::k_MontageSelection_Key, std::make_any<<<<NOT_IMPLEMENTED>>>>({}));
  args.insert(IlluminationCorrection::k_CellAttributeMatrixName_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insert(IlluminationCorrection::k_ImageDataArrayName_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insert(IlluminationCorrection::k_CorrectedImageDataArrayName_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insert(IlluminationCorrection::k_BackgroundDataContainerPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(IlluminationCorrection::k_BackgroundCellAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(IlluminationCorrection::k_BackgroundImageArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insert(IlluminationCorrection::k_LowThreshold_Key, std::make_any<int32>(1234356));
  args.insert(IlluminationCorrection::k_HighThreshold_Key, std::make_any<int32>(1234356));
  args.insert(IlluminationCorrection::k_ApplyMedianFilter_Key, std::make_any<bool>(false));
  args.insert(IlluminationCorrection::k_MedianRadius_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(3)));
  args.insert(IlluminationCorrection::k_ApplyCorrection_Key, std::make_any<bool>(false));
  args.insert(IlluminationCorrection::k_ExportCorrectedImages_Key, std::make_any<bool>(false));
  args.insert(IlluminationCorrection::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path("/Path/To/Output/Directory/To/Read")));
  args.insert(IlluminationCorrection::k_FileExtension_Key, std::make_any<StringParameter::ValueType>("SomeString"));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("ITKImageProcessing::IlluminationCorrection: Valid filter execution")
//{
//
//}

// TEST_CASE("ITKImageProcessing::IlluminationCorrection: InValid filter execution")
//{
//
//}
