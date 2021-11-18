/**
 * This file is auto generated from the original DREAM3DReview/ImportPrintRiteTDMSFiles
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
 * When you start working on this unit test remove "[ImportPrintRiteTDMSFiles][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/GeneratedFileListParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "DREAM3DReview/DREAM3DReview_test_dirs.hpp"
#include "DREAM3DReview/Filters/ImportPrintRiteTDMSFiles.hpp"

using namespace complex;

TEST_CASE("DREAM3DReview::ImportPrintRiteTDMSFiles: Instantiation and Parameter Check", "[DREAM3DReview][ImportPrintRiteTDMSFiles][.][UNIMPLEMENTED][!mayfail]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ImportPrintRiteTDMSFiles filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ImportPrintRiteTDMSFiles::k_LayerThickness_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(ImportPrintRiteTDMSFiles::k_LaserOnArrayOption_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(ImportPrintRiteTDMSFiles::k_LaserOnThreshold_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(ImportPrintRiteTDMSFiles::k_DowncastRawData_Key, std::make_any<bool>(false));
  args.insertOrAssign(ImportPrintRiteTDMSFiles::k_ScaleLaserPower_Key, std::make_any<bool>(false));
  args.insertOrAssign(ImportPrintRiteTDMSFiles::k_PowerScalingCoefficients_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(2)));
  args.insertOrAssign(ImportPrintRiteTDMSFiles::k_ScalePyrometerTemperature_Key, std::make_any<bool>(false));
  args.insertOrAssign(ImportPrintRiteTDMSFiles::k_TemperatureScalingCoefficients_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>(2)));
  args.insertOrAssign(ImportPrintRiteTDMSFiles::k_LayerForScaling_Key, std::make_any<int32>(1234356));
  args.insertOrAssign(ImportPrintRiteTDMSFiles::k_SearchRadius_Key, std::make_any<float32>(1.23345f));
  args.insertOrAssign(ImportPrintRiteTDMSFiles::k_SplitRegions1_Key, std::make_any<bool>(false));
  args.insertOrAssign(ImportPrintRiteTDMSFiles::k_SplitRegions2_Key, std::make_any<bool>(false));
  args.insertOrAssign(ImportPrintRiteTDMSFiles::k_STLFilePath1_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path("/Path/To/Input/File/To/Read.data")));
  args.insertOrAssign(ImportPrintRiteTDMSFiles::k_STLFilePath2_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path("/Path/To/Input/File/To/Read.data")));
  args.insertOrAssign(ImportPrintRiteTDMSFiles::k_InputSpatialTransformFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path("/Path/To/Input/File/To/Read.data")));
  args.insertOrAssign(ImportPrintRiteTDMSFiles::k_InputFilesList_Key, std::make_any<GeneratedFileListParameter::ValueType>(GeneratedFileListParameter::ValueType{}));
  args.insertOrAssign(ImportPrintRiteTDMSFiles::k_OutputDirectory_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path("/Path/To/Output/Directory/To/Read")));
  args.insertOrAssign(ImportPrintRiteTDMSFiles::k_OutputFilePrefix_Key, std::make_any<StringParameter::ValueType>("SomeString"));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("DREAM3DReview::ImportPrintRiteTDMSFiles: Valid filter execution")
//{
//
//}

// TEST_CASE("DREAM3DReview::ImportPrintRiteTDMSFiles: InValid filter execution")
//{
//
//}
