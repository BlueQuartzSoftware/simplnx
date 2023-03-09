#include <catch2/catch.hpp>

#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;

#include "OrientationAnalysis/Filters/EnsembleInfoReaderFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace complex;
using namespace unit_test;

namespace
{
inline const std::string k_TestFileIni = k_BinaryTestOutputDir.str() + "/EnsembleInfoTest.ini";
inline const std::string k_TestFileTxt = k_BinaryTestOutputDir.str() + "/EnsembleInfoTest.txt";
inline const std::string k_TestFileDoc = k_BinaryTestOutputDir.str() + "/EnsembleInfoTest.doc";

// -----------------------------------------------------------------------------
void WriteTestFile(const std::string& filePath, const std::string& groups)
{
  std::ofstream ofs(filePath);
  REQUIRE(ofs.good());
  ofs << groups;
  ofs.close();

  // FILE* f = nullptr;
  // f = fopen(filePath.data(), "wb");
  // if(nullptr == f)
  //{
  //   REQUIRE(0 == 1);
  // }
  // fprintf(f, "%s", groups.data());
  // fclose(f);
}
} // namespace

TEST_CASE("OrientationAnalysis::EnsembleInfoReaderFilter: Valid Filter Execution", "[OrientationAnalysis][EnsembleInfoReaderFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  EnsembleInfoReaderFilter filter;
  DataStructure ds;
  DataGroup* topLevelGroup = DataGroup::Create(ds, Constants::k_DataContainer);
  Arguments args;
  args.insertOrAssign(EnsembleInfoReaderFilter::k_DataContainerName_Key, std::make_any<DataPath>(DataPath({Constants::k_DataContainer})));
  args.insertOrAssign(EnsembleInfoReaderFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_EnsembleAttributeMatrix));
  args.insertOrAssign(EnsembleInfoReaderFilter::k_CrystalStructuresArrayName_Key, std::make_any<std::string>(Constants::k_CrystalStructures));
  args.insertOrAssign(EnsembleInfoReaderFilter::k_PhaseTypesArrayName_Key, std::make_any<std::string>(Constants::k_Phases));

  std::string groups = "[EnsembleInfo]\nNumber_Phases = 2\n\n\
          [1]\nCrystalStructure = Cubic_High\nPhaseType = PrimaryPhase\n\n\
          [2]\nCrystalStructure = Hexagonal_High\nPhaseType = MatrixPhase\n"; // the correct groups and keys for writing the .ini file

  SECTION("txt extension")
  {
    WriteTestFile(k_TestFileTxt, groups);
    args.insertOrAssign(EnsembleInfoReaderFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(k_TestFileTxt)));
  }
  SECTION("ini extension")
  {
    WriteTestFile(k_TestFileIni, groups);
    args.insertOrAssign(EnsembleInfoReaderFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(k_TestFileIni)));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  const auto* crystalStructures = ds.getDataAs<UInt32Array>(DataPath({Constants::k_DataContainer, Constants::k_EnsembleAttributeMatrix, Constants::k_CrystalStructures}));
  REQUIRE(crystalStructures != nullptr);
  REQUIRE(crystalStructures->getNumberOfTuples() == 3);
  REQUIRE((*crystalStructures)[0] == 999);
  REQUIRE((*crystalStructures)[1] == 1);
  REQUIRE((*crystalStructures)[2] == 0);

  const auto* phaseTypes = ds.getDataAs<UInt32Array>(DataPath({Constants::k_DataContainer, Constants::k_EnsembleAttributeMatrix, Constants::k_Phases}));
  REQUIRE(phaseTypes != nullptr);
  REQUIRE(phaseTypes->getNumberOfTuples() == 3);
  REQUIRE((*phaseTypes)[0] == 999);
  REQUIRE((*phaseTypes)[1] == 0);
  REQUIRE((*phaseTypes)[2] == 3);
}

TEST_CASE("OrientationAnalysis::EnsembleInfoReaderFilter: InValid Filter Execution")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  EnsembleInfoReaderFilter filter;
  DataStructure ds;
  DataGroup* topLevelGroup = DataGroup::Create(ds, Constants::k_DataContainer);
  Arguments args;
  args.insertOrAssign(EnsembleInfoReaderFilter::k_DataContainerName_Key, std::make_any<DataPath>(DataPath({Constants::k_DataContainer})));
  args.insertOrAssign(EnsembleInfoReaderFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_EnsembleAttributeMatrix));
  args.insertOrAssign(EnsembleInfoReaderFilter::k_CrystalStructuresArrayName_Key, std::make_any<std::string>(Constants::k_CrystalStructures));
  args.insertOrAssign(EnsembleInfoReaderFilter::k_PhaseTypesArrayName_Key, std::make_any<std::string>(Constants::k_Phases));

  SECTION("doc extension")
  {
    std::string groups = "[EnsembleInfo]\nNumber_Phases = 2\n\n\
          [1]\nCrystalStructure = Cubic_High\nPhaseType = PrimaryPhase\n\n\
          [2]\nCrystalStructure = Hexagonal_High\nPhaseType = MatrixPhase\n"; // the correct groups and keys for writing the .ini file
    WriteTestFile(k_TestFileDoc, groups);
    args.insertOrAssign(EnsembleInfoReaderFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(k_TestFileDoc)));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  }
  SECTION("EnsembleInfo misspelled")
  {
    std::string groups = "[EnsembleInf]\nNumber_Phases = 2\n\n[1]\nCrystalStructure = Cubic_High\nPhaseType = PrimaryPhase\n\n[2]\nCrystalStructure = Hexagonal_High\nPhaseType = MatrixPhase\n";
    WriteTestFile(k_TestFileIni, groups);
    args.insertOrAssign(EnsembleInfoReaderFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(k_TestFileIni)));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  }
  SECTION("NumberPhases missing")
  {
    std::string groups = "[EnsembleInfo]\n\n[1]\nCrystalStructure = Cubic_High\nPhaseType = PrimaryPhase\n\n[2]\nCrystalStructure = Hexagonal_High\nPhaseType = MatrixPhase\n";
    WriteTestFile(k_TestFileIni, groups);
    args.insertOrAssign(EnsembleInfoReaderFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(k_TestFileIni)));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  }
  SECTION("Wrong number of phases")
  {
    std::string groups = "[EnsembleInfo]\nNumber_Phases = 0\n\n[1]\nCrystalStructure = Cubic_High\nPhaseType = PrimaryPhase\n\n[2]\nCrystalStructure = Hexagonal_High\nPhaseType = MatrixPhase\n";
    WriteTestFile(k_TestFileIni, groups);
    args.insertOrAssign(EnsembleInfoReaderFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(k_TestFileIni)));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
  }
  SECTION("PhaseType missing")
  {
    std::string groups = "[EnsembleInfo]\nNumber_Phases = 2\n\n[1]\nCrystalStructure = Cubic_High\n\n\n[2]\nCrystalStructure = Hexagonal_High\nPhaseType = MatrixPhase\n";
    WriteTestFile(k_TestFileIni, groups);
    args.insertOrAssign(EnsembleInfoReaderFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(k_TestFileIni)));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  }
  SECTION("CrystalStructure missing")
  {
    std::string groups = "[EnsembleInfo]\nNumber_Phases = 2\n\n[1]\nCrystalStructure = Cubic_High\nPhaseType = PrimaryPhase\n\n[2]\n\nPhaseType = MatrixPhase\n";
    WriteTestFile(k_TestFileIni, groups);
    args.insertOrAssign(EnsembleInfoReaderFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(k_TestFileIni)));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  }
  SECTION("PrimaryPhase misspelled")
  {
    std::string groups = "[EnsembleInfo]\nNumber_Phases = 2\n\n[1]\nCrystalStructure = Cubic_High\nPhaseType = PrimaryPase\n\n[2]\nCrystalStructure = Hexagonal_High\nPhaseType = MatrixPhase\n";
    WriteTestFile(k_TestFileIni, groups);
    args.insertOrAssign(EnsembleInfoReaderFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(k_TestFileIni)));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  }
  SECTION("Cubic misspelled")
  {
    std::string groups = "[EnsembleInfo]\nNumber_Phases = 2\n\n[1]\nCrystalStructure = Cubc\nPhaseType = PrimaryPhase\n\n[2]\nCrystalStructure = Hexagonal_High\nPhaseType = MatrixPhase\n";
    WriteTestFile(k_TestFileIni, groups);
    args.insertOrAssign(EnsembleInfoReaderFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(k_TestFileIni)));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  }
  SECTION("Incorrect group numbers")
  {
    std::string groups = "[EnsembleInfo]\nNumber_Phases = 2\n\n[1]\nCrystalStructure = Cubic_High\nPhaseType = PrimaryPhase\n\n[3]\nCrystalStructure = Hexagonal_High\nPhaseType = MatrixPhase\n";
    WriteTestFile(k_TestFileIni, groups);
    args.insertOrAssign(EnsembleInfoReaderFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(k_TestFileIni)));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  }

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_INVALID(executeResult.result)
}
