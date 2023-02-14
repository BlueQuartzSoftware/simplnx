#include <catch2/catch.hpp>

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/ImportBinaryCTNorthstarFilter.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;

using namespace complex;

namespace
{
const std::string k_NsiHeaderName = "binary_header.nsihdr";
const std::string k_NsiDataFileName1 = "binary_file1.nsidat";
const std::string k_NsiDataFileName2 = "binary_file2.nsidat";
const usize xDim = 5;
const usize yDim = 5;
const usize zDim = 7;
const usize file1ZCount = 3;
const usize file2ZCount = 4;
const DataPath densityArrayPath = {{"ImageGeometry", "CellData", "Density"}};

void WriteNsiHeaderFile(const fs::path& nsiHeaderPath)
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
  nsiHeaderStream << "    <Name>binary_file1.nsidat\n";
  nsiHeaderStream << fmt::format("    <NbSlices> {}\n", file1ZCount);
  nsiHeaderStream << "    <Name>binary_file2.nsidat\n";
  nsiHeaderStream << fmt::format("    <NbSlices> {}\n", file2ZCount);
  nsiHeaderStream << "  </Files>\n";
  nsiHeaderStream << "</North Star Imaging Volume Header>";

  nsiHeaderStream.close();
}

void WriteNsiBinaryDataFiles(const fs::path& binaryFilePath1, const fs::path& binaryFilePath2)
{
  {
    std::ofstream binaryDataStream(binaryFilePath1.string(), std::ios::out | std::ios::binary);
    REQUIRE(binaryDataStream.good() == true);

    std::array<float32, xDim* yDim* file1ZCount> floatArray = {};
    std::generate(floatArray.begin(), floatArray.end(), [val = usize(0)]() mutable { return val++; });
    binaryDataStream.write(reinterpret_cast<const char*>(&floatArray), sizeof(float32) * floatArray.size());

    binaryDataStream.close();
  }
  {
    std::ofstream binaryDataStream(binaryFilePath2.string(), std::ios::out | std::ios::binary);
    REQUIRE(binaryDataStream.good() == true);

    std::array<float32, xDim* yDim* file2ZCount> floatArray = {};
    std::generate(floatArray.begin(), floatArray.end(), [val = usize(xDim * yDim * file1ZCount)]() mutable { return val++; });
    binaryDataStream.write(reinterpret_cast<const char*>(&floatArray), sizeof(float32) * floatArray.size());

    binaryDataStream.close();
  }
}
} // namespace

TEST_CASE("Plugins::ReadBinaryCTNorthStarFilter: Valid filter execution")
{
  // Write binary test files
  fs::path testDirPath = fs::temp_directory_path();
  fs::path nsiHeaderPath = testDirPath / k_NsiHeaderName;
  fs::path binaryFilePath1 = testDirPath / k_NsiDataFileName1;
  fs::path binaryFilePath2 = testDirPath / k_NsiDataFileName2;

  WriteNsiHeaderFile(nsiHeaderPath);
  WriteNsiBinaryDataFiles(binaryFilePath1, binaryFilePath2);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ImportBinaryCTNorthstarFilter filter;
  DataStructure dataStructure;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ImportBinaryCTNorthstarFilter::k_InputHeaderFile_Key, std::make_any<FileSystemPathParameter::ValueType>(nsiHeaderPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(densityArrayPath));
  const Float32Array& densityResult = dataStructure.getDataRefAs<Float32Array>(densityArrayPath);
  REQUIRE(densityResult.getSize() == xDim * yDim * zDim);
  for(usize i = 0; i < densityResult.getSize(); i++)
  {
    REQUIRE(densityResult[i] == i);
  }
}
