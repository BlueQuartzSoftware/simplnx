/**
 * @file ComputeIPFColorsTest.cpp
 *
 * @brief Verifies simplnx routing around EbsdLib IPF color calculation.
 *
 * EbsdLib TSLColorKeyTest, PUCMColorKeyTest, NolzeHielscherColorKeyTest, and
 * ColorKeyKindTest verify per-Laue-class color math. This file verifies simplnx
 * dispatch, indexing, masking, direction normalization, phase bounds, and
 * output packing. The inline fixture avoids a circular oracle from legacy or
 * prior simplnx output.
 */

#include "OrientationAnalysis/Filters/ComputeIPFColorsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Common/RgbColor.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

#include <catch2/catch.hpp>

#include <EbsdLib/Core/EbsdLibConstants.h>
#include <EbsdLib/LaueOps/LaueOps.h>

#include <array>
#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::UnitTest;

namespace
{
namespace CS = ebsdlib::CrystalStructure;

// Six cells cover cubic and hexagonal colors, mask suppression, and an invalid
// crystal structure. Cell zero is the cubic [001] reference case.
constexpr usize k_NumCells = 6;
constexpr usize k_NumEnsembles = 3;

const DataPath k_GeomPath({"DataContainer"});
const DataPath k_CellDataPath = k_GeomPath.createChildPath("CellData");
const DataPath k_EulersPath = k_CellDataPath.createChildPath("EulerAngles");
const DataPath k_PhasesPath = k_CellDataPath.createChildPath("Phases");
const DataPath k_MaskPath = k_CellDataPath.createChildPath("Mask");
const DataPath k_MaskU8Path = k_CellDataPath.createChildPath("MaskU8");
const DataPath k_CrystalStructuresPath = k_GeomPath.createChildPath("CellEnsembleData").createChildPath("CrystalStructures");
const std::string k_IpfColorsName = "IPFColors";
const DataPath k_IpfColorsPath = k_CellDataPath.createChildPath(k_IpfColorsName);

// These cells must receive non-black colors when the mask is enabled.
constexpr std::array<usize, 4> k_ColoredCells = {0, 1, 2, 5};
constexpr usize k_MaskedCell = 3;    // masked "bad" -> black
constexpr usize k_InvalidCsCell = 4; // crystal structure 999 -> black

DataStructure BuildAnalyticalDataset()
{
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "DataContainer");
  imageGeom->setDimensions({k_NumCells, 1, 1});

  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", {k_NumCells}, imageGeom->getId());
  auto* eulersArray = CreateTestDataArray<float32>(dataStructure, "EulerAngles", {k_NumCells}, {3}, cellAM->getId());
  auto* phasesArray = CreateTestDataArray<int32>(dataStructure, "Phases", {k_NumCells}, {1}, cellAM->getId());
  auto* maskArray = CreateTestDataArray<bool>(dataStructure, "Mask", {k_NumCells}, {1}, cellAM->getId());
  auto* maskU8Array = CreateTestDataArray<uint8>(dataStructure, "MaskU8", {k_NumCells}, {1}, cellAM->getId());

  auto* ensembleAM = AttributeMatrix::Create(dataStructure, "CellEnsembleData", {k_NumEnsembles}, imageGeom->getId());
  auto* crystalStructuresArray = CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {k_NumEnsembles}, {1}, ensembleAM->getId());

  auto& csStore = crystalStructuresArray->getDataStoreRef();
  csStore[0] = CS::UnknownCrystalStructure;
  csStore[1] = CS::Cubic_High;
  csStore[2] = CS::Hexagonal_High;

  auto& eulerStore = eulersArray->getDataStoreRef();
  auto setEuler = [&](usize cell, float32 p1, float32 p, float32 p2) {
    eulerStore[cell * 3] = p1;
    eulerStore[cell * 3 + 1] = p;
    eulerStore[cell * 3 + 2] = p2;
  };
  setEuler(0, 0.0F, 0.0F, 0.0F);
  setEuler(1, 0.3F, 0.4F, 0.5F);
  setEuler(2, 0.1F, 0.2F, 0.3F);
  setEuler(3, 0.3F, 0.4F, 0.5F);
  setEuler(4, 0.3F, 0.4F, 0.5F);
  setEuler(5, 0.7F, 0.8F, 0.9F);

  auto& phaseStore = phasesArray->getDataStoreRef();
  const std::array<int32, k_NumCells> phases = {1, 1, 2, 1, 0, 2};
  for(usize i = 0; i < k_NumCells; i++)
  {
    phaseStore[i] = phases[i];
  }

  auto& maskStore = maskArray->getDataStoreRef();
  auto& maskU8Store = maskU8Array->getDataStoreRef();
  for(usize i = 0; i < k_NumCells; i++)
  {
    const bool good = (i != k_MaskedCell);
    maskStore[i] = good;
    maskU8Store[i] = good ? 1 : 0;
  }

  return dataStructure;
}

// The baseline [0,0,1] reference direction is already normalized.
Arguments MakeArgs(bool useMask, const DataPath& maskPath, const std::vector<float32>& refDir, ChoicesParameter::ValueType colorKey, const std::string& outputName)
{
  Arguments args;
  args.insertOrAssign(ComputeIPFColorsFilter::k_ReferenceDir_Key, std::make_any<VectorFloat32Parameter::ValueType>(refDir));
  args.insertOrAssign(ComputeIPFColorsFilter::k_ColorKey_Key, std::make_any<ChoicesParameter::ValueType>(colorKey));
  args.insertOrAssign(ComputeIPFColorsFilter::k_UseMask_Key, std::make_any<bool>(useMask));
  args.insertOrAssign(ComputeIPFColorsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskPath));
  args.insertOrAssign(ComputeIPFColorsFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(k_EulersPath));
  args.insertOrAssign(ComputeIPFColorsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));
  args.insertOrAssign(ComputeIPFColorsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresPath));
  args.insertOrAssign(ComputeIPFColorsFilter::k_CellIPFColorsArrayName_Key, std::make_any<std::string>(outputName));
  return args;
}

// Use EbsdLib directly to verify filter routing without retesting color math.
std::array<uint8, 3> EbsdLibReferenceColor(const std::array<double, 3>& euler, uint32 crystalStruct, const std::array<double, 3>& refDir, ebsdlib::ColorKeyKind kind)
{
  std::vector<ebsdlib::LaueOps::Pointer> ops = ebsdlib::LaueOps::GetAllOrientationOps();
  std::array<double, 3> dEuler = euler;
  std::array<double, 3> dRefDir = refDir;
  ebsdlib::Rgb argb = ops[crystalStruct]->generateIPFColor(dEuler.data(), dRefDir.data(), false, kind);
  return {static_cast<uint8>(RgbColor::dRed(argb)), static_cast<uint8>(RgbColor::dGreen(argb)), static_cast<uint8>(RgbColor::dBlue(argb))};
}
} // namespace

TEST_CASE("OrientationAnalysis::ComputeIPFColorsFilter: Class 1/2/3 Oracle (inline analytical dataset)", "[OrientationAnalysis][ComputeIPFColorsFilter]")
{
  // AlgorithmTestScope forces the selected path and records its target-call
  // witness.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  DataStructure dataStructure = BuildAnalyticalDataset();

  ComputeIPFColorsFilter filter;
  const Arguments args = MakeArgs(true, k_MaskPath, {0.0F, 0.0F, 1.0F}, 0 /*TSL*/, k_IpfColorsName);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_IpfColorsPath));
  const auto& colors = dataStructure.getDataRefAs<UInt8Array>(k_IpfColorsPath);
  // Class 4 checks the output shape.
  REQUIRE(colors.getNumberOfTuples() == k_NumCells);
  REQUIRE(colors.getNumberOfComponents() == 3);

  // Use the filter inputs for the independent EbsdLib reference.
  const auto& eulerStore = dataStructure.getDataRefAs<Float32Array>(k_EulersPath).getDataStoreRef();
  const auto& phaseStore = dataStructure.getDataRefAs<Int32Array>(k_PhasesPath).getDataStoreRef();
  const auto& csStore = dataStructure.getDataRefAs<UInt32Array>(k_CrystalStructuresPath).getDataStoreRef();
  const auto& colorStore = colors.getDataStoreRef();

  auto colorAt = [&](usize cell) { return std::array<uint8, 3>{colorStore[cell * 3], colorStore[cell * 3 + 1], colorStore[cell * 3 + 2]}; };
  const std::array<double, 3> refDir = {0.0, 0.0, 1.0};

  SECTION("Class 3 - identity cubic orientation viewed down [001] is the red IPF corner")
  {
    // EbsdLib maps [001] to the red TSL IPF corner.
    const std::array<uint8, 3> c0 = colorAt(0);
    REQUIRE(c0[0] >= 250); // Allow rounding at the exact red corner.
    REQUIRE(c0[1] == 0);
    REQUIRE(c0[2] == 0);
  }

  SECTION("Class 2 - each colored cell equals a direct EbsdLib generateIPFColor call")
  {
    for(usize cell : k_ColoredCells)
    {
      const std::array<double, 3> euler = {eulerStore[cell * 3], eulerStore[cell * 3 + 1], eulerStore[cell * 3 + 2]};
      const uint32 crystalStruct = csStore[phaseStore[cell]];
      const std::array<uint8, 3> expected = EbsdLibReferenceColor(euler, crystalStruct, refDir, ebsdlib::ColorKeyKind::TSL);
      const std::array<uint8, 3> actual = colorAt(cell);
      INFO("cell " << cell);
      REQUIRE(actual == expected);
      // Class 4 requires a colored cell to be non-black.
      REQUIRE((actual[0] != 0 || actual[1] != 0 || actual[2] != 0));
    }
  }

  SECTION("Class 1 - masked-off cell is black")
  {
    REQUIRE(colorAt(k_MaskedCell) == std::array<uint8, 3>{0, 0, 0});
  }

  SECTION("Class 1 - cell whose crystal structure is >= LaueGroupEnd is black")
  {
    REQUIRE(colorAt(k_InvalidCsCell) == std::array<uint8, 3>{0, 0, 0});
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeIPFColorsFilter: uint8 mask array drives the black-out path", "[OrientationAnalysis][ComputeIPFColorsFilter]")
{
  // AlgorithmTestScope forces the selected path and records its target-call
  // witness.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  DataStructure dataStructure = BuildAnalyticalDataset();

  ComputeIPFColorsFilter filter;
  // A uint8 mask selects the uint8 conversion path.
  const Arguments args = MakeArgs(true, k_MaskU8Path, {0.0F, 0.0F, 1.0F}, 0 /*TSL*/, k_IpfColorsName);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& colorStore = dataStructure.getDataRefAs<UInt8Array>(k_IpfColorsPath).getDataStoreRef();
  auto colorAt = [&](usize cell) { return std::array<uint8, 3>{colorStore[cell * 3], colorStore[cell * 3 + 1], colorStore[cell * 3 + 2]}; };

  REQUIRE(colorAt(k_MaskedCell) == std::array<uint8, 3>{0, 0, 0});
  const std::array<uint8, 3> c0 = colorAt(0);
  REQUIRE((c0[0] != 0 || c0[1] != 0 || c0[2] != 0));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeIPFColorsFilter: no-mask path colors every valid cell", "[OrientationAnalysis][ComputeIPFColorsFilter]")
{
  // AlgorithmTestScope forces the selected path and records its target-call
  // witness.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  DataStructure dataStructure = BuildAnalyticalDataset();

  ComputeIPFColorsFilter filter;
  // A null mask colors cell three but keeps the invalid crystal structure black.
  const Arguments args = MakeArgs(false, DataPath{}, {0.0F, 0.0F, 1.0F}, 0 /*TSL*/, k_IpfColorsName);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& eulerStore = dataStructure.getDataRefAs<Float32Array>(k_EulersPath).getDataStoreRef();
  const auto& phaseStore = dataStructure.getDataRefAs<Int32Array>(k_PhasesPath).getDataStoreRef();
  const auto& csStore = dataStructure.getDataRefAs<UInt32Array>(k_CrystalStructuresPath).getDataStoreRef();
  const auto& colorStore = dataStructure.getDataRefAs<UInt8Array>(k_IpfColorsPath).getDataStoreRef();
  auto colorAt = [&](usize cell) { return std::array<uint8, 3>{colorStore[cell * 3], colorStore[cell * 3 + 1], colorStore[cell * 3 + 2]}; };

  const std::array<double, 3> euler3 = {eulerStore[k_MaskedCell * 3], eulerStore[k_MaskedCell * 3 + 1], eulerStore[k_MaskedCell * 3 + 2]};
  const std::array<uint8, 3> expected3 = EbsdLibReferenceColor(euler3, csStore[phaseStore[k_MaskedCell]], {0.0, 0.0, 1.0}, ebsdlib::ColorKeyKind::TSL);
  REQUIRE(colorAt(k_MaskedCell) == expected3);
  REQUIRE(colorAt(k_InvalidCsCell) == std::array<uint8, 3>{0, 0, 0});

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeIPFColorsFilter: reference direction is normalized", "[OrientationAnalysis][ComputeIPFColorsFilter]")
{
  // AlgorithmTestScope forces the selected path and records its target-call
  // witness.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  DataStructure dataStructure = BuildAnalyticalDataset();

  ComputeIPFColorsFilter filter;
  // Normalization makes [0,0,5] and [0,0,1] produce identical colors.
  auto runWith = [&](const std::vector<float32>& refDir, const std::string& outputName) {
    ComputeIPFColorsFilter f;
    const Arguments args = MakeArgs(true, k_MaskPath, refDir, 0 /*TSL*/, outputName);
    auto preflightResult = f.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(f, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  };

  runWith({0.0F, 0.0F, 1.0F}, "IPF_Unit");
  runWith({0.0F, 0.0F, 5.0F}, "IPF_NonUnit");

  const auto& unitColors = dataStructure.getDataRefAs<UInt8Array>(k_CellDataPath.createChildPath("IPF_Unit"));
  const auto& nonUnitColors = dataStructure.getDataRefAs<UInt8Array>(k_CellDataPath.createChildPath("IPF_NonUnit"));
  REQUIRE(unitColors.getSize() == nonUnitColors.getSize());
  REQUIRE(std::equal(unitColors.cbegin(), unitColors.cend(), nonUnitColors.cbegin()));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeIPFColorsFilter: phase index out of range returns -48000", "[OrientationAnalysis][ComputeIPFColorsFilter]")
{
  // AlgorithmTestScope forces the selected path and records its target-call
  // witness.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  DataStructure dataStructure = BuildAnalyticalDataset();

  // Phase five exceeds the three-entry ensemble array and must report -48000.
  auto& phaseStore = dataStructure.getDataRefAs<Int32Array>(k_PhasesPath).getDataStoreRef();
  phaseStore[0] = 5;

  ComputeIPFColorsFilter filter;
  const Arguments args = MakeArgs(false, DataPath{}, {0.0F, 0.0F, 1.0F}, 0 /*TSL*/, k_IpfColorsName);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -48000);
}

// Verify that ColorKey selection reaches EbsdLib. EbsdLib tests the color math.
// This fixture requires non-default keys to differ from TSL.
TEST_CASE("OrientationAnalysis::ComputeIPFColorsFilter: ColorKey choice reaches algorithm", "[OrientationAnalysis][ComputeIPFColorsFilter]")
{
  // AlgorithmTestScope forces the selected path and records its target-call
  // witness.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);
  UnitTest::LoadPlugins();

  DataStructure dataStructure = BuildAnalyticalDataset();

  auto runWithKind = [&](ChoicesParameter::ValueType kindIndex, const std::string& outputName) {
    ComputeIPFColorsFilter filter;
    const Arguments args = MakeArgs(true, k_MaskPath, {0.0F, 0.0F, 1.0F}, kindIndex, outputName);
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  };

  runWithKind(0, "IPFColors_TSL");
  runWithKind(1, "IPFColors_PUCM");
  runWithKind(2, "IPFColors_NH");

  const auto& tslColors = dataStructure.getDataRefAs<UInt8Array>(k_CellDataPath.createChildPath("IPFColors_TSL"));
  const auto& pucmColors = dataStructure.getDataRefAs<UInt8Array>(k_CellDataPath.createChildPath("IPFColors_PUCM"));
  const auto& nhColors = dataStructure.getDataRefAs<UInt8Array>(k_CellDataPath.createChildPath("IPFColors_NH"));

  REQUIRE(tslColors.getSize() == pucmColors.getSize());
  REQUIRE(tslColors.getSize() == nhColors.getSize());

  // Identical arrays would show that dispatch collapsed every key to TSL.
  REQUIRE(!std::equal(tslColors.cbegin(), tslColors.cend(), pucmColors.cbegin()));
  REQUIRE(!std::equal(tslColors.cbegin(), tslColors.cend(), nhColors.cbegin()));
}

TEST_CASE("OrientationAnalysis::ComputeIPFColorsFilter: Preflight Error - Cell array tuple count mismatch (-651)", "[OrientationAnalysis][ComputeIPFColorsFilter][preflight]")
{
  UnitTest::LoadPlugins();

  // EulerAngles and Phases have different tuple counts. The disabled mask keeps
  // the preflight check limited to these two arrays.
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "DataContainer");
  imageGeom->setDimensions({10, 1, 1});

  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", {10}, imageGeom->getId());
  UnitTest::CreateTestDataArray<float32>(dataStructure, "EulerAngles", {10}, {3}, cellAM->getId());

  // The separate group makes Phases contain nine tuples instead of ten.
  auto* mismatchAM = AttributeMatrix::Create(dataStructure, "MismatchData", {9}, imageGeom->getId());
  UnitTest::CreateTestDataArray<int32>(dataStructure, "Phases", {9}, {1}, mismatchAM->getId());

  auto* ensembleAM = AttributeMatrix::Create(dataStructure, "CellEnsembleData", {2}, imageGeom->getId());
  UnitTest::CreateTestDataArray<uint32>(dataStructure, "CrystalStructures", {2}, {1}, ensembleAM->getId());

  ComputeIPFColorsFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeIPFColorsFilter::k_ReferenceDir_Key, std::make_any<VectorFloat32Parameter::ValueType>({0.0F, 0.0F, 1.0F}));
  args.insertOrAssign(ComputeIPFColorsFilter::k_ColorKey_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(ComputeIPFColorsFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeIPFColorsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ComputeIPFColorsFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellData", "EulerAngles"})));
  args.insertOrAssign(ComputeIPFColorsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "MismatchData", "Phases"})));
  args.insertOrAssign(ComputeIPFColorsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "CellEnsembleData", "CrystalStructures"})));
  args.insertOrAssign(ComputeIPFColorsFilter::k_CellIPFColorsArrayName_Key, std::make_any<std::string>("IPFColors"));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -651);
}

TEST_CASE("OrientationAnalysis::ComputeIPFColorsFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeIPFColorsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeIPFColorsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeIPFColorsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeIPFColorsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      // Pipeline loading verifies FloatVec3FilterParameterConverter.
      CHECK(args.value<bool>(ComputeIPFColorsFilter::k_UseMask_Key) == true);
      CHECK(args.value<DataPath>(ComputeIPFColorsFilter::k_CellEulerAnglesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeIPFColorsFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeIPFColorsFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeIPFColorsFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeIPFColorsFilter::k_CellIPFColorsArrayName_Key) == "TestName");
    }
  }
}
