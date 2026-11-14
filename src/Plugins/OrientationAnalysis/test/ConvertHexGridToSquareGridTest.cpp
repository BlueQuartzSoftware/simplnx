#include "OrientationAnalysis/Filters/Algorithms/ConvertHexGridToSquareGrid.hpp"
#include "H5Support/H5Lite.h"
#include "H5Support/H5Utilities.h"
#include "OrientationAnalysis/Filters/ConvertHexGridToSquareGridFilter.hpp"
#include "OrientationAnalysis/Filters/EbsdToH5EbsdFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"
#include <chrono>

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

namespace
{
std::string SyntheticAng(float32 step = 1.0F, const std::string& comment = "#\n")
{
  return "# TEM_PIXperUM 1\n# x-star 0.5\n# y-star 0.5\n# z-star 0.5\n# WorkingDistance 15\n"
         "# Phase 1\n# MaterialName Nickel\n# Formula Ni\n# Symmetry 43\n# LatticeConstants 3.52 3.52 3.52 90 90 90\n"
         "# NumberFamilies 0\n# Categories 0 0 0 0 0\n# GRID: HexGrid\n# XSTEP: " +
         std::to_string(step) + "\n# YSTEP: 1\n# NCOLS_ODD: 2\n# NCOLS_EVEN: 1\n# NROWS: 2\n" + comment +
         "0.1 0.2 0.3 0 0 10 0.5 1 100 1\n0.4 0.5 0.6 1 0 20 0.5 1 100 1\n0.7 0.8 0.9 0.5 1 30 0.5 1 100 1\n";
}
void WriteSyntheticAng(const fs::path& path, float32 step = 1.0F, const std::string& comment = "#\n")
{
  std::ofstream file(path, std::ios::binary);
  file << SyntheticAng(step, comment);
  REQUIRE(file.good());
}
Arguments HexArguments(const fs::path& path)
{
  Arguments args;
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_MultipleFiles_Key, false);
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_InputPath_Key, path);
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_OutputPath_Key, path.parent_path());
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_OutputPrefix_Key, std::string("square_"));
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_Spacing_Key, std::vector<float32>{1, 1});
  return args;
}
} // namespace
TEST_CASE("OrientationAnalysis::ConvertHexGridToSquareGrid: EBSD regression suffix", "[OrientationAnalysis][ConvertHexGridToSquareGrid]")
{
  const fs::path path = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ebsd_suffix.ANG";
  const auto cleanup = MakeScopeGuard([&]() noexcept {
    std::error_code ec;
    fs::remove(path, ec);
    fs::remove(path.parent_path() / "square_ebsd_suffix.ANG", ec);
  });
  WriteSyntheticAng(path);
  ConvertHexGridToSquareGridFilter filter;
  DataStructure ds;
  auto result = filter.execute(ds, HexArguments(path));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(fs::exists(path.parent_path() / "square_ebsd_suffix.ANG"));
}
TEST_CASE("OrientationAnalysis::EbsdToH5Ebsd: EBSD regression suffix", "[OrientationAnalysis][EbsdToH5Ebsd]")
{
  const auto extension = GENERATE(std::string(".ang"), std::string(".ANG"), std::string(".Ctf"));
  const fs::path dir = fs::path(unit_test::k_BinaryTestOutputDir.view());
  const fs::path input = dir / ("ebsd_stack1" + extension);
  const auto cleanup = MakeScopeGuard([&]() noexcept {
    std::error_code ec;
    fs::remove(input, ec);
    fs::remove(dir / "ebsd_stack.h5ebsd", ec);
  });
  {
    std::ofstream file(input);
    if(extension == ".Ctf")
    {
      file << "Channel Text File\nPrj\tSynthetic\nAuthor\tTest\nJobMode\tGrid\n"
              "XCells\t1\nYCells\t1\nXStep\t1\nYStep\t1\nAcqE1\t0\nAcqE2\t0\nAcqE3\t0\n"
              "Euler angles refer to Sample Coordinate system (CS0)!\tMag\t2000\tCoverage\t100\tDevice\t0\tKV\t20\tTiltAngle\t70\tTiltAxis\t0\n"
              "Phases\t1\n3.52;3.52;3.52\t90;90;90\tNickel\t11\tSynthetic\n"
              "Phase\tX\tY\tBands\tError\tEuler1\tEuler2\tEuler3\tMAD\tBC\tBS\n"
              "1\t0\t0\t7\t0\t10\t20\t30\t0.5\t105\t177\n";
    }
    else
    {
      file << StringUtilities::replace(StringUtilities::replace(StringUtilities::replace(SyntheticAng(), "HexGrid", "SqrGrid"), "NCOLS_EVEN: 1", "NCOLS_EVEN: 2"), "0.9 0.5 1", "0.9 0 1")
           << "1.0 1.1 1.2 1 1 40 0.5 1 100 1\n";
    }
    REQUIRE(file.good());
  }
  GeneratedFileListParameter::ValueType list;
  list.inputPath = dir.string();
  list.filePrefix = "ebsd_stack";
  list.fileExtension = extension;
  list.paddingDigits = 0;
  list.startIndex = 1;
  list.endIndex = 1;
  list.incrementIndex = 1;
  Arguments args;
  args.insertOrAssign(EbsdToH5EbsdFilter::k_InputFileListInfo_Key, list);
  args.insertOrAssign(EbsdToH5EbsdFilter::k_OutputPath_Key, dir / "ebsd_stack.h5ebsd");
  EbsdToH5EbsdFilter filter;
  DataStructure ds;
  auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(fs::exists(dir / "ebsd_stack.h5ebsd"));
}
TEST_CASE("OrientationAnalysis::ConvertHexGridToSquareGrid: EBSD regression cache older timestamp", "[OrientationAnalysis][ConvertHexGridToSquareGrid]")
{
  const fs::path path = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ebsd_cache.ang";
  const auto cleanup = MakeScopeGuard([&]() noexcept {
    std::error_code ec;
    fs::remove(path, ec);
  });
  WriteSyntheticAng(path);
  ConvertHexGridToSquareGridFilter filter;
  DataStructure ds;
  auto first = filter.preflight(ds, HexArguments(path));
  SIMPLNX_RESULT_REQUIRE_VALID(first.outputActions);
  const auto originalTime = fs::last_write_time(path);
  WriteSyntheticAng(path, 2.0F);
  fs::last_write_time(path, originalTime - std::chrono::hours(1));
  auto second = filter.preflight(ds, HexArguments(path));
  SIMPLNX_RESULT_REQUIRE_VALID(second.outputActions);
  REQUIRE(second.outputValues.at(0).value != first.outputValues.at(0).value);
  auto third = filter.preflight(ds, HexArguments(path));
  SIMPLNX_RESULT_REQUIRE_VALID(third.outputActions);
  REQUIRE(third.outputValues.at(0).value == second.outputValues.at(0).value);
}
TEST_CASE("OrientationAnalysis::ConvertHexGridToSquareGrid: EBSD regression same file", "[OrientationAnalysis][ConvertHexGridToSquareGrid]")
{
  const fs::path path = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ebsd_preserve.ang";
  const auto cleanup = MakeScopeGuard([&]() noexcept {
    std::error_code ec;
    fs::remove(path, ec);
  });
  WriteSyntheticAng(path);
  ConvertHexGridToSquareGridFilter filter;
  DataStructure ds;
  auto args = HexArguments(path);
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_OutputPrefix_Key, std::string{});
  auto preflight = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
  REQUIRE(preflight.outputActions.errors().front().code == -44604);
  ConvertHexGridToSquareGridInputValues values;
  values.MultiFile = false;
  values.InputPath = path;
  values.OutputPath = path.parent_path() / ".";
  values.OutputFilePrefix = "";
  values.XYSpacing = {1, 1};
  std::atomic_bool cancel = false;
  auto result = ConvertHexGridToSquareGrid(ds, {}, cancel, &values)();
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().front().code == -44604);
  std::ifstream file(path);
  const std::string bytes((std::istreambuf_iterator<char>(file)), {});
  REQUIRE(bytes == SyntheticAng());
}
TEST_CASE("OrientationAnalysis::ConvertHexGridToSquareGrid: EBSD regression short comments", "[OrientationAnalysis][ConvertHexGridToSquareGrid]")
{
  const auto comment = GENERATE(std::string("# \n"), std::string("# Notes\n"));
  const fs::path path = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ebsd_comment.ang";
  const auto output = path.parent_path() / "square_ebsd_comment.ang";
  const auto cleanup = MakeScopeGuard([&]() noexcept {
    std::error_code ec;
    fs::remove(path, ec);
    fs::remove(output, ec);
  });
  WriteSyntheticAng(path, 1, comment);
  ConvertHexGridToSquareGridFilter filter;
  DataStructure ds;
  auto result = filter.execute(ds, HexArguments(path));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  std::ifstream file(output);
  const std::string bytes((std::istreambuf_iterator<char>(file)), {});
  REQUIRE(bytes.find(comment) != std::string::npos);
}

TEST_CASE("OrientationAnalysis::ConvertHexGridToSquareGrid: CTF suffix is case insensitive", "[OrientationAnalysis][ConvertHexGridToSquareGrid]")
{
  const auto extension = GENERATE(std::string(".ctf"), std::string(".Ctf"), std::string(".CTF"));
  const fs::path path = fs::path(unit_test::k_BinaryTestOutputDir.view()) / ("ebsd_not_hex1" + extension);
  const auto cleanup = MakeScopeGuard([&]() noexcept {
    std::error_code ec;
    fs::remove(path, ec);
  });
  WriteSyntheticAng(path);
  ConvertHexGridToSquareGridFilter filter;
  DataStructure ds;
  auto args = HexArguments(path);
  GeneratedFileListParameter::ValueType list;
  list.inputPath = path.parent_path().string();
  list.filePrefix = "ebsd_not_hex";
  list.fileExtension = extension;
  list.startIndex = 1;
  list.endIndex = 1;
  list.incrementIndex = 1;
  list.paddingDigits = 0;
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_MultipleFiles_Key, true);
  args.insertOrAssign(ConvertHexGridToSquareGridFilter::k_GeneratedFileList_Key, list);
  auto result = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().front().code == -44601);
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
