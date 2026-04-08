#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;

#include "OrientationAnalysis/Filters/WritePoleFigureFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace nx::core;
using namespace nx::core::UnitTest;

// #define SIMPLNX_WRITE_TEST_OUTPUT

namespace
{
const std::string k_ImagePrefix("Discrete Pole Figure");

template <typename T>
void CompareComponentsOfArrays(const DataStructure& dataStructure, const DataPath& exemplaryDataPath, const DataPath& computedPath, usize compIndex)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(exemplaryDataPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(computedPath));

  const auto& exemplaryDataArray = dataStructure.getDataRefAs<DataArray<T>>(exemplaryDataPath);
  const auto& generatedDataArray = dataStructure.getDataRefAs<DataArray<T>>(computedPath);
  REQUIRE(generatedDataArray.getNumberOfTuples() == exemplaryDataArray.getNumberOfTuples());

  usize exemplaryNumComp = exemplaryDataArray.getNumberOfComponents();
  usize generatedNumComp = generatedDataArray.getNumberOfComponents();

  REQUIRE(compIndex < exemplaryNumComp);
  REQUIRE(compIndex < generatedNumComp);

  INFO(fmt::format("Bad Comparison\n  Input Data Array:'{}'\n  Output DataArray: '{}'", exemplaryDataPath.toString(), computedPath.toString()));

  const usize numTuples = exemplaryDataArray.getNumberOfTuples();
  const usize totalElements = numTuples * exemplaryNumComp;

  // Bulk-read both arrays into local buffers to avoid per-element OOC overhead
  std::vector<T> exemplarBuf(totalElements);
  std::vector<T> generatedBuf(totalElements);
  exemplaryDataArray.getDataStoreRef().copyIntoBuffer(0, nonstd::span<T>(exemplarBuf.data(), totalElements));
  generatedDataArray.getDataStoreRef().copyIntoBuffer(0, nonstd::span<T>(generatedBuf.data(), totalElements));

  for(usize i = 0; i < numTuples; i++)
  {
    auto oldVal = exemplarBuf[i * exemplaryNumComp + compIndex];
    auto newVal = generatedBuf[i * generatedNumComp + compIndex];
    INFO(fmt::format("Index: {} Comp: {}", i, compIndex));

    REQUIRE(oldVal == newVal);
  }
}

} // namespace

TEST_CASE("OrientationAnalysis::WritePoleFigureFilter-Discrete", "[OrientationAnalysis][WritePoleFigureFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "PoleFigure_Exemplars_v5.tar.gz", "PoleFigure_Exemplars_v5", true, true);

  // Read the test data
  auto baseDataFilePath = fs::path(fmt::format("{}/PoleFigure_Exemplars_v5/PoleFigure_Exemplars_v5.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  WritePoleFigureFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(WritePoleFigureFilter::k_Title_Key, std::make_any<StringParameter::ValueType>("Discrete Pole Figure"));
  args.insertOrAssign(WritePoleFigureFilter::k_LambertSize_Key, std::make_any<int32>(64));
  args.insertOrAssign(WritePoleFigureFilter::k_NumColors_Key, std::make_any<int32>(32));
  args.insertOrAssign(WritePoleFigureFilter::k_GenerationAlgorithm_Key, std::make_any<ChoicesParameter::ValueType>(1));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageLayout_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(WritePoleFigureFilter::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(fmt::format("{}/Dir1/Dir2", unit_test::k_BinaryTestOutputDir))));
  args.insertOrAssign(WritePoleFigureFilter::k_ImagePrefix_Key, std::make_any<StringParameter::ValueType>(k_ImagePrefix));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageSize_Key, std::make_any<int32>(1024));
  args.insertOrAssign(WritePoleFigureFilter::k_SaveAsImageGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(WritePoleFigureFilter::k_WriteImageToDisk, std::make_any<bool>(true));
  args.insertOrAssign(WritePoleFigureFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Discrete Pole Figure [CALCULATED]"})));
  args.insertOrAssign(WritePoleFigureFilter::k_SaveIntensityDataArrays, std::make_any<bool>(true));
  args.insertOrAssign(WritePoleFigureFilter::k_IntensityGeometryPath, std::make_any<DataPath>(DataPath({"Discrete Count MRD [CALCULATED]"})));
  args.insertOrAssign(WritePoleFigureFilter::k_NormalizeToMRD, std::make_any<bool>(true));

  args.insertOrAssign(WritePoleFigureFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "Cell Data", "EulerAngles"})));
  args.insertOrAssign(WritePoleFigureFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "Cell Data", "Phases"})));
  args.insertOrAssign(WritePoleFigureFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "Cell Data", "ThresholdArray"})));
  args.insertOrAssign(WritePoleFigureFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "CellEnsembleData", "CrystalStructures"})));
  args.insertOrAssign(WritePoleFigureFilter::k_MaterialNameArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "CellEnsembleData", "MaterialName"})));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/write_pole_figure-Discrete.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  {
    DataPath calculatedImageData({"Discrete Pole Figure [CALCULATED]", "Cell Data", fmt::format("Phase_{}", 1)});
    DataPath exemplarImageData({"Discrete Pole Figure", "Cell Data", "Phase_1"});
    CompareComponentsOfArrays<uint8>(dataStructure, exemplarImageData, calculatedImageData, 0);
    CompareComponentsOfArrays<uint8>(dataStructure, exemplarImageData, calculatedImageData, 1);
    CompareComponentsOfArrays<uint8>(dataStructure, exemplarImageData, calculatedImageData, 2);
  }

  {
    DataPath calculatedImageData({"Discrete Count MRD", "Cell Data", "Phase_1_<001>"});
    DataPath exemplarImageData({"Discrete Count MRD [CALCULATED]", "Cell Data", "Phase_1_<001>"});
    CompareComponentsOfArrays<float64>(dataStructure, exemplarImageData, calculatedImageData, 0);
  }

  {
    DataPath calculatedImageData({"Discrete Count MRD", "Cell Data", "Phase_1_<011>"});
    DataPath exemplarImageData({"Discrete Count MRD [CALCULATED]", "Cell Data", "Phase_1_<011>"});
    CompareComponentsOfArrays<float64>(dataStructure, exemplarImageData, calculatedImageData, 0);
  }

  {
    DataPath calculatedImageData({"Discrete Count MRD", "Cell Data", "Phase_1_<111>"});
    DataPath exemplarImageData({"Discrete Count MRD [CALCULATED]", "Cell Data", "Phase_1_<111>"});
    CompareComponentsOfArrays<float64>(dataStructure, exemplarImageData, calculatedImageData, 0);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::WritePoleFigureFilter-Discrete-Masked", "[OrientationAnalysis][WritePoleFigureFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "PoleFigure_Exemplars_v5.tar.gz", "PoleFigure_Exemplars_v5", true, true);

  // Read the test data
  auto baseDataFilePath = fs::path(fmt::format("{}/PoleFigure_Exemplars_v5/PoleFigure_Exemplars_v5.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  WritePoleFigureFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(WritePoleFigureFilter::k_Title_Key, std::make_any<StringParameter::ValueType>("Discrete Pole Figure Masked"));
  args.insertOrAssign(WritePoleFigureFilter::k_LambertSize_Key, std::make_any<int32>(64));
  args.insertOrAssign(WritePoleFigureFilter::k_NumColors_Key, std::make_any<int32>(32));
  args.insertOrAssign(WritePoleFigureFilter::k_GenerationAlgorithm_Key, std::make_any<ChoicesParameter::ValueType>(1));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageLayout_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(WritePoleFigureFilter::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(fmt::format("{}/Dir1/Dir2", unit_test::k_BinaryTestOutputDir))));
  args.insertOrAssign(WritePoleFigureFilter::k_ImagePrefix_Key, std::make_any<StringParameter::ValueType>(k_ImagePrefix));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageSize_Key, std::make_any<int32>(1024));
  args.insertOrAssign(WritePoleFigureFilter::k_SaveAsImageGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(WritePoleFigureFilter::k_WriteImageToDisk, std::make_any<bool>(true));
  args.insertOrAssign(WritePoleFigureFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Discrete Pole Figure Masked [CALCULATED]"})));
  args.insertOrAssign(WritePoleFigureFilter::k_SaveIntensityDataArrays, std::make_any<bool>(true));
  args.insertOrAssign(WritePoleFigureFilter::k_IntensityGeometryPath, std::make_any<DataPath>(DataPath({"Discrete Count MRD Masked [CALCULATED]"})));
  args.insertOrAssign(WritePoleFigureFilter::k_NormalizeToMRD, std::make_any<bool>(true));

  args.insertOrAssign(WritePoleFigureFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "Cell Data", "EulerAngles"})));
  args.insertOrAssign(WritePoleFigureFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "Cell Data", "Phases"})));
  args.insertOrAssign(WritePoleFigureFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "Cell Data", "Mask"})));
  args.insertOrAssign(WritePoleFigureFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "CellEnsembleData", "CrystalStructures"})));
  args.insertOrAssign(WritePoleFigureFilter::k_MaterialNameArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "CellEnsembleData", "MaterialName"})));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/write_pole_figure-Discrete-Masked.dream3d", unit_test::k_BinaryTestOutputDir));
#endif
  {
    DataPath calculatedImageData({"Discrete Pole Figure Masked [CALCULATED]", "Cell Data", fmt::format("Phase_{}", 1)});
    DataPath exemplarImageData({"Discrete Pole Figure Masked", "Cell Data", "Phase_1"});
    CompareComponentsOfArrays<uint8>(dataStructure, exemplarImageData, calculatedImageData, 0);
    CompareComponentsOfArrays<uint8>(dataStructure, exemplarImageData, calculatedImageData, 1);
    CompareComponentsOfArrays<uint8>(dataStructure, exemplarImageData, calculatedImageData, 2);
  }

  {
    DataPath calculatedImageData({"Discrete Count MRD Masked", "Cell Data", "Phase_1_<001>"});
    DataPath exemplarImageData({"Discrete Count MRD Masked [CALCULATED]", "Cell Data", "Phase_1_<001>"});
    CompareComponentsOfArrays<float64>(dataStructure, exemplarImageData, calculatedImageData, 0);
  }

  {
    DataPath calculatedImageData({"Discrete Count MRD Masked", "Cell Data", "Phase_1_<011>"});
    DataPath exemplarImageData({"Discrete Count MRD Masked [CALCULATED]", "Cell Data", "Phase_1_<011>"});
    CompareComponentsOfArrays<float64>(dataStructure, exemplarImageData, calculatedImageData, 0);
  }
  {
    DataPath calculatedImageData({"Discrete Count MRD Masked", "Cell Data", "Phase_1_<111>"});
    DataPath exemplarImageData({"Discrete Count MRD Masked [CALCULATED]", "Cell Data", "Phase_1_<111>"});
    CompareComponentsOfArrays<float64>(dataStructure, exemplarImageData, calculatedImageData, 0);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::WritePoleFigureFilter-Color", "[OrientationAnalysis][WritePoleFigureFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "PoleFigure_Exemplars_v5.tar.gz", "PoleFigure_Exemplars_v5", true, true);

  // Read the test data
  auto baseDataFilePath = fs::path(fmt::format("{}/PoleFigure_Exemplars_v5/PoleFigure_Exemplars_v5.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  WritePoleFigureFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(WritePoleFigureFilter::k_Title_Key, std::make_any<StringParameter::ValueType>("Color Pole Figure"));
  args.insertOrAssign(WritePoleFigureFilter::k_LambertSize_Key, std::make_any<int32>(64));
  args.insertOrAssign(WritePoleFigureFilter::k_NumColors_Key, std::make_any<int32>(32));
  args.insertOrAssign(WritePoleFigureFilter::k_GenerationAlgorithm_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageLayout_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(WritePoleFigureFilter::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(fmt::format("{}/Dir1/Dir2", unit_test::k_BinaryTestOutputDir))));
  args.insertOrAssign(WritePoleFigureFilter::k_ImagePrefix_Key, std::make_any<StringParameter::ValueType>(k_ImagePrefix));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageSize_Key, std::make_any<int32>(1024));
  args.insertOrAssign(WritePoleFigureFilter::k_SaveAsImageGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(WritePoleFigureFilter::k_WriteImageToDisk, std::make_any<bool>(true));
  args.insertOrAssign(WritePoleFigureFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Color Pole Figure [CALCULATED]"})));
  args.insertOrAssign(WritePoleFigureFilter::k_SaveIntensityDataArrays, std::make_any<bool>(true));
  args.insertOrAssign(WritePoleFigureFilter::k_IntensityGeometryPath, std::make_any<DataPath>(DataPath({"Color Count MRD [CALCULATED]"})));
  args.insertOrAssign(WritePoleFigureFilter::k_NormalizeToMRD, std::make_any<bool>(true));

  args.insertOrAssign(WritePoleFigureFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "Cell Data", "EulerAngles"})));
  args.insertOrAssign(WritePoleFigureFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "Cell Data", "Phases"})));
  args.insertOrAssign(WritePoleFigureFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "Cell Data", "Mask"})));
  args.insertOrAssign(WritePoleFigureFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "CellEnsembleData", "CrystalStructures"})));
  args.insertOrAssign(WritePoleFigureFilter::k_MaterialNameArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "CellEnsembleData", "MaterialName"})));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/write_pole_figure-Color.dream3d", unit_test::k_BinaryTestOutputDir));
#endif
  {
    DataPath calculatedImageData({"Color Pole Figure [CALCULATED]", "Cell Data", fmt::format("Phase_{}", 1)});
    DataPath exemplarImageData({"Color Pole Figure", "Cell Data", "Phase_1"});

    CompareComponentsOfArrays<uint8>(dataStructure, exemplarImageData, calculatedImageData, 0);
    CompareComponentsOfArrays<uint8>(dataStructure, exemplarImageData, calculatedImageData, 1);
    CompareComponentsOfArrays<uint8>(dataStructure, exemplarImageData, calculatedImageData, 2);
  }

  {
    DataPath calculatedImageData({"Color Count MRD", "Cell Data", "Phase_1_<001>"});
    DataPath exemplarImageData({"Color Count MRD [CALCULATED]", "Cell Data", "Phase_1_<001>"});
    UnitTest::CompareFloatArraysWithNans<float64>(dataStructure, exemplarImageData, calculatedImageData, 0.0001f, false);
  }

  {
    DataPath calculatedImageData({"Color Count MRD", "Cell Data", "Phase_1_<011>"});
    DataPath exemplarImageData({"Color Count MRD [CALCULATED]", "Cell Data", "Phase_1_<011>"});
    UnitTest::CompareFloatArraysWithNans<float64>(dataStructure, exemplarImageData, calculatedImageData, 0.0001f, false);
  }

  {
    DataPath calculatedImageData({"Color Count MRD", "Cell Data", "Phase_1_<111>"});
    DataPath exemplarImageData({"Color Count MRD [CALCULATED]", "Cell Data", "Phase_1_<111>"});
    UnitTest::CompareFloatArraysWithNans<float64>(dataStructure, exemplarImageData, calculatedImageData, 0.0001f, false);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::WritePoleFigureFilter-Color-Masked", "[OrientationAnalysis][WritePoleFigureFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "PoleFigure_Exemplars_v5.tar.gz", "PoleFigure_Exemplars_v5", true, true);

  // Read the test data
  auto baseDataFilePath = fs::path(fmt::format("{}/PoleFigure_Exemplars_v5/PoleFigure_Exemplars_v5.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  WritePoleFigureFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(WritePoleFigureFilter::k_Title_Key, std::make_any<StringParameter::ValueType>("Color Pole Figure Masked"));
  args.insertOrAssign(WritePoleFigureFilter::k_LambertSize_Key, std::make_any<int32>(64));
  args.insertOrAssign(WritePoleFigureFilter::k_NumColors_Key, std::make_any<int32>(32));
  args.insertOrAssign(WritePoleFigureFilter::k_GenerationAlgorithm_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageLayout_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(WritePoleFigureFilter::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(fmt::format("{}/Dir1/Dir2", unit_test::k_BinaryTestOutputDir))));
  args.insertOrAssign(WritePoleFigureFilter::k_ImagePrefix_Key, std::make_any<StringParameter::ValueType>(k_ImagePrefix));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageSize_Key, std::make_any<int32>(1024));
  args.insertOrAssign(WritePoleFigureFilter::k_SaveAsImageGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(WritePoleFigureFilter::k_WriteImageToDisk, std::make_any<bool>(true));
  args.insertOrAssign(WritePoleFigureFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Color Pole Figure Masked [CALCULATED]"})));
  args.insertOrAssign(WritePoleFigureFilter::k_SaveIntensityDataArrays, std::make_any<bool>(true));
  args.insertOrAssign(WritePoleFigureFilter::k_IntensityGeometryPath, std::make_any<DataPath>(DataPath({"Color Count MRD Masked [CALCULATED]"})));
  args.insertOrAssign(WritePoleFigureFilter::k_NormalizeToMRD, std::make_any<bool>(true));

  args.insertOrAssign(WritePoleFigureFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "Cell Data", "EulerAngles"})));
  args.insertOrAssign(WritePoleFigureFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "Cell Data", "Phases"})));
  args.insertOrAssign(WritePoleFigureFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "Cell Data", "Mask"})));
  args.insertOrAssign(WritePoleFigureFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "CellEnsembleData", "CrystalStructures"})));
  args.insertOrAssign(WritePoleFigureFilter::k_MaterialNameArrayPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr", "CellEnsembleData", "MaterialName"})));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/write_pole_figure-Color-Masked.dream3d", unit_test::k_BinaryTestOutputDir));
#endif
  {
    DataPath calculatedImageData({"Color Pole Figure Masked [CALCULATED]", "Cell Data", fmt::format("Phase_{}", 1)});
    DataPath exemplarImageData({"Color Pole Figure Masked", "Cell Data", "Phase_1"});

    CompareComponentsOfArrays<uint8>(dataStructure, exemplarImageData, calculatedImageData, 0);
    CompareComponentsOfArrays<uint8>(dataStructure, exemplarImageData, calculatedImageData, 1);
    CompareComponentsOfArrays<uint8>(dataStructure, exemplarImageData, calculatedImageData, 2);
  }

  {
    DataPath calculatedImageData({"Color Count MRD Masked", "Cell Data", "Phase_1_<001>"});
    DataPath exemplarImageData({"Color Count MRD Masked [CALCULATED]", "Cell Data", "Phase_1_<001>"});
    UnitTest::CompareFloatArraysWithNans<float64>(dataStructure, exemplarImageData, calculatedImageData, 0.0001f, false);
  }

  {
    DataPath calculatedImageData({"Color Count MRD Masked", "Cell Data", "Phase_1_<011>"});
    DataPath exemplarImageData({"Color Count MRD Masked [CALCULATED]", "Cell Data", "Phase_1_<011>"});
    UnitTest::CompareFloatArraysWithNans<float64>(dataStructure, exemplarImageData, calculatedImageData, 0.0001f, false);
  }

  {
    DataPath calculatedImageData({"Color Count MRD Masked", "Cell Data", "Phase_1_<111>"});
    DataPath exemplarImageData({"Color Count MRD Masked [CALCULATED]", "Cell Data", "Phase_1_<111>"});
    UnitTest::CompareFloatArraysWithNans<float64>(dataStructure, exemplarImageData, calculatedImageData, 0.0001f, false);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::WritePoleFigureFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][WritePoleFigureFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "WritePoleFigureFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "WritePoleFigureFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<WritePoleFigureFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      if(label == "SIMPL 6.5 (UUID)")
      {
        CHECK(args.value<ChoicesParameter::ValueType>(WritePoleFigureFilter::k_ImageFormat_Key) == 0);
      }
      CHECK(args.value<std::string>(WritePoleFigureFilter::k_Title_Key) == "TestName");
      CHECK(args.value<ChoicesParameter::ValueType>(WritePoleFigureFilter::k_GenerationAlgorithm_Key) == 0);
      CHECK(args.value<int32>(WritePoleFigureFilter::k_LambertSize_Key) == 5);
      CHECK(args.value<int32>(WritePoleFigureFilter::k_NumColors_Key) == 5);
      CHECK(args.value<ChoicesParameter::ValueType>(WritePoleFigureFilter::k_ImageLayout_Key) == 0);
      CHECK(args.value<FileSystemPathParameter::ValueType>(WritePoleFigureFilter::k_OutputPath_Key) == fs::path("/test/path/file.txt"));
      CHECK(args.value<std::string>(WritePoleFigureFilter::k_ImagePrefix_Key) == "TestName");
      CHECK(args.value<int32>(WritePoleFigureFilter::k_ImageSize_Key) == 5);
      CHECK(args.value<bool>(WritePoleFigureFilter::k_UseMask_Key) == true);
      CHECK(args.value<DataPath>(WritePoleFigureFilter::k_CellEulerAnglesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(WritePoleFigureFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(WritePoleFigureFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(WritePoleFigureFilter::k_ImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(WritePoleFigureFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(WritePoleFigureFilter::k_MaterialNameArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}
