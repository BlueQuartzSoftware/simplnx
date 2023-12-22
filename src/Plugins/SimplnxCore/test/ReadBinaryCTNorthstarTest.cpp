#include <catch2/catch.hpp>

#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/ReadBinaryCTNorthstarFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
const fs::path k_TestDirPath = fs::path(nx::core::unit_test::k_BinaryTestOutputDir.str());
const std::string k_NsiHeaderName = "binary_header.nsihdr";
const fs::path k_NsiHeaderPath = k_TestDirPath / k_NsiHeaderName;
const std::string k_NsiDataFileName1 = "binary_file1.nsidat";
const std::string k_NsiDataFileName2 = "binary_file2.nsidat";
const DataPath densityArrayPath = {{"CT Image Geometry", "CT Scan Data", "Density"}};
const usize xDim = 5;
const usize yDim = 5;
const usize zDim = 7;
const usize file1ZCount = 3;
const usize file2ZCount = 4;

void WriteNsiHeaderFile(const fs::path& nsiHeaderPath, const std::string& file1Name, const std::string& file2Name)
{
  std::ofstream nsiHeaderStream(nsiHeaderPath.string());
  REQUIRE(nsiHeaderStream.good() == true);

  nsiHeaderStream << "<North Star Imaging Volume Header>\n";
  nsiHeaderStream << fmt::format("  <Voxels> {} {} {}\n", yDim, zDim, xDim);
  nsiHeaderStream << "  <Location>\n";
  nsiHeaderStream << "    <Min> 0 0 0\n";
  nsiHeaderStream << fmt::format("    <Max> {} {} {}\n", yDim, zDim, xDim);
  nsiHeaderStream << "  </Location>\n";
  nsiHeaderStream << fmt::format("  <DataRange> 0 {}\n", (xDim * yDim * zDim) - 1);
  nsiHeaderStream << "  <Files>\n";
  nsiHeaderStream << fmt::format("    <Name>{}\n", file1Name);
  nsiHeaderStream << fmt::format("    <NbSlices> {}\n", file1ZCount);
  nsiHeaderStream << fmt::format("    <Name>{}\n", file2Name);
  nsiHeaderStream << fmt::format("    <NbSlices> {}\n", file2ZCount);
  nsiHeaderStream << "  </Files>\n";
  nsiHeaderStream << "</North Star Imaging Volume Header>";

  nsiHeaderStream.close();
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

TEST_CASE("SimplnxCore::ReadBinaryCTNorthStarFilter: Valid filter execution")
{
  // Write binary test files
  WriteNsiHeaderFile(k_NsiHeaderPath, k_NsiDataFileName1, k_NsiDataFileName2);
  WriteNsiBinaryDataFiles<xDim, yDim, file1ZCount, file2ZCount>(k_TestDirPath / k_NsiDataFileName1, k_TestDirPath / k_NsiDataFileName2);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ReadBinaryCTNorthstarFilter filter;
  DataStructure dataStructure;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_InputHeaderFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_NsiHeaderPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(densityArrayPath));
  const Float32Array& densityResult = dataStructure.getDataRefAs<Float32Array>(densityArrayPath);
  REQUIRE(densityResult.getSize() == xDim * yDim * zDim);
  for(usize i = 0; i < densityResult.getSize(); i++)
  {
    REQUIRE(densityResult[i] == i);
  }
}

TEST_CASE("SimplnxCore::ReadBinaryCTNorthStarFilter: Valid filter execution with subvolume")
{
  // Write binary test files
  WriteNsiHeaderFile(k_NsiHeaderPath, k_NsiDataFileName1, k_NsiDataFileName2);
  WriteNsiBinaryDataFiles<xDim, yDim, file1ZCount, file2ZCount>(k_TestDirPath / k_NsiDataFileName1, k_TestDirPath / k_NsiDataFileName2);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ReadBinaryCTNorthstarFilter filter;
  DataStructure dataStructure;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_InputHeaderFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_NsiHeaderPath));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_ImportSubvolume_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_StartVoxelCoord_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{1, 2, 4}));
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_EndVoxelCoord_Key, std::make_any<VectorInt32Parameter::ValueType>(std::vector<int32>{4, 3, 6}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(densityArrayPath));
  const Float32Array& densityResult = dataStructure.getDataRefAs<Float32Array>(densityArrayPath);
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
        auto index = static_cast<float32>((z * xDim * yDim) + (y * xDim) + x);
        REQUIRE(densityResult[densityIdx++] == index);
      }
    }
  }
}

TEST_CASE("SimplnxCore::ReadBinaryCTNorthStarFilter: Invalid filter execution - Data file size too small")
{
  const usize file1WrongZCount = 2;

  // Write binary test files
  WriteNsiHeaderFile(k_NsiHeaderPath, k_NsiDataFileName1, k_NsiDataFileName2);
  WriteNsiBinaryDataFiles<xDim, yDim, file1WrongZCount, file2ZCount>(k_TestDirPath / k_NsiDataFileName1, k_TestDirPath / k_NsiDataFileName2);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ReadBinaryCTNorthstarFilter filter;
  DataStructure dataStructure;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_InputHeaderFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_NsiHeaderPath));

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors()[0].code == -38705);
}

TEST_CASE("SimplnxCore::ReadBinaryCTNorthStarFilter: Invalid filter execution - Data file does not exist")
{
  const std::string k_MissingFileName = "this_should_not_exist.nsidat";

  // Write binary test files
  WriteNsiHeaderFile(k_NsiHeaderPath, k_NsiDataFileName1, k_MissingFileName);
  WriteNsiBinaryDataFiles<xDim, yDim, file1ZCount, file2ZCount>(k_TestDirPath / k_NsiDataFileName1, k_TestDirPath / k_NsiDataFileName2);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ReadBinaryCTNorthstarFilter filter;
  DataStructure dataStructure;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ReadBinaryCTNorthstarFilter::k_InputHeaderFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_NsiHeaderPath));

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors()[0].code == -38704);
}

TEST_CASE("SimplnxCore::ReadBinaryCTNorthStarFilter: Invalid filter execution - Incorrect Start/End Coords")
{
  // Write binary test files
  WriteNsiHeaderFile(k_NsiHeaderPath, k_NsiDataFileName1, k_NsiDataFileName2);
  WriteNsiBinaryDataFiles<xDim, yDim, file1ZCount, file2ZCount>(k_TestDirPath / k_NsiDataFileName1, k_TestDirPath / k_NsiDataFileName2);

  // Instantiate the filter, a DataStructure object and an Arguments Object
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
}
