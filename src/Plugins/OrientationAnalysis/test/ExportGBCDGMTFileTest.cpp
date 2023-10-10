#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/ExportGBCDGMTFileFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Parameters/util/TextImporterData.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace complex;
using namespace complex::UnitTest;

namespace
{
inline constexpr StringLiteral k_FaceEnsembleDataPath("FaceEnsembleData [NX]");

inline constexpr StringLiteral k_TextImporterData_Key = "text_importer_data";
inline constexpr StringLiteral k_TupleDims_Key = "tuple_dimensions";
inline constexpr StringLiteral k_UseExistingGroup_Key = "use_existing_group";
inline constexpr StringLiteral k_SelectedDataGroup_Key = "selected_data_group";
inline constexpr StringLiteral k_CreatedDataGroup_Key = "created_data_group";

inline constexpr StringLiteral k_ExemplarGMT1("ExemplarGMT1");
inline constexpr StringLiteral k_ExemplarGMT2("ExemplarGMT2");
inline constexpr StringLiteral k_ExemplarGMT3("ExemplarGMT3");
inline constexpr StringLiteral k_GMT1("GMT1");
inline constexpr StringLiteral k_GMT2("GMT2");
inline constexpr StringLiteral k_GMT3("GMT3");

} // namespace

TEST_CASE("OrientationAnalysis::ExportGBCDGMTFileFilter", "[OrientationAnalysis][ExportGBCDGMTFile]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_Small_IN100_GBCD.tar.gz", "6_6_Small_IN100_GBCD");

  const std::shared_ptr<make_shared_enabler> app = std::make_shared<make_shared_enabler>();
  app->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterList = Application::Instance()->getFilterList();

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_Small_IN100_GBCD/6_6_Small_IN100_GBCD.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  const DataPath smallIn100Group({complex::Constants::k_SmallIN100});
  const DataPath featureDataPath = smallIn100Group.createChildPath(Constants::k_Grain_Data);
  const DataPath avgEulerAnglesPath = featureDataPath.createChildPath(Constants::k_AvgEulerAngles);
  const DataPath featurePhasesPath = featureDataPath.createChildPath(Constants::k_Phases);

  const DataPath ensembleDataPath = smallIn100Group.createChildPath(Constants::k_Phase_Data);
  const DataPath crystalStructurePath = ensembleDataPath.createChildPath(Constants::k_CrystalStructures);

  const DataPath triangleDataContainerPath({Constants::k_TriangleDataContainerName});
  const DataPath faceDataGroup = triangleDataContainerPath.createChildPath(Constants::k_FaceData);
  const DataPath faceEnsemblePath = triangleDataContainerPath.createChildPath(k_FaceEnsembleDataPath);

  const DataPath faceLabels = faceDataGroup.createChildPath(Constants::k_FaceLabels);
  const DataPath faceNormals = faceDataGroup.createChildPath(Constants::k_FaceNormals);
  const DataPath faceAreas = faceDataGroup.createChildPath(Constants::k_FaceAreas);

  const DataPath gbcdArrayPath = triangleDataContainerPath.createChildPath("FaceEnsembleData").createChildPath(Constants::k_GBCD_Name);

  const DataPath gmtGroupPath = triangleDataContainerPath.createChildPath("GMTResults");

  SECTION("Pole Figures for Sigma 3  60@[111]")
  {
    // Create Pole Figure
    auto outputFile = fs::path(fmt::format("{}/small_in100_sigma_3_1.dat", unit_test::k_BinaryTestOutputDir));
    {
      const ExportGBCDGMTFileFilter gmtFilter;
      Arguments args;

      args.insertOrAssign(ExportGBCDGMTFileFilter::k_PhaseOfInterest_Key, std::make_any<Int32Parameter::ValueType>(1));
      args.insertOrAssign(ExportGBCDGMTFileFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32Parameter::ValueType>({60.0F, 1.0F, 1.0F, 1.0F}));
      args.insertOrAssign(ExportGBCDGMTFileFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(outputFile));
      args.insertOrAssign(ExportGBCDGMTFileFilter::k_GBCDArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(gbcdArrayPath));
      args.insertOrAssign(ExportGBCDGMTFileFilter::k_CrystalStructuresArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(crystalStructurePath));

      // Preflight the filter and check result
      auto preflightResult = gmtFilter.preflight(dataStructure, args);
      COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

      // Execute the filter and check the result
      auto executeResult = gmtFilter.execute(dataStructure, args);
      COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
    }

    // Compare the Output Pole Figure
    auto importDataFilter = filterList->createFilter(k_ImportTextDataFilterHandle);
    REQUIRE(nullptr != importDataFilter);

    {
      Arguments args;
      TextImporterData data;
      data.inputFilePath = fmt::format("{}/6_6_Small_IN100_GBCD/small_in100_sigma_3_1.dat", unit_test::k_TestFilesDir);
      data.customHeaders = {k_ExemplarGMT1, k_ExemplarGMT2, k_ExemplarGMT3};
      data.dataTypes = {DataType::float32, DataType::float32, DataType::float32};
      data.skippedArrayMask = {false, false, false};
      data.startImportRow = 2;
      data.spaceAsDelimiter = true;
      data.tupleDims = {3751};

      args.insertOrAssign(k_TextImporterData_Key, std::make_any<TextImporterData>(data));
      args.insertOrAssign(k_UseExistingGroup_Key, std::make_any<bool>(false));
      args.insertOrAssign(k_CreatedDataGroup_Key, std::make_any<DataPath>(gmtGroupPath));
      args.insertOrAssign(k_SelectedDataGroup_Key, std::make_any<DataPath>(faceEnsemblePath));

      auto executeResult = importDataFilter->execute(dataStructure, args);
      COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
    }

    {
      Arguments args;
      TextImporterData data;
      data.inputFilePath = outputFile.string();
      data.customHeaders = {k_GMT1, k_GMT2, k_GMT3};
      data.dataTypes = {DataType::float32, DataType::float32, DataType::float32};
      data.skippedArrayMask = {false, false, false};
      data.startImportRow = 2;
      data.spaceAsDelimiter = true;
      data.tupleDims = {3751};

      args.insertOrAssign(k_TextImporterData_Key, std::make_any<TextImporterData>(data));
      args.insertOrAssign(k_UseExistingGroup_Key, std::make_any<bool>(true));
      args.insertOrAssign(k_CreatedDataGroup_Key, std::make_any<DataPath>(gmtGroupPath));
      args.insertOrAssign(k_SelectedDataGroup_Key, std::make_any<DataPath>(gmtGroupPath));

      auto executeResult = importDataFilter->execute(dataStructure, args);
      COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
    }

    const DataPath gmt1ArrayPath = gmtGroupPath.createChildPath(k_GMT1);
    const DataPath gmt2ArrayPath = gmtGroupPath.createChildPath(k_GMT2);
    const DataPath gmt3ArrayPath = gmtGroupPath.createChildPath(k_GMT3);
    const DataPath exemplarGmt1ArrayPath = gmtGroupPath.createChildPath(k_ExemplarGMT1);
    const DataPath exemplarGmt2ArrayPath = gmtGroupPath.createChildPath(k_ExemplarGMT2);
    const DataPath exemplarGmt3ArrayPath = gmtGroupPath.createChildPath(k_ExemplarGMT3);
    REQUIRE(dataStructure.getDataAs<Float32Array>(gmt1ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32Array>(gmt2ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32Array>(gmt3ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32Array>(exemplarGmt1ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32Array>(exemplarGmt2ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32Array>(exemplarGmt3ArrayPath) != nullptr);

    UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, exemplarGmt1ArrayPath, gmt1ArrayPath);
    UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, exemplarGmt2ArrayPath, gmt2ArrayPath);
    UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, exemplarGmt3ArrayPath, gmt3ArrayPath);
  }

  SECTION("Pole Figures for Sigma 9  39@[110]")
  {
    // Create Pole Figure
    auto outputFile = fs::path(fmt::format("{}/small_in100_sigma_9_1.dat", unit_test::k_BinaryTestOutputDir));

    {
      const ExportGBCDGMTFileFilter filter;
      Arguments args;

      args.insertOrAssign(ExportGBCDGMTFileFilter::k_PhaseOfInterest_Key, std::make_any<Int32Parameter::ValueType>(1));
      args.insertOrAssign(ExportGBCDGMTFileFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32Parameter::ValueType>({39.0F, 1.0F, 1.0F, 0.0F}));
      args.insertOrAssign(ExportGBCDGMTFileFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(outputFile));
      args.insertOrAssign(ExportGBCDGMTFileFilter::k_GBCDArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(gbcdArrayPath));
      args.insertOrAssign(ExportGBCDGMTFileFilter::k_CrystalStructuresArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(crystalStructurePath));

      // Preflight the filter and check result
      auto preflightResult = filter.preflight(dataStructure, args);
      COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

      // Execute the filter and check the result
      auto executeResult = filter.execute(dataStructure, args);
      COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
    }

    // Compare the Output Pole Figure
    auto importDataFilter = filterList->createFilter(k_ImportTextDataFilterHandle);
    REQUIRE(nullptr != importDataFilter);

    {
      Arguments args;
      TextImporterData data;
      data.inputFilePath = fmt::format("{}/6_6_Small_IN100_GBCD/small_in100_sigma_9_1.dat", unit_test::k_TestFilesDir);
      data.customHeaders = {k_ExemplarGMT1, k_ExemplarGMT2, k_ExemplarGMT3};
      data.dataTypes = {DataType::float32, DataType::float32, DataType::float32};
      data.skippedArrayMask = {false, false, false};
      data.startImportRow = 2;
      data.spaceAsDelimiter = true;
      data.tupleDims = {3751};

      args.insertOrAssign(k_TextImporterData_Key, std::make_any<TextImporterData>(data));
      args.insertOrAssign(k_UseExistingGroup_Key, std::make_any<bool>(false));
      args.insertOrAssign(k_CreatedDataGroup_Key, std::make_any<DataPath>(gmtGroupPath));
      args.insertOrAssign(k_SelectedDataGroup_Key, std::make_any<DataPath>(faceEnsemblePath));

      auto executeResult = importDataFilter->execute(dataStructure, args);
      COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
    }

    {
      Arguments args;
      TextImporterData data;
      data.inputFilePath = outputFile.string();
      data.customHeaders = {k_GMT1, k_GMT2, k_GMT3};
      data.dataTypes = {DataType::float32, DataType::float32, DataType::float32};
      data.skippedArrayMask = {false, false, false};
      data.startImportRow = 2;
      data.spaceAsDelimiter = true;
      data.tupleDims = {3751};

      args.insertOrAssign(k_TextImporterData_Key, std::make_any<TextImporterData>(data));
      args.insertOrAssign(k_UseExistingGroup_Key, std::make_any<bool>(true));
      args.insertOrAssign(k_CreatedDataGroup_Key, std::make_any<DataPath>(gmtGroupPath));
      args.insertOrAssign(k_SelectedDataGroup_Key, std::make_any<DataPath>(gmtGroupPath));

      auto executeResult = importDataFilter->execute(dataStructure, args);
      COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
    }

    const DataPath gmt1ArrayPath = gmtGroupPath.createChildPath(k_GMT1);
    const DataPath gmt2ArrayPath = gmtGroupPath.createChildPath(k_GMT2);
    const DataPath gmt3ArrayPath = gmtGroupPath.createChildPath(k_GMT3);
    const DataPath exemplarGmt1ArrayPath = gmtGroupPath.createChildPath(k_ExemplarGMT1);
    const DataPath exemplarGmt2ArrayPath = gmtGroupPath.createChildPath(k_ExemplarGMT2);
    const DataPath exemplarGmt3ArrayPath = gmtGroupPath.createChildPath(k_ExemplarGMT3);
    REQUIRE(dataStructure.getDataAs<Float32Array>(gmt1ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32Array>(gmt2ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32Array>(gmt3ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32Array>(exemplarGmt1ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32Array>(exemplarGmt2ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32Array>(exemplarGmt3ArrayPath) != nullptr);

    UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, exemplarGmt1ArrayPath, gmt1ArrayPath);
    UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, exemplarGmt2ArrayPath, gmt2ArrayPath);
    UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, exemplarGmt3ArrayPath, gmt3ArrayPath);
  }

  SECTION("Pole Figures for Sigma 11  50.5@[110]")
  {
    // Create Pole Figure
    auto outputFile = fs::path(fmt::format("{}/small_in100_sigma_11_1.dat", unit_test::k_BinaryTestOutputDir));

    {
      const ExportGBCDGMTFileFilter filter;
      Arguments args;

      args.insertOrAssign(ExportGBCDGMTFileFilter::k_PhaseOfInterest_Key, std::make_any<Int32Parameter::ValueType>(1));
      args.insertOrAssign(ExportGBCDGMTFileFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32Parameter::ValueType>({50.5F, 1.0F, 1.0F, 0.0F}));
      args.insertOrAssign(ExportGBCDGMTFileFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(outputFile));
      args.insertOrAssign(ExportGBCDGMTFileFilter::k_GBCDArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(gbcdArrayPath));
      args.insertOrAssign(ExportGBCDGMTFileFilter::k_CrystalStructuresArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(crystalStructurePath));

      // Preflight the filter and check result
      auto preflightResult = filter.preflight(dataStructure, args);
      COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

      // Execute the filter and check the result
      auto executeResult = filter.execute(dataStructure, args);
      COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
    }

    // Compare the Output Pole Figure
    auto importDataFilter = filterList->createFilter(k_ImportTextDataFilterHandle);
    REQUIRE(nullptr != importDataFilter);

    {
      Arguments args;
      TextImporterData data;
      data.inputFilePath = fmt::format("{}/6_6_Small_IN100_GBCD/small_in100_sigma_11_1.dat", unit_test::k_TestFilesDir);
      data.customHeaders = {k_ExemplarGMT1, k_ExemplarGMT2, k_ExemplarGMT3};
      data.dataTypes = {DataType::float32, DataType::float32, DataType::float32};
      data.skippedArrayMask = {false, false, false};
      data.startImportRow = 2;
      data.spaceAsDelimiter = true;
      data.tupleDims = {3751};

      args.insertOrAssign(k_TextImporterData_Key, std::make_any<TextImporterData>(data));
      args.insertOrAssign(k_UseExistingGroup_Key, std::make_any<bool>(false));
      args.insertOrAssign(k_CreatedDataGroup_Key, std::make_any<DataPath>(gmtGroupPath));
      args.insertOrAssign(k_SelectedDataGroup_Key, std::make_any<DataPath>(faceEnsemblePath));

      auto executeResult = importDataFilter->execute(dataStructure, args);
      COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
    }

    {
      Arguments args;
      TextImporterData data;
      data.inputFilePath = outputFile.string();
      data.customHeaders = {k_GMT1, k_GMT2, k_GMT3};
      data.dataTypes = {DataType::float32, DataType::float32, DataType::float32};
      data.skippedArrayMask = {false, false, false};
      data.startImportRow = 2;
      data.spaceAsDelimiter = true;
      data.tupleDims = {3751};

      args.insertOrAssign(k_TextImporterData_Key, std::make_any<TextImporterData>(data));
      args.insertOrAssign(k_UseExistingGroup_Key, std::make_any<bool>(true));
      args.insertOrAssign(k_CreatedDataGroup_Key, std::make_any<DataPath>(gmtGroupPath));
      args.insertOrAssign(k_SelectedDataGroup_Key, std::make_any<DataPath>(gmtGroupPath));

      auto executeResult = importDataFilter->execute(dataStructure, args);
      COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
    }

    const DataPath gmt1ArrayPath = gmtGroupPath.createChildPath(k_GMT1);
    const DataPath gmt2ArrayPath = gmtGroupPath.createChildPath(k_GMT2);
    const DataPath gmt3ArrayPath = gmtGroupPath.createChildPath(k_GMT3);
    const DataPath exemplarGmt1ArrayPath = gmtGroupPath.createChildPath(k_ExemplarGMT1);
    const DataPath exemplarGmt2ArrayPath = gmtGroupPath.createChildPath(k_ExemplarGMT2);
    const DataPath exemplarGmt3ArrayPath = gmtGroupPath.createChildPath(k_ExemplarGMT3);
    REQUIRE(dataStructure.getDataAs<Float32Array>(gmt1ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32Array>(gmt2ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32Array>(gmt3ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32Array>(exemplarGmt1ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32Array>(exemplarGmt2ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32Array>(exemplarGmt3ArrayPath) != nullptr);

    UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, exemplarGmt1ArrayPath, gmt1ArrayPath);
    UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, exemplarGmt2ArrayPath, gmt2ArrayPath);
    UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, exemplarGmt3ArrayPath, gmt3ArrayPath);
  }

#ifdef COMPLEX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/gbcd_gmt.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
}
