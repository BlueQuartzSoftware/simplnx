#include "OrientationAnalysis/Filters/ConvertHexGridToSquareGridFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Parameters/GeneratedFileListParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

using namespace nx::core;

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
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "convert_hex_grid_to_square_grid_test.tar.gz", k_HexToSqrTestFilesDir);
  fs::path kOutPath = fs::path(fmt::format("{}/single", nx::core::unit_test::k_BinaryTestOutputDir));

  if(!exists(kOutPath))
  {
    fs::create_directories(kOutPath);
  }

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ConvertHexGridToSquareGridFilter filter;
    DataStructure dataStructure;
    Arguments args;

    fs::path kInPath = fs::path(fmt::format("{}/{}/single/hex_grid.ang", nx::core::unit_test::k_TestFilesDir, ::k_HexToSqrTestFilesDir));

    // Input Parameters
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_MultipleFiles_Key, std::make_any<bool>(false));
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_InputPath_Key, std::make_any<fs::path>(kInPath));
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(k_Spacing));

    // Output Parameters
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_OutputPath_Key, std::make_any<fs::path>(kOutPath));
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_OutputPrefix_Key, std::make_any<std::string>("Sqr_"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }

  REQUIRE(::CompareFiles(fmt::format("{}/{}/single/exemplar/Sqr_SIMPL_hex_grid.ang", nx::core::unit_test::k_TestFilesDir, ::k_HexToSqrTestFilesDir), kOutPath.string() + "/Sqr_hex_grid.ang"));
}

TEST_CASE("OrientationAnalysis::ConvertHexGridToSquareGridFilter: Multiple File Valid Execution", "[OrientationAnalysis][ConvertHexGridToSquareGridFilter]")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "convert_hex_grid_to_square_grid_test.tar.gz", k_HexToSqrTestFilesDir);
  fs::path kOutPath = fs::path(fmt::format("{}/multi", nx::core::unit_test::k_BinaryTestOutputDir));

  if(!exists(kOutPath))
  {
    fs::create_directories(kOutPath);
  }

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ConvertHexGridToSquareGridFilter filter;
    DataStructure dataStructure;
    Arguments args;

    GeneratedFileListParameter::ValueType kInPath;
    kInPath.start_index = 1;
    kInPath.increment_index = 1;
    kInPath.end_index = 2;
    kInPath.input_path = fmt::format("{}/{}/multi", nx::core::unit_test::k_TestFilesDir, ::k_HexToSqrTestFilesDir);
    kInPath.file_prefix = "hex_grid";
    kInPath.ordering = GeneratedFileListParameter::OrderingType::LowToHigh;
    kInPath.padding_digits = 0;
    kInPath.file_extension = ".ang";
    kInPath.file_suffix = "";

    // Input Parameters
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_MultipleFiles_Key, std::make_any<bool>(true));
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_GeneratedFileList_Key, std::make_any<GeneratedFileListParameter::ValueType>(kInPath));
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(k_Spacing));

    // Output Parameters
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_OutputPath_Key, std::make_any<fs::path>(kOutPath));
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_OutputPrefix_Key, std::make_any<std::string>("Sqr_"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }

  REQUIRE((fs::exists(kOutPath) && fs::is_directory(kOutPath)));

  REQUIRE(::CompareFiles(fmt::format("{}/{}/multi/exemplars/Sqr_SIMPL_hex_grid1.ang", nx::core::unit_test::k_TestFilesDir, ::k_HexToSqrTestFilesDir), kOutPath.string() + "/Sqr_hex_grid1.ang"));
  REQUIRE(::CompareFiles(fmt::format("{}/{}/multi/exemplars/Sqr_SIMPL_hex_grid2.ang", nx::core::unit_test::k_TestFilesDir, ::k_HexToSqrTestFilesDir), kOutPath.string() + "/Sqr_hex_grid2.ang"));
}

TEST_CASE("OrientationAnalysis::ConvertHexGridToSquareGridFilter: Cancellation before publication preserves each output", "[OrientationAnalysis][ConvertHexGridToSquareGridFilter]")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "convert_hex_grid_to_square_grid_test.tar.gz", k_HexToSqrTestFilesDir);
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "convert_hex_grid_cancellation";
  const fs::path existingOutputPath = outputPath / "Sqr_hex_grid1.ang";
  const fs::path absentOutputPath = outputPath / "Sqr_hex_grid2.ang";
  const std::string sentinel = "existing converted-file bytes";

  std::error_code cleanupError;
  fs::remove_all(outputPath, cleanupError);
  REQUIRE_FALSE(cleanupError);
  fs::create_directories(outputPath, cleanupError);
  REQUIRE_FALSE(cleanupError);
  {
    std::ofstream outputStream(existingOutputPath, std::ios::binary);
    REQUIRE(outputStream.is_open());
    outputStream << sentinel;
  }
  REQUIRE_FALSE(fs::exists(absentOutputPath));

  ConvertHexGridToSquareGridFilter filter;
  DataStructure dataStructure;
  Arguments args;
  GeneratedFileListParameter::ValueType inputFileList;
  inputFileList.start_index = 1;
  inputFileList.increment_index = 1;
  inputFileList.end_index = 2;
  inputFileList.input_path = fmt::format("{}/{}/multi", unit_test::k_TestFilesDir, k_HexToSqrTestFilesDir);
  inputFileList.file_prefix = "hex_grid";
  inputFileList.ordering = GeneratedFileListParameter::OrderingType::LowToHigh;
  inputFileList.padding_digits = 0;
  inputFileList.file_extension = ".ang";
  inputFileList.file_suffix = "";
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_MultipleFiles_Key, std::make_any<bool>(true));
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_GeneratedFileList_Key, std::make_any<GeneratedFileListParameter::ValueType>(inputFileList));
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(k_Spacing));
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_OutputPath_Key, std::make_any<fs::path>(outputPath));
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_OutputPrefix_Key, std::make_any<std::string>("Sqr_"));

  auto preflightResult2 = filter.preflight(dataStructure, args).output_actions;
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult2);
  std::atomic_bool shouldCancel = false;
  bool sawSaveCheckpoint = false;
  IFilter::MessageHandler cancelAtSaveCheckpoint{[&](const IFilter::Message& message) {
    if(message.type == IFilter::Message::Type::Info && message.message == "Saving converted files")
    {
      sawSaveCheckpoint = true;
      shouldCancel.store(true);
    }
  }};
  const auto executeResult = filter.execute(dataStructure, args, nullptr, cancelAtSaveCheckpoint, shouldCancel);

  REQUIRE(sawSaveCheckpoint);
  REQUIRE(shouldCancel.load());
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE_FALSE(executeResult.result.errors().empty());
  REQUIRE(executeResult.result.errors().front().code == -1);
  std::ifstream inputStream(existingOutputPath, std::ios::binary);
  REQUIRE(inputStream.is_open());
  const std::string contents((std::istreambuf_iterator<char>(inputStream)), std::istreambuf_iterator<char>());
  REQUIRE(contents == sentinel);
  inputStream.close();
  REQUIRE_FALSE(fs::exists(absentOutputPath));

  fs::remove_all(outputPath, cleanupError);
  REQUIRE_FALSE(cleanupError);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
