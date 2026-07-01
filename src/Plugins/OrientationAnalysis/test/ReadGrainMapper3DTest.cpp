#include <catch2/catch.hpp>

#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/ReadGrainMapper3DFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
const std::string k_LabDCTGeometryName("LabDCT");
const std::string k_AbsorptionCTGeometryName("AbsorptionCT");

} // namespace

TEST_CASE("OrientationAnalysis::ReadGrainMapper3D:Default_Parameters", "[OrientationAnalysis][ReadGrainMapper3D]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "GrainMapper3D_Test_Files.tar.gz", "GrainMapper3D_Test_Files");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/GrainMapper3D_Test_Files/7_0_SimulatedMultiPhase.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  DataPath computedDCTGeometryPath({fmt::format("{} computed", k_LabDCTGeometryName)});
  DataPath computedAbsorptionCTGeometryPath({fmt::format("{} computed", k_AbsorptionCTGeometryName)});

  DataPath exemplarDCTGeometryPath({fmt::format("{} (default)", k_LabDCTGeometryName)});
  DataPath exemplarAbsorptionCTGeometryPath({fmt::format("{} (default)", k_AbsorptionCTGeometryName)});

  auto inputGM3DFilePath = fs::path(fmt::format("{}/GrainMapper3D_Test_Files/SimulatedMultiPhase.h5", unit_test::k_TestFilesDir));
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ReadGrainMapper3DFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    // Create default Parameters for the filter.
    args.insertOrAssign(ReadGrainMapper3DFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputGM3DFilePath)));
    args.insertOrAssign(ReadGrainMapper3DFilter::k_ReadLabDCT_Key, std::make_any<bool>(true));
    args.insertOrAssign(ReadGrainMapper3DFilter::k_CreatedDCTImageGeometryPath_Key, std::make_any<DataPath>(computedDCTGeometryPath));
    args.insertOrAssign(ReadGrainMapper3DFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Data));
    args.insertOrAssign(ReadGrainMapper3DFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Ensemble_Data));

    args.insertOrAssign(ReadGrainMapper3DFilter::k_ConvertPhaseToInt32_Key, std::make_any<bool>(true));
    args.insertOrAssign(ReadGrainMapper3DFilter::k_ConvertOrientationData_Key, std::make_any<bool>(true));

    args.insertOrAssign(ReadGrainMapper3DFilter::k_ReadAbsorptionCT_Key, std::make_any<bool>(true));
    args.insertOrAssign(ReadGrainMapper3DFilter::k_CreatedAbsorptionGeometryPath_Key, std::make_any<DataPath>(computedAbsorptionCTGeometryPath));
    args.insertOrAssign(ReadGrainMapper3DFilter::k_CellAbsorptionAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Data));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/read_grainmapper_3d_default.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CompareImageGeometry(dataStructure, exemplarDCTGeometryPath, computedDCTGeometryPath);
  UnitTest::CompareImageGeometry(dataStructure, exemplarAbsorptionCTGeometryPath, computedAbsorptionCTGeometryPath);

  UnitTest::CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarDCTGeometryPath.createChildPath(k_Cell_Data), dataStructure, computedDCTGeometryPath.createChildPath(k_Cell_Data));
  UnitTest::CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarAbsorptionCTGeometryPath.createChildPath(k_Cell_Data), dataStructure,
                                                     computedAbsorptionCTGeometryPath.createChildPath(k_Cell_Data));
  UnitTest::CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarDCTGeometryPath.createChildPath(k_Cell_Ensemble_Data), dataStructure,
                                                     computedDCTGeometryPath.createChildPath(k_Cell_Ensemble_Data));

  // The exemplar .dream3d predates the UniversalHermannMauguin ensemble array and therefore cannot be used
  // to compare it. Verify the parsed values directly against the known phase metadata so a regression in the
  // dataset that is read (UniversalHermannMauguin vs. Name) is caught. Index 0 is the reserved invalid phase.
  {
    const std::vector<std::string> expectedHermannMauguin = {
        "Invalid Phase",    "F d -3 m :2 (a-1/8,b-1/8,c-1/8)", "F d -3 m :2 (a-1/8,b-1/8,c-1/8)", "P 63/m m c", "P 42/m n m", "R -3 c :H (-y+z,x+z,-x+y+z)", "P 32 2 1", "P b c n", "C 1 2/m 1",
        "P -1 (a+b,a-b,-c)"};
    const DataPath hermannMauguinPath = computedDCTGeometryPath.createChildPath(k_Cell_Ensemble_Data).createChildPath("UniversalHermannMauguin");
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(hermannMauguinPath));
    const auto& hermannMauguinArray = dataStructure.getDataRefAs<StringArray>(hermannMauguinPath);
    REQUIRE(hermannMauguinArray.getNumberOfTuples() == expectedHermannMauguin.size());
    for(usize i = 0; i < expectedHermannMauguin.size(); i++)
    {
      CAPTURE(i);
      REQUIRE(hermannMauguinArray[i] == expectedHermannMauguin[i]);
    }
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ReadGrainMapper3D:NonCompatible_Parameters", "[OrientationAnalysis][ReadGrainMapper3D]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "GrainMapper3D_Test_Files.tar.gz", "GrainMapper3D_Test_Files");

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/GrainMapper3D_Test_Files/7_0_SimulatedMultiPhase.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  DataPath computedDCTGeometryPath({fmt::format("{} computed", k_LabDCTGeometryName)});
  DataPath computedAbsorptionCTGeometryPath({fmt::format("{} computed", k_AbsorptionCTGeometryName)});

  DataPath exemplarDCTGeometryPath({fmt::format("{} (non-compatible)", k_LabDCTGeometryName)});
  DataPath exemplarAbsorptionCTGeometryPath({fmt::format("{} (non-compatible)", k_AbsorptionCTGeometryName)});

  auto inputGM3DFilePath = fs::path(fmt::format("{}/GrainMapper3D_Test_Files/SimulatedMultiPhase.h5", unit_test::k_TestFilesDir));
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ReadGrainMapper3DFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    // Create default Parameters for the filter.
    args.insertOrAssign(ReadGrainMapper3DFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputGM3DFilePath)));
    args.insertOrAssign(ReadGrainMapper3DFilter::k_ReadLabDCT_Key, std::make_any<bool>(true));
    args.insertOrAssign(ReadGrainMapper3DFilter::k_CreatedDCTImageGeometryPath_Key, std::make_any<DataPath>(computedDCTGeometryPath));
    args.insertOrAssign(ReadGrainMapper3DFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Data));
    args.insertOrAssign(ReadGrainMapper3DFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Ensemble_Data));

    args.insertOrAssign(ReadGrainMapper3DFilter::k_ConvertPhaseToInt32_Key, std::make_any<bool>(false));
    args.insertOrAssign(ReadGrainMapper3DFilter::k_ConvertOrientationData_Key, std::make_any<bool>(false));

    args.insertOrAssign(ReadGrainMapper3DFilter::k_ReadAbsorptionCT_Key, std::make_any<bool>(true));
    args.insertOrAssign(ReadGrainMapper3DFilter::k_CreatedAbsorptionGeometryPath_Key, std::make_any<DataPath>(computedAbsorptionCTGeometryPath));
    args.insertOrAssign(ReadGrainMapper3DFilter::k_CellAbsorptionAttributeMatrixName_Key, std::make_any<std::string>(k_Cell_Data));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }
// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/read_grainmapper_3d_non_compatible.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CompareImageGeometry(dataStructure, exemplarDCTGeometryPath, computedDCTGeometryPath);
  UnitTest::CompareImageGeometry(dataStructure, exemplarAbsorptionCTGeometryPath, computedAbsorptionCTGeometryPath);

  UnitTest::CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarDCTGeometryPath.createChildPath(k_Cell_Data), dataStructure, computedDCTGeometryPath.createChildPath(k_Cell_Data));
  UnitTest::CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarAbsorptionCTGeometryPath.createChildPath(k_Cell_Data), dataStructure,
                                                     computedAbsorptionCTGeometryPath.createChildPath(k_Cell_Data));
  UnitTest::CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarDCTGeometryPath.createChildPath(k_Cell_Ensemble_Data), dataStructure,
                                                     computedDCTGeometryPath.createChildPath(k_Cell_Ensemble_Data));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
