#include <catch2/catch.hpp>

#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/AvizoUniformCoordinateWriterFilter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;
using namespace complex::Constants;

namespace
{
const DataPath k_FeatureIdsPath({k_SmallIN100, k_EbsdScanData, k_FeatureIds});

void CompareAMFiles(std::ifstream& computedFile, std::ifstream& exemplarFile)
{
  std::vector<std::string> computedLines;
  std::vector<std::string> exemplarLines;
  for(std::string line; std::getline(computedFile, line);)
  {
    computedLines.push_back(line);
  }
  for(std::string line; std::getline(exemplarFile, line);)
  {
    exemplarLines.push_back(line);
  }

  REQUIRE(computedLines.size() == exemplarLines.size());
  for(size_t i = 0; i < computedLines.size(); ++i)
  {
    if(i == 6 || i == 7) // skip the author & DateTime lines
    {
      continue;
    }

    REQUIRE(computedLines[i] == exemplarLines[i]);
  }
}
} // namespace

TEST_CASE("ComplexCore::AvizoUniformCoordinateWriterFilter: Valid Filter Execution", "[ComplexCore][AvizoUniformCoordinateWriterFilter]")
{
  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_avizo_writers/6_6_avizo_writers_input.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  AvizoUniformCoordinateWriterFilter filter;
  Arguments args;

  fs::path exemplarOutputPath = fs::path(fmt::format("{}/6_6_avizo_writers/6_6_avizo_uniform_coordinate_writer.am", unit_test::k_TestFilesDir));
  fs::path computedOutputPath(fmt::format("{}/NX_AvisoUniformOutput.am", unit_test::k_BinaryTestOutputDir));
  fs::path exemplarBinaryOutputPath = fs::path(fmt::format("{}/6_6_avizo_writers/6_6_avizo_uniform_coordinate_writer_binary.am", unit_test::k_TestFilesDir));
  fs::path computedBinaryOutputPath(fmt::format("{}/NX_AvisoUniformOutput_binary.am", unit_test::k_BinaryTestOutputDir));

  // Create default Parameters for the filter.
  args.insertOrAssign(AvizoUniformCoordinateWriterFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedOutputPath));
  args.insertOrAssign(AvizoUniformCoordinateWriterFilter::k_WriteBinaryFile_Key, std::make_any<bool>(false));
  args.insertOrAssign(AvizoUniformCoordinateWriterFilter::k_GeometryPath_Key, std::make_any<DataPath>(DataPath({k_SmallIN100})));
  args.insertOrAssign(AvizoUniformCoordinateWriterFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(AvizoUniformCoordinateWriterFilter::k_Units_Key, std::make_any<StringParameter::ValueType>("microns"));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  args.insertOrAssign(AvizoUniformCoordinateWriterFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedBinaryOutputPath));
  args.insertOrAssign(AvizoUniformCoordinateWriterFilter::k_WriteBinaryFile_Key, std::make_any<bool>(true));

  preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  std::ifstream computedFile(computedOutputPath);
  std::ifstream exemplarFile(exemplarOutputPath);
  CompareAMFiles(computedFile, exemplarFile);
  std::ifstream computedBinaryFile(computedBinaryOutputPath);
  std::ifstream exemplarBinaryFile(exemplarBinaryOutputPath);
  CompareAMFiles(computedBinaryFile, exemplarBinaryFile);
}
