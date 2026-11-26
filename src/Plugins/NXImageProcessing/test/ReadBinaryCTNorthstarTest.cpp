#include "simplnx/UnitTest/ReaderGeometryTest.hpp"

#include <catch2/catch.hpp>

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "NXImageProcessing/Filters/ReadBinaryCTNorthstarFilter.hpp"
#include "NXImageProcessing/NXImageProcessing_test_dirs.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string_view>

#include <nonstd/span.hpp>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
const fs::path k_TestDirPath = fs::path(nx::core::unit_test::k_BinaryTestOutputDir.str());
const std::string k_NsiHeaderName = "binary_header.nsihdr";
const fs::path k_NsiHeaderPath = k_TestDirPath / k_NsiHeaderName;
const std::string k_NsiDataFileName1 = "binary_file1.nsidat";
const std::string k_NsiDataFileName2 = "binary_file2.nsidat";
const DataPath k_DensityArrayPath = {{"CT Image Geometry", "CT Scan Data", "Density"}};
const usize k_XDim = 5;
const usize k_YDim = 5;
const usize k_ZDim = 7;
const usize k_File1ZCount = 3;
const usize k_File2ZCount = 4;

constexpr usize k_BenchmarkDim = 200;
constexpr usize k_BenchmarkSliceTuples = k_BenchmarkDim * k_BenchmarkDim;
constexpr usize k_BenchmarkTotalTuples = k_BenchmarkDim * k_BenchmarkDim * k_BenchmarkDim;
const std::string k_BenchmarkHeaderName = "ReadBinaryCTNorthstarOocBenchmark.nsihdr";
const std::string k_BenchmarkDataName = "ReadBinaryCTNorthstarOocBenchmark.nsidat";
const fs::path k_BenchmarkHeaderPath = k_TestDirPath / k_BenchmarkHeaderName;
const fs::path k_BenchmarkDataPath = k_TestDirPath / k_BenchmarkDataName;

void WriteNsiHeaderFile(const fs::path& nsiHeaderPath, const std::string& file1Name, const std::string& file2Name, usize voxelXDim = k_XDim, std::string_view minX = "0", std::string_view maxX = {})
{
  std::ofstream nsiHeaderStream(nsiHeaderPath.string());
  REQUIRE(nsiHeaderStream.good() == true);

  nsiHeaderStream << "<North Star Imaging Volume Header>\n";
  nsiHeaderStream << fmt::format("  <Voxels> {} {} {}\n", k_YDim, k_ZDim, voxelXDim);
  nsiHeaderStream << "  <Location>\n";
  nsiHeaderStream << fmt::format("    <Min> 0 0 {}\n", minX);
  nsiHeaderStream << fmt::format("    <Max> {} {} {}\n", k_YDim, k_ZDim, maxX.empty() ? std::to_string(voxelXDim) : std::string(maxX));
  nsiHeaderStream << "  </Location>\n";
  nsiHeaderStream << fmt::format("  <DataRange> 0 {}\n", (voxelXDim * k_YDim * k_ZDim) - 1);
  nsiHeaderStream << "  <Files>\n";
  nsiHeaderStream << fmt::format("    <Name>{}\n", file1Name);
  nsiHeaderStream << fmt::format("    <NbSlices> {}\n", k_File1ZCount);
  nsiHeaderStream << fmt::format("    <Name>{}\n", file2Name);
  nsiHeaderStream << fmt::format("    <NbSlices> {}\n", k_File2ZCount);
  nsiHeaderStream << "  </Files>\n";
  nsiHeaderStream << "</North Star Imaging Volume Header>";

  nsiHeaderStream.close();
}

void WriteBenchmarkNsiHeaderFile()
{
  std::ofstream nsiHeaderStream(k_BenchmarkHeaderPath.string());
  REQUIRE(nsiHeaderStream.good());

  nsiHeaderStream << "<North Star Imaging Volume Header>\n";
  nsiHeaderStream << fmt::format("  <Voxels> {} {} {}\n", k_BenchmarkDim, k_BenchmarkDim, k_BenchmarkDim);
  nsiHeaderStream << "  <Location>\n";
  nsiHeaderStream << "    <Min> 0 0 0\n";
  nsiHeaderStream << fmt::format("    <Max> {} {} {}\n", k_BenchmarkDim, k_BenchmarkDim, k_BenchmarkDim);
  nsiHeaderStream << "  </Location>\n";
  nsiHeaderStream << "  <Files>\n";
  nsiHeaderStream << fmt::format("    <Name>{}\n", k_BenchmarkDataName);
  nsiHeaderStream << fmt::format("    <NbSlices> {}\n", k_BenchmarkDim);
  nsiHeaderStream << "  </Files>\n";
  nsiHeaderStream << "</North Star Imaging Volume Header>";
}

void WriteBenchmarkNsiDataFile()
{
  std::ofstream binaryDataStream(k_BenchmarkDataPath.string(), std::ios::out | std::ios::binary);
  REQUIRE(binaryDataStream.good());

  auto sliceBuffer = std::make_unique<float32[]>(k_BenchmarkSliceTuples);
  for(usize offset = 0; offset < k_BenchmarkTotalTuples; offset += k_BenchmarkSliceTuples)
  {
    for(usize index = 0; index < k_BenchmarkSliceTuples; index++)
    {
      sliceBuffer[index] = static_cast<float32>(offset + index);
    }
    binaryDataStream.write(reinterpret_cast<const char*>(sliceBuffer.get()), static_cast<std::streamsize>(k_BenchmarkSliceTuples * sizeof(float32)));
    REQUIRE(binaryDataStream.good());
  }
}

template <usize voxelXDim, usize voxelYDim, usize file1Slices, usize file2Slices>
void WriteNsiBinaryDataFiles(const fs::path& binaryFilePath1, const fs::path& binaryFilePath2)
{
  const usize array1Size = voxelXDim * voxelYDim * file1Slices;
  const usize array2Size = voxelXDim * voxelYDim * file2Slices;

  {
    std::ofstream binaryDataStream(binaryFilePath1.string(), std::ios::out | std::ios::binary);
    REQUIRE(binaryDataStream.good() == true);

    std::array<float32, array1Size> floatArray = {};
    std::generate(floatArray.begin(), floatArray.end(), [val = usize(0)]() mutable { return static_cast<float32>(val++); });
    binaryDataStream.write(reinterpret_cast<const char*>(&floatArray), sizeof(float32) * floatArray.size());

    binaryDataStream.close();
  }
  {
    std::ofstream binaryDataStream(binaryFilePath2.string(), std::ios::out | std::ios::binary);
    REQUIRE(binaryDataStream.good() == true);

    std::array<float32, array2Size> floatArray = {};
    std::generate(floatArray.begin(), floatArray.end(), [val = usize(array1Size)]() mutable { return static_cast<float32>(val++); });
    binaryDataStream.write(reinterpret_cast<const char*>(&floatArray), sizeof(float32) * floatArray.size());

    binaryDataStream.close();
  }
}
} // namespace

TEST_CASE("NXImageProcessing::ReadBinaryCTNorthStarFilter: Valid filter execution")
{
  UnitTest::LoadPlugins();

  // Write the binary fixture files.
  WriteNsiHeaderFile(k_NsiHeaderPath, k_NsiDataFileName1, k_NsiDataFileName2);
  WriteNsiBinaryDataFiles<k_XDim, k_YDim, k_File1ZCount, k_File2ZCount>(k_TestDirPath / k_NsiDataFileName1, k_TestDirPath / k_NsiDataFileName2);

  // Configure the filter arguments.
  ReadBinaryCTNorthstarFilter filter;
  DataStructure dataStructure;
  Arguments args;

  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_InputHeaderFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_NsiHeaderPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_DensityArrayPath));
  const Float32Array& densityResult = dataStructure.getDataRefAs<Float32Array>(k_DensityArrayPath);
  REQUIRE(densityResult.getSize() == k_XDim * k_YDim * k_ZDim);
  for(usize i = 0; i < densityResult.getSize(); i++)
  {
    REQUIRE(densityResult[i] == i);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::ReadBinaryCTNorthStarFilter: Valid relative header path")
{
  UnitTest::LoadPlugins();

  const fs::path relativeFixtureDir = k_TestDirPath / "relative_northstar";
  fs::create_directories(relativeFixtureDir);
  WriteNsiHeaderFile(relativeFixtureDir / k_NsiHeaderName, k_NsiDataFileName1, k_NsiDataFileName2);
  WriteNsiBinaryDataFiles<k_XDim, k_YDim, k_File1ZCount, k_File2ZCount>(relativeFixtureDir / k_NsiDataFileName1, relativeFixtureDir / k_NsiDataFileName2);

  const fs::path originalWorkingDirectory = fs::current_path();
  auto workingDirectoryGuard = MakeScopeGuard([&originalWorkingDirectory]() noexcept {
    std::error_code restoreError;
    fs::current_path(originalWorkingDirectory, restoreError);
  });
  fs::current_path(k_TestDirPath);
  const fs::path relativeHeaderPath = fs::path("relative_northstar") / k_NsiHeaderName;

  ReadBinaryCTNorthstarFilter filter;
  DataStructure dataStructure;
  Arguments args;
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_InputHeaderFile_Key, std::make_any<FileSystemPathParameter::ValueType>(relativeHeaderPath));

  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_DensityArrayPath));
  const Float32Array& densityResult = dataStructure.getDataRefAs<Float32Array>(k_DensityArrayPath);
  REQUIRE(densityResult.getSize() == k_XDim * k_YDim * k_ZDim);
  for(usize index = 0; index < densityResult.getSize(); index++)
  {
    REQUIRE(densityResult[index] == index);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::ReadBinaryCTNorthStarFilter: Valid filter execution with subvolume")
{
  UnitTest::LoadPlugins();

  // Write the binary fixture files.
  WriteNsiHeaderFile(k_NsiHeaderPath, k_NsiDataFileName1, k_NsiDataFileName2);
  WriteNsiBinaryDataFiles<k_XDim, k_YDim, k_File1ZCount, k_File2ZCount>(k_TestDirPath / k_NsiDataFileName1, k_TestDirPath / k_NsiDataFileName2);

  // Configure the filter arguments.
  ReadBinaryCTNorthstarFilter filter;
  DataStructure dataStructure;
  Arguments args;

  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_InputHeaderFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_NsiHeaderPath));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_ImportSubvolume_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_StartVoxelCoord_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{1, 2, 4}));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_EndVoxelCoord_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{4, 3, 6}));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(k_DensityArrayPath));
  const Float32Array& densityResult = dataStructure.getDataRefAs<Float32Array>(k_DensityArrayPath);
  auto startVoxelCoord = args.value<VectorInt32Parameter::ValueType>(ReadBinaryCTNorthstarFilter::k_StartVoxelCoord_Key);
  auto endVoxelCoord = args.value<VectorInt32Parameter::ValueType>(ReadBinaryCTNorthstarFilter::k_EndVoxelCoord_Key);
  usize subvolumeXDim = endVoxelCoord[0] - startVoxelCoord[0] + 1;
  usize subvolumeYDim = endVoxelCoord[1] - startVoxelCoord[1] + 1;
  usize subvolumeZDim = endVoxelCoord[2] - startVoxelCoord[2] + 1;
  REQUIRE(densityResult.getSize() == subvolumeXDim * subvolumeYDim * subvolumeZDim);
  usize densityIdx = 0;
  for(usize z = startVoxelCoord[2]; z <= endVoxelCoord[2]; z++)
  {
    for(usize y = startVoxelCoord[1]; y <= endVoxelCoord[1]; y++)
    {
      for(usize x = startVoxelCoord[0]; x <= endVoxelCoord[0]; x++)
      {
        auto index = static_cast<float32>((z * k_XDim * k_YDim) + (y * k_XDim) + x);
        REQUIRE(densityResult[densityIdx++] == index);
      }
    }
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::ReadBinaryCTNorthStarFilter: Invalid filter execution - Data file size too small")
{
  UnitTest::LoadPlugins();

  const usize file1WrongZCount = 2;

  // Write the binary fixture files.
  WriteNsiHeaderFile(k_NsiHeaderPath, k_NsiDataFileName1, k_NsiDataFileName2);
  WriteNsiBinaryDataFiles<k_XDim, k_YDim, file1WrongZCount, k_File2ZCount>(k_TestDirPath / k_NsiDataFileName1, k_TestDirPath / k_NsiDataFileName2);

  // Configure the filter arguments.
  ReadBinaryCTNorthstarFilter filter;
  DataStructure dataStructure;
  Arguments args;

  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_InputHeaderFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_NsiHeaderPath));

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors()[0].code == -38705);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::ReadBinaryCTNorthStarFilter: Invalid filter execution - Data file does not exist")
{
  UnitTest::LoadPlugins();

  const std::string kMissingFileName = "this_should_not_exist.nsidat";

  // Write the binary fixture files.
  WriteNsiHeaderFile(k_NsiHeaderPath, k_NsiDataFileName1, kMissingFileName);
  WriteNsiBinaryDataFiles<k_XDim, k_YDim, k_File1ZCount, k_File2ZCount>(k_TestDirPath / k_NsiDataFileName1, k_TestDirPath / k_NsiDataFileName2);

  // Configure the filter arguments.
  ReadBinaryCTNorthstarFilter filter;
  DataStructure dataStructure;
  Arguments args;

  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_InputHeaderFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_NsiHeaderPath));

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors()[0].code == -38704);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::ReadBinaryCTNorthStarFilter: Invalid filter execution - Incorrect Start/End Coords")
{
  UnitTest::LoadPlugins();

  // Write the binary fixture files.
  WriteNsiHeaderFile(k_NsiHeaderPath, k_NsiDataFileName1, k_NsiDataFileName2);
  WriteNsiBinaryDataFiles<k_XDim, k_YDim, k_File1ZCount, k_File2ZCount>(k_TestDirPath / k_NsiDataFileName1, k_TestDirPath / k_NsiDataFileName2);

  // Configure the filter arguments.
  ReadBinaryCTNorthstarFilter filter;
  DataStructure dataStructure;
  Arguments args;

  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_InputHeaderFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_NsiHeaderPath));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_ImportSubvolume_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_StartVoxelCoord_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{4, 2, 4}));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_EndVoxelCoord_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{3, 3, 6}));

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors()[0].code == -38712);

  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_InputHeaderFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_NsiHeaderPath));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_ImportSubvolume_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_StartVoxelCoord_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{3, 3, 4}));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_EndVoxelCoord_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{4, 2, 6}));

  executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors()[0].code == -38713);

  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_InputHeaderFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_NsiHeaderPath));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_ImportSubvolume_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_StartVoxelCoord_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{3, 2, 6}));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_EndVoxelCoord_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{4, 3, 4}));

  executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors()[0].code == -38714);

  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_InputHeaderFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_NsiHeaderPath));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_ImportSubvolume_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_StartVoxelCoord_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{-1, 2, 4}));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_EndVoxelCoord_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{4, 3, 6}));

  executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors()[0].code == -38715);

  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_InputHeaderFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_NsiHeaderPath));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_ImportSubvolume_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_StartVoxelCoord_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{3, -1, 4}));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_EndVoxelCoord_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{4, 3, 6}));

  executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors()[0].code == -38715);

  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_InputHeaderFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_NsiHeaderPath));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_ImportSubvolume_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_StartVoxelCoord_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{3, 2, -1}));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_EndVoxelCoord_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{4, 3, 6}));

  executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors()[0].code == -38715);

  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_InputHeaderFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_NsiHeaderPath));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_ImportSubvolume_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_StartVoxelCoord_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{3, 2, 4}));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_EndVoxelCoord_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{9, 3, 6}));

  executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors()[0].code == -38716);

  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_InputHeaderFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_NsiHeaderPath));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_ImportSubvolume_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_StartVoxelCoord_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{3, 2, 4}));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_EndVoxelCoord_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{4, 9, 6}));

  executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors()[0].code == -38717);

  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_InputHeaderFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_NsiHeaderPath));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_ImportSubvolume_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_StartVoxelCoord_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{3, 2, 4}));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_EndVoxelCoord_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{4, 3, 9}));

  executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors()[0].code == -38718);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::ReadBinaryCTNorthstarFilter: SIMPL Backwards Compatibility", "[NXImageProcessing][ReadBinaryCTNorthstarFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ReadBinaryCTNorthstarFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterListPtr);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilterPtr = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilterPtr != nullptr);

      const IFilter* filterPtr = pipelineFilterPtr->getFilter();
      REQUIRE(filterPtr != nullptr);
      REQUIRE(filterPtr->uuid() == FilterTraits<ReadBinaryCTNorthstarFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
      CHECK(args.value<FileSystemPathParameter::ValueType>(ReadBinaryCTNorthstarFilter::k_InputHeaderFile_Key) == fs::path("/test/path/file.nsihdr"));
      CHECK(args.value<DataPath>(ReadBinaryCTNorthstarFilter::k_ImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<std::string>(ReadBinaryCTNorthstarFilter::k_CellAttributeMatrixName_Key) == "CellData");
      CHECK(args.value<std::string>(ReadBinaryCTNorthstarFilter::k_DensityArrayName_Key) == "Density");
      CHECK(args.value<ChoicesParameter::ValueType>(ReadBinaryCTNorthstarFilter::k_LengthUnit_Key) == 0);
      CHECK(args.value<bool>(ReadBinaryCTNorthstarFilter::k_ImportSubvolume_Key) == true);
    }
  }
}

TEST_CASE("NXImageProcessing::ReadBinaryCTNorthstarFilter: override spacing before subvolume positioning", "[NXImageProcessing][ReadBinaryCTNorthstarFilter]")
{
  WriteNsiHeaderFile(k_NsiHeaderPath, k_NsiDataFileName1, k_NsiDataFileName2);
  WriteNsiBinaryDataFiles<k_XDim, k_YDim, k_File1ZCount, k_File2ZCount>(k_TestDirPath / k_NsiDataFileName1, k_TestDirPath / k_NsiDataFileName2);
  std::string header;
  {
    std::ifstream input(k_NsiHeaderPath);
    header.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
  }
  const auto start = header.find("<Max>");
  REQUIRE(start != std::string::npos);
  header.replace(start, header.find('\n', start) - start, "<Max> 0 0 0");
  {
    std::ofstream output(k_NsiHeaderPath, std::ios::trunc);
    output << header;
    REQUIRE(output.good());
  }

  const ReadBinaryCTNorthstarFilter filter;
  Arguments args;
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_InputHeaderFile_Key, k_NsiHeaderPath);
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_ImportSubvolume_Key, true);
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_StartVoxelCoord_Key, std::vector<int32>{1, 2, 4});
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_EndVoxelCoord_Key, std::vector<int32>{4, 3, 6});
  UnitTest::RequireBadReaderMetadata(filter, args, k_NsiHeaderPath, true);
  UnitTest::SetReaderGeometryOverrides(args, {4, -3, 8}, {2, 3, 4});
  // The cropped origin is input origin plus start voxel times spacing.
  UnitTest::RequireReaderGeometryExecution(filter, args, DataPath({"CT Image Geometry"}), {6, 3, 24}, {2, 3, 4});
}

TEST_CASE("NXImageProcessing::ReadBinaryCTNorthstarFilter: spacing for a wide location range", "[NXImageProcessing][ReadBinaryCTNorthstarFilter]")
{
  const ReadBinaryCTNorthstarFilter filter;
  DataStructure dataStructure;
  Arguments args;
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_InputHeaderFile_Key, k_NsiHeaderPath);
  const DataPath geometryPath({"CT Image Geometry"});

  SECTION("Four voxels have representable spacing")
  {
    WriteNsiHeaderFile(k_NsiHeaderPath, k_NsiDataFileName1, k_NsiDataFileName2, 4, "-2e38", "2e38");
    WriteNsiBinaryDataFiles<4, k_YDim, k_File1ZCount, k_File2ZCount>(k_TestDirPath / k_NsiDataFileName1, k_TestDirPath / k_NsiDataFileName2);
    const auto preflight = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
    const auto volumeInfo = std::ranges::find(preflight.outputValues, "Original Volume", &IFilter::PreflightValue::name);
    REQUIRE(volumeInfo != preflight.outputValues.end());
    CHECK(volumeInfo->value.find("inf") == std::string::npos);
    const auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    // A span of 4e38 over four X voxels gives 1e38 per voxel.
    UnitTest::RequireReaderGeometry(dataStructure, geometryPath, {-2e38F, 0, 0}, {1e38F, 1, 1});
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("One voxel needs a spacing override")
  {
    WriteNsiHeaderFile(k_NsiHeaderPath, k_NsiDataFileName1, k_NsiDataFileName2, 1, "-2e38", "2e38");
    WriteNsiBinaryDataFiles<1, k_YDim, k_File1ZCount, k_File2ZCount>(k_TestDirPath / k_NsiDataFileName1, k_TestDirPath / k_NsiDataFileName2);
    UnitTest::RequireBadReaderMetadata(filter, args, k_NsiHeaderPath, true);
    args.insertOrAssign(ImageIO::k_ChangeSpacing, true);
    args.insertOrAssign(ImageIO::k_Spacing, std::vector<float32>{2, 3, 4});
    UnitTest::RequireReaderGeometryExecution(filter, args, geometryPath, {-2e38F, 0, 0}, {2, 3, 4});
  }
}

TEST_CASE("NXImageProcessing::ReadBinaryCTNorthstarFilter: overrides out-of-range location coordinates", "[NXImageProcessing][ReadBinaryCTNorthstarFilter]")
{
  const bool badMin = GENERATE(false, true);
  CAPTURE(badMin);
  WriteNsiHeaderFile(k_NsiHeaderPath, k_NsiDataFileName1, k_NsiDataFileName2);
  WriteNsiBinaryDataFiles<k_XDim, k_YDim, k_File1ZCount, k_File2ZCount>(k_TestDirPath / k_NsiDataFileName1, k_TestDirPath / k_NsiDataFileName2);
  std::string contents;
  {
    std::ifstream input(k_NsiHeaderPath);
    contents.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
  }
  const auto coordinate = contents.find(badMin ? "<Min>" : "<Max>");
  REQUIRE(coordinate != std::string::npos);
  contents.replace(coordinate, contents.find('\n', coordinate) - coordinate, badMin ? "<Min> 0 0 1e40" : "<Max> 5 7 1e40");
  {
    std::ofstream output(k_NsiHeaderPath, std::ios::trunc);
    output << contents;
    REQUIRE(output.good());
  }

  const ReadBinaryCTNorthstarFilter filter;
  Arguments args;
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_InputHeaderFile_Key, k_NsiHeaderPath);
  UnitTest::SetReaderGeometryOverrides(args, {4, -3, 8}, {2, 3, 4});
  for(const bool replaceSpacing : {false, true})
  {
    if(!badMin && replaceSpacing)
    {
      continue;
    }
    args.insertOrAssign(ImageIO::k_ChangeOrigin, !replaceSpacing);
    args.insertOrAssign(ImageIO::k_ChangeSpacing, replaceSpacing);
    DataStructure dataStructure;
    const auto strict = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(strict.outputActions);
    const auto& diagnostic = strict.outputActions.errors().front();
    INFO(diagnostic.message);
    CHECK(diagnostic.code == -10352);
    CHECK(diagnostic.message.find(k_NsiHeaderPath.string()) != std::string::npos);
    CHECK(diagnostic.message.find(badMin ? "minimum X coordinate" : "maximum X coordinate") != std::string::npos);
    CHECK(diagnostic.message.find("1e40") != std::string::npos);
    CHECK(diagnostic.message.find(badMin ? "Enable 'Set Origin' and 'Set Spacing' to use your own values." : "Enable 'Set Spacing' to use your own values.") != std::string::npos);
  }
  args.insertOrAssign(ImageIO::k_ChangeOrigin, badMin);
  args.insertOrAssign(ImageIO::k_ChangeSpacing, true);
  UnitTest::RequireReaderGeometryExecution(filter, args, DataPath({"CT Image Geometry"}), badMin ? FloatVec3{4, -3, 8} : FloatVec3{0, 0, 0}, {2, 3, 4});
}
