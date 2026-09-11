#include "SimplnxCore/Filters/WriteAbaqusHexahedronFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const DataPath k_ImageGeometryPath({"Image Geometry"});
const DataPath k_CellDataPath = k_ImageGeometryPath.createChildPath("Cell Data");
const DataPath k_FeatureIdsPath = k_CellDataPath.createChildPath("FeatureIds");
const DataPath k_PhasesPath = k_CellDataPath.createChildPath("Phases");
const DataPath k_WrongPhasesPath({"Wrong Phases"});

DataStructure CreateDataStructure(const SizeVec3& dimensions, const std::vector<int32>& featureIds, const std::vector<int32>& phases, usize phaseTupleCount = 0)
{
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeometryPath.getTargetName());
  REQUIRE(imageGeom != nullptr);
  imageGeom->setDimensions(dimensions);
  imageGeom->setSpacing({0.5F, 0.5F, 0.5F});
  imageGeom->setOrigin({0.0F, 0.0F, 0.0F});

  const ShapeType cellShape = {dimensions[2], dimensions[1], dimensions[0]};
  auto* cellData = AttributeMatrix::Create(dataStructure, k_CellDataPath.getTargetName(), cellShape, imageGeom->getId());
  REQUIRE(cellData != nullptr);
  imageGeom->setCellData(*cellData);

  auto featureIdsStore = DataStoreUtilities::CreateDataStore<int32>(cellShape, {1}, IDataAction::Mode::Execute);
  auto* featureIdsArray = DataArray<int32>::Create(dataStructure, k_FeatureIdsPath.getTargetName(), featureIdsStore, cellData->getId());
  REQUIRE(featureIdsArray != nullptr);

  const ShapeType phaseShape = phaseTupleCount == 0 ? cellShape : ShapeType{phaseTupleCount};
  auto phasesStore = DataStoreUtilities::CreateDataStore<int32>(phaseShape, {1}, IDataAction::Mode::Execute);
  auto* phasesArray = phaseTupleCount == 0 ? DataArray<int32>::Create(dataStructure, k_PhasesPath.getTargetName(), phasesStore, cellData->getId()) :
                                             DataArray<int32>::Create(dataStructure, k_WrongPhasesPath.getTargetName(), phasesStore);
  REQUIRE(phasesArray != nullptr);

  REQUIRE(featureIds.size() == featureIdsArray->getNumberOfTuples());
  REQUIRE(phases.size() == phasesArray->getNumberOfTuples());
  std::copy(featureIds.cbegin(), featureIds.cend(), featureIdsArray->begin());
  std::copy(phases.cbegin(), phases.cend(), phasesArray->begin());

  return dataStructure;
}

Arguments CreateArguments(const fs::path& outputPath, const std::string& prefix, bool writeDummyNode = false)
{
  Arguments args = WriteAbaqusHexahedronFilter().getDefaultArguments();
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_WriteDummyNode_Key, std::make_any<bool>(writeDummyNode));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_HourglassStiffness_Key, std::make_any<int32>(417));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_JobName_Key, std::make_any<StringParameter::ValueType>("UnitTest"));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(outputPath));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_FilePrefix_Key, std::make_any<StringParameter::ValueType>(prefix));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeometryPath));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));
  return args;
}

std::string ReadFile(const fs::path& filePath)
{
  std::ifstream input(filePath, std::ios::binary);
  REQUIRE(input.is_open());
  return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
} // namespace

TEST_CASE("SimplnxCore::WriteAbaqusHexahedronFilter: Canonical Output", "[SimplnxCore][WriteAbaqusHexahedronFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateDataStructure({2, 2, 1}, {1, 1, 2, 2}, {1, 1, 2, 2});
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteAbaqusHexahedron" / "CanonicalOutput";
  fs::create_directories(outputPath);
  const std::string prefix = "Abaqus_Hex_Test";
  const WriteAbaqusHexahedronFilter filter;
  Arguments args = CreateArguments(outputPath, prefix);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const std::string expectedNodes = R"(*NODE, NSET=ALLNODES
1, 0.000, 0.000, 0.000
2, 0.500, 0.000, 0.000
3, 1.000, 0.000, 0.000
4, 0.000, 0.500, 0.000
5, 0.500, 0.500, 0.000
6, 1.000, 0.500, 0.000
7, 0.000, 1.000, 0.000
8, 0.500, 1.000, 0.000
9, 1.000, 1.000, 0.000
10, 0.000, 0.000, 0.500
11, 0.500, 0.000, 0.500
12, 1.000, 0.000, 0.500
13, 0.000, 0.500, 0.500
14, 0.500, 0.500, 0.500
15, 1.000, 0.500, 0.500
16, 0.000, 1.000, 0.500
17, 0.500, 1.000, 0.500
18, 1.000, 1.000, 0.500
)";

  const std::string expectedElements = R"(*ELEMENT, TYPE=C3D8R, ELSET=ALLELEMENTS
1, 1, 2, 5, 4, 10, 11, 14, 13
2, 2, 3, 6, 5, 11, 12, 15, 14
3, 4, 5, 8, 7, 13, 14, 17, 16
4, 5, 6, 9, 8, 14, 15, 18, 17
)";

  const std::string expectedElsets = R"(*Elset, elset=Grain1_Phase1_set
1, 2
*Elset, elset=Grain2_Phase2_set
3, 4
)";

  const std::string expectedSections = R"(*Solid Section, elset=Grain1_Phase1_set, material=Grain1_Phase1_mat
*Hourglass Stiffness
417
*Solid Section, elset=Grain2_Phase2_set, material=Grain2_Phase2_mat
*Hourglass Stiffness
417
)";

  const std::string expectedMaster = R"(*Heading
UnitTest
** Job name : UnitTest
*Preprint, echo = NO, model = NO, history = NO, contact = NO
**
*Include, Input = Abaqus_Hex_Test_nodes.inp
*Include, Input = Abaqus_Hex_Test_elems.inp
*Include, Input = Abaqus_Hex_Test_sects.inp
*Include, Input = Abaqus_Hex_Test_elset.inp
**
)";

  REQUIRE(ReadFile(outputPath / fmt::format("{}_nodes.inp", prefix)) == expectedNodes);
  REQUIRE(ReadFile(outputPath / fmt::format("{}_elems.inp", prefix)) == expectedElements);
  REQUIRE(ReadFile(outputPath / fmt::format("{}_elset.inp", prefix)) == expectedElsets);
  REQUIRE(ReadFile(outputPath / fmt::format("{}_sects.inp", prefix)) == expectedSections);
  REQUIRE(ReadFile(outputPath / fmt::format("{}.inp", prefix)) == expectedMaster);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAbaqusHexahedronFilter: Standard Integration", "[SimplnxCore][WriteAbaqusHexahedronFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateDataStructure({1, 1, 1}, {1}, {1});
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteAbaqusHexahedron" / "StandardIntegration";
  fs::create_directories(outputPath);
  const std::string prefix = "Standard_Integration";
  const WriteAbaqusHexahedronFilter filter;
  Arguments args = CreateArguments(outputPath, prefix);
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_UseReducedIntegration_Key, std::make_any<bool>(false));

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const std::string elements = ReadFile(outputPath / fmt::format("{}_elems.inp", prefix));
  REQUIRE(elements.starts_with("*ELEMENT, TYPE=C3D8, ELSET=ALLELEMENTS\n"));
  REQUIRE(elements.find("C3D8R") == std::string::npos);

  const std::string sections = ReadFile(outputPath / fmt::format("{}_sects.inp", prefix));
  REQUIRE(sections.find("*Hourglass Stiffness") == std::string::npos);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAbaqusHexahedronFilter: Dummy Node", "[SimplnxCore][WriteAbaqusHexahedronFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateDataStructure({1, 1, 1}, {1}, {1});
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteAbaqusHexahedron" / "DummyNode";
  fs::create_directories(outputPath);
  const std::string prefix = "Dummy_Node";
  const WriteAbaqusHexahedronFilter filter;
  Arguments args = CreateArguments(outputPath, prefix, true);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const std::string expectedNodes = R"(*NODE, NSET=ALLNODES
1, 0.000, 0.000, 0.000
2, 0.500, 0.000, 0.000
3, 0.000, 0.500, 0.000
4, 0.500, 0.500, 0.000
5, 0.000, 0.000, 0.500
6, 0.500, 0.000, 0.500
7, 0.000, 0.500, 0.500
8, 0.500, 0.500, 0.500
9, 0.000, 0.000, 0.000
)";
  REQUIRE(ReadFile(outputPath / fmt::format("{}_nodes.inp", prefix)) == expectedNodes);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAbaqusHexahedronFilter: Cell Phases Tuple Count Mismatch", "[SimplnxCore][WriteAbaqusHexahedronFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateDataStructure({2, 2, 1}, {1, 1, 2, 2}, {1, 2, 2}, 3);
  const WriteAbaqusHexahedronFilter filter;
  Arguments args = CreateArguments(fs::path(unit_test::k_BinaryTestOutputDir.view()), "Tuple_Count_Mismatch");
  args.insertOrAssign(WriteAbaqusHexahedronFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_WrongPhasesPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.invalid());
  REQUIRE(preflightResult.outputActions.errors()[0].message.find(k_WrongPhasesPath.toString()) != std::string::npos);
  REQUIRE(preflightResult.outputActions.errors()[0].message.find("3") != std::string::npos);
  REQUIRE(preflightResult.outputActions.errors()[0].message.find("4") != std::string::npos);

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
      REQUIRE(args.contains(WriteAbaqusHexahedronFilter::k_CellPhasesArrayPath_Key));
      CHECK(args.value<DataPath>(WriteAbaqusHexahedronFilter::k_CellPhasesArrayPath_Key).empty());
    }
  }
}
