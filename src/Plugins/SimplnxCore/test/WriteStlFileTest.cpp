#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/WriteStlFileFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const std::string k_ExemplarDir = fmt::format("{}/6_6_write_stl_test", unit_test::k_TestFilesDir);

std::vector<char> readIn(fs::path filePath)
{
  std::ifstream file(filePath.string(), std::ios_base::binary);

  if(file)
  {
    // get file size
    file.seekg(0, std::ios::end);
    std::streampos length = file.tellg();
    file.seekg(0, std::ios::beg);

    // read whole file into a vector
    std::vector<char> contents(length); // act as a buffer
    file.read(contents.data(), length);

    // build string from psuedo-buffer
    return contents;
  }
  return {};
}

void CompareResults() // compare hash of both file strings
{
  fs::path writtenFilePath = fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/TriangleFeature_0.stl");
  REQUIRE(fs::exists(writtenFilePath));
  fs::path exemplarFilePath = fs::path(k_ExemplarDir + "/ExemplarFeature_0.stl");
  REQUIRE(fs::exists(exemplarFilePath));
  REQUIRE(readIn(writtenFilePath) == readIn(exemplarFilePath));
  fs::path writtenFilePath2 = fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/TriangleFeature_1.stl");
  REQUIRE(fs::exists(writtenFilePath2));
  fs::path exemplarFilePath2 = fs::path(k_ExemplarDir + "/ExemplarFeature_1.stl");
  REQUIRE(fs::exists(exemplarFilePath2));
  REQUIRE(readIn(writtenFilePath2) == readIn(exemplarFilePath2));
}
} // namespace

TEST_CASE("SimplnxCore::WriteStlFileFilter: Valid Filter Execution", "[SimplnxCore][WriteStlFileFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_write_stl_test.tar.gz", "6_6_write_stl_test");

  // Instantiate the filter, a DataStructure object and an Arguments Object
  WriteStlFileFilter filter;
  auto exemplarFilePath = fs::path(fmt::format("{}/exemplar.dream3d", k_ExemplarDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(WriteStlFileFilter::k_GroupingType_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(WriteStlFileFilter::k_OutputStlDirectory_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(std::string(unit_test::k_BinaryTestOutputDir))));
  args.insertOrAssign(WriteStlFileFilter::k_OutputStlPrefix_Key, std::make_any<StringParameter::ValueType>("Triangle"));
  args.insertOrAssign(WriteStlFileFilter::k_TriangleGeomPath_Key, std::make_any<DataPath>(DataPath({"TriangleDataContainer"})));
  args.insertOrAssign(WriteStlFileFilter::k_FeatureIdsPath_Key, std::make_any<DataPath>(DataPath({"TriangleDataContainer", "FaceData", "FaceLabels"})));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());

  ::CompareResults();
}
