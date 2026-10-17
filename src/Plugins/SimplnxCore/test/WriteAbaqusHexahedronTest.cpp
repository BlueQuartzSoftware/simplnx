#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <algorithm>
#include <catch2/catch.hpp>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>

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

/**
 * @brief Compares all five output files against the legacy exemplar.
 * @param exemplarDir Directory that contains the approved exemplar files.
 */
void CompareResults(const std::string& exemplarDir)
{
  const fs::path outputDir(unit_test::k_BinaryTestOutputDir.view());
  const std::string prefix = "Abaqus_Hexahedron_Writer_Test";
  for(const auto* suffix : {".inp", "_elems.inp", "_elset.inp", "_sects.inp", "_nodes.inp"})
  {
    const fs::path writtenPath = outputDir / (prefix + suffix);
    const fs::path exemplarPath = fs::path(exemplarDir) / (prefix + suffix);
    REQUIRE(fs::exists(writtenPath));
    REQUIRE(fs::exists(exemplarPath));
    const auto writtenBytes = readIn(writtenPath);
    const auto exemplarBytes = readIn(exemplarPath);
    const std::string written(writtenBytes.begin(), writtenBytes.end());
    std::string expected(exemplarBytes.begin(), exemplarBytes.end());
    if(std::string(suffix) == "_sects.inp")
    {
      // The exemplar predates the integration-type option; standard C3D8 output omits hourglass records.
      const std::string record = "*Hourglass Stiffness\n250\n";
      usize position = 0;
      while((position = expected.find(record, position)) != std::string::npos)
      {
        expected.erase(position, record.size());
      }
    }
    if(std::string(suffix) != "_nodes.inp")
    {
      REQUIRE(written == expected);
      continue;
    }
    // The exemplar uses legacy fixed 6-decimal formatting; the writer now prints 9 significant digits.
    std::istringstream writtenLines(written);
    std::istringstream expectedLines(expected);
    std::string writtenLine;
    std::string expectedLine;
    while(std::getline(expectedLines, expectedLine))
    {
      REQUIRE(static_cast<bool>(std::getline(writtenLines, writtenLine)));
      if(expectedLine.empty() || expectedLine.front() < '0' || expectedLine.front() > '9')
      {
        REQUIRE(writtenLine == expectedLine);
        continue;
      }
      std::istringstream actualNode(writtenLine);
      std::istringstream expectedNode(expectedLine);
      std::string actualId;
      std::string expectedId;
      REQUIRE(static_cast<bool>(std::getline(actualNode, actualId, ',')));
      REQUIRE(static_cast<bool>(std::getline(expectedNode, expectedId, ',')));
      REQUIRE(actualId == expectedId);
      for(usize component = 0; component < 3; component++)
      {
        double actualCoordinate = 0;
        double expectedCoordinate = 0;
        REQUIRE(static_cast<bool>(actualNode >> actualCoordinate));
        REQUIRE(static_cast<bool>(expectedNode >> expectedCoordinate));
        REQUIRE(std::abs(actualCoordinate - expectedCoordinate) <= 1.0e-6);
        if(component < 2)
        {
          char actualComma = 0;
          char expectedComma = 0;
          REQUIRE(static_cast<bool>(actualNode >> actualComma));
          REQUIRE(static_cast<bool>(expectedNode >> expectedComma));
          REQUIRE(actualComma == ',');
          REQUIRE(expectedComma == ',');
        }
      }
      actualNode >> std::ws;
      expectedNode >> std::ws;
      REQUIRE(actualNode.eof());
      REQUIRE(expectedNode.eof());
    }
    REQUIRE_FALSE(static_cast<bool>(std::getline(writtenLines, writtenLine)));
  }
}
} // namespace

TEST_CASE("SimplnxCore::WriteAbaqusHexahedronFilter: Missing Output Directory", "[SimplnxCore][WriteAbaqusHexahedronFilter]")
{
  UnitTest::LoadPlugins();

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

  DataStructure dataStructure = CreateIntegrationTypeTestDataStructure();
  const auto uniqueSuffix = std::chrono::high_resolution_clock::now().time_since_epoch().count();
  const fs::path missingPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / fmt::format("WriteAbaqusHexahedron_Missing_{}", uniqueSuffix) / "Nested";
  REQUIRE_FALSE(fs::exists(missingPath.parent_path()));
  REQUIRE_FALSE(fs::exists(missingPath));

  const WriteAbaqusHexahedronFilter filter;
  const std::string prefix = "Missing_Output_Directory";
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(missingPath));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_FilePrefix_Key, std::make_any<StringParameter::ValueType>(prefix));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_TestImageGeometryPath));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_TestFeatureIdsPath));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(std::count_if(preflightResult.outputActions.warnings().cbegin(), preflightResult.outputActions.warnings().cend(), [](const Warning& warning) { return warning.code == -17; }) == 1);
  REQUIRE_FALSE(fs::exists(missingPath.parent_path()));

  auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE(fs::is_directory(missingPath));
  for(const auto* suffix : {"_nodes.inp", "_elems.inp", "_sects.inp", "_elset.inp", ".inp"})
  {
    REQUIRE(fs::is_regular_file(missingPath / (prefix + suffix)));
  }

  fs::remove_all(missingPath.parent_path());
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAbaqusHexahedronFilter: Output Path Is A File", "[SimplnxCore][WriteAbaqusHexahedronFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateIntegrationTypeTestDataStructure();
  const auto uniqueSuffix = std::chrono::high_resolution_clock::now().time_since_epoch().count();
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / fmt::format("WriteAbaqusHexahedron_File_{}", uniqueSuffix);
  fs::create_directories(outputPath.parent_path());
  REQUIRE_FALSE(fs::exists(outputPath));
  {
    std::ofstream output(outputPath);
    REQUIRE(output.is_open());
    output << "Existing regular file";
  }
  REQUIRE(fs::is_regular_file(outputPath));

  const WriteAbaqusHexahedronFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(outputPath));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_FilePrefix_Key, std::make_any<StringParameter::ValueType>("Output_Path_Is_A_File"));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_TestImageGeometryPath));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_TestFeatureIdsPath));
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.invalid());
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors().front().code == -1112);

  fs::remove(outputPath);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAbaqusHexahedronFilter: Default Omits Dummy Node", "[SimplnxCore][WriteAbaqusHexahedronFilter]")
{
  UnitTest::LoadPlugins();
  const Arguments args = WriteAbaqusHexahedronFilter().getDefaultArguments();
  REQUIRE_FALSE(args.value<bool>(WriteAbaqusHexahedronFilter::k_WriteDummyNode_Key));
}

TEST_CASE("SimplnxCore::WriteAbaqusHexahedronFilter: Integration Type", "[SimplnxCore][WriteAbaqusHexahedronFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
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

    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
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

    auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
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

  ::CompareResults(fmt::format("{}/7_0_abaqus_hexahedron_writer_test/dummy_node", unit_test::k_TestFilesDir));

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

  ::CompareResults(fmt::format("{}/7_0_abaqus_hexahedron_writer_test/raw", unit_test::k_TestFilesDir));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAbaqusHexahedronFilter: SIMPL Backwards Compatibility", "[SimplnxCore][WriteAbaqusHexahedronFilter][BackwardsCompatibility]")
{
  UnitTest::LoadPlugins();
  auto app = Application::GetOrCreateInstance();
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
      CHECK(args.value<bool>(WriteAbaqusHexahedronFilter::k_WriteDummyNode_Key));
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

TEST_CASE("SimplnxCore::WriteAbaqusHexahedronFilter: Label And Feature Limits", "[SimplnxCore][WriteAbaqusHexahedronFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreateIntegrationTypeTestDataStructure();
  const WriteAbaqusHexahedronFilter filter;
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteAbaqusHexahedron" / "Limits";
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(outputPath));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_FilePrefix_Key, std::make_any<std::string>("Limits"));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_TestImageGeometryPath));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_TestFeatureIdsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(k_TestImageGeometryPath));
  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_TestImageGeometryPath);
  SECTION("Element count exceeds the label limit")
  {
    imageGeom.setDimensions({1000, 1000, 1000});
    auto result = filter.preflight(dataStructure, args);
    REQUIRE(result.outputActions.invalid());
    REQUIRE(result.outputActions.errors().front().code == -1121);
    REQUIRE(result.outputActions.errors().front().message.find("element count") != std::string::npos);
    REQUIRE(result.outputActions.errors().front().message.find("999999999") != std::string::npos);
  }
  SECTION("Node count exceeds the limit while element count fits")
  {
    imageGeom.setDimensions({1000, 1000, 999});
    auto result = filter.preflight(dataStructure, args);
    REQUIRE(result.outputActions.invalid());
    REQUIRE(result.outputActions.errors().front().code == -1121);
    REQUIRE(result.outputActions.errors().front().message.find("node count") != std::string::npos);
  }
  SECTION("Dimension products cannot overflow the check")
  {
    imageGeom.setDimensions({std::numeric_limits<usize>::max(), 2, 2});
    auto result = filter.preflight(dataStructure, args);
    REQUIRE(result.outputActions.invalid());
    REQUIRE(result.outputActions.errors().front().code == -1121);
  }
  SECTION("Sparse enormous feature ID fails before table allocation")
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_TestFeatureIdsPath));
    dataStructure.getDataRefAs<Int32Array>(k_TestFeatureIdsPath)[0] = 100'000'001;
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto result = filter.execute(dataStructure, args);
    REQUIRE(result.result.invalid());
    REQUIRE(result.result.errors().front().code == -1120);
    REQUIRE(result.result.errors().front().message.find("Renumber") != std::string::npos);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAbaqusHexahedronFilter: Tiny Spacing", "[SimplnxCore][WriteAbaqusHexahedronFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);
  DataStructure dataStructure = CreateIntegrationTypeTestDataStructure();
  const WriteAbaqusHexahedronFilter filter;
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteAbaqusHexahedron" / "TinySpacing";
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(outputPath));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_FilePrefix_Key, std::make_any<std::string>("Limits"));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_TestImageGeometryPath));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_TestFeatureIdsPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(k_TestImageGeometryPath));
  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_TestImageGeometryPath);
  imageGeom.setSpacing({1.0e-7F, 1.0e-7F, 1.0e-7F});
  auto result = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  std::ifstream nodes(outputPath / "Limits_nodes.inp");
  REQUIRE(nodes.is_open());
  std::string line;
  bool found = false;
  while(std::getline(nodes, line))
  {
    if(line.starts_with("2, "))
    {
      const double x = std::stod(line.substr(3));
      REQUIRE(std::abs(x - 1.0e-7) < 1.0e-14);
      found = true;
    }
  }
  REQUIRE(found);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAbaqusHexahedronFilter: Invalid Output File Prefix", "[SimplnxCore][WriteAbaqusHexahedronFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreateIntegrationTypeTestDataStructure();
  const WriteAbaqusHexahedronFilter filter;
  const fs::path outputPath(unit_test::k_BinaryTestOutputDir.view());
  for(const std::string prefix : {"", "my prefix", "../x", "/abs/x", "a\tb", ".", "..", "a\\b", "a\nb", "a\rb", "a\vb", "a\fb"})
  {
    DYNAMIC_SECTION("Prefix '" << prefix << "'")
    {
      Arguments args = filter.getDefaultArguments();
      args.insertOrAssign(WriteAbaqusHexahedronFilter::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(outputPath));
      args.insertOrAssign(WriteAbaqusHexahedronFilter::k_FilePrefix_Key, std::make_any<StringParameter::ValueType>(prefix));
      args.insertOrAssign(WriteAbaqusHexahedronFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_TestImageGeometryPath));
      args.insertOrAssign(WriteAbaqusHexahedronFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_TestFeatureIdsPath));
      const auto result = filter.preflight(dataStructure, args);
      REQUIRE(result.outputActions.invalid());
      REQUIRE(result.outputActions.errors().size() == 1);
      REQUIRE(result.outputActions.errors().front().code == -1122);
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAbaqusHexahedronFilter: Output Target Is A Directory", "[SimplnxCore][WriteAbaqusHexahedronFilter]")
{
  UnitTest::LoadPlugins();
  DataStructure dataStructure = CreateIntegrationTypeTestDataStructure();
  const WriteAbaqusHexahedronFilter filter;
  const auto uniqueSuffix = std::chrono::high_resolution_clock::now().time_since_epoch().count();
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / fmt::format("WriteAbaqusHexahedron_TargetDirectory_{}", uniqueSuffix);
  REQUIRE_FALSE(fs::exists(outputPath));
  const std::string prefix = "Target_Directory";
  for(const auto* suffix : {"_nodes.inp", "_elems.inp", "_sects.inp", "_elset.inp", ".inp"})
  {
    DYNAMIC_SECTION("Target suffix " << suffix)
    {
      const fs::path targetPath = outputPath / (prefix + suffix);
      REQUIRE(fs::create_directories(targetPath));
      Arguments args = filter.getDefaultArguments();
      args.insertOrAssign(WriteAbaqusHexahedronFilter::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(outputPath));
      args.insertOrAssign(WriteAbaqusHexahedronFilter::k_FilePrefix_Key, std::make_any<StringParameter::ValueType>(prefix));
      args.insertOrAssign(WriteAbaqusHexahedronFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_TestImageGeometryPath));
      args.insertOrAssign(WriteAbaqusHexahedronFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_TestFeatureIdsPath));
      const auto result = filter.preflight(dataStructure, args);
      fs::remove_all(outputPath);
      REQUIRE(result.outputActions.invalid());
      REQUIRE(result.outputActions.errors().size() == 1);
      REQUIRE(result.outputActions.errors().front().code == -1123);
      REQUIRE(result.outputActions.errors().front().message.find(targetPath.string()) != std::string::npos);
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
