/*
 * This suite validates the ReadCtfDataFilter value-add independently of EbsdLib parsing.
 * The oracle checks geometry construction, phase preservation, Euler interleaving,
 * optional hexagonal alignment, degree-to-radian conversion, and ensemble mapping.
 * EbsdLib remains the reference for .ctf parsing and parser error codes.
 * Fixture values use exact float32 inputs. Conversion expectations use double
 * precision before the final float32 result.
 */

#include "OrientationAnalysis/Filters/ReadCtfDataFilter.hpp"
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

#include <fmt/format.h>

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
// The toy fixture has a 3 by 2 grid and two phases.
// It includes hexagonal, cubic, and unindexed phase values.
// All fields use tab delimiters and the standard 11 CTF columns.
const std::string k_HeaderPrefix = "Channel Text File\n"
                                   "Prj\tVV Oracle Toy\n"
                                   "Author\tVVOracle\n"
                                   "JobMode\tGrid\n";

// {fmt} placeholders: XCells, YCells, extra header lines (e.g. ZCells/ZStep or nothing)
const std::string k_GridBlockFmt = "XCells\t{}\n"
                                   "YCells\t{}\n"
                                   "XStep\t0.25\n"
                                   "YStep\t0.5\n"
                                   "AcqE1\t0\n"
                                   "AcqE2\t0\n"
                                   "AcqE3\t0\n"
                                   "{}"
                                   "Euler angles refer to Sample Coordinate system (CS0)!\tMag\t2000\tCoverage\t100\tDevice\t0\tKV\t20\tTiltAngle\t70\tTiltAxis\t0\n";

// Phase sections contain lattice constants, angles, names, Laue groups, and comments.
// The reader drops the final comment character for LF-only fixtures, so tests omit comments.
const std::string k_TwoPhaseBlock = "Phases\t2\n"
                                    "2.5;2.5;4.25\t90;90;120\tHex Phase A\t9\tVV toy hexagonal phase\n"
                                    "3.625;3.625;3.625\t90;90;90\tCopper\t11\tVV toy cubic phase\n";

const std::string k_ZeroPhaseBlock = "Phases\t0\n";

const std::string k_ColumnHeader = "Phase\tX\tY\tBands\tError\tEuler1\tEuler2\tEuler3\tMAD\tBC\tBS\n";

// Point 3's Euler3 (7.125) and point 4's Euler1/Euler2 (3.375, 6.75) are chosen so the
// degrees->radians results DIFFER between float32 arithmetic and the double-precision
// intermediates the algorithm must use (and 7.125+30 = 37.125 differs too) — they pin the
// double-precision code path; the remaining values happen to round identically either way.
const std::string k_DataBlock = "1\t0\t0\t7\t0\t0.125\t0.25\t0.375\t0.5\t105\t177\n"
                                "2\t0.25\t0\t6\t0\t0.5\t0.625\t0.75\t0.25\t120\t209\n"
                                "0\t0.5\t0\t0\t3\t0.875\t1\t1.125\t0\t73\t178\n"
                                "1\t0\t0.5\t5\t0\t1.25\t1.375\t7.125\t0.375\t134\t232\n"
                                "2\t0.25\t0.5\t4\t0\t3.375\t6.75\t1.875\t0.625\t110\t216\n"
                                "0\t0.5\t0.5\t0\t3\t2\t2.125\t2.25\t0\t68\t151\n";

//------------------------------------------------------------------------------
std::string MakeToyCtf(const std::string& extraHeaderLines = "", const std::string& phaseBlock = k_TwoPhaseBlock, const std::string& columnHeader = k_ColumnHeader,
                       const std::string& dataBlock = k_DataBlock, int32 xCells = 3, int32 yCells = 2)
{
  return k_HeaderPrefix + fmt::format(fmt::runtime(k_GridBlockFmt), xCells, yCells, extraHeaderLines) + phaseBlock + columnHeader + dataBlock;
}

//------------------------------------------------------------------------------
fs::path WriteCtfFile(const std::string& fileName, const std::string& contents)
{
  const fs::path filePath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / fileName;
  std::ofstream out(filePath, std::ios::out | std::ios::trunc);
  REQUIRE(out.is_open());
  out << contents;
  out.close();
  return filePath;
}

//------------------------------------------------------------------------------
Arguments MakeDefaultArgs(const fs::path& inputCtfFile, bool degreesToRadians = false, bool edaxHexagonalAlignment = false)
{
  Arguments args;
  args.insertOrAssign(ReadCtfDataFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(inputCtfFile));
  args.insertOrAssign(ReadCtfDataFilter::k_DegreesToRadians_Key, std::make_any<bool>(degreesToRadians));
  args.insertOrAssign(ReadCtfDataFilter::k_EdaxHexagonalAlignment_Key, std::make_any<bool>(edaxHexagonalAlignment));
  args.insertOrAssign(ReadCtfDataFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(k_DataContainerPath));
  args.insertOrAssign(ReadCtfDataFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_CellData));
  args.insertOrAssign(ReadCtfDataFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_EnsembleAttributeMatrix));
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
// is hand-derived from the fixture text at the top of this file. Conversions
// are OFF so every stored value is a verbatim float32-exact copy.
//------------------------------------------------------------------------------
TEST_CASE("OrientationAnalysis::ReadCtfDataFilter: Class 1 Analytical Oracle", "[OrientationAnalysis][ReadCtfDataFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path inputCtfFile = WriteCtfFile("read_ctf_vv_oracle.ctf", MakeToyCtf());

  ReadCtfDataFilter filter;
  DataStructure dataStructure;
  Arguments args = MakeDefaultArgs(inputCtfFile);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath cellAMPath = k_DataContainerPath.createChildPath(k_CellData);
  const DataPath ensembleAMPath = k_DataContainerPath.createChildPath(k_EnsembleAttributeMatrix);

  // --- Geometry (Class 1: dims/spacing straight from the header; origin, z-extent
  // --- and units are the filter's hard-coded value-add for a 2D file) ---------
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(k_DataContainerPath));
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_DataContainerPath);
  {
    const SizeVec3 dims = imageGeom.getDimensions(); // XCells=3, YCells=2, no ZCells key -> 1 slice
    REQUIRE(dims == SizeVec3(3, 2, 1));
    const FloatVec3 spacing = imageGeom.getSpacing(); // XStep=0.25, YStep=0.5, no ZStep key -> 1.0
    REQUIRE(spacing == FloatVec3(0.25F, 0.5F, 1.0F));
    const FloatVec3 origin = imageGeom.getOrigin(); // hard-coded (0,0,0)
    REQUIRE(origin == FloatVec3(0.0F, 0.0F, 0.0F));
    REQUIRE(imageGeom.getUnits() == IGeometry::LengthUnit::Micrometer);
  }

  // --- Cell data (Class 1) ---------------------------------------------------
  // Phases: file column 1 is {1,2,0,1,2,0}, copied VERBATIM. Phase 0 (unindexed
  // point) is preserved — the legacy phase<1 -> 1 remap was deliberately removed.
  CompareArrayValues<int32>(dataStructure, cellAMPath.createChildPath("Phases"), {1, 2, 0, 1, 2, 0});

  // EulerAngles: file columns 6-8 interleaved per point; conversions off.
  CompareArrayValues<float32>(dataStructure, cellAMPath.createChildPath("EulerAngles"),
                              {0.125F, 0.25F, 0.375F, 0.5F, 0.625F, 0.75F, 0.875F, 1.0F, 1.125F, 1.25F, 1.375F, 7.125F, 3.375F, 6.75F, 1.875F, 2.0F, 2.125F, 2.25F});

  // Pass-through columns, copied verbatim from the file.
  CompareArrayValues<int32>(dataStructure, cellAMPath.createChildPath("Bands"), {7, 6, 0, 5, 4, 0});
  CompareArrayValues<int32>(dataStructure, cellAMPath.createChildPath("Error"), {0, 0, 3, 0, 0, 3});
  CompareArrayValues<float32>(dataStructure, cellAMPath.createChildPath("MAD"), {0.5F, 0.25F, 0.0F, 0.375F, 0.625F, 0.0F});
  CompareArrayValues<int32>(dataStructure, cellAMPath.createChildPath("BC"), {105, 120, 73, 134, 110, 68});
  CompareArrayValues<int32>(dataStructure, cellAMPath.createChildPath("BS"), {177, 209, 178, 232, 216, 151});
  CompareArrayValues<float32>(dataStructure, cellAMPath.createChildPath("X"), {0.0F, 0.25F, 0.5F, 0.0F, 0.25F, 0.5F});
  CompareArrayValues<float32>(dataStructure, cellAMPath.createChildPath("Y"), {0.0F, 0.0F, 0.0F, 0.5F, 0.5F, 0.5F});

  // --- Ensemble data ----------------------------------------------------------
  // Class 4 invariant: tuple count is always (number of phases in file) + 1.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(ensembleAMPath));
  const auto& ensembleAM = dataStructure.getDataRefAs<AttributeMatrix>(ensembleAMPath);
  REQUIRE(ensembleAM.getShape() == std::vector<usize>{3});

  // CrystalStructures (Class 1): slot 0 is the UnknownCrystalStructure default
  // (999); .ctf Laue group 9 (6/mmm) -> Hexagonal_High (0); Laue group 11
  // (m-3m) -> Cubic_High (1).
  CompareArrayValues<uint32>(dataStructure, ensembleAMPath.createChildPath("CrystalStructures"), {999, 0, 1});

  // LatticeConstants (Class 1): slot 0 zero-filled default; slots 1/2 copied
  // from the phase lines (a, b, c, alpha, beta, gamma). All float32-exact.
  CompareArrayValues<float32>(dataStructure, ensembleAMPath.createChildPath("LatticeConstants"),
                              {0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 2.5F, 2.5F, 4.25F, 90.0F, 90.0F, 120.0F, 3.625F, 3.625F, 3.625F, 90.0F, 90.0F, 90.0F});

  // MaterialName (Class 1): slot 0 is the "Invalid Phase" default; phase names
  // are tab-delimited tokens copied verbatim (embedded spaces preserved).
  {
    const DataPath materialNamePath = ensembleAMPath.createChildPath("MaterialName");
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(materialNamePath));
    const auto& materialNamesRef = dataStructure.getDataRefAs<StringArray>(materialNamePath);
    REQUIRE(materialNamesRef.getNumberOfTuples() == 3);
    REQUIRE(materialNamesRef[0] == "Invalid Phase");
    REQUIRE(materialNamesRef[1] == "Hex Phase A");
    REQUIRE(materialNamesRef[2] == "Copper");
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// The oracle covers DegreesToRadians and EdaxHexagonalAlignment combinations.
// The 30-degree adjustment applies only to Hexagonal_High points.
// Cubic and unindexed points are never shifted.
// Expected values use double precision before the final float32 conversion:
//   +30      : float32(float64(e3) + 30.0)
//   radians  : float32(float64(e) * (pi/180))
//------------------------------------------------------------------------------
TEST_CASE("OrientationAnalysis::ReadCtfDataFilter: Euler Conversion Combinations", "[OrientationAnalysis][ReadCtfDataFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path inputCtfFile = WriteCtfFile("read_ctf_vv_conversions.ctf", MakeToyCtf());

  struct ConversionCase
  {
    std::string name;
    bool degreesToRadians;
    bool edaxHexagonalAlignment;
    std::vector<float32> expectedEulerAngles;
  };

  const std::vector<ConversionCase> cases = {
      {"Degrees, No Hex Alignment", false, false, {0.125F, 0.25F, 0.375F, 0.5F, 0.625F, 0.75F, 0.875F, 1.0F, 1.125F, 1.25F, 1.375F, 7.125F, 3.375F, 6.75F, 1.875F, 2.0F, 2.125F, 2.25F}},
      // 0.375 + 30 = 30.375 and 7.125 + 30 = 37.125 are exact in float32.
      {"Degrees, Hex Alignment", false, true, {0.125F, 0.25F, 30.375F, 0.5F, 0.625F, 0.75F, 0.875F, 1.0F, 1.125F, 1.25F, 1.375F, 37.125F, 3.375F, 6.75F, 1.875F, 2.0F, 2.125F, 2.25F}},
      // These values distinguish double-intermediate conversion from direct float32 arithmetic.
      {"Radians, No Hex Alignment",
       true,
       false,
       {0.0021816615F, 0.004363323F, 0.006544985F, 0.008726646F, 0.0109083075F, 0.01308997F, 0.015271631F, 0.017453292F, 0.019634955F, 0.021816615F, 0.023998277F, 0.12435471F, 0.058904864F,
        0.11780973F, 0.032724924F, 0.034906585F, 0.037088245F, 0.03926991F}},
      // Points 0/3 (hex): float32(float64(30.375) * pi/180) = 0.53014374,
      //                   float32(float64(37.125) * pi/180) = 0.6479535 (float32 arithmetic: 0.64795345).
      {"Radians, Hex Alignment",
       true,
       true,
       {0.0021816615F, 0.004363323F, 0.53014374F, 0.008726646F, 0.0109083075F, 0.01308997F, 0.015271631F, 0.017453292F, 0.019634955F, 0.021816615F, 0.023998277F, 0.6479535F, 0.058904864F, 0.11780973F,
        0.032724924F, 0.034906585F, 0.037088245F, 0.03926991F}},
  };

  for(const ConversionCase& conversionCase : cases)
  {
    DYNAMIC_SECTION(conversionCase.name)
    {
      ReadCtfDataFilter filter;
      DataStructure dataStructure;
      Arguments args = MakeDefaultArgs(inputCtfFile, conversionCase.degreesToRadians, conversionCase.edaxHexagonalAlignment);

      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

      auto executeResult = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

      const DataPath cellAMPath = k_DataContainerPath.createChildPath(k_CellData);
      CompareArrayValues<float32>(dataStructure, cellAMPath.createChildPath("EulerAngles"), conversionCase.expectedEulerAngles);
      // The Phase column is never altered by either conversion option.
      CompareArrayValues<int32>(dataStructure, cellAMPath.createChildPath("Phases"), {1, 2, 0, 1, 2, 0});

      UnitTest::CheckArraysInheritTupleDims(dataStructure);
    }
  }
}

// Multi-slice files use ZCells for the geometry extent and ZStep for thickness.
// Missing or zero ZStep uses thickness 1. Data rows remain slice-major.
TEST_CASE("OrientationAnalysis::ReadCtfDataFilter: 3D Multi-Slice CTF", "[OrientationAnalysis][ReadCtfDataFilter]")
{
  UnitTest::LoadPlugins();

  // 2 cols x 2 rows x 2 slices, single cubic phase. 8 data rows in slice-major order.
  const std::string dataBlock3D = "1\t0\t0\t7\t0\t0.125\t0.25\t0.375\t0.5\t105\t177\n"
                                  "1\t0.25\t0\t6\t0\t0.5\t0.625\t0.75\t0.25\t120\t209\n"
                                  "0\t0\t0.5\t0\t3\t0.875\t1\t1.125\t0\t73\t178\n"
                                  "1\t0.25\t0.5\t5\t0\t1.25\t1.375\t1.5\t0.375\t134\t232\n"
                                  "1\t0\t0\t4\t0\t1.625\t1.75\t1.875\t0.625\t110\t216\n"
                                  "0\t0.25\t0\t0\t3\t2\t2.125\t2.25\t0\t68\t151\n"
                                  "1\t0\t0.5\t3\t0\t2.375\t2.5\t2.625\t0.125\t99\t142\n"
                                  "1\t0.25\t0.5\t2\t0\t2.75\t2.875\t3\t0.875\t88\t123\n";
  const std::string cubicPhaseBlock = "Phases\t1\n"
                                      "3.625;3.625;3.625\t90;90;90\tCopper\t11\tVV toy cubic phase\n";

  struct SliceCase
  {
    std::string name;
    std::string extraHeaderLines;
    float32 expectedZSpacing;
  };
  const std::vector<SliceCase> cases = {
      {"ZStep 0.75", "ZCells\t2\nZStep\t0.75\n", 0.75F},
      // A ZCells key without a usable ZStep (0) falls back to a slice thickness of 1.
      {"ZStep absent", "ZCells\t2\n", 1.0F},
  };

  for(const SliceCase& sliceCase : cases)
  {
    DYNAMIC_SECTION(sliceCase.name)
    {
      const fs::path inputCtfFile = WriteCtfFile("read_ctf_vv_3d.ctf", MakeToyCtf(sliceCase.extraHeaderLines, cubicPhaseBlock, k_ColumnHeader, dataBlock3D, 2, 2));

      ReadCtfDataFilter filter;
      DataStructure dataStructure;
      Arguments args = MakeDefaultArgs(inputCtfFile);

      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

      auto executeResult = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

      REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(k_DataContainerPath));
      const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(k_DataContainerPath);
      REQUIRE(imageGeom.getDimensions() == SizeVec3(2, 2, 2));
      REQUIRE(imageGeom.getSpacing() == FloatVec3(0.25F, 0.5F, sliceCase.expectedZSpacing));

      const DataPath cellAMPath = k_DataContainerPath.createChildPath(k_CellData);
      CompareArrayValues<int32>(dataStructure, cellAMPath.createChildPath("Phases"), {1, 1, 0, 1, 1, 0, 1, 1});
      CompareArrayValues<int32>(dataStructure, cellAMPath.createChildPath("Bands"), {7, 6, 0, 5, 4, 0, 3, 2});
      CompareArrayValues<float32>(dataStructure, cellAMPath.createChildPath("EulerAngles"), {0.125F, 0.25F, 0.375F, 0.5F, 0.625F, 0.75F, 0.875F, 1.0F, 1.125F, 1.25F, 1.375F, 1.5F,
                                                                                             1.625F, 1.75F, 1.875F, 2.0F, 2.125F, 2.25F, 2.375F, 2.5F, 2.625F, 2.75F, 2.875F, 3.0F});
      CompareArrayValues<float32>(dataStructure, cellAMPath.createChildPath("MAD"), {0.5F, 0.25F, 0.0F, 0.375F, 0.625F, 0.0F, 0.125F, 0.875F});

      UnitTest::CheckArraysInheritTupleDims(dataStructure);
    }
  }
}

// A header with no phases has no usable ensemble information.
// Execution must return -19600 after preflight accepts the header.
TEST_CASE("OrientationAnalysis::ReadCtfDataFilter: Empty Phases rejected (-19600)", "[OrientationAnalysis][ReadCtfDataFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path inputCtfFile = WriteCtfFile("read_ctf_vv_no_phase.ctf", MakeToyCtf("", k_ZeroPhaseBlock));

  ReadCtfDataFilter filter;
  DataStructure dataStructure;
  Arguments args = MakeDefaultArgs(inputCtfFile);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -19600);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// A data section missing a standard column must return -19601 at execution.
TEST_CASE("OrientationAnalysis::ReadCtfDataFilter: Missing Data Column rejected (-19601)", "[OrientationAnalysis][ReadCtfDataFilter]")
{
  UnitTest::LoadPlugins();

  const std::string columnHeaderNoBS = "Phase\tX\tY\tBands\tError\tEuler1\tEuler2\tEuler3\tMAD\tBC\n";
  const std::string dataBlockNoBS = "1\t0\t0\t7\t0\t0.125\t0.25\t0.375\t0.5\t105\n"
                                    "2\t0.25\t0\t6\t0\t0.5\t0.625\t0.75\t0.25\t120\n"
                                    "0\t0.5\t0\t0\t3\t0.875\t1\t1.125\t0\t73\n"
                                    "1\t0\t0.5\t5\t0\t1.25\t1.375\t1.5\t0.375\t134\n"
                                    "2\t0.25\t0.5\t4\t0\t1.625\t1.75\t1.875\t0.625\t110\n"
                                    "0\t0.5\t0.5\t0\t3\t2\t2.125\t2.25\t0\t68\n";
  const fs::path inputCtfFile = WriteCtfFile("read_ctf_vv_missing_column.ctf", MakeToyCtf("", k_TwoPhaseBlock, columnHeaderNoBS, dataBlockNoBS));

  ReadCtfDataFilter filter;
  DataStructure dataStructure;
  Arguments args = MakeDefaultArgs(inputCtfFile);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -19601);
  // The missing column's name is the user-facing value of this error message.
  REQUIRE(executeResult.result.errors()[0].message.find("'BS'") != std::string::npos);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// A data-row phase outside the header range must return -19602.
// With two phases, identifier 3 is the first invalid value.
TEST_CASE("OrientationAnalysis::ReadCtfDataFilter: Out-of-Range Phase Value rejected (-19602)", "[OrientationAnalysis][ReadCtfDataFilter]")
{
  UnitTest::LoadPlugins();

  std::string corruptDataBlock = k_DataBlock;
  corruptDataBlock.replace(0, 1, "3"); // First data row's Phase column: 1 -> 3 (first out-of-range value)
  const fs::path inputCtfFile = WriteCtfFile("read_ctf_vv_phase_range.ctf", MakeToyCtf("", k_TwoPhaseBlock, k_ColumnHeader, corruptDataBlock));

  ReadCtfDataFilter filter;
  DataStructure dataStructure;
  Arguments args = MakeDefaultArgs(inputCtfFile);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -19602);
  // The offending value and the valid range are the user-facing value of this error message.
  REQUIRE(executeResult.result.errors()[0].message.find("phase value 3") != std::string::npos);
  REQUIRE(executeResult.result.errors()[0].message.find("[0, 2]") != std::string::npos);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

//------------------------------------------------------------------------------
// Value-add error path: a "ZCells 0" header makes CtfReader::readData() succeed
// with ZERO elements (its z loop never runs), while the preflight geometry is
// sized x*y*1. The cell-count guard rejects the mismatch at execute with -19603
// instead of reading past the end of the reader's (empty) buffers.
//------------------------------------------------------------------------------
TEST_CASE("OrientationAnalysis::ReadCtfDataFilter: Reader/Geometry Cell Count Mismatch rejected (-19603)", "[OrientationAnalysis][ReadCtfDataFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path inputCtfFile = WriteCtfFile("read_ctf_vv_zero_zcells.ctf", MakeToyCtf("ZCells\t0\n"));

  ReadCtfDataFilter filter;
  DataStructure dataStructure;
  Arguments args = MakeDefaultArgs(inputCtfFile);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -19603);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

//------------------------------------------------------------------------------
// Value-add error path: CtfReader::readHeaderOnly() reports success even when
// the header has no usable dimensions (a missing or zero XCells/YCells key
// parses as 0, and a negative ZCells would make readData() read zero data
// lines), so preflight rejects those files with -19604.
//------------------------------------------------------------------------------
TEST_CASE("OrientationAnalysis::ReadCtfDataFilter: Invalid Cells Preflight Error (-19604)", "[OrientationAnalysis][ReadCtfDataFilter]")
{
  UnitTest::LoadPlugins();

  const std::vector<std::pair<std::string, std::string>> cases = {
      {"Zero XCells", MakeToyCtf("", k_TwoPhaseBlock, k_ColumnHeader, k_DataBlock, 0, 2)},
      {"Negative ZCells", MakeToyCtf("ZCells\t-2\n")},
  };

  for(const auto& [label, contents] : cases)
  {
    DYNAMIC_SECTION(label)
    {
      const fs::path inputCtfFile = WriteCtfFile("read_ctf_vv_invalid_cells.ctf", contents);

      ReadCtfDataFilter filter;
      DataStructure dataStructure;
      Arguments args = MakeDefaultArgs(inputCtfFile);

      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
      REQUIRE(preflightResult.outputActions.errors()[0].code == -19604);

      UnitTest::CheckArraysInheritTupleDims(dataStructure);
    }
  }
}

//------------------------------------------------------------------------------
// EbsdLib error passthrough: a zero X Step is accepted at preflight (header-only
// read performs no step validation) but CtfReader::readFile() rejects it at
// execute. CtfReader only sets its error MESSAGE on this path (the error code
// member stays 0), so the filter reports the readFile() return value: -102.
//------------------------------------------------------------------------------
TEST_CASE("OrientationAnalysis::ReadCtfDataFilter: EbsdLib Error Passthrough - Zero Step (-102)", "[OrientationAnalysis][ReadCtfDataFilter]")
{
  UnitTest::LoadPlugins();

  std::string contents = MakeToyCtf();
  const std::string xStepLine = "XStep\t0.25\n";
  contents.replace(contents.find(xStepLine), xStepLine.size(), "XStep\t0\n");
  const fs::path inputCtfFile = WriteCtfFile("read_ctf_vv_zero_step.ctf", contents);

  ReadCtfDataFilter filter;
  DataStructure dataStructure;
  Arguments args = MakeDefaultArgs(inputCtfFile);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -102);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

//------------------------------------------------------------------------------
// EbsdLib error passthrough: the header declares 3x2 points but the data block
// is truncated to 4 rows; CtfReader reports -105 (premature end of file).
//------------------------------------------------------------------------------
TEST_CASE("OrientationAnalysis::ReadCtfDataFilter: EbsdLib Error Passthrough - Truncated Data (-105)", "[OrientationAnalysis][ReadCtfDataFilter]")
{
  UnitTest::LoadPlugins();

  // Keep only the first 4 of 6 data rows.
  std::string truncatedData = k_DataBlock;
  for(int i = 0; i < 2; i++)
  {
    truncatedData.erase(truncatedData.find_last_of('\n', truncatedData.size() - 2) + 1);
  }
  const fs::path inputCtfFile = WriteCtfFile("read_ctf_vv_truncated.ctf", MakeToyCtf("", k_TwoPhaseBlock, k_ColumnHeader, truncatedData));

  ReadCtfDataFilter filter;
  DataStructure dataStructure;
  Arguments args = MakeDefaultArgs(inputCtfFile);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -105);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

//------------------------------------------------------------------------------
// EbsdLib error passthrough: an unrecognized data-column name is rejected when
// CtfReader allocates its column buffers; the filter reports -107.
//------------------------------------------------------------------------------
TEST_CASE("OrientationAnalysis::ReadCtfDataFilter: EbsdLib Error Passthrough - Unknown Column (-107)", "[OrientationAnalysis][ReadCtfDataFilter]")
{
  UnitTest::LoadPlugins();

  const std::string bogusColumnHeader = "Phase\tX\tY\tBands\tError\tEuler1\tEuler2\tEuler3\tMAD\tBC\tBogus\n";
  const fs::path inputCtfFile = WriteCtfFile("read_ctf_vv_unknown_column.ctf", MakeToyCtf("", k_TwoPhaseBlock, bogusColumnHeader, k_DataBlock));

  ReadCtfDataFilter filter;
  DataStructure dataStructure;
  Arguments args = MakeDefaultArgs(inputCtfFile);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -107);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

//------------------------------------------------------------------------------
// SIMPL Backwards Compatibility — validates UUID + parameter conversion from
// the legacy ReadCtfData (SIMPL UUID d1df969c-0428-53c3-b61d-99ea2bb6da28).
//------------------------------------------------------------------------------
TEST_CASE("OrientationAnalysis::ReadCtfDataFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ReadCtfDataFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ReadCtfDataFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ReadCtfDataFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ReadCtfDataFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<FileSystemPathParameter::ValueType>(ReadCtfDataFilter::k_InputFile_Key) == fs::path("/some/test/path/data.ctf"));
      CHECK(args.value<bool>(ReadCtfDataFilter::k_DegreesToRadians_Key) == true);
      CHECK(args.value<bool>(ReadCtfDataFilter::k_EdaxHexagonalAlignment_Key) == false);
      CHECK(args.value<DataPath>(ReadCtfDataFilter::k_CreatedImageGeometryPath_Key) == DataPath({"ImageDataContainer"}));
      CHECK(args.value<std::string>(ReadCtfDataFilter::k_CellAttributeMatrixName_Key) == "CellData");
      CHECK(args.value<std::string>(ReadCtfDataFilter::k_CellEnsembleAttributeMatrixName_Key) == "CellEnsembleData");
    }
  }
}
