#include "SimplnxCore/Filters/WriteAbaqusCrystalPlasticityFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <numbers>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const DataPath k_ImageGeomPath({"Image Geometry"});
const DataPath k_CellDataPath = k_ImageGeomPath.createChildPath("Cell Data");
const DataPath k_FeatureIdsPath = k_CellDataPath.createChildPath("FeatureIds");
const DataPath k_EulerAnglesPath = k_CellDataPath.createChildPath("EulerAngles");
const DataPath k_PhasesPath = k_CellDataPath.createChildPath("Phases");
const DataPath k_WrongEulerAnglesPath({"Wrong EulerAngles"});

DataStructure CreateDataStructure(const SizeVec3& dimensions, const std::vector<int32>& featureIds, const std::vector<int32>& phases, const std::vector<float32>& eulerAngles,
                                  usize eulerTupleCount = 0)
{
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeomPath.getTargetName());
  REQUIRE(imageGeom != nullptr);
  imageGeom->setDimensions(dimensions);
  imageGeom->setSpacing({0.5F, 0.5F, 0.5F});
  imageGeom->setOrigin({0.0F, 0.0F, 0.0F});

  const ShapeType cellShape = {dimensions[2], dimensions[1], dimensions[0]};
  auto* cellData = AttributeMatrix::Create(dataStructure, k_CellDataPath.getTargetName(), cellShape, imageGeom->getId());
  REQUIRE(cellData != nullptr);
  imageGeom->setCellData(*cellData);

  auto featureIdsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_FeatureIdsPath, cellShape, {1});
  auto* featureIdsArray = DataArray<int32>::Create(dataStructure, k_FeatureIdsPath.getTargetName(), featureIdsStore, cellData->getId());
  REQUIRE(featureIdsArray != nullptr);

  auto phasesStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_PhasesPath, cellShape, {1});
  auto* phasesArray = DataArray<int32>::Create(dataStructure, k_PhasesPath.getTargetName(), phasesStore, cellData->getId());
  REQUIRE(phasesArray != nullptr);

  const ShapeType eulerShape = eulerTupleCount == 0 ? cellShape : ShapeType{eulerTupleCount};
  auto eulerAnglesStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, eulerTupleCount == 0 ? k_EulerAnglesPath : k_WrongEulerAnglesPath, eulerShape, {3});
  auto* eulerAnglesArray = eulerTupleCount == 0 ? DataArray<float32>::Create(dataStructure, k_EulerAnglesPath.getTargetName(), eulerAnglesStore, cellData->getId()) :
                                                  DataArray<float32>::Create(dataStructure, k_WrongEulerAnglesPath.getTargetName(), eulerAnglesStore);
  REQUIRE(eulerAnglesArray != nullptr);

  REQUIRE(featureIds.size() == featureIdsArray->getNumberOfTuples());
  REQUIRE(phases.size() == phasesArray->getNumberOfTuples());
  REQUIRE(eulerAngles.size() == eulerAnglesArray->getSize());

  std::copy(featureIds.cbegin(), featureIds.cend(), featureIdsArray->begin());
  std::copy(phases.cbegin(), phases.cend(), phasesArray->begin());
  std::copy(eulerAngles.cbegin(), eulerAngles.cend(), eulerAnglesArray->begin());

  return dataStructure;
}

Arguments CreateArguments(const fs::path& outputPath, const std::string& prefix, const DynamicTableParameter::ValueType& materialConstants = {{1.5}, {2.25}})
{
  Arguments args = WriteAbaqusCrystalPlasticityFilter().getDefaultArguments();
  args.insertOrAssign(WriteAbaqusCrystalPlasticityFilter::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(outputPath));
  args.insertOrAssign(WriteAbaqusCrystalPlasticityFilter::k_FilePrefix_Key, std::make_any<StringParameter::ValueType>(prefix));
  args.insertOrAssign(WriteAbaqusCrystalPlasticityFilter::k_JobName_Key, std::make_any<StringParameter::ValueType>("UnitTest"));
  args.insertOrAssign(WriteAbaqusCrystalPlasticityFilter::k_NumDepvar_Key, std::make_any<int32>(3));
  args.insertOrAssign(WriteAbaqusCrystalPlasticityFilter::k_NumUserOutVar_Key, std::make_any<int32>(2));
  args.insertOrAssign(WriteAbaqusCrystalPlasticityFilter::k_MaterialConstants_Key, std::make_any<DynamicTableParameter::ValueType>(materialConstants));
  args.insertOrAssign(WriteAbaqusCrystalPlasticityFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(WriteAbaqusCrystalPlasticityFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(WriteAbaqusCrystalPlasticityFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(k_EulerAnglesPath));
  args.insertOrAssign(WriteAbaqusCrystalPlasticityFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));
  return args;
}

std::string ReadFile(const fs::path& filePath)
{
  std::ifstream input(filePath, std::ios::binary);
  REQUIRE(input.is_open());
  return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

usize CountOccurrences(const std::string& text, const std::string& value)
{
  usize count = 0;
  usize position = 0;
  while((position = text.find(value, position)) != std::string::npos)
  {
    count++;
    position += value.size();
  }
  return count;
}
} // namespace

TEST_CASE("SimplnxCore::WriteAbaqusCrystalPlasticityFilter: Synthetic Two Grain", "[SimplnxCore][WriteAbaqusCrystalPlasticityFilter]")
{
  UnitTest::LoadPlugins();

  constexpr float32 k_Pi = std::numbers::pi_v<float32>;
  DataStructure dataStructure =
      CreateDataStructure({2, 2, 1}, {1, 1, 2, 2}, {1, 1, 2, 2}, {0.0F, k_Pi / 2.0F, k_Pi, 0.0F, k_Pi / 2.0F, k_Pi, k_Pi / 4.0F, k_Pi / 6.0F, k_Pi / 3.0F, k_Pi / 4.0F, k_Pi / 6.0F, k_Pi / 3.0F});

  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteAbaqusCrystalPlasticity" / "SyntheticTwoGrain";
  fs::create_directories(outputPath);
  const std::string prefix = "Abaqus_CP_Test";
  const WriteAbaqusCrystalPlasticityFilter filter;
  Arguments args = CreateArguments(outputPath, prefix);
  args.insertOrAssign(WriteAbaqusCrystalPlasticityFilter::k_UseReducedIntegration_Key, std::make_any<bool>(true));
  args.insertOrAssign(WriteAbaqusCrystalPlasticityFilter::k_HourglassStiffness_Key, std::make_any<int32>(417));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const std::string expectedNodes = R"(** ----------------------------------------------------------------
**
*Node
1, 0.000000, 0.000000, 0.000000
2, 0.500000, 0.000000, 0.000000
3, 1.000000, 0.000000, 0.000000
4, 0.000000, 0.500000, 0.000000
5, 0.500000, 0.500000, 0.000000
6, 1.000000, 0.500000, 0.000000
7, 0.000000, 1.000000, 0.000000
8, 0.500000, 1.000000, 0.000000
9, 1.000000, 1.000000, 0.000000
10, 0.000000, 0.000000, 0.500000
11, 0.500000, 0.000000, 0.500000
12, 1.000000, 0.000000, 0.500000
13, 0.000000, 0.500000, 0.500000
14, 0.500000, 0.500000, 0.500000
15, 1.000000, 0.500000, 0.500000
16, 0.000000, 1.000000, 0.500000
17, 0.500000, 1.000000, 0.500000
18, 1.000000, 1.000000, 0.500000
**
** ----------------------------------------------------------------
**
)";

  const std::string expectedElements = R"(** ----------------------------------------------------------------
**
*Element, type=C3D8R
1, 1, 2, 5, 4, 10, 11, 14, 13
2, 2, 3, 6, 5, 11, 12, 15, 14
3, 4, 5, 8, 7, 13, 14, 17, 16
4, 5, 6, 9, 8, 14, 15, 18, 17
**
** ----------------------------------------------------------------
**
)";

  const std::string expectedElsets = R"(** ----------------------------------------------------------------
**
** The element sets
*Elset, elset=cube, generate
1, 4, 1
**
** Each Grain is made up of multiple elements
**
*Elset, elset=Grain1_Phase1_set
1, 2
*Elset, elset=Grain2_Phase2_set
3, 4
**
** ----------------------------------------------------------------
**
)";

  const std::string expectedMaster = R"(*Heading
UnitTest
** Job name : UnitTest
*Preprint, echo = NO, model = NO, history = NO, contact = NO
**
** ----------------------------Geometry----------------------------
**
*Include, Input = Abaqus_CP_Test_nodes.inp
*Include, Input = Abaqus_CP_Test_elems.inp
*Include, Input = Abaqus_CP_Test_elset.inp
*Include, Input = Abaqus_CP_Test_sects.inp
**
** ----------------------------Materials---------------------------
**
*Material, name=Grain1_Phase1_mat
*Depvar
3
*User Material, constants=7, unsymm
1, 1, 0, 1.5707964, 3.1415927, 1.5, 2.25
*User Output Variables
2
*Material, name=Grain2_Phase2_mat
*Depvar
3
*User Material, constants=7, unsymm
2, 2, 0.7853982, 0.5235988, 1.0471976, 1.5, 2.25
*User Output Variables
2
**
** ----------------------------------------------------------------
**
)";

  const std::string expectedSections = R"(** ----------------------------------------------------------------
**
** Each section is a separate grain
** Section: Grain1_Phase1
*Solid Section, elset=Grain1_Phase1_set, material=Grain1_Phase1_mat
*Hourglass Stiffness
417
** --------------------------------------
** Section: Grain2_Phase2
*Solid Section, elset=Grain2_Phase2_set, material=Grain2_Phase2_mat
*Hourglass Stiffness
417
** --------------------------------------
**
** ----------------------------------------------------------------
**
)";

  REQUIRE(ReadFile(outputPath / fmt::format("{}_nodes.inp", prefix)) == expectedNodes);
  REQUIRE(ReadFile(outputPath / fmt::format("{}_elems.inp", prefix)) == expectedElements);
  REQUIRE(ReadFile(outputPath / fmt::format("{}_elset.inp", prefix)) == expectedElsets);
  REQUIRE(ReadFile(outputPath / fmt::format("{}.inp", prefix)) == expectedMaster);
  REQUIRE(ReadFile(outputPath / fmt::format("{}_sects.inp", prefix)) == expectedSections);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAbaqusCrystalPlasticityFilter: Euler Angles In Degrees", "[SimplnxCore][WriteAbaqusCrystalPlasticityFilter]")
{
  UnitTest::LoadPlugins();

  constexpr float32 k_Pi = std::numbers::pi_v<float32>;
  DataStructure dataStructure = CreateDataStructure({1, 1, 1}, {1}, {1}, {k_Pi / 4.0F, k_Pi / 6.0F, k_Pi / 3.0F});
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteAbaqusCrystalPlasticity" / "EulerDegrees";
  fs::create_directories(outputPath);
  const std::string prefix = "Euler_Degrees";
  const WriteAbaqusCrystalPlasticityFilter filter;
  Arguments args = CreateArguments(outputPath, prefix);
  args.insertOrAssign(WriteAbaqusCrystalPlasticityFilter::k_WriteEulerAnglesInDegrees_Key, std::make_any<bool>(true));

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const std::string master = ReadFile(outputPath / fmt::format("{}.inp", prefix));
  REQUIRE(master.find("1, 1, 45, 30, 60, 1.5, 2.25\n") != std::string::npos);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAbaqusCrystalPlasticityFilter: Default Integration Is Standard", "[SimplnxCore][WriteAbaqusCrystalPlasticityFilter]")
{
  const Arguments args = WriteAbaqusCrystalPlasticityFilter().getDefaultArguments();
  REQUIRE_FALSE(args.value<bool>(WriteAbaqusCrystalPlasticityFilter::k_UseReducedIntegration_Key));
}

TEST_CASE("SimplnxCore::WriteAbaqusCrystalPlasticityFilter: Standard Integration", "[SimplnxCore][WriteAbaqusCrystalPlasticityFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateDataStructure({1, 1, 1}, {1}, {1}, std::vector<float32>(3, 0.0F));
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteAbaqusCrystalPlasticity" / "StandardIntegration";
  fs::create_directories(outputPath);
  const std::string prefix = "Standard_Integration";
  const WriteAbaqusCrystalPlasticityFilter filter;
  Arguments args = CreateArguments(outputPath, prefix);
  args.insertOrAssign(WriteAbaqusCrystalPlasticityFilter::k_UseReducedIntegration_Key, std::make_any<bool>(false));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const std::string elements = ReadFile(outputPath / fmt::format("{}_elems.inp", prefix));
  REQUIRE(elements.find("*Element, type=C3D8\n") != std::string::npos);
  REQUIRE(elements.find("C3D8R") == std::string::npos);

  const std::string sections = ReadFile(outputPath / fmt::format("{}_sects.inp", prefix));
  REQUIRE(sections.find("*Hourglass Stiffness") == std::string::npos);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAbaqusCrystalPlasticityFilter: Wrapping Rules", "[SimplnxCore][WriteAbaqusCrystalPlasticityFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateDataStructure({4, 4, 2}, std::vector<int32>(32, 1), std::vector<int32>(32, 1), std::vector<float32>(96, 0.0F));
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteAbaqusCrystalPlasticity" / "WrappingRules";
  fs::create_directories(outputPath);
  const std::string prefix = "Wrapping_Rules";
  const WriteAbaqusCrystalPlasticityFilter filter;
  Arguments args = CreateArguments(outputPath, prefix, {{1.0}, {2.0}, {3.0}, {4.0}, {5.0}});
  args.insertOrAssign(WriteAbaqusCrystalPlasticityFilter::k_NumDepvar_Key, std::make_any<int32>(2));
  args.insertOrAssign(WriteAbaqusCrystalPlasticityFilter::k_NumUserOutVar_Key, std::make_any<int32>(1));

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const std::string expectedElsets = R"(** ----------------------------------------------------------------
**
** The element sets
*Elset, elset=cube, generate
1, 32, 1
**
** Each Grain is made up of multiple elements
**
*Elset, elset=Grain1_Phase1_set
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32
**
** ----------------------------------------------------------------
**
)";
  REQUIRE(ReadFile(outputPath / fmt::format("{}_elset.inp", prefix)) == expectedElsets);

  const std::string expectedMaster = R"(*Heading
UnitTest
** Job name : UnitTest
*Preprint, echo = NO, model = NO, history = NO, contact = NO
**
** ----------------------------Geometry----------------------------
**
*Include, Input = Wrapping_Rules_nodes.inp
*Include, Input = Wrapping_Rules_elems.inp
*Include, Input = Wrapping_Rules_elset.inp
*Include, Input = Wrapping_Rules_sects.inp
**
** ----------------------------Materials---------------------------
**
*Material, name=Grain1_Phase1_mat
*Depvar
2
*User Material, constants=10, unsymm
1, 1, 0, 0, 0, 1, 2, 3
4, 5
*User Output Variables
1
**
** ----------------------------------------------------------------
**
)";
  REQUIRE(ReadFile(outputPath / fmt::format("{}.inp", prefix)) == expectedMaster);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAbaqusCrystalPlasticityFilter: Material Constant Precision", "[SimplnxCore][WriteAbaqusCrystalPlasticityFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateDataStructure({1, 1, 1}, {1}, {1}, std::vector<float32>(3, 0.0F));
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteAbaqusCrystalPlasticity" / "ConstantPrecision";
  fs::create_directories(outputPath);
  const std::string prefix = "Constant_Precision";
  const WriteAbaqusCrystalPlasticityFilter filter;
  Arguments args = CreateArguments(outputPath, prefix, {{1.0e-5}, {2.1e11}, {0.123456789012}});

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const std::string master = ReadFile(outputPath / fmt::format("{}.inp", prefix));
  // Eight values per line: grain, phase, three angles, then the three constants.
  REQUIRE(master.find("1, 1, 0, 0, 0, 1e-05, 210000000000, 0.123456789012\n") != std::string::npos);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAbaqusCrystalPlasticityFilter: Custom Constant Layout", "[SimplnxCore][WriteAbaqusCrystalPlasticityFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateDataStructure({1, 1, 1}, {1}, {1}, {0.25F, 0.5F, 0.75F});
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteAbaqusCrystalPlasticity" / "CustomLayout";
  fs::create_directories(outputPath);
  const WriteAbaqusCrystalPlasticityFilter filter;

  SECTION("Euler angles at 17-19 without grain and phase IDs")
  {
    DynamicTableParameter::ValueType constants;
    for(int32 i = 1; i <= 17; i++)
    {
      constants.push_back({static_cast<float64>(i * 10)});
    }
    const std::string prefix = "Layout_17";
    Arguments args = CreateArguments(outputPath, prefix, constants);
    args.insertOrAssign(WriteAbaqusCrystalPlasticityFilter::k_IncludeGrainAndPhaseIds_Key, std::make_any<bool>(false));
    args.insertOrAssign(WriteAbaqusCrystalPlasticityFilter::k_EulerAnglesStartIndex_Key, std::make_any<int32>(17));
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const std::string master = ReadFile(outputPath / fmt::format("{}.inp", prefix));
    REQUIRE(master.find("*User Material, constants=20, unsymm\n"
                        "10, 20, 30, 40, 50, 60, 70, 80\n"
                        "90, 100, 110, 120, 130, 140, 150, 160\n"
                        "0.25, 0.5, 0.75, 170\n") != std::string::npos);
  }

  SECTION("Euler angles first without grain and phase IDs")
  {
    const std::string prefix = "Layout_EulerOnly";
    Arguments args = CreateArguments(outputPath, prefix, {{7.5}});
    args.insertOrAssign(WriteAbaqusCrystalPlasticityFilter::k_IncludeGrainAndPhaseIds_Key, std::make_any<bool>(false));
    args.insertOrAssign(WriteAbaqusCrystalPlasticityFilter::k_EulerAnglesStartIndex_Key, std::make_any<int32>(1));
    args.insertOrAssign(WriteAbaqusCrystalPlasticityFilter::k_UseUnsymmetricSolver_Key, std::make_any<bool>(false));
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const std::string master = ReadFile(outputPath / fmt::format("{}.inp", prefix));
    REQUIRE(master.find("*User Material, constants=4\n0.25, 0.5, 0.75, 7.5\n") != std::string::npos);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAbaqusCrystalPlasticityFilter: Euler Start Index Out Of Range", "[SimplnxCore][WriteAbaqusCrystalPlasticityFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateDataStructure({1, 1, 1}, {1}, {1}, std::vector<float32>(3, 0.0F));
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteAbaqusCrystalPlasticity" / "StartIndexRange";
  fs::create_directories(outputPath);
  const WriteAbaqusCrystalPlasticityFilter filter;

  // Two user constants with IDs included: valid range is 3..5.
  const std::vector<int32> badIndices = {0, 2, 6};
  for(const int32 badIndex : badIndices)
  {
    DYNAMIC_SECTION("Start index " << badIndex)
    {
      Arguments args = CreateArguments(outputPath, "Start_Index_Range");
      args.insertOrAssign(WriteAbaqusCrystalPlasticityFilter::k_EulerAnglesStartIndex_Key, std::make_any<int32>(badIndex));
      auto preflightResult = filter.preflight(dataStructure, args);
      REQUIRE(preflightResult.outputActions.errors().size() == 1);
      REQUIRE(preflightResult.outputActions.errors()[0].code == -12017);
    }
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAbaqusCrystalPlasticityFilter: Gap In Feature Ids", "[SimplnxCore][WriteAbaqusCrystalPlasticityFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateDataStructure({2, 2, 1}, {1, 1, 5, 5}, {1, 1, 2, 2}, std::vector<float32>(12, 0.0F));
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteAbaqusCrystalPlasticity" / "GapInFeatureIds";
  fs::create_directories(outputPath);
  const std::string prefix = "Gap_In_Feature_Ids";
  const WriteAbaqusCrystalPlasticityFilter filter;
  Arguments args = CreateArguments(outputPath, prefix);
  bool warningEmitted = false;
  const IFilter::MessageHandler messageHandler{[&warningEmitted](const IFilter::Message& message) {
    if(message.type == IFilter::Message::Type::Warning && message.message == "3 feature ids in [1, 5] have no cells. Empty element sets and materials with zero orientation were written for them.")
    {
      warningEmitted = true;
    }
  }};

  auto executeResult = filter.execute(dataStructure, args, nullptr, messageHandler);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE(warningEmitted);

  const std::string expectedElsets = R"(** ----------------------------------------------------------------
**
** The element sets
*Elset, elset=cube, generate
1, 4, 1
**
** Each Grain is made up of multiple elements
**
*Elset, elset=Grain1_Phase1_set
1, 2
*Elset, elset=Grain2_Phase0_set

*Elset, elset=Grain3_Phase0_set

*Elset, elset=Grain4_Phase0_set

*Elset, elset=Grain5_Phase2_set
3, 4
**
** ----------------------------------------------------------------
**
)";
  REQUIRE(ReadFile(outputPath / fmt::format("{}_elset.inp", prefix)) == expectedElsets);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAbaqusCrystalPlasticityFilter: No Positive Feature Ids", "[SimplnxCore][WriteAbaqusCrystalPlasticityFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateDataStructure({2, 2, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}, std::vector<float32>(12, 0.0F));
  const auto uniqueSuffix = std::chrono::high_resolution_clock::now().time_since_epoch().count();
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / fmt::format("WriteAbaqusCrystalPlasticity_NoGrains_{}", uniqueSuffix);
  fs::create_directories(outputPath);
  const std::string prefix = "No_Positive_Feature_Ids";
  const WriteAbaqusCrystalPlasticityFilter filter;
  Arguments args = CreateArguments(outputPath, prefix);

  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.invalid());
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors()[0].code == -12011);
  REQUIRE(executeResult.result.errors()[0].message.find(k_FeatureIdsPath.toString()) != std::string::npos);

  const std::vector<fs::path> outputFiles = {outputPath / fmt::format("{}_nodes.inp", prefix), outputPath / fmt::format("{}_elems.inp", prefix), outputPath / fmt::format("{}_sects.inp", prefix),
                                             outputPath / fmt::format("{}_elset.inp", prefix), outputPath / fmt::format("{}.inp", prefix)};
  for(const auto& outputFile : outputFiles)
  {
    REQUIRE_FALSE(fs::exists(outputFile));
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAbaqusCrystalPlasticityFilter: Negative Depvar Fails Preflight", "[SimplnxCore][WriteAbaqusCrystalPlasticityFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateDataStructure({1, 1, 1}, {1}, {1}, std::vector<float32>(3, 0.0F));
  const WriteAbaqusCrystalPlasticityFilter filter;
  Arguments args = CreateArguments(fs::path(unit_test::k_BinaryTestOutputDir.view()), "Negative_Depvar");
  args.insertOrAssign(WriteAbaqusCrystalPlasticityFilter::k_NumDepvar_Key, std::make_any<int32>(-2));

  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.invalid());
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -12014);
  REQUIRE(preflightResult.outputActions.errors()[0].message.find("-2") != std::string::npos);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAbaqusCrystalPlasticityFilter: Negative User Output Variables Fails Preflight", "[SimplnxCore][WriteAbaqusCrystalPlasticityFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateDataStructure({1, 1, 1}, {1}, {1}, std::vector<float32>(3, 0.0F));
  const WriteAbaqusCrystalPlasticityFilter filter;
  Arguments args = CreateArguments(fs::path(unit_test::k_BinaryTestOutputDir.view()), "Negative_User_Output_Variables");
  args.insertOrAssign(WriteAbaqusCrystalPlasticityFilter::k_NumUserOutVar_Key, std::make_any<int32>(-3));

  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.invalid());
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -12015);
  REQUIRE(preflightResult.outputActions.errors()[0].message.find("-3") != std::string::npos);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAbaqusCrystalPlasticityFilter: Material Constants Row Width Fails Preflight", "[SimplnxCore][WriteAbaqusCrystalPlasticityFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateDataStructure({1, 1, 1}, {1}, {1}, std::vector<float32>(3, 0.0F));
  const WriteAbaqusCrystalPlasticityFilter filter;
  Arguments args = CreateArguments(fs::path(unit_test::k_BinaryTestOutputDir.view()), "Material_Constants_Row_Width", {{2.0, 3.0}});

  auto preflightResult = filter.preflight(dataStructure, args);
  // The single static table column rejects the two-column row before preflightImpl runs.
  REQUIRE(preflightResult.outputActions.invalid());
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -3);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAbaqusCrystalPlasticityFilter: Feature Id Zero Skipped", "[SimplnxCore][WriteAbaqusCrystalPlasticityFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateDataStructure({3, 1, 1}, {0, 1, 1}, {0, 1, 1}, std::vector<float32>(9, 0.0F));
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "WriteAbaqusCrystalPlasticity" / "FeatureIdZero";
  fs::create_directories(outputPath);
  const std::string prefix = "Feature_Id_Zero";
  const WriteAbaqusCrystalPlasticityFilter filter;
  Arguments args = CreateArguments(outputPath, prefix);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const std::string expectedElsets = R"(** ----------------------------------------------------------------
**
** The element sets
*Elset, elset=cube, generate
1, 3, 1
**
** Each Grain is made up of multiple elements
**
*Elset, elset=Grain1_Phase1_set
2, 3
**
** ----------------------------------------------------------------
**
)";
  REQUIRE(ReadFile(outputPath / fmt::format("{}_elset.inp", prefix)) == expectedElsets);

  const std::string master = ReadFile(outputPath / fmt::format("{}.inp", prefix));
  REQUIRE(CountOccurrences(master, "*Material, name=") == 1);
  REQUIRE(master.find("Grain1_Phase1_mat") != std::string::npos);
  REQUIRE(master.find("Grain2_") == std::string::npos);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAbaqusCrystalPlasticityFilter: Tuple Count Mismatch", "[SimplnxCore][WriteAbaqusCrystalPlasticityFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateDataStructure({2, 2, 1}, {1, 1, 2, 2}, {1, 1, 2, 2}, std::vector<float32>(9, 0.0F), 3);
  const WriteAbaqusCrystalPlasticityFilter filter;
  Arguments args = CreateArguments(fs::path(unit_test::k_BinaryTestOutputDir.view()), "Tuple_Count_Mismatch");
  args.insertOrAssign(WriteAbaqusCrystalPlasticityFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(k_WrongEulerAnglesPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.invalid());

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAbaqusCrystalPlasticityFilter: Missing Output Directory", "[SimplnxCore][WriteAbaqusCrystalPlasticityFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateDataStructure({1, 1, 1}, {1}, {1}, std::vector<float32>(3, 0.0F));
  const auto uniqueSuffix = std::chrono::high_resolution_clock::now().time_since_epoch().count();
  const fs::path missingPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / fmt::format("WriteAbaqusCrystalPlasticity_Missing_{}", uniqueSuffix) / "Nested";
  REQUIRE_FALSE(fs::exists(missingPath.parent_path()));
  REQUIRE_FALSE(fs::exists(missingPath));

  const WriteAbaqusCrystalPlasticityFilter filter;
  const std::string prefix = "Missing_Output_Directory";
  Arguments args = CreateArguments(missingPath, prefix);
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(std::count_if(preflightResult.outputActions.warnings().cbegin(), preflightResult.outputActions.warnings().cend(), [](const Warning& warning) { return warning.code == -17; }) == 1);
  REQUIRE_FALSE(fs::exists(missingPath.parent_path()));

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE(fs::is_directory(missingPath));
  for(const auto* suffix : {"_nodes.inp", "_elems.inp", "_sects.inp", "_elset.inp", ".inp"})
  {
    REQUIRE(fs::is_regular_file(missingPath / (prefix + suffix)));
  }

  fs::remove_all(missingPath.parent_path());
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAbaqusCrystalPlasticityFilter: Output Path Is A File", "[SimplnxCore][WriteAbaqusCrystalPlasticityFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateDataStructure({1, 1, 1}, {1}, {1}, std::vector<float32>(3, 0.0F));
  const auto uniqueSuffix = std::chrono::high_resolution_clock::now().time_since_epoch().count();
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / fmt::format("WriteAbaqusCrystalPlasticity_File_{}", uniqueSuffix);
  fs::create_directories(outputPath.parent_path());
  REQUIRE_FALSE(fs::exists(outputPath));
  {
    std::ofstream output(outputPath);
    REQUIRE(output.is_open());
    output << "Existing regular file";
  }
  REQUIRE(fs::is_regular_file(outputPath));

  const WriteAbaqusCrystalPlasticityFilter filter;
  Arguments args = CreateArguments(outputPath, "Output_Path_Is_A_File");
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.invalid());
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors().front().code == -12001);

  fs::remove(outputPath);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAbaqusCrystalPlasticityFilter: SIMPL Backwards Compatibility", "[SimplnxCore][WriteAbaqusCrystalPlasticityFilter]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";
  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "WriteAbaqusCrystalPlasticityFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "WriteAbaqusCrystalPlasticityFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<WriteAbaqusCrystalPlasticityFilter>::uuid);
      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<bool>(WriteAbaqusCrystalPlasticityFilter::k_UseReducedIntegration_Key));
      CHECK(args.value<bool>(WriteAbaqusCrystalPlasticityFilter::k_WriteEulerAnglesInDegrees_Key));
      CHECK_FALSE(args.value<bool>(WriteAbaqusCrystalPlasticityFilter::k_UseUnsymmetricSolver_Key));
      CHECK(args.value<bool>(WriteAbaqusCrystalPlasticityFilter::k_IncludeGrainAndPhaseIds_Key));
      CHECK(args.value<int32>(WriteAbaqusCrystalPlasticityFilter::k_EulerAnglesStartIndex_Key) == 3);
      CHECK(args.value<int32>(WriteAbaqusCrystalPlasticityFilter::k_HourglassStiffness_Key) == 250);
      CHECK(args.value<FileSystemPathParameter::ValueType>(WriteAbaqusCrystalPlasticityFilter::k_OutputPath_Key) == fs::path("/test/path"));
      CHECK(args.value<std::string>(WriteAbaqusCrystalPlasticityFilter::k_FilePrefix_Key) == "TestPrefix");
      CHECK(args.value<std::string>(WriteAbaqusCrystalPlasticityFilter::k_JobName_Key) == "TestJob");
      CHECK(args.value<int32>(WriteAbaqusCrystalPlasticityFilter::k_NumDepvar_Key) == 7);
      CHECK(args.value<int32>(WriteAbaqusCrystalPlasticityFilter::k_NumUserOutVar_Key) == 4);
      CHECK(args.value<DynamicTableParameter::ValueType>(WriteAbaqusCrystalPlasticityFilter::k_MaterialConstants_Key) == DynamicTableParameter::ValueType{{1.0}, {2.0}, {3.0}});
      CHECK(args.value<DataPath>(WriteAbaqusCrystalPlasticityFilter::k_ImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(WriteAbaqusCrystalPlasticityFilter::k_FeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "FeatureIds"}));
      CHECK(args.value<DataPath>(WriteAbaqusCrystalPlasticityFilter::k_CellEulerAnglesArrayPath_Key) == DataPath({"DataContainer", "CellData", "EulerAngles"}));
      CHECK(args.value<DataPath>(WriteAbaqusCrystalPlasticityFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "Phases"}));
    }
  }
}
