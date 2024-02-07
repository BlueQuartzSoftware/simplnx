
#include <catch2/catch.hpp>

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "OrientationAnalysis/Filters/WritePoleFigureFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace nx::core;
using namespace nx::core::UnitTest;

namespace
{
const std::string k_ImagePrefix("fw-ar-IF1-aptr12-corr Discrete Pole Figure");

template <typename T>
void CompareComponentsOfArrays(const DataStructure& dataStructure, const DataPath& exemplaryDataPath, const DataPath& computedPath, usize compIndex)
{
  // DataPath exemplaryDataPath = featureGroup.createChildPath("SurfaceFeatures");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(exemplaryDataPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(computedPath));

  const auto& exemplaryDataArray = dataStructure.getDataRefAs<DataArray<T>>(exemplaryDataPath);
  const auto& generatedDataArray = dataStructure.getDataRefAs<DataArray<T>>(computedPath);
  REQUIRE(generatedDataArray.getNumberOfTuples() == exemplaryDataArray.getNumberOfTuples());

  usize exemplaryNumComp = exemplaryDataArray.getNumberOfComponents();
  usize generatedNumComp = generatedDataArray.getNumberOfComponents();

  REQUIRE(exemplaryNumComp == 4);
  REQUIRE(generatedNumComp == 3);

  REQUIRE(compIndex < exemplaryNumComp);
  REQUIRE(compIndex < generatedNumComp);

  INFO(fmt::format("Bad Comparison\n  Input Data Array:'{}'\n  Output DataArray: '{}'", exemplaryDataPath.toString(), computedPath.toString()));

  usize start = 0;
  usize numTuples = exemplaryDataArray.getNumberOfTuples();
  for(usize i = start; i < numTuples; i++)
  {
    auto oldVal = exemplaryDataArray[i * exemplaryNumComp + compIndex];
    auto newVal = generatedDataArray[i * generatedNumComp + compIndex];
    INFO(fmt::format("Index: {} Comp: {}", i, compIndex));

    REQUIRE(oldVal == newVal);
  }
}

} // namespace

TEST_CASE("OrientationAnalysis::WritePoleFigureFilter-1", "[OrientationAnalysis][WritePoleFigureFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "PoleFigure_Exemplars.tar.gz", "PoleFigure_Exemplars");

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
  args.insertOrAssign(WritePoleFigureFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr Discrete Pole Figure [CALCULATED]"})));

  DataPath calculatedImageData({"fw-ar-IF1-aptr12-corr Discrete Pole Figure [CALCULATED]", "CellData", fmt::format("{}{}", k_ImagePrefix, 1)});
  DataPath exemplarImageData({"fw-ar-IF1-aptr12-corr Discrete Pole Figure", "CellData", "Image"});

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/write_pole_figure-1.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  CompareComponentsOfArrays<uint8>(dataStructure, exemplarImageData, calculatedImageData, 0);
  CompareComponentsOfArrays<uint8>(dataStructure, exemplarImageData, calculatedImageData, 1);
  CompareComponentsOfArrays<uint8>(dataStructure, exemplarImageData, calculatedImageData, 2);
}

TEST_CASE("OrientationAnalysis::WritePoleFigureFilter-2", "[OrientationAnalysis][WritePoleFigureFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "PoleFigure_Exemplars.tar.gz", "PoleFigure_Exemplars");

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
  args.insertOrAssign(WritePoleFigureFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr Discrete Pole Figure Masked [CALCULATED]"})));

  DataPath calculatedImageData({"fw-ar-IF1-aptr12-corr Discrete Pole Figure Masked [CALCULATED]", "CellData", fmt::format("{}{}", k_ImagePrefix, 1)});
  DataPath exemplarImageData({"fw-ar-IF1-aptr12-corr Discrete Pole Figure Masked", "CellData", "Image"});

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/write_pole_figure-2.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  CompareComponentsOfArrays<uint8>(dataStructure, exemplarImageData, calculatedImageData, 0);
  CompareComponentsOfArrays<uint8>(dataStructure, exemplarImageData, calculatedImageData, 1);
  CompareComponentsOfArrays<uint8>(dataStructure, exemplarImageData, calculatedImageData, 2);
}

TEST_CASE("OrientationAnalysis::WritePoleFigureFilter-3", "[OrientationAnalysis][WritePoleFigureFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "PoleFigure_Exemplars.tar.gz", "PoleFigure_Exemplars");

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
  args.insertOrAssign(WritePoleFigureFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"fw-ar-IF1-aptr12-corr Discrete Pole Figure Masked Color [CALCULATED]"})));

  DataPath calculatedImageData({"fw-ar-IF1-aptr12-corr Discrete Pole Figure Masked Color [CALCULATED]", "CellData", fmt::format("{}{}", k_ImagePrefix, 1)});
  DataPath exemplarImageData({"fw-ar-IF1-aptr12-corr Discrete Pole Figure Masked Color", "CellData", "Image"});

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
  WriteTestDataStructure(dataStructure, fmt::format("{}/write_pole_figure-3.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  CompareComponentsOfArrays<uint8>(dataStructure, exemplarImageData, calculatedImageData, 0);
  CompareComponentsOfArrays<uint8>(dataStructure, exemplarImageData, calculatedImageData, 1);
  CompareComponentsOfArrays<uint8>(dataStructure, exemplarImageData, calculatedImageData, 2);
}
