#include "OrientationAnalysis/Filters/ConvertHexGridToSquareGridFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <algorithm>

using namespace complex;

namespace
{
const std::string k_HexToSqrTestFilesDir = "convert_hex_grid_to_square_grid_test";
std::vector<float64> k_Spacing = {};

bool CompareFiles(const std::string& p1, const std::string& p2) {
  std::ifstream f1(p1, std::ifstream::binary|std::ifstream::ate);
  std::ifstream f2(p2, std::ifstream::binary|std::ifstream::ate);

  if (f1.fail() || f2.fail()) {
    return false; //file problem
  }

  if (f1.tellg() != f2.tellg()) {
    return false; //size mismatch
  }

  //seek back to beginning and use std::equal to compare contents
  f1.seekg(0, std::ifstream::beg);
  f2.seekg(0, std::ifstream::beg);
  return std::equal(std::istreambuf_iterator<char>(f1.rdbuf()),
                    std::istreambuf_iterator<char>(),
                    std::istreambuf_iterator<char>(f2.rdbuf()));
}

}

TEST_CASE("OrientationAnalysis::ConvertHexGridToSquareGridFilter: Single File Valid Execution", "[OrientationAnalysis][ConvertHexGridToSquareGridFilter]")
{
//  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "convert_hex_grid_to_square_grid_test.tar.gz",
//                                                             k_HexToSqrTestFilesDir);

  fs::path k_InPath = fs::path(fmt::format("{}/{}/single/hex_grid.ang", complex::unit_test::k_TestFilesDir, ::k_HexToSqrTestFilesDir));
  fs::path k_OutPath = fs::path(fmt::format("{}/single/Sqr_hex_grid.ang", complex::unit_test::k_BinaryTestOutputDir));

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ConvertHexGridToSquareGridFilter filter;
    DataStructure dataStructure;
    Arguments args;

    // Input Parameters
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_MultipleFiles_Key, std::make_any<bool>(false));
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_InputPath_Key, std::make_any<fs::path>(k_InPath));
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_Spacing_Key, std::make_any<std::vector<float64>>(k_Spacing));

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

  REQUIRE(::CompareFiles(fmt::format("{}/{}/single/exemplar/Sqr_hex_grid.ang", complex::unit_test::k_TestFilesDir, ::k_HexToSqrTestFilesDir), k_OutPath.string()));
}

TEST_CASE("OrientationAnalysis::ConvertHexGridToSquareGridFilter: Multiple File Valid Execution", "[OrientationAnalysis][ConvertHexGridToSquareGridFilter]")
{
//  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "convert_hex_grid_to_square_grid_test.tar.gz",
//                                                             k_HexToSqrTestFilesDir);

  fs::path k_InPath = fs::path(fmt::format("{}/{}/multi", complex::unit_test::k_TestFilesDir, ::k_HexToSqrTestFilesDir));
  fs::path k_OutPath = fs::path(fmt::format("{}/multi", complex::unit_test::k_BinaryTestOutputDir));

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ConvertHexGridToSquareGridFilter filter;
    DataStructure dataStructure;
    Arguments args;

    // Input Parameters
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_MultipleFiles_Key, std::make_any<bool>(true));
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_GeneratedFileList_Key, std::make_any<fs::path>(k_InPath));
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_Spacing_Key, std::make_any<std::vector<float64>>(k_Spacing));

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
  usize fileCount = std::distance(fs::directory_iterator(k_OutPath), fs::directory_iterator{});

  auto outputIterator = fs::directory_iterator(k_OutPath);
  auto exemplarIterator = fs::directory_iterator(fs::path(fmt::format("{}/{}/multi/exemplars", complex::unit_test::k_TestFilesDir, ::k_HexToSqrTestFilesDir)));

  for(usize i = 0; i < fileCount; i++)
  {
    REQUIRE(outputIterator->file_size() == outputIterator->file_size());
    outputIterator++;
    exemplarIterator++;
  }
}
