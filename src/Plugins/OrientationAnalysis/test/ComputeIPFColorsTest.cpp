/**
 * @file ComputeIPFColorsTest.cpp
 *
 * V&V oracle for ComputeIPFColorsFilter.
 *
 * ComputeIPFColors is a thin orchestration layer on top of EbsdLib's
 * LaueOps::generateIPFColor(). The per-Laue-class correctness of the actual
 * color math (TSL / PUCM / Nolze-Hielscher) is verified upstream by EbsdLib's
 * own suite (TSLColorKeyTest, PUCMColorKeyTest, NolzeHielscherColorKeyTest,
 * ColorKeyKindTest). This file therefore V&Vs only the SIMPLNX *value-add* --
 * the per-cell dispatch, indexing, masking, reference-direction normalization,
 * phase bounds handling, and output packing -- NOT the color algorithm itself.
 *
 * Oracle design (see src/Plugins/OrientationAnalysis/vv/ComputeIPFColorsFilter.md):
 *   Class 1 (Analytical) : masked cell -> (0,0,0); invalid crystal structure ->
 *                          (0,0,0); reference-direction normalization; phase out
 *                          of range -> error -48000.
 *   Class 2 (Reference)  : colored cell == a direct in-process EbsdLib
 *                          generateIPFColor() call on the same inputs. Verifies
 *                          the filter routes data correctly, without re-testing
 *                          EbsdLib's color math.
 *   Class 3 (Paper/std)  : identity-orientation cubic cell viewed down [001]
 *                          is the red corner of the standard IPF triangle
 *                          (per EbsdLib TSLColorKeyTest: [001] -> r=1,g=0,b=0).
 *   Class 4 (Invariant)  : output is 3-component uint8; non-colored cells are
 *                          exactly (0,0,0); colored cells are non-black.
 *
 * The dataset is built entirely in-line (no exemplar archive) so no golden file
 * captured from legacy DREAM3D or a prior SIMPLNX run can act as a circular
 * oracle. The retired archive-based comparison used so3_cubic_high_ipf_001.dream3d
 * whose IPFColors array was produced by legacy SIMPL/DREAM3D itself.
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

//------------------------------------------------------------------------------
// Inline analytical dataset. 6 cells, chosen so that each code path in
// ComputeIPFColors::convert() is exercised. Crystal-structure ensemble:
//   ensemble 0 -> UnknownCrystalStructure (999)  (the conventional DREAM3D
//                 "invalid phase" placeholder; >= LaueGroupEnd, so never colored)
//   ensemble 1 -> Cubic_High (1)
//   ensemble 2 -> Hexagonal_High (0)
//
//  cell phase crystalStruct       euler (rad)     mask   role
//   0    1    Cubic_High          (0,0,0)         true   Class 3 identity -> red corner
//   1    1    Cubic_High          (0.3,0.4,0.5)   true   Class 2 arbitrary cubic
//   2    2    Hexagonal_High      (0.1,0.2,0.3)   true   Class 2 arbitrary hex (2nd Laue class)
//   3    1    Cubic_High          (0.3,0.4,0.5)   false  Class 1 masked -> black
//   4    0    Unknown (999)       (0.3,0.4,0.5)   true   Class 1 invalid crystal structure -> black
//   5    2    Hexagonal_High      (0.7,0.8,0.9)   true   Class 2 arbitrary hex
//------------------------------------------------------------------------------
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

// Cells that must receive a real (non-black) IPF color when the mask is on.
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
  csStore[0] = CS::UnknownCrystalStructure; // 999
  csStore[1] = CS::Cubic_High;              // 1
  csStore[2] = CS::Hexagonal_High;          // 0

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

// Build the base filter arguments for the analytical dataset. refDir defaults to
// [0,0,1] (already unit length so the reference call needs no separate normalize).
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

// The Class 2 reference: a direct, independent EbsdLib call reproducing what the
// filter should compute for one colored cell. This is EbsdLib the trusted
// reference implementation (Class 2), not the filter's own output.
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
  UnitTest::LoadPlugins();

  DataStructure dataStructure = BuildAnalyticalDataset();

  ComputeIPFColorsFilter filter;
  const Arguments args = MakeArgs(true, k_MaskPath, {0.0F, 0.0F, 1.0F}, 0 /*TSL*/, k_IpfColorsName);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_IpfColorsPath));
  const auto& colors = dataStructure.getDataRefAs<UInt8Array>(k_IpfColorsPath);
  // Class 4: output is 3-component uint8 with the parent tuple count.
  REQUIRE(colors.getNumberOfTuples() == k_NumCells);
  REQUIRE(colors.getNumberOfComponents() == 3);

  // Pull the inputs back out so the reference call is driven by the same data the filter saw.
  const auto& eulerStore = dataStructure.getDataRefAs<Float32Array>(k_EulersPath).getDataStoreRef();
  const auto& phaseStore = dataStructure.getDataRefAs<Int32Array>(k_PhasesPath).getDataStoreRef();
  const auto& csStore = dataStructure.getDataRefAs<UInt32Array>(k_CrystalStructuresPath).getDataStoreRef();
  const auto& colorStore = colors.getDataStoreRef();

  auto colorAt = [&](usize cell) { return std::array<uint8, 3>{colorStore[cell * 3], colorStore[cell * 3 + 1], colorStore[cell * 3 + 2]}; };
  const std::array<double, 3> refDir = {0.0, 0.0, 1.0};

  SECTION("Class 3 - identity cubic orientation viewed down [001] is the red IPF corner")
  {
    // Per EbsdLib TSLColorKeyTest: the [001] direction maps to r=1,g=0,b=0.
    const std::array<uint8, 3> c0 = colorAt(0);
    REQUIRE(c0[0] >= 250); // red channel saturated (allow rounding at the exact corner)
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
      // Class 4: a colored cell must not be black.
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
  UnitTest::LoadPlugins();

  DataStructure dataStructure = BuildAnalyticalDataset();

  ComputeIPFColorsFilter filter;
  // Same as the main oracle but the mask is a uint8 array -> exercises convert<uint8>.
  const Arguments args = MakeArgs(true, k_MaskU8Path, {0.0F, 0.0F, 1.0F}, 0 /*TSL*/, k_IpfColorsName);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& colorStore = dataStructure.getDataRefAs<UInt8Array>(k_IpfColorsPath).getDataStoreRef();
  auto colorAt = [&](usize cell) { return std::array<uint8, 3>{colorStore[cell * 3], colorStore[cell * 3 + 1], colorStore[cell * 3 + 2]}; };

  // The uint8-masked "bad" cell is black; a good cell is colored.
  REQUIRE(colorAt(k_MaskedCell) == std::array<uint8, 3>{0, 0, 0});
  const std::array<uint8, 3> c0 = colorAt(0);
  REQUIRE((c0[0] != 0 || c0[1] != 0 || c0[2] != 0));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeIPFColorsFilter: no-mask path colors every valid cell", "[OrientationAnalysis][ComputeIPFColorsFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = BuildAnalyticalDataset();

  ComputeIPFColorsFilter filter;
  // useMask == false -> the algorithm's goodVoxels pointer is null (convert<bool>
  // with a null mask). Cell 3, which was masked-black in the main oracle, must now
  // be colored; cell 4 (invalid crystal structure) is still black.
  const Arguments args = MakeArgs(false, DataPath{}, {0.0F, 0.0F, 1.0F}, 0 /*TSL*/, k_IpfColorsName);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& eulerStore = dataStructure.getDataRefAs<Float32Array>(k_EulersPath).getDataStoreRef();
  const auto& phaseStore = dataStructure.getDataRefAs<Int32Array>(k_PhasesPath).getDataStoreRef();
  const auto& csStore = dataStructure.getDataRefAs<UInt32Array>(k_CrystalStructuresPath).getDataStoreRef();
  const auto& colorStore = dataStructure.getDataRefAs<UInt8Array>(k_IpfColorsPath).getDataStoreRef();
  auto colorAt = [&](usize cell) { return std::array<uint8, 3>{colorStore[cell * 3], colorStore[cell * 3 + 1], colorStore[cell * 3 + 2]}; };

  // Cell 3 is now colored and matches the EbsdLib reference (mask no longer suppresses it).
  const std::array<double, 3> euler3 = {eulerStore[k_MaskedCell * 3], eulerStore[k_MaskedCell * 3 + 1], eulerStore[k_MaskedCell * 3 + 2]};
  const std::array<uint8, 3> expected3 = EbsdLibReferenceColor(euler3, csStore[phaseStore[k_MaskedCell]], {0.0, 0.0, 1.0}, ebsdlib::ColorKeyKind::TSL);
  REQUIRE(colorAt(k_MaskedCell) == expected3);
  REQUIRE(colorAt(k_InvalidCsCell) == std::array<uint8, 3>{0, 0, 0});

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeIPFColorsFilter: reference direction is normalized", "[OrientationAnalysis][ComputeIPFColorsFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = BuildAnalyticalDataset();

  ComputeIPFColorsFilter filter;
  // A non-unit reference direction [0,0,5] must produce exactly the same colors as
  // the unit direction [0,0,1] because the algorithm normalizes it first.
  auto runWith = [&](const std::vector<float32>& refDir, const std::string& outputName) {
    ComputeIPFColorsFilter f;
    const Arguments args = MakeArgs(true, k_MaskPath, refDir, 0 /*TSL*/, outputName);
    auto preflightResult = f.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = f.execute(dataStructure, args);
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
  UnitTest::LoadPlugins();

  DataStructure dataStructure = BuildAnalyticalDataset();

  // Corrupt one cell's phase to reference an ensemble index that does not exist
  // (numEnsembles == 3, so phase 5 is out of range). The per-cell loop increments
  // the phase-warning count and the algorithm returns error -48000.
  auto& phaseStore = dataStructure.getDataRefAs<Int32Array>(k_PhasesPath).getDataStoreRef();
  phaseStore[0] = 5;

  ComputeIPFColorsFilter filter;
  const Arguments args = MakeArgs(false, DataPath{}, {0.0F, 0.0F, 1.0F}, 0 /*TSL*/, k_IpfColorsName);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -48000);
}

// -----------------------------------------------------------------------------
// Plumbing test: the k_ColorKey_Key choice index must route through executeImpl's
// switch into the right ebsdlib::ColorKeyKind and reach generateIPFColor. The
// per-Laue-class correctness of TSL / PUCM / Nolze-Hielscher is covered by
// EbsdLib's color-key tests; here we only assert that the simplnx side wiring is
// intact -- non-default choices must produce a different output array than the
// default (TSL) run on the same input data.
TEST_CASE("OrientationAnalysis::ComputeIPFColorsFilter: ColorKey choice reaches algorithm", "[OrientationAnalysis][ComputeIPFColorsFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = BuildAnalyticalDataset();

  auto runWithKind = [&](ChoicesParameter::ValueType kindIndex, const std::string& outputName) {
    ComputeIPFColorsFilter filter;
    const Arguments args = MakeArgs(true, k_MaskPath, {0.0F, 0.0F, 1.0F}, kindIndex, outputName);
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
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

  // If the switch in executeImpl ever silently collapsed every kind onto TSL,
  // these arrays would be identical.
  REQUIRE(!std::equal(tslColors.cbegin(), tslColors.cend(), pucmColors.cbegin()));
  REQUIRE(!std::equal(tslColors.cbegin(), tslColors.cend(), nhColors.cbegin()));
}

TEST_CASE("OrientationAnalysis::ComputeIPFColorsFilter: Preflight Error - Cell array tuple count mismatch (-651)", "[OrientationAnalysis][ComputeIPFColorsFilter][preflight]")
{
  UnitTest::LoadPlugins();

  // Build a minimal synthetic DataStructure where the two cell-level arrays that are
  // validated together (Euler Angles and Phases) do NOT share the same tuple count.
  // This drives the validateNumberOfTuples() guard in preflightImpl that emits -651.
  // The mask is left disabled so only Euler Angles + Phases participate in the check.
  DataStructure dataStructure;
  auto* imageGeom = ImageGeom::Create(dataStructure, "DataContainer");
  imageGeom->setDimensions({10, 1, 1});

  auto* cellAM = AttributeMatrix::Create(dataStructure, "CellData", {10}, imageGeom->getId());
  UnitTest::CreateTestDataArray<float32>(dataStructure, "EulerAngles", {10}, {3}, cellAM->getId());

  // Phases lives in a separate AttributeMatrix with a deliberately different tuple
  // count (9 != 10) so the cross-array tuple-count check fails.
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
      // Complex type (FloatVec3FilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<bool>(ComputeIPFColorsFilter::k_UseMask_Key) == true);
      CHECK(args.value<DataPath>(ComputeIPFColorsFilter::k_CellEulerAnglesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeIPFColorsFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeIPFColorsFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeIPFColorsFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeIPFColorsFilter::k_CellIPFColorsArrayName_Key) == "TestName");
    }
  }
}
