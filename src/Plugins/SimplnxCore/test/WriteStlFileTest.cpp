#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/CombineStlFilesFilter.hpp"
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
using namespace nx::core::Constants;
namespace
{
const std::string k_ExemplarDir = fmt::format("{}/6_6_write_stl_file_test", unit_test::k_TestFilesDir);
const DataPath k_ComputedTriangleDataContainerName({"ComputedTriangleDataContainer"});
const DataPath k_ExemplarTriangleDataContainerName({k_TriangleDataContainerName});
const std::string k_PartNumberName = "Part Number";

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

void CompareMultipleResults() // compare hash of both file strings
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

void CompareSingleResult() // compare hash of both file strings
{
  fs::path writtenFilePath = fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/Generated.stl");
  REQUIRE(fs::exists(writtenFilePath));
  fs::path exemplarFilePath = fs::path(k_ExemplarDir + "/Exemplar.stl");
  REQUIRE(fs::exists(exemplarFilePath));
  REQUIRE(readIn(writtenFilePath) == readIn(exemplarFilePath));
}
} // namespace

TEST_CASE("SimplnxCore::WriteStlFileFilter: Multiple File Valid", "[SimplnxCore][WriteStlFileFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_write_stl_file_test.tar.gz", "6_6_write_stl_file_test");

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

  ::CompareMultipleResults();
}

TEST_CASE("SimplnxCore::WriteStlFileFilter: Single File Valid", "[SimplnxCore][WriteStlFileFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_write_stl_file_test.tar.gz", "6_6_write_stl_file_test");

  // Instantiate the filter, a DataStructure object and an Arguments Object
  WriteStlFileFilter filter;
  auto exemplarFilePath = fs::path(fmt::format("{}/exemplar.dream3d", k_ExemplarDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(WriteStlFileFilter::k_GroupingType_Key, std::make_any<ChoicesParameter::ValueType>(2));
  args.insertOrAssign(WriteStlFileFilter::k_OutputStlFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/Generated.stl")));
  args.insertOrAssign(WriteStlFileFilter::k_TriangleGeomPath_Key, std::make_any<DataPath>(DataPath({"TriangleDataContainer"})));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());

  ::CompareSingleResult();
}

TEST_CASE("SimplnxCore::WriteStlFileFilter:Part_Number", "[SimplnxCore][WriteStlFileFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_write_stl_file_test.tar.gz", "6_6_write_stl_file_test");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel2(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_combine_stl_files_v2.tar.gz",
                                                               "6_6_combine_stl_files.dream3d");
  DataStructure dataStructure;

  {
    CombineStlFilesFilter filter;
    Arguments args;
    std::string inputStlDir = fmt::format("{}/6_6_combine_stl_files_v2/STL_Models", unit_test::k_TestFilesDir.view());

    // Create default Parameters for the filter.
    args.insertOrAssign(CombineStlFilesFilter::k_StlFilesPath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputStlDir)));
    args.insertOrAssign(CombineStlFilesFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(k_ComputedTriangleDataContainerName));
    args.insertOrAssign(CombineStlFilesFilter::k_FaceAttributeMatrixName_Key, std::make_any<std::string>(k_FaceData));
    args.insertOrAssign(CombineStlFilesFilter::k_FaceNormalsArrayName_Key, std::make_any<std::string>("Face Normals"));
    args.insertOrAssign(CombineStlFilesFilter::k_VertexAttributeMatrixName_Key, std::make_any<std::string>(k_VertexData));
    args.insertOrAssign(CombineStlFilesFilter::k_LabelFaces_Key, std::make_any<bool>(true));
    args.insertOrAssign(CombineStlFilesFilter::k_FaceLabelName_Key, std::make_any<std::string>(k_PartNumberName));
    args.insertOrAssign(CombineStlFilesFilter::k_LabelVertices_Key, std::make_any<bool>(true));
    args.insertOrAssign(CombineStlFilesFilter::k_VertexLabelName_Key, std::make_any<std::string>(k_PartNumberName));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    WriteStlFileFilter filter;
    // auto exemplarFilePath = fs::path(fmt::format("{}/exemplar.dream3d", k_ExemplarDir));
    // DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(WriteStlFileFilter::k_GroupingType_Key, std::make_any<ChoicesParameter::ValueType>(3));
    args.insertOrAssign(WriteStlFileFilter::k_OutputStlDirectory_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(std::string(unit_test::k_BinaryTestOutputDir))));
    args.insertOrAssign(WriteStlFileFilter::k_OutputStlPrefix_Key, std::make_any<StringParameter::ValueType>("Part_Number_"));
    args.insertOrAssign(WriteStlFileFilter::k_TriangleGeomPath_Key, std::make_any<DataPath>(k_ComputedTriangleDataContainerName));
    args.insertOrAssign(WriteStlFileFilter::k_PartNumberPath_Key, std::make_any<DataPath>(k_ComputedTriangleDataContainerName.createChildPath(k_FaceData).createChildPath(k_PartNumberName)));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  fs::path writtenFilePath = fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/Part_Number_1.stl");
  REQUIRE(fs::exists(writtenFilePath));
  auto fileContents = readIn(writtenFilePath);
  std::string md5Hash = nx::core::UnitTest::ComputeMD5Hash(fileContents);
  REQUIRE(md5Hash == "a0383b898d0668d70f08e135e5064efb");

  writtenFilePath = fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/Part_Number_2.stl");
  REQUIRE(fs::exists(writtenFilePath));
  fileContents = readIn(writtenFilePath);
  md5Hash = nx::core::UnitTest::ComputeMD5Hash(fileContents);
  REQUIRE(md5Hash == "d45a0d99495df506384fdbbb46a79f5c");
}
