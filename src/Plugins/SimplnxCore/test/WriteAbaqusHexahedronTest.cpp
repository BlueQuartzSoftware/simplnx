#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

#include "SimplnxCore/Filters/WriteAbaqusHexahedronFilter.hpp"

#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const DataPath k_FeatureIdsPath = DataPath({Constants::k_DataContainer}).createChildPath(Constants::k_EbsdScanData).createChildPath(Constants::k_FeatureIds);

std::vector<char> readIn(const fs::path& filePath)
{
  std::ifstream file(filePath.string(), std::ios_base::binary);

  if(file)
  {
    // get file size
    file.seekg(0, std::ios::end);
    const std::streampos length = file.tellg();
    file.seekg(0, std::ios::beg);

    // read whole file into a vector
    std::vector<char> contents(length); // act as a buffer
    file.read(contents.data(), length);

    // build string from psuedo-buffer
    return contents;
  }
  return {};
}

void CompareResults(const std::string& exemplarDir) // compare hash of both file strings
{
  const fs::path writtenFilePath = fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/Abaqus_Hexahedron_Writer_Test.inp");
  REQUIRE(fs::exists(writtenFilePath));
  const fs::path exemplarFilePath = fs::path(exemplarDir + "/Abaqus_Hexahedron_Writer_Test.inp");
  REQUIRE(fs::exists(exemplarFilePath));
  REQUIRE(readIn(writtenFilePath) == readIn(exemplarFilePath));
  const fs::path writtenFilePath2 = fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/Abaqus_Hexahedron_Writer_Test_elems.inp");
  REQUIRE(fs::exists(writtenFilePath2));
  const fs::path exemplarFilePath2 = fs::path(exemplarDir + "/Abaqus_Hexahedron_Writer_Test_elems.inp");
  REQUIRE(fs::exists(exemplarFilePath2));
  REQUIRE(readIn(writtenFilePath2) == readIn(exemplarFilePath2));
  const fs::path writtenFilePath3 = fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/Abaqus_Hexahedron_Writer_Test_elset.inp");
  REQUIRE(fs::exists(writtenFilePath3));
  const fs::path exemplarFilePath3 = fs::path(exemplarDir + "/Abaqus_Hexahedron_Writer_Test_elset.inp");
  REQUIRE(fs::exists(exemplarFilePath3));
  REQUIRE(readIn(writtenFilePath3) == readIn(exemplarFilePath3));
  const fs::path writtenFilePath4 = fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/Abaqus_Hexahedron_Writer_Test_nodes.inp");
  REQUIRE(fs::exists(writtenFilePath4));
  const fs::path exemplarFilePath4 = fs::path(exemplarDir + "/Abaqus_Hexahedron_Writer_Test_nodes.inp");
  REQUIRE(fs::exists(exemplarFilePath4));
  REQUIRE(readIn(writtenFilePath4) == readIn(exemplarFilePath4));
  const fs::path writtenFilePath5 = fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/Abaqus_Hexahedron_Writer_Test_sects.inp");
  REQUIRE(fs::exists(writtenFilePath5));
  const fs::path exemplarFilePath5 = fs::path(exemplarDir + "/Abaqus_Hexahedron_Writer_Test_sects.inp");
  REQUIRE(fs::exists(exemplarFilePath5));
  REQUIRE(readIn(writtenFilePath5) == readIn(exemplarFilePath5));
}
} // namespace

TEST_CASE("SimplnxCore::WriteAbaqusHexahedronFilter: Valid Dummy Node", "[SimplnxCore][WriteAbaqusHexahedronFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "7_0_abaqus_hexahedron_writer_test.tar.gz", "7_0_abaqus_hexahedron_writer_test");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_find_feature_centroids.tar.gz", "6_6_find_feature_centroids.dream3d");

  // Instantiate the filter, a DataStructure object and an Arguments Object
  const WriteAbaqusHexahedronFilter filter;
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/6_6_find_feature_centroids.dream3d", unit_test::k_TestFilesDir)));
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_WriteDummyNode_Key, std::make_any<bool>(true));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_HourglassStiffness_Key, std::make_any<int32>(250));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_JobName_Key, std::make_any<StringParameter::ValueType>("UnitTest"));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(std::string(unit_test::k_BinaryTestOutputDir))));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_FilePrefix_Key, std::make_any<StringParameter::ValueType>("Abaqus_Hexahedron_Writer_Test"));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  ::CompareResults(fmt::format("{}/7_0_abaqus_hexahedron_writer_test/dummy_node", unit_test::k_TestFilesDir));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAbaqusHexahedronFilter: No Dummy Node", "[SimplnxCore][WriteAbaqusHexahedronFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "7_0_abaqus_hexahedron_writer_test.tar.gz", "7_0_abaqus_hexahedron_writer_test");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_find_feature_centroids.tar.gz", "6_6_find_feature_centroids.dream3d");

  // Instantiate the filter, a DataStructure object and an Arguments Object
  const WriteAbaqusHexahedronFilter filter;
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/6_6_find_feature_centroids.dream3d", unit_test::k_TestFilesDir)));
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_WriteDummyNode_Key, std::make_any<bool>(false));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_HourglassStiffness_Key, std::make_any<int32>(250));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_JobName_Key, std::make_any<StringParameter::ValueType>("UnitTest"));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(std::string(unit_test::k_BinaryTestOutputDir))));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_FilePrefix_Key, std::make_any<StringParameter::ValueType>("Abaqus_Hexahedron_Writer_Test"));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  ::CompareResults(fmt::format("{}/7_0_abaqus_hexahedron_writer_test/raw", unit_test::k_TestFilesDir));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAbaqusHexahedronFilter: SIMPL Backwards Compatibility", "[SimplnxCore][WriteAbaqusHexahedronFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "WriteAbaqusHexahedronFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "WriteAbaqusHexahedronFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<WriteAbaqusHexahedronFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<int32>(WriteAbaqusHexahedronFilter::k_HourglassStiffness_Key) == 5);
      CHECK(args.value<std::string>(WriteAbaqusHexahedronFilter::k_JobName_Key) == "TestName");
      CHECK(args.value<FileSystemPathParameter::ValueType>(WriteAbaqusHexahedronFilter::k_OutputPath_Key) == fs::path("/test/path/file.txt"));
      CHECK(args.value<std::string>(WriteAbaqusHexahedronFilter::k_FilePrefix_Key) == "TestName");
      CHECK(args.value<DataPath>(WriteAbaqusHexahedronFilter::k_ImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(WriteAbaqusHexahedronFilter::k_FeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}
