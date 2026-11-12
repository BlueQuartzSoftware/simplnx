#include "OrientationAnalysis/Filters/ConvertHexGridToSquareGridFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Common/ScopeGuard.hpp"
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
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "convert_hex_grid_to_square_grid_test.tar.gz", k_HexToSqrTestFilesDir);
  fs::path k_OutPath = fs::path(fmt::format("{}/single", nx::core::unit_test::k_BinaryTestOutputDir));

  if(!exists(k_OutPath))
  {
    fs::create_directories(k_OutPath);
  }

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ConvertHexGridToSquareGridFilter filter;
    DataStructure dataStructure;
    Arguments args;

    fs::path k_InPath = fs::path(fmt::format("{}/{}/single/hex_grid.ang", nx::core::unit_test::k_TestFilesDir, ::k_HexToSqrTestFilesDir));

    // Input Parameters
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_MultipleFiles_Key, std::make_any<bool>(false));
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_InputPath_Key, std::make_any<fs::path>(k_InPath));
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(k_Spacing));

    // Output Parameters
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_OutputPath_Key, std::make_any<fs::path>(k_OutPath));
    args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_OutputPrefix_Key, std::make_any<std::string>("Sqr_"));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  REQUIRE(::CompareFiles(fmt::format("{}/{}/single/exemplar/Sqr_SIMPL_hex_grid.ang", nx::core::unit_test::k_TestFilesDir, ::k_HexToSqrTestFilesDir), k_OutPath.string() + "/Sqr_hex_grid.ang"));
}

TEST_CASE("OrientationAnalysis::ConvertHexGridToSquareGridFilter: Multiple File Valid Execution", "[OrientationAnalysis][ConvertHexGridToSquareGridFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "convert_hex_grid_to_square_grid_test.tar.gz", k_HexToSqrTestFilesDir);
  fs::path k_OutPath = fs::path(fmt::format("{}/multi", nx::core::unit_test::k_BinaryTestOutputDir));

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
    k_InPath.inputPath = fmt::format("{}/{}/multi", nx::core::unit_test::k_TestFilesDir, ::k_HexToSqrTestFilesDir);
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
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  REQUIRE((fs::exists(k_OutPath) && fs::is_directory(k_OutPath)));

  REQUIRE(::CompareFiles(fmt::format("{}/{}/multi/exemplars/Sqr_SIMPL_hex_grid1.ang", nx::core::unit_test::k_TestFilesDir, ::k_HexToSqrTestFilesDir), k_OutPath.string() + "/Sqr_hex_grid1.ang"));
  REQUIRE(::CompareFiles(fmt::format("{}/{}/multi/exemplars/Sqr_SIMPL_hex_grid2.ang", nx::core::unit_test::k_TestFilesDir, ::k_HexToSqrTestFilesDir), k_OutPath.string() + "/Sqr_hex_grid2.ang"));
}

TEST_CASE("OrientationAnalysis::ConvertHexGridToSquareGridFilter: Cancellation before publication preserves each output", "[OrientationAnalysis][ConvertHexGridToSquareGridFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "convert_hex_grid_to_square_grid_test.tar.gz", k_HexToSqrTestFilesDir);
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
  inputFileList.startIndex = 1;
  inputFileList.incrementIndex = 1;
  inputFileList.endIndex = 2;
  inputFileList.inputPath = fmt::format("{}/{}/multi", unit_test::k_TestFilesDir, k_HexToSqrTestFilesDir);
  inputFileList.filePrefix = "hex_grid";
  inputFileList.ordering = GeneratedFileListParameter::Ordering::LowToHigh;
  inputFileList.paddingDigits = 0;
  inputFileList.fileExtension = ".ang";
  inputFileList.fileSuffix = "";
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_MultipleFiles_Key, std::make_any<bool>(true));
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_GeneratedFileList_Key, std::make_any<GeneratedFileListParameter::ValueType>(inputFileList));
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(k_Spacing));
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_OutputPath_Key, std::make_any<fs::path>(outputPath));
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_OutputPrefix_Key, std::make_any<std::string>("Sqr_"));

  auto preflightResult2 = filter.preflight(dataStructure, args).outputActions;
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
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ConvertHexGridToSquareGridFilter: ANG extension in any letter case is converted", "[OrientationAnalysis][ConvertHexGridToSquareGridFilter]")
{
  const UnitTest::TestFileSentinel sentinel(unit_test::k_TestFilesDir, "convert_hex_grid_to_square_grid_test.tar.gz", k_HexToSqrTestFilesDir);
  const std::string extension = GENERATE(".ang", ".Ang", ".ANG");
  const bool multipleFiles = GENERATE(false, true);
  CAPTURE(extension, multipleFiles);
  const fs::path root = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "extension_case";
  auto cleanup = MakeScopeGuard([&root]() noexcept {
    std::error_code error;
    fs::remove_all(root, error);
  });
  fs::remove_all(root);
  fs::create_directories(root);
  const fs::path sourceRoot = fs::path(unit_test::k_TestFilesDir.view()) / k_HexToSqrTestFilesDir;
  const fs::path input = root / ("probe1" + extension);
  fs::copy_file(sourceRoot / "single/hex_grid.ang", input);
  if(multipleFiles)
  {
    fs::copy_file(sourceRoot / "single/hex_grid.ang", root / ("probe2" + extension));
  }

  ConvertHexGridToSquareGridFilter filter;
  DataStructure dataStructure;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_MultipleFiles_Key, std::make_any<bool>(multipleFiles));
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_InputPath_Key, std::make_any<fs::path>(input));
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_OutputPath_Key, std::make_any<fs::path>(root / "output"));
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_OutputPrefix_Key, std::make_any<std::string>("Sqr_"));
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(k_Spacing));
  GeneratedFileListParameter::ValueType fileList;
  fileList.inputPath = root.string();
  fileList.filePrefix = "probe";
  fileList.fileSuffix = "";
  fileList.fileExtension = extension;
  fileList.startIndex = 1;
  fileList.endIndex = 2;
  fileList.incrementIndex = 1;
  fileList.paddingDigits = 0;
  fileList.ordering = GeneratedFileListParameter::Ordering::LowToHigh;
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_GeneratedFileList_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileList));

  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto execute = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(execute.result);
  for(int32 index = 1; index <= (multipleFiles ? 2 : 1); ++index)
  {
    const std::string filename = "Sqr_probe" + std::to_string(index) + extension;
    REQUIRE(CompareFiles((sourceRoot / "single/exemplar/Sqr_SIMPL_hex_grid.ang").string(), (root / "output" / filename).string()));
    // Inspect directory entries as existence checks alone cannot verify case on macOS.
    REQUIRE(std::any_of(fs::directory_iterator(root / "output"), fs::directory_iterator{}, [&](const auto& entry) { return entry.path().filename().string() == filename; }));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ConvertHexGridToSquareGridFilter: Unsupported extensions in any letter case are rejected", "[OrientationAnalysis][ConvertHexGridToSquareGridFilter]")
{
  const UnitTest::TestFileSentinel sentinel(unit_test::k_TestFilesDir, "convert_hex_grid_to_square_grid_test.tar.gz", k_HexToSqrTestFilesDir);
  UnitTest::LoadPlugins();

  const auto [extension, expectedCode] = GENERATE(table<std::string, int32>({{".ctf", -44601}, {".Ctf", -44601}, {".CTF", -44601}, {".txt", -44602}, {".Txt", -44602}}));
  const int32 fileCount = GENERATE(1, 2);
  CAPTURE(extension, fileCount);
  const fs::path root = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "extension_case";
  auto cleanup = MakeScopeGuard([&root]() noexcept {
    std::error_code error;
    fs::remove_all(root, error);
  });
  fs::remove_all(root);
  fs::create_directories(root);
  const fs::path sourceRoot = fs::path(unit_test::k_TestFilesDir.view()) / k_HexToSqrTestFilesDir;
  const fs::path input = root / ("probe1" + extension);
  fs::copy_file(sourceRoot / "single/hex_grid.ang", input);
  if(fileCount == 2)
  {
    fs::copy_file(sourceRoot / "single/hex_grid.ang", root / ("probe2" + extension));
  }

  ConvertHexGridToSquareGridFilter filter;
  DataStructure dataStructure;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_MultipleFiles_Key, std::make_any<bool>(true));
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_InputPath_Key, std::make_any<fs::path>(input));
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_OutputPath_Key, std::make_any<fs::path>(root / "output"));
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_OutputPrefix_Key, std::make_any<std::string>("Sqr_"));
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(k_Spacing));
  GeneratedFileListParameter::ValueType fileList;
  fileList.inputPath = root.string();
  fileList.filePrefix = "probe";
  fileList.fileSuffix = "";
  fileList.fileExtension = extension;
  fileList.startIndex = 1;
  fileList.endIndex = fileCount;
  fileList.incrementIndex = 1;
  fileList.paddingDigits = 0;
  fileList.ordering = GeneratedFileListParameter::Ordering::LowToHigh;
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_GeneratedFileList_Key, std::make_any<GeneratedFileListParameter::ValueType>(fileList));

  // The single-file parameter accepts ANG only; use the generated list to reach the filter's extension diagnostics.
  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
  REQUIRE_FALSE(preflight.outputActions.errors().empty());
  const auto& error = preflight.outputActions.errors().front();
  REQUIRE(error.code == expectedCode);
  if(expectedCode == -44602)
  {
    // The path also contains the extension, so match the phrase that reports it.
    CHECK(error.message.find(fmt::format("has the extension '{}'", extension)) != std::string::npos);
    CHECK(error.message.find(".ang") != std::string::npos);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
