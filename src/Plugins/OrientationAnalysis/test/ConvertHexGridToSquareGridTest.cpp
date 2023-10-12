#include "OrientationAnalysis/Filters/ConvertHexGridToSquareGridFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "complex/Parameters/GeneratedFileListParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <fstream>

using namespace complex;

namespace
{
const std::string k_HexToSqrTestFilesDir = "convert_hex_grid_to_square_grid_test";
const std::vector<float32> k_Spacing = {0.05f, 0.0403};

bool CompareFiles(const std::string& p1, const std::string& p2)
{
  fs::path path1(p1);
  fs::path path2(p2);

  if(!fs::exists(path1))
  {
    return false;
  }

  if(!fs::exists(path2))
  {
    return false;
  }

  std::ifstream f1(path1, std::ifstream::binary | std::ifstream::ate);
  std::ifstream f2(path2, std::ifstream::binary | std::ifstream::ate);

  if(!f1.is_open() || !f2.is_open())
  {
    return false; // file problem
  }

  if(f1.tellg() != f2.tellg())
  {
    return false; // size mismatch
  }

  // seek back to beginning and use std::equal to compare contents
  f1.seekg(0, std::ifstream::beg);
  f2.seekg(0, std::ifstream::beg);
  return std::equal(std::istreambuf_iterator<char>(f1.rdbuf()), std::istreambuf_iterator<char>(), std::istreambuf_iterator<char>(f2.rdbuf()));
}

} // namespace

TEST_CASE("OrientationAnalysis::ConvertHexGridToSquareGridFilter: Single File Valid Execution", "[OrientationAnalysis][ConvertHexGridToSquareGridFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "convert_hex_grid_to_square_grid_test.tar.gz",
                                                             k_HexToSqrTestFilesDir);
  fs::path k_OutPath = fs::path(fmt::format("{}/single", complex::unit_test::k_BinaryTestOutputDir));

  if(!exists(k_OutPath))
  {
    fs::create_directories(k_OutPath);
  }

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ConvertHexGridToSquareGridFilter filter;
    DataStructure dataStructure;
    Arguments args;

    fs::path k_InPath = fs::path(fmt::format("{}/{}/single/hex_grid.ang", complex::unit_test::k_TestFilesDir, ::k_HexToSqrTestFilesDir));

    // Input Parameters
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_MultipleFiles_Key, std::make_any<bool>(false));
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_InputPath_Key, std::make_any<fs::path>(k_InPath));
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(k_Spacing));

    // Output Parameters
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_OutputPath_Key, std::make_any<fs::path>(k_OutPath));
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_OutputPrefix_Key, std::make_any<std::string>("Sqr_"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  REQUIRE(::CompareFiles(fmt::format("{}/{}/single/exemplar/Sqr_SIMPL_hex_grid.ang", complex::unit_test::k_TestFilesDir, ::k_HexToSqrTestFilesDir), k_OutPath.string() + "/Sqr_hex_grid.ang"));
}

TEST_CASE("OrientationAnalysis::ConvertHexGridToSquareGridFilter: Multiple File Valid Execution", "[OrientationAnalysis][ConvertHexGridToSquareGridFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "convert_hex_grid_to_square_grid_test.tar.gz",
                                                             k_HexToSqrTestFilesDir);
  fs::path k_OutPath = fs::path(fmt::format("{}/multi", complex::unit_test::k_BinaryTestOutputDir));

  if(!exists(k_OutPath))
  {
    fs::create_directories(k_OutPath);
  }

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ConvertHexGridToSquareGridFilter filter;
    DataStructure dataStructure;
    Arguments args;

    GeneratedFileListParameter::ValueType k_InPath;
    k_InPath.startIndex = 1;
    k_InPath.incrementIndex = 1;
    k_InPath.endIndex = 2;
    k_InPath.inputPath = fs::path(fmt::format("{}/{}/multi", complex::unit_test::k_TestFilesDir, ::k_HexToSqrTestFilesDir));
    k_InPath.filePrefix = "hex_grid";
    k_InPath.ordering = GeneratedFileListParameter::Ordering::LowToHigh;
    k_InPath.paddingDigits = 0;
    k_InPath.fileExtension = ".ang";
    k_InPath.fileSuffix = "";

    // Input Parameters
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_MultipleFiles_Key, std::make_any<bool>(true));
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_GeneratedFileList_Key, std::make_any<GeneratedFileListParameter::ValueType>(k_InPath));
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(k_Spacing));

    // Output Parameters
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_OutputPath_Key, std::make_any<fs::path>(k_OutPath));
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_OutputPrefix_Key, std::make_any<std::string>("Sqr_"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  REQUIRE((fs::exists(k_OutPath) && fs::is_directory(k_OutPath)));

  REQUIRE(::CompareFiles(fmt::format("{}/{}/multi/exemplars/Sqr_SIMPL_hex_grid1.ang", complex::unit_test::k_TestFilesDir, ::k_HexToSqrTestFilesDir), k_OutPath.string() + "/Sqr_hex_grid1.ang"));
  REQUIRE(::CompareFiles(fmt::format("{}/{}/multi/exemplars/Sqr_SIMPL_hex_grid2.ang", complex::unit_test::k_TestFilesDir, ::k_HexToSqrTestFilesDir), k_OutPath.string() + "/Sqr_hex_grid2.ang"));
}
