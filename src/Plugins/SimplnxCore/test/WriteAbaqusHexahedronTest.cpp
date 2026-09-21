#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

#include "SimplnxCore/Filters/WriteAbaqusHexahedronFilter.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const DataPath k_FeatureIdsPath = DataPath({Constants::k_DataContainer}).createChildPath(Constants::k_EbsdScanData).createChildPath(Constants::k_FeatureIds);
const DataPath k_TestImageGeometryPath({"ImageGeometry"});
const DataPath k_TestCellDataPath = k_TestImageGeometryPath.createChildPath("CellData");
const DataPath k_TestFeatureIdsPath = k_TestCellDataPath.createChildPath("FeatureIds");

DataStructure CreateIntegrationTypeTestDataStructure()
{
  DataStructure dataStructure;

  auto* imageGeom = ImageGeom::Create(dataStructure, k_TestImageGeometryPath.getTargetName());
  imageGeom->setDimensions({1, 1, 1});
  imageGeom->setOrigin({0.0F, 0.0F, 0.0F});
  imageGeom->setSpacing({1.0F, 1.0F, 1.0F});

  const ShapeType cellShape = {1, 1, 1};
  auto* cellData = AttributeMatrix::Create(dataStructure, k_TestCellDataPath.getTargetName(), cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellData);

  auto featureIdsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_TestFeatureIdsPath, cellShape, {1});
  auto* featureIds = DataArray<int32>::Create(dataStructure, k_TestFeatureIdsPath.getTargetName(), featureIdsStore, cellData->getId());
  featureIds->getDataStoreRef()[0] = 1;

  return dataStructure;
}

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

void CompareUnchangedResults(const std::string& exemplarDir)
{
  const fs::path writtenFilePath = fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/Abaqus_Hexahedron_Writer_Test.inp");
  REQUIRE(fs::exists(writtenFilePath));
  const fs::path exemplarFilePath = fs::path(exemplarDir + "/Abaqus_Hexahedron_Writer_Test.inp");
  REQUIRE(fs::exists(exemplarFilePath));
  REQUIRE(readIn(writtenFilePath) == readIn(exemplarFilePath));
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
}
} // namespace

TEST_CASE("SimplnxCore::WriteAbaqusHexahedronFilter: Integration Type", "[SimplnxCore][WriteAbaqusHexahedronFilter]")
{
  const WriteAbaqusHexahedronFilter filter;
  const fs::path outputPath(unit_test::k_BinaryTestOutputDir.view());

  SECTION("Standard integration is the default")
  {
    DataStructure dataStructure = CreateIntegrationTypeTestDataStructure();
    Arguments args = filter.getDefaultArguments();
    args.insertOrAssign(WriteAbaqusHexahedronFilter::k_WriteDummyNode_Key, std::make_any<bool>(false));
    args.insertOrAssign(WriteAbaqusHexahedronFilter::k_HourglassStiffness_Key, std::make_any<int32>(417));
    args.insertOrAssign(WriteAbaqusHexahedronFilter::k_JobName_Key, std::make_any<StringParameter::ValueType>("Standard Integration"));
    args.insertOrAssign(WriteAbaqusHexahedronFilter::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(outputPath));
    args.insertOrAssign(WriteAbaqusHexahedronFilter::k_FilePrefix_Key, std::make_any<StringParameter::ValueType>("Abaqus_Standard_Integration_Test"));
    args.insertOrAssign(WriteAbaqusHexahedronFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_TestImageGeometryPath));
    args.insertOrAssign(WriteAbaqusHexahedronFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_TestFeatureIdsPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const std::vector<char> elemsBytes = readIn(outputPath / "Abaqus_Standard_Integration_Test_elems.inp");
    const std::string elems(elemsBytes.cbegin(), elemsBytes.cend());
    CHECK(elems.find("*Element, type=C3D8\n") != std::string::npos);

    const std::vector<char> sectionsBytes = readIn(outputPath / "Abaqus_Standard_Integration_Test_sects.inp");
    const std::string sections(sectionsBytes.cbegin(), sectionsBytes.cend());
    CHECK(sections.find("*Hourglass Stiffness") == std::string::npos);
    CHECK(sections.find("417") == std::string::npos);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  SECTION("Reduced integration uses hourglass stiffness")
  {
    DataStructure dataStructure = CreateIntegrationTypeTestDataStructure();
    Arguments args = filter.getDefaultArguments();
    args.insertOrAssign(WriteAbaqusHexahedronFilter::k_UseReducedIntegration_Key, std::make_any<bool>(true));
    args.insertOrAssign(WriteAbaqusHexahedronFilter::k_WriteDummyNode_Key, std::make_any<bool>(false));
    args.insertOrAssign(WriteAbaqusHexahedronFilter::k_HourglassStiffness_Key, std::make_any<int32>(417));
    args.insertOrAssign(WriteAbaqusHexahedronFilter::k_JobName_Key, std::make_any<StringParameter::ValueType>("Reduced Integration"));
    args.insertOrAssign(WriteAbaqusHexahedronFilter::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(outputPath));
    args.insertOrAssign(WriteAbaqusHexahedronFilter::k_FilePrefix_Key, std::make_any<StringParameter::ValueType>("Abaqus_Reduced_Integration_Test"));
    args.insertOrAssign(WriteAbaqusHexahedronFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_TestImageGeometryPath));
    args.insertOrAssign(WriteAbaqusHexahedronFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_TestFeatureIdsPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const std::vector<char> elemsBytes = readIn(outputPath / "Abaqus_Reduced_Integration_Test_elems.inp");
    const std::string elems(elemsBytes.cbegin(), elemsBytes.cend());
    CHECK(elems.find("*Element, type=C3D8R\n") != std::string::npos);

    const std::vector<char> sectionsBytes = readIn(outputPath / "Abaqus_Reduced_Integration_Test_sects.inp");
    const std::string sections(sectionsBytes.cbegin(), sectionsBytes.cend());
    CHECK(sections.find("*Hourglass Stiffness\n417\n") != std::string::npos);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::WriteAbaqusHexahedronFilter: Valid Dummy Node", "[SimplnxCore][WriteAbaqusHexahedronFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "7_0_abaqus_hexahedron_writer_test.tar.gz", "7_0_abaqus_hexahedron_writer_test");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_find_feature_centroids.tar.gz", "6_6_find_feature_centroids.dream3d");

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  const WriteAbaqusHexahedronFilter filter;
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/6_6_find_feature_centroids.dream3d", unit_test::k_TestFilesDir)));
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_WriteDummyNode_Key, std::make_any<bool>(true));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_UseReducedIntegration_Key, std::make_any<bool>(false));
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
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
  auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  ::CompareUnchangedResults(fmt::format("{}/7_0_abaqus_hexahedron_writer_test/dummy_node", unit_test::k_TestFilesDir));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAbaqusHexahedronFilter: No Dummy Node", "[SimplnxCore][WriteAbaqusHexahedronFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "7_0_abaqus_hexahedron_writer_test.tar.gz", "7_0_abaqus_hexahedron_writer_test");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_find_feature_centroids.tar.gz", "6_6_find_feature_centroids.dream3d");

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  const WriteAbaqusHexahedronFilter filter;
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/6_6_find_feature_centroids.dream3d", unit_test::k_TestFilesDir)));
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_WriteDummyNode_Key, std::make_any<bool>(false));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_UseReducedIntegration_Key, std::make_any<bool>(false));
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
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
  auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  ::CompareUnchangedResults(fmt::format("{}/7_0_abaqus_hexahedron_writer_test/raw", unit_test::k_TestFilesDir));

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
      CHECK_FALSE(args.value<bool>(WriteAbaqusHexahedronFilter::k_UseReducedIntegration_Key));
      CHECK(args.value<int32>(WriteAbaqusHexahedronFilter::k_HourglassStiffness_Key) == 5);
      CHECK(args.value<std::string>(WriteAbaqusHexahedronFilter::k_JobName_Key) == "TestName");
      CHECK(args.value<FileSystemPathParameter::ValueType>(WriteAbaqusHexahedronFilter::k_OutputPath_Key) == fs::path("/test/path/file.txt"));
      CHECK(args.value<std::string>(WriteAbaqusHexahedronFilter::k_FilePrefix_Key) == "TestName");
      CHECK(args.value<DataPath>(WriteAbaqusHexahedronFilter::k_ImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(WriteAbaqusHexahedronFilter::k_FeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}
