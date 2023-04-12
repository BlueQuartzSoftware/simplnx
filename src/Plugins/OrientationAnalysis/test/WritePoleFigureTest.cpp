/**
 * This file is auto generated from the original OrientationAnalysis/WritePoleFigureFilter
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
 * When you start working on this unit test remove "[WritePoleFigureFilter][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "OrientationAnalysis/Filters/WritePoleFigureFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace complex;

#include <fstream>
#include <vector>
typedef unsigned char BYTE;

std::vector<BYTE> readFile(const char* filename)
{
  // open the file:
  std::streampos fileSize;
  std::ifstream file(filename, std::ios::binary);

  // get its size:
  file.seekg(0, std::ios::end);
  fileSize = file.tellg();
  file.seekg(0, std::ios::beg);

  // read the data:
  std::vector<BYTE> fileData(fileSize);
  file.read((char*)&fileData[0], fileSize);
  return fileData;
}

TEST_CASE("OrientationAnalysis::WritePoleFigureFilter: Valid Filter Execution", "[OrientationAnalysis][WritePoleFigureFilter]")
{

  // std::vector<uint8_t> contents = readFile("/Users/mjackson/Workspace1/DREAM3DNX/src/NXComponents/resources/fonts/FiraSans-Regular.ttf");
  std::vector<uint8_t> contents = readFile("/tmp/Lato-Bold.ttf.b64");

  std::string fontName = "LatoBold";
  std::ofstream header("/Users/mjackson/Workspace1/complex/src/Plugins/OrientationAnalysis/src/OrientationAnalysis/utilities/LatoBold.hpp", std::ios_base::out);

  header << "#pragma once\n";
  header << "#include <vector>\n";
  header << "namespace fonts {\n";
  header << "// clang-format off\n";
  header << "  char const k_" << fontName << "Base64 [] = \n    ";

  header << "\"";
  int count = 0;
  for(size_t i = 0; i < contents.size(); i++)
  {
    header << contents[i];
    // header << static_cast<int>(contents[i]) << ", ";

    count++;
    if(count == 72)
    {
      header << "\"\n    \"";
      count = 0;
    }
  }
  header << "\";\n\n";
  header << "// clang-format on\n";
  header << "}\n";

  // Instantiate the filter, a DataStructure object and an Arguments Object
  WritePoleFigureFilter filter;
  DataStructure ds;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(WritePoleFigureFilter::k_Title_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insertOrAssign(WritePoleFigureFilter::k_LambertSize_Key, std::make_any<int32>(1234356));
  args.insertOrAssign(WritePoleFigureFilter::k_NumColors_Key, std::make_any<int32>(1234356));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageFormat_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageLayout_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(WritePoleFigureFilter::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path("/Path/To/Output/Directory/To/Read")));
  args.insertOrAssign(WritePoleFigureFilter::k_ImagePrefix_Key, std::make_any<StringParameter::ValueType>("SomeString"));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageSize_Key, std::make_any<int32>(1234356));
  args.insertOrAssign(WritePoleFigureFilter::k_UseGoodVoxels_Key, std::make_any<bool>(false));
  args.insertOrAssign(WritePoleFigureFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(WritePoleFigureFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(WritePoleFigureFilter::k_GoodVoxelsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(WritePoleFigureFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(WritePoleFigureFilter::k_MaterialNameArrayPath_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());
}

// TEST_CASE("OrientationAnalysis::WritePoleFigureFilter: InValid Filter Execution")
//{
//
// }
