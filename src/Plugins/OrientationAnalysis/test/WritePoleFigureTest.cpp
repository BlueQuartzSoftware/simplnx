
#include <catch2/catch.hpp>

#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "OrientationAnalysis/Filters/WritePoleFigureFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace complex;
using namespace complex::UnitTest;

namespace
{
const std::string k_ImagePrefix("fw-ar-IF1-aptr12-corr Discrete Pole Figure");
}

TEST_CASE("OrientationAnalysis::WritePoleFigureFilter-1", "[OrientationAnalysis][WritePoleFigureFilter]")
{
  const std::string kDataInputArchive = "PoleFigure_Exemplars.tar.gz";
  const std::string kExpectedOutputTopLevel = "PoleFigure_Exemplars";
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, kDataInputArchive, kExpectedOutputTopLevel,
                                                             complex::unit_test::k_BinaryTestOutputDir);

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/PoleFigure_Exemplars/fw-ar-IF1-aptr12-corr.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  WritePoleFigureFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(WritePoleFigureFilter::k_Title_Key, std::make_any<StringParameter::ValueType>("fw-ar-IF1-aptr12-corr Discrete Pole Figure"));
  args.insertOrAssign(WritePoleFigureFilter::k_LambertSize_Key, std::make_any<int32>(64));
  args.insertOrAssign(WritePoleFigureFilter::k_NumColors_Key, std::make_any<int32>(32));
  args.insertOrAssign(WritePoleFigureFilter::k_GenerationAlgorithm_Key, std::make_any<ChoicesParameter::ValueType>(1));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageLayout_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(WritePoleFigureFilter::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(fmt::format("{}/Dir1/Dir2", unit_test::k_BinaryTestOutputDir))));
  args.insertOrAssign(WritePoleFigureFilter::k_ImagePrefix_Key, std::make_any<StringParameter::ValueType>(k_ImagePrefix));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageSize_Key, std::make_any<int32>(1024));
  args.insertOrAssign(WritePoleFigureFilter::k_SaveAsImageGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(WritePoleFigureFilter::k_WriteImageToDisk, std::make_any<bool>(true));
  args.insertOrAssign(WritePoleFigureFilter::k_UseGoodVoxels_Key, std::make_any<bool>(false));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr Discrete Pole Figure [CALCULATED]"})));

  DataPath calculatedImageData({"fw-ar-IF1-aptr12-corr Discrete Pole Figure [CALCULATED]", "CellData", fmt::format("{}Phase_{}", k_ImagePrefix, 1)});
  DataPath exemplarImageData({"fw-ar-IF1-aptr12-corr Discrete Pole Figure", "CellData", "Image"});

  args.insertOrAssign(WritePoleFigureFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "Cell Data", "EulerAngles"})));
  args.insertOrAssign(WritePoleFigureFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "Cell Data", "Phases"})));
  args.insertOrAssign(WritePoleFigureFilter::k_GoodVoxelsArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "Cell Data", "ThresholdArray"})));
  args.insertOrAssign(WritePoleFigureFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "CellEnsembleData", "CrystalStructures"})));
  args.insertOrAssign(WritePoleFigureFilter::k_MaterialNameArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "CellEnsembleData", "MaterialName"})));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/write_pole_figure-1.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  CompareArrays<uint8>(dataStructure, exemplarImageData, calculatedImageData);
}

TEST_CASE("OrientationAnalysis::WritePoleFigureFilter-2", "[OrientationAnalysis][WritePoleFigureFilter]")
{
  const std::string kDataInputArchive = "PoleFigure_Exemplars.tar.gz";
  const std::string kExpectedOutputTopLevel = "PoleFigure_Exemplars";
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, kDataInputArchive, kExpectedOutputTopLevel,
                                                             complex::unit_test::k_BinaryTestOutputDir);

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/PoleFigure_Exemplars/fw-ar-IF1-aptr12-corr.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  WritePoleFigureFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(WritePoleFigureFilter::k_Title_Key, std::make_any<StringParameter::ValueType>("fw-ar-IF1-aptr12-corr Discrete Pole Figure Masked"));
  args.insertOrAssign(WritePoleFigureFilter::k_LambertSize_Key, std::make_any<int32>(64));
  args.insertOrAssign(WritePoleFigureFilter::k_NumColors_Key, std::make_any<int32>(32));
  args.insertOrAssign(WritePoleFigureFilter::k_GenerationAlgorithm_Key, std::make_any<ChoicesParameter::ValueType>(1));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageLayout_Key, std::make_any<ChoicesParameter::ValueType>(1));
  args.insertOrAssign(WritePoleFigureFilter::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(fmt::format("{}/Dir1/Dir2", unit_test::k_BinaryTestOutputDir))));
  args.insertOrAssign(WritePoleFigureFilter::k_ImagePrefix_Key, std::make_any<StringParameter::ValueType>(k_ImagePrefix));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageSize_Key, std::make_any<int32>(1024));
  args.insertOrAssign(WritePoleFigureFilter::k_SaveAsImageGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(WritePoleFigureFilter::k_WriteImageToDisk, std::make_any<bool>(true));
  args.insertOrAssign(WritePoleFigureFilter::k_UseGoodVoxels_Key, std::make_any<bool>(true));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr Discrete Pole Figure Masked [CALCULATED]"})));

  DataPath calculatedImageData({"fw-ar-IF1-aptr12-corr Discrete Pole Figure Masked [CALCULATED]", "CellData", fmt::format("{}Phase_{}", k_ImagePrefix, 1)});
  DataPath exemplarImageData({"fw-ar-IF1-aptr12-corr Discrete Pole Figure Masked", "CellData", "Image"});

  args.insertOrAssign(WritePoleFigureFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "Cell Data", "EulerAngles"})));
  args.insertOrAssign(WritePoleFigureFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "Cell Data", "Phases"})));
  args.insertOrAssign(WritePoleFigureFilter::k_GoodVoxelsArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "Cell Data", "ThresholdArray"})));
  args.insertOrAssign(WritePoleFigureFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "CellEnsembleData", "CrystalStructures"})));
  args.insertOrAssign(WritePoleFigureFilter::k_MaterialNameArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "CellEnsembleData", "MaterialName"})));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/write_pole_figure-2.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  CompareArrays<uint8>(dataStructure, exemplarImageData, calculatedImageData);
}

TEST_CASE("OrientationAnalysis::WritePoleFigureFilter-3", "[OrientationAnalysis][WritePoleFigureFilter]")
{
  const std::string kDataInputArchive = "PoleFigure_Exemplars.tar.gz";
  const std::string kExpectedOutputTopLevel = "PoleFigure_Exemplars";
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, kDataInputArchive, kExpectedOutputTopLevel,
                                                             complex::unit_test::k_BinaryTestOutputDir);

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/PoleFigure_Exemplars/fw-ar-IF1-aptr12-corr.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  WritePoleFigureFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(WritePoleFigureFilter::k_Title_Key, std::make_any<StringParameter::ValueType>("fw-ar-IF1-aptr12-corr Discrete Pole Figure Masked Color"));
  args.insertOrAssign(WritePoleFigureFilter::k_LambertSize_Key, std::make_any<int32>(64));
  args.insertOrAssign(WritePoleFigureFilter::k_NumColors_Key, std::make_any<int32>(32));
  args.insertOrAssign(WritePoleFigureFilter::k_GenerationAlgorithm_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageLayout_Key, std::make_any<ChoicesParameter::ValueType>(2));
  args.insertOrAssign(WritePoleFigureFilter::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(fmt::format("{}/Dir1/Dir2", unit_test::k_BinaryTestOutputDir))));
  args.insertOrAssign(WritePoleFigureFilter::k_ImagePrefix_Key, std::make_any<StringParameter::ValueType>(k_ImagePrefix));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageSize_Key, std::make_any<int32>(1024));
  args.insertOrAssign(WritePoleFigureFilter::k_SaveAsImageGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(WritePoleFigureFilter::k_WriteImageToDisk, std::make_any<bool>(true));
  args.insertOrAssign(WritePoleFigureFilter::k_UseGoodVoxels_Key, std::make_any<bool>(true));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr Discrete Pole Figure Masked Color [CALCULATED]"})));

  DataPath calculatedImageData({"fw-ar-IF1-aptr12-corr Discrete Pole Figure Masked Color [CALCULATED]", "CellData", fmt::format("{}Phase_{}", k_ImagePrefix, 1)});
  DataPath exemplarImageData({"fw-ar-IF1-aptr12-corr Discrete Pole Figure Masked Color", "CellData", "Image"});

  args.insertOrAssign(WritePoleFigureFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "Cell Data", "EulerAngles"})));
  args.insertOrAssign(WritePoleFigureFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "Cell Data", "Phases"})));
  args.insertOrAssign(WritePoleFigureFilter::k_GoodVoxelsArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "Cell Data", "ThresholdArray"})));
  args.insertOrAssign(WritePoleFigureFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "CellEnsembleData", "CrystalStructures"})));
  args.insertOrAssign(WritePoleFigureFilter::k_MaterialNameArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "CellEnsembleData", "MaterialName"})));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/write_pole_figure-3.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  CompareArrays<uint8>(dataStructure, exemplarImageData, calculatedImageData);
}
