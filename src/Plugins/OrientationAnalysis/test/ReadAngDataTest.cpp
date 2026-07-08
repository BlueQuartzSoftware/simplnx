/* ============================================================================
 * ReadAngData V&V test suite.
 *
 * Verification is established INDEPENDENTLY of legacy DREAM3D, per the V&V
 * policy (src/Plugins/OrientationAnalysis/vv/ReadAngDataFilter.md):
 *
 *  - .ang parsing        : Class 2 (EbsdLib reference, trusted & NOT re-tested).
 *                          EbsdLib's AngReader owns header parsing, data-column
 *                          parsing, grid-order fix-up and its own error codes.
 *                          We do not re-test any of that here.
 *  - SIMPLNX value-add   : Class 1 (analytical) + Class 4 (invariant). The
 *                          filter's value-add is deterministic data plumbing on
 *                          top of AngReader: geometry construction from header
 *                          values, per-column array creation, phase<1 -> 1
 *                          remap, Euler-angle interleave, ensemble slot-0
 *                          defaults, symmetry -> LaueOps index mapping,
 *                          material-name trimming and lattice-constant copy.
 *                          The toy .ang fixture below is hand-authored and every
 *                          expected value is hand-derived from the fixture text.
 *
 * The Euler/IQ/CI/position/fit column values are exactly representable in
 * float32 (multiples of 1/8), so they are asserted with exact equality. The
 * lattice constants (e.g. 3.520, 2.950, 4.680) are NOT exact float32 values;
 * they compare equal because runtime strtof and the compiler's parse of the
 * same decimal literal round identically to the same float32 bit pattern.
 *
 * The prior exemplar archive read_ang_test.tar.gz was a CIRCULAR ORACLE (the
 * exemplar .dream3d was generated from this filter's own output) and has been
 * retired in favor of the inline oracle below.
 * ========================================================================== */

#include "OrientationAnalysis/Filters/ReadAngDataFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
//------------------------------------------------------------------------------
// Toy .ang fixture: 3 columns x 2 rows, square grid, XSTEP 0.25, YSTEP 0.5,
// two phases (Cubic Nickel, Hexagonal Titanium). Data columns are
//   phi1 PHI phi2 x y IQ CI Phase SEM Fit
// Points 2 and 5 carry Phase 0 to exercise the filter's phase<1 -> 1 remap.
//------------------------------------------------------------------------------
const std::string k_HeaderPrefix = R"(# TEM_PIXperUM          1.000000
# x-star                0.500000
# y-star                0.500000
# z-star                0.500000
# WorkingDistance       15.000000
#
)";

const std::string k_PhaseBlock = R"(# Phase 1
# MaterialName  Nickel
# Formula     Ni
# Info
# Symmetry              43
# LatticeConstants      3.520 3.520 3.520  90.000  90.000  90.000
# NumberFamilies        1
# hklFamilies    1  1  1 1 8.469246 1
# Categories 0 0 0 0 0
#
# Phase 2
# MaterialName  Titanium (Alpha)
# Formula     Ti
# Info
# Symmetry              62
# LatticeConstants      2.950 2.950 4.680  90.000  90.000 120.000
# NumberFamilies        1
# hklFamilies    1  0  0 1 1.000000 1
# Categories 0 0 0 0 0
#
)";

// Only the second phase section of k_PhaseBlock: a legal-but-unusual file whose
// phase indices do not start at 1. Exercises the non-contiguous-phase sizing.
const std::string k_SparsePhaseBlock = R"(# Phase 2
# MaterialName  Titanium (Alpha)
# Formula     Ti
# Info
# Symmetry              62
# LatticeConstants      2.950 2.950 4.680  90.000  90.000 120.000
# NumberFamilies        1
# hklFamilies    1  0  0 1 1.000000 1
# Categories 0 0 0 0 0
#
)";

// A "# Phase 0" section. DREAM3D 6.5.171 tolerated this (it wrote Phase 0 into ensemble slot 0),
// but .ang phase numbering starts at 1, so SIMPLNX rejects it at execute with -19502. A static
// fixture trips this deterministically — no file-mutation injection is needed.
const std::string k_Phase0Block = R"(# Phase 0
# MaterialName  Nickel
# Formula     Ni
# Info
# Symmetry              43
# LatticeConstants      3.520 3.520 3.520  90.000  90.000  90.000
# NumberFamilies        1
# hklFamilies    1  1  1 1 8.469246 1
# Categories 0 0 0 0 0
#
)";

// {} is the GRID line (or empty to omit it entirely)
const std::string k_GridBlockFmt = R"({}# XSTEP: 0.250000
# YSTEP: 0.500000
# NCOLS_ODD: 3
# NCOLS_EVEN: 3
# NROWS: 2
#
# OPERATOR:  VVOracle
#
# SAMPLEID:
#
# SCANID:
#
)";

const std::string k_DataBlock = R"(  0.12500   0.25000   0.37500      0.00000      0.00000    10.5000  0.500  1      100.000  0.750
  0.50000   0.62500   0.75000      0.25000      0.00000    20.2500  0.250  2      200.000  1.500
  0.87500   1.00000   1.12500      0.50000      0.00000    30.7500  0.125  0      300.000  2.250
  1.25000   1.37500   1.50000      0.00000      0.50000    40.5000  0.375  1      400.000  3.000
  1.62500   1.75000   1.87500      0.25000      0.50000    50.2500  0.625  2      500.000  3.750
  2.00000   2.12500   2.25000      0.50000      0.50000    60.7500  0.875  0      600.000  4.500
)";

//------------------------------------------------------------------------------
std::string SqrGridBlock()
{
  return fmt::format(fmt::runtime(k_GridBlockFmt), "# GRID: SqrGrid\n");
}

//------------------------------------------------------------------------------
fs::path WriteAngFile(const std::string& fileName, const std::string& contents)
{
  const fs::path filePath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / fileName;
  std::ofstream out(filePath, std::ios::out | std::ios::trunc);
  REQUIRE(out.is_open());
  out << contents;
  out.close();
  return filePath;
}

//------------------------------------------------------------------------------
Arguments MakeDefaultArgs(const fs::path& inputAngFile)
{
  Arguments args;
  args.insertOrAssign(ReadAngDataFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(inputAngFile));
  args.insertOrAssign(ReadAngDataFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
  args.insertOrAssign(ReadAngDataFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_CellData));
  args.insertOrAssign(ReadAngDataFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_EnsembleAttributeMatrix));
  return args;
}

//------------------------------------------------------------------------------
template <typename T>
void CompareArrayValues(const DataStructure& dataStructure, const DataPath& arrayPath, const std::vector<T>& expected)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(arrayPath));
  const auto& dataArrayRef = dataStructure.getDataRefAs<DataArray<T>>(arrayPath);
  REQUIRE(dataArrayRef.getSize() == expected.size());
  for(usize i = 0; i < expected.size(); i++)
  {
    INFO(fmt::format("Array '{}' index {}", arrayPath.toString(), i));
    REQUIRE(dataArrayRef[i] == expected[i]);
  }
}
} // namespace

//------------------------------------------------------------------------------
// Class 1 (analytical) + Class 4 (invariant) oracle. Every expected value below
// is hand-derived from the fixture text at the top of this file; none of it was
// produced by running DREAM3D (any version).
//------------------------------------------------------------------------------
TEST_CASE("OrientationAnalysis::ReadAngDataFilter: Class 1 Analytical Oracle", "[OrientationAnalysis][ReadAngDataFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path inputAngFile = WriteAngFile("read_ang_vv_oracle.ang", k_HeaderPrefix + k_PhaseBlock + SqrGridBlock() + k_DataBlock);

  ReadAngDataFilter filter;
  DataStructure dataStructure;
  Arguments args = MakeDefaultArgs(inputAngFile);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath cellAMPath = k_DataContainerPath.createChildPath(k_CellData);
  const DataPath ensembleAMPath = k_DataContainerPath.createChildPath(k_EnsembleAttributeMatrix);

  // --- Geometry (Class 1: dims/spacing straight from the header; origin and
  // --- z-extent are the filter's hard-coded value-add) -----------------------
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(k_DataContainerPath));
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_DataContainerPath);
  {
    const SizeVec3 dims = imageGeom.getDimensions(); // NCOLS_EVEN=3, NROWS=2, one slice
    REQUIRE(dims == SizeVec3(3, 2, 1));
    const FloatVec3 spacing = imageGeom.getSpacing(); // XSTEP=0.25, YSTEP=0.5, z hard-coded 1.0
    REQUIRE(spacing == FloatVec3(0.25F, 0.5F, 1.0F));
    const FloatVec3 origin = imageGeom.getOrigin(); // hard-coded (0,0,0)
    REQUIRE(origin == FloatVec3(0.0F, 0.0F, 0.0F));
    REQUIRE(imageGeom.getUnits() == IGeometry::LengthUnit::Micrometer);
  }

  // --- Cell data (Class 1) ---------------------------------------------------
  // Phases: file column 8 is {1,2,0,1,2,0}; the filter remaps phase<1 -> 1.
  CompareArrayValues<int32>(dataStructure, cellAMPath.createChildPath("Phases"), {1, 2, 1, 1, 2, 1});

  // EulerAngles: file columns 1-3 interleaved per point (phi1_i, PHI_i, phi2_i).
  CompareArrayValues<float32>(dataStructure, cellAMPath.createChildPath("EulerAngles"),
                              {0.125F, 0.25F, 0.375F, 0.5F, 0.625F, 0.75F, 0.875F, 1.0F, 1.125F, 1.25F, 1.375F, 1.5F, 1.625F, 1.75F, 1.875F, 2.0F, 2.125F, 2.25F});

  // Pass-through columns, copied verbatim from the file.
  CompareArrayValues<float32>(dataStructure, cellAMPath.createChildPath("Image Quality"), {10.5F, 20.25F, 30.75F, 40.5F, 50.25F, 60.75F});
  CompareArrayValues<float32>(dataStructure, cellAMPath.createChildPath("Confidence Index"), {0.5F, 0.25F, 0.125F, 0.375F, 0.625F, 0.875F});
  CompareArrayValues<float32>(dataStructure, cellAMPath.createChildPath("SEM Signal"), {100.0F, 200.0F, 300.0F, 400.0F, 500.0F, 600.0F});
  CompareArrayValues<float32>(dataStructure, cellAMPath.createChildPath("Fit"), {0.75F, 1.5F, 2.25F, 3.0F, 3.75F, 4.5F});
  CompareArrayValues<float32>(dataStructure, cellAMPath.createChildPath("X Position"), {0.0F, 0.25F, 0.5F, 0.0F, 0.25F, 0.5F});
  CompareArrayValues<float32>(dataStructure, cellAMPath.createChildPath("Y Position"), {0.0F, 0.0F, 0.0F, 0.5F, 0.5F, 0.5F});

  // --- Ensemble data ----------------------------------------------------------
  // Class 4 invariant: tuple count is always (number of phases in file) + 1.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(ensembleAMPath));
  const auto& ensembleAM = dataStructure.getDataRefAs<AttributeMatrix>(ensembleAMPath);
  REQUIRE(ensembleAM.getShape() == std::vector<usize>{3});

  // CrystalStructures (Class 1): slot 0 is the UnknownCrystalStructure default
  // (999); TSL Symmetry 43 (Oh, m-3m) -> Cubic_High (1); TSL Symmetry 62
  // (D6h, 6/mmm) -> Hexagonal_High (0).
  CompareArrayValues<uint32>(dataStructure, ensembleAMPath.createChildPath("CrystalStructures"), {999, 1, 0});

  // LatticeConstants (Class 1): slot 0 zero-filled default; slots 1/2 copied
  // from the LatticeConstants header lines (a, b, c, alpha, beta, gamma).
  CompareArrayValues<float32>(dataStructure, ensembleAMPath.createChildPath("LatticeConstants"),
                              {0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 3.52F, 3.52F, 3.52F, 90.0F, 90.0F, 90.0F, 2.95F, 2.95F, 4.68F, 90.0F, 90.0F, 120.0F});

  // MaterialName (Class 1): slot 0 is the "Invalid Phase" default; EbsdLib's
  // AngPhase::parseMaterialName() rejoins tokens with a TRAILING space
  // ("Nickel ", "Titanium (Alpha) ") and the filter's value-add trims it.
  {
    const DataPath materialNamePath = ensembleAMPath.createChildPath("MaterialName");
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(materialNamePath));
    const auto& materialNamesRef = dataStructure.getDataRefAs<StringArray>(materialNamePath);
    REQUIRE(materialNamesRef.getNumberOfTuples() == 3);
    REQUIRE(materialNamesRef[0] == "Invalid Phase");
    REQUIRE(materialNamesRef[1] == "Nickel");
    REQUIRE(materialNamesRef[2] == "Titanium (Alpha)");
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

//------------------------------------------------------------------------------
// Class 4 invariant: a file whose phase indices do NOT start at 1 (only a
// "# Phase 2" section) must still produce ensemble arrays sized to
// maxPhaseIndex + 1, with every slot not covered by a phase section holding
// the "Invalid Phase" defaults. Regression pin for the out-of-bounds ensemble
// write found during the V&V algorithm review.
//------------------------------------------------------------------------------
TEST_CASE("OrientationAnalysis::ReadAngDataFilter: Non-Contiguous Phase Index", "[OrientationAnalysis][ReadAngDataFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path inputAngFile = WriteAngFile("read_ang_vv_sparse_phase.ang", k_HeaderPrefix + k_SparsePhaseBlock + SqrGridBlock() + k_DataBlock);

  ReadAngDataFilter filter;
  DataStructure dataStructure;
  Arguments args = MakeDefaultArgs(inputAngFile);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath ensembleAMPath = k_DataContainerPath.createChildPath(k_EnsembleAttributeMatrix);

  // maxPhaseIndex = 2 -> 3 tuples, even though the file defines only one phase.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(ensembleAMPath));
  const auto& ensembleAM = dataStructure.getDataRefAs<AttributeMatrix>(ensembleAMPath);
  REQUIRE(ensembleAM.getShape() == std::vector<usize>{3});

  // Slot 1 (no phase section in the file) keeps the "Invalid Phase" defaults.
  CompareArrayValues<uint32>(dataStructure, ensembleAMPath.createChildPath("CrystalStructures"), {999, 999, 0});
  CompareArrayValues<float32>(dataStructure, ensembleAMPath.createChildPath("LatticeConstants"),
                              {0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 2.95F, 2.95F, 4.68F, 90.0F, 90.0F, 120.0F});
  {
    const DataPath materialNamePath = ensembleAMPath.createChildPath("MaterialName");
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(materialNamePath));
    const auto& materialNamesRef = dataStructure.getDataRefAs<StringArray>(materialNamePath);
    REQUIRE(materialNamesRef[0] == "Invalid Phase");
    REQUIRE(materialNamesRef[1] == "Invalid Phase");
    REQUIRE(materialNamesRef[2] == "Titanium (Alpha)");
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

//------------------------------------------------------------------------------
// Value-add error path: the filter refuses HexGrid files at preflight (-19500).
//------------------------------------------------------------------------------
TEST_CASE("OrientationAnalysis::ReadAngDataFilter: HexGrid Preflight Error (-19500)", "[OrientationAnalysis][ReadAngDataFilter]")
{
  UnitTest::LoadPlugins();

  const std::string hexGridBlock = fmt::format(fmt::runtime(k_GridBlockFmt), "# GRID: HexGrid\n");
  const fs::path inputAngFile = WriteAngFile("read_ang_vv_hexgrid.ang", k_HeaderPrefix + k_PhaseBlock + hexGridBlock + k_DataBlock);

  ReadAngDataFilter filter;
  DataStructure dataStructure;
  Arguments args = MakeDefaultArgs(inputAngFile);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -19500);
}

//------------------------------------------------------------------------------
// Value-add error path: a file without a GRID header key fails preflight (-19501).
//------------------------------------------------------------------------------
TEST_CASE("OrientationAnalysis::ReadAngDataFilter: Missing GRID Preflight Error (-19501)", "[OrientationAnalysis][ReadAngDataFilter]")
{
  UnitTest::LoadPlugins();

  const std::string noGridBlock = fmt::format(fmt::runtime(k_GridBlockFmt), "");
  const fs::path inputAngFile = WriteAngFile("read_ang_vv_missing_grid.ang", k_HeaderPrefix + k_PhaseBlock + noGridBlock + k_DataBlock);

  ReadAngDataFilter filter;
  DataStructure dataStructure;
  Arguments args = MakeDefaultArgs(inputAngFile);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -19501);
}

//------------------------------------------------------------------------------
// EbsdLib error passthrough: a header with no Phase sections preflights fine
// (header parse succeeds) but execute fails with AngReader's -150.
//------------------------------------------------------------------------------
TEST_CASE("OrientationAnalysis::ReadAngDataFilter: EbsdLib Error Passthrough - No Phase (-150)", "[OrientationAnalysis][ReadAngDataFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path inputAngFile = WriteAngFile("read_ang_vv_no_phase.ang", k_HeaderPrefix + SqrGridBlock() + k_DataBlock);

  ReadAngDataFilter filter;
  DataStructure dataStructure;
  Arguments args = MakeDefaultArgs(inputAngFile);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -150);
}

//------------------------------------------------------------------------------
// Deviation D5: a "# Phase 0" section is accepted by legacy 6.5.171 but rejected
// by SIMPLNX at execute with -19502 (.ang phase numbering starts at 1). This is a
// STATIC fixture — it trips the guard deterministically, without any file-mutation
// injection between preflight and execute.
//------------------------------------------------------------------------------
TEST_CASE("OrientationAnalysis::ReadAngDataFilter: Phase 0 rejected (-19502)", "[OrientationAnalysis][ReadAngDataFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path inputAngFile = WriteAngFile("read_ang_vv_phase0.ang", k_HeaderPrefix + k_Phase0Block + SqrGridBlock() + k_DataBlock);

  ReadAngDataFilter filter;
  DataStructure dataStructure;
  Arguments args = MakeDefaultArgs(inputAngFile);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -19502);
}

//------------------------------------------------------------------------------
// EbsdLib error passthrough: the header declares 3x2 points but the data block
// is truncated to 4 lines; AngReader reports -600 at execute.
//------------------------------------------------------------------------------
TEST_CASE("OrientationAnalysis::ReadAngDataFilter: EbsdLib Error Passthrough - Truncated Data (-600)", "[OrientationAnalysis][ReadAngDataFilter]")
{
  UnitTest::LoadPlugins();

  // Keep only the first 4 of 6 data lines.
  std::string truncatedData = k_DataBlock;
  for(int i = 0; i < 2; i++)
  {
    truncatedData.erase(truncatedData.find_last_of('\n', truncatedData.size() - 2) + 1);
  }
  const fs::path inputAngFile = WriteAngFile("read_ang_vv_truncated.ang", k_HeaderPrefix + k_PhaseBlock + SqrGridBlock() + truncatedData);

  ReadAngDataFilter filter;
  DataStructure dataStructure;
  Arguments args = MakeDefaultArgs(inputAngFile);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -600);
}

//------------------------------------------------------------------------------
// SIMPL Backwards Compatibility — validates UUID + parameter conversion from
// the legacy ReadAngData (SIMPL UUID b8e128a8-c2a3-5e6c-a7ad-e4fb864e5d40).
//------------------------------------------------------------------------------
TEST_CASE("OrientationAnalysis::ReadAngDataFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ReadAngDataFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ReadAngDataFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ReadAngDataFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ReadAngDataFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<FileSystemPathParameter::ValueType>(ReadAngDataFilter::k_InputFile_Key) == fs::path("/some/test/path/data.ang"));
      CHECK(args.value<DataPath>(ReadAngDataFilter::k_CreatedImageGeometryPath_Key) == DataPath({"ImageDataContainer"}));
      CHECK(args.value<std::string>(ReadAngDataFilter::k_CellAttributeMatrixName_Key) == "CellData");
      CHECK(args.value<std::string>(ReadAngDataFilter::k_CellEnsembleAttributeMatrixName_Key) == "CellEnsembleData");
    }
  }
}
