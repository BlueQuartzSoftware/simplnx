#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/ExportGBCDTriangleDataFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/util/CSVWizardData.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace complex;
using namespace complex::UnitTest;

namespace
{
inline constexpr StringLiteral k_WizardData_Key = "wizard_data";
inline constexpr StringLiteral k_TupleDims_Key = "tuple_dimensions";
inline constexpr StringLiteral k_UseExistingGroup_Key = "use_existing_group";
inline constexpr StringLiteral k_SelectedDataGroup_Key = "selected_data_group";
inline constexpr StringLiteral k_CreatedDataGroup_Key = "created_data_group";

inline constexpr StringLiteral k_Phi1Right("Phi1Right");
inline constexpr StringLiteral k_PhiRight("PhiRight");
inline constexpr StringLiteral k_Phi2Right("Phi2Right");
inline constexpr StringLiteral k_Phi1Left("Phi1Left");
inline constexpr StringLiteral k_PhiLeft("PhiLeft");
inline constexpr StringLiteral k_Phi2Left("Phi2Left");
inline constexpr StringLiteral k_TriangleNormal0("TriangleNormal0");
inline constexpr StringLiteral k_TriangleNormal1("TriangleNormal1");
inline constexpr StringLiteral k_TriangleNormal2("TriangleNormal2");
inline constexpr StringLiteral k_SurfaceArea("SurfaceArea");

inline constexpr StringLiteral k_ExemplarTriangleDumperResults("ExemplarTriangleDumperResults");
inline constexpr StringLiteral k_NXTriangleDumperResults("NXTriangleDumperResults");

inline constexpr float32 k_EPSILON = 0.001;

} // namespace

TEST_CASE("OrientationAnalysis::ExportGBCDTriangleDataFilter: Valid filter execution")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_Small_IN100_GBCD.tar.gz", "6_6_Small_IN100_GBCD");

  std::shared_ptr<make_shared_enabler> app = std::make_shared<make_shared_enabler>();
  app->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterList = Application::Instance()->getFilterList();

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_Small_IN100_GBCD/6_6_Small_IN100_GBCD.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  DataPath smallIn100Group({Constants::k_SmallIN100});
  DataPath featureDataPath = smallIn100Group.createChildPath(Constants::k_Grain_Data);
  DataPath avgEulerAnglesPath = featureDataPath.createChildPath(Constants::k_AvgEulerAngles);
  DataPath triangleDataContainerPath({Constants::k_TriangleDataContainerName});
  DataPath faceDataGroup = triangleDataContainerPath.createChildPath(Constants::k_FaceData);
  DataPath faceLabels = faceDataGroup.createChildPath(Constants::k_FaceLabels);
  DataPath faceNormals = faceDataGroup.createChildPath(Constants::k_FaceNormals);
  DataPath faceAreas = faceDataGroup.createChildPath(Constants::k_FaceAreas);

  DataPath exemplarResultsGroupPath({k_ExemplarTriangleDumperResults});
  DataPath generatedResultsGroupPath({k_NXTriangleDumperResults});

  // Run ExportGBCDTriangleData
  auto outputFile = fs::path(fmt::format("{}/small_in100_gbcd_triangles.ph", unit_test::k_BinaryTestOutputDir));
  {
    ExportGBCDTriangleDataFilter filter;
    Arguments args;
    args.insertOrAssign(ExportGBCDTriangleDataFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(outputFile));
    args.insertOrAssign(ExportGBCDTriangleDataFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(faceLabels));
    args.insertOrAssign(ExportGBCDTriangleDataFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(faceNormals));
    args.insertOrAssign(ExportGBCDTriangleDataFilter::k_SurfaceMeshFaceAreasArrayPath_Key, std::make_any<DataPath>(faceAreas));
    args.insertOrAssign(ExportGBCDTriangleDataFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(avgEulerAnglesPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare the Output triangles files
  auto importDataFilter = filterList->createFilter(k_ImportCSVDataFilterHandle);
  REQUIRE(nullptr != importDataFilter);

  // read in exemplar
  {
    Arguments args;
    CSVWizardData data;
    data.inputFilePath = fmt::format("{}/6_6_Small_IN100_GBCD/6_6_Small_IN100_GBCD_Triangles.ph", unit_test::k_TestFilesDir);
    data.customHeaders = {k_Phi1Right, k_PhiRight, k_Phi2Right, k_Phi1Left, k_PhiLeft, k_Phi2Left, k_TriangleNormal0, k_TriangleNormal1, k_TriangleNormal2, k_SurfaceArea};
    data.dataTypes = {DataType::float32, DataType::float32, DataType::float32, DataType::float32, DataType::float32,
                      DataType::float32, DataType::float64, DataType::float64, DataType::float64, DataType::float64};
    data.startImportRow = 6;
    data.spaceAsDelimiter = true;

    args.insertOrAssign(k_WizardData_Key, std::make_any<CSVWizardData>(data));
    args.insertOrAssign(k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{static_cast<float64>(636474)}}));
    args.insertOrAssign(k_UseExistingGroup_Key, std::make_any<bool>(false));
    args.insertOrAssign(k_CreatedDataGroup_Key, std::make_any<DataPath>(exemplarResultsGroupPath));
    args.insertOrAssign(k_SelectedDataGroup_Key, std::make_any<DataPath>(DataPath{}));

    auto executeResult = importDataFilter->execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // read in generated
  {
    Arguments args;
    CSVWizardData data;
    data.inputFilePath = outputFile.string();
    data.customHeaders = {k_Phi1Right, k_PhiRight, k_Phi2Right, k_Phi1Left, k_PhiLeft, k_Phi2Left, k_TriangleNormal0, k_TriangleNormal1, k_TriangleNormal2, k_SurfaceArea};
    data.dataTypes = {DataType::float32, DataType::float32, DataType::float32, DataType::float32, DataType::float32,
                      DataType::float32, DataType::float64, DataType::float64, DataType::float64, DataType::float64};
    data.startImportRow = 5;
    data.spaceAsDelimiter = true;

    args.insertOrAssign(k_WizardData_Key, std::make_any<CSVWizardData>(data));
    args.insertOrAssign(k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{static_cast<float64>(636474)}}));
    args.insertOrAssign(k_UseExistingGroup_Key, std::make_any<bool>(false));
    args.insertOrAssign(k_CreatedDataGroup_Key, std::make_any<DataPath>(generatedResultsGroupPath));
    args.insertOrAssign(k_SelectedDataGroup_Key, std::make_any<DataPath>(generatedResultsGroupPath));

    auto executeResult = importDataFilter->execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // do comparison
  {
    DataPath phi1RightArrayPath = generatedResultsGroupPath.createChildPath(k_Phi1Right);
    DataPath phiRightArrayPath = generatedResultsGroupPath.createChildPath(k_PhiRight);
    DataPath phi2RightArrayPath = generatedResultsGroupPath.createChildPath(k_Phi2Right);
    DataPath phi1LeftArrayPath = generatedResultsGroupPath.createChildPath(k_Phi1Left);
    DataPath phiLeftArrayPath = generatedResultsGroupPath.createChildPath(k_PhiLeft);
    DataPath phi2LeftArrayPath = generatedResultsGroupPath.createChildPath(k_Phi2Left);
    DataPath triNormal0ArrayPath = generatedResultsGroupPath.createChildPath(k_TriangleNormal0);
    DataPath triNormal1ArrayPath = generatedResultsGroupPath.createChildPath(k_TriangleNormal1);
    DataPath triNormal2ArrayPath = generatedResultsGroupPath.createChildPath(k_TriangleNormal2);
    DataPath surfaceAreaArrayPath = generatedResultsGroupPath.createChildPath(k_SurfaceArea);

    DataPath exemplarPhi1RightArrayPath = exemplarResultsGroupPath.createChildPath(k_Phi1Right);
    DataPath exemplarPhiRightArrayPath = exemplarResultsGroupPath.createChildPath(k_PhiRight);
    DataPath exemplarPhi2RightArrayPath = exemplarResultsGroupPath.createChildPath(k_Phi2Right);
    DataPath exemplarPhi1LeftArrayPath = exemplarResultsGroupPath.createChildPath(k_Phi1Left);
    DataPath exemplarPhiLeftArrayPath = exemplarResultsGroupPath.createChildPath(k_PhiLeft);
    DataPath exemplarPhi2LeftArrayPath = exemplarResultsGroupPath.createChildPath(k_Phi2Left);
    DataPath exemplarTriNormal0ArrayPath = exemplarResultsGroupPath.createChildPath(k_TriangleNormal0);
    DataPath exemplarTriNormal1ArrayPath = exemplarResultsGroupPath.createChildPath(k_TriangleNormal1);
    DataPath exemplarTriNormal2ArrayPath = exemplarResultsGroupPath.createChildPath(k_TriangleNormal2);
    DataPath exemplarSurfaceAreaArrayPath = exemplarResultsGroupPath.createChildPath(k_SurfaceArea);

    REQUIRE(dataStructure.getDataAs<Float32Array>(phi1RightArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32Array>(phiRightArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32Array>(phi2RightArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32Array>(phi1LeftArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32Array>(phiLeftArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32Array>(phi2LeftArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float64Array>(triNormal0ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float64Array>(triNormal1ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float64Array>(triNormal2ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float64Array>(surfaceAreaArrayPath) != nullptr);

    REQUIRE(dataStructure.getDataAs<Float32Array>(exemplarPhi1RightArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32Array>(exemplarPhiRightArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32Array>(exemplarPhi2RightArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32Array>(exemplarPhi1LeftArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32Array>(exemplarPhiLeftArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32Array>(exemplarPhi2LeftArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float64Array>(exemplarTriNormal0ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float64Array>(exemplarTriNormal1ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float64Array>(exemplarTriNormal2ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float64Array>(exemplarSurfaceAreaArrayPath) != nullptr);

    UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, exemplarPhi1RightArrayPath, phi1RightArrayPath, k_EPSILON);
    UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, exemplarPhiRightArrayPath, phiRightArrayPath, k_EPSILON);
    UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, exemplarPhi2RightArrayPath, phi2RightArrayPath, k_EPSILON);
    UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, exemplarPhi1LeftArrayPath, phi1LeftArrayPath, k_EPSILON);
    UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, exemplarPhiLeftArrayPath, phiLeftArrayPath, k_EPSILON);
    UnitTest::CompareFloatArraysWithNans<float32>(dataStructure, exemplarPhi2LeftArrayPath, phi2LeftArrayPath, k_EPSILON);
    UnitTest::CompareFloatArraysWithNans<float64>(dataStructure, exemplarTriNormal0ArrayPath, triNormal0ArrayPath, k_EPSILON);
    UnitTest::CompareFloatArraysWithNans<float64>(dataStructure, exemplarTriNormal1ArrayPath, triNormal1ArrayPath, k_EPSILON);
    UnitTest::CompareFloatArraysWithNans<float64>(dataStructure, exemplarTriNormal2ArrayPath, triNormal2ArrayPath, k_EPSILON);
    UnitTest::CompareFloatArraysWithNans<float64>(dataStructure, exemplarSurfaceAreaArrayPath, surfaceAreaArrayPath, k_EPSILON);
  }
}

TEST_CASE("OrientationAnalysis::ExportGBCDTriangleDataFilter: InValid filter execution")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_Small_IN100_GBCD.tar.gz", "6_6_Small_IN100_GBCD");

  // Instantiate the filter and an Arguments Object
  ExportGBCDTriangleDataFilter filter;
  Arguments args;

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_Small_IN100_GBCD/6_6_Small_IN100_GBCD.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  DataPath smallIn100Group({Constants::k_SmallIN100});
  DataPath featureDataPath = smallIn100Group.createChildPath(Constants::k_Grain_Data);
  DataPath avgEulerAnglesPath = featureDataPath.createChildPath(Constants::k_AvgEulerAngles);
  DataPath triangleDataContainerPath({Constants::k_TriangleDataContainerName});
  DataPath faceDataGroup = triangleDataContainerPath.createChildPath(Constants::k_FaceData);
  DataPath faceLabels = faceDataGroup.createChildPath(Constants::k_FaceLabels);
  DataPath faceNormals = faceDataGroup.createChildPath(Constants::k_FaceNormals);
  DataPath faceAreas = faceDataGroup.createChildPath(Constants::k_FaceAreas);

  DataPath gbcd = faceDataGroup.createChildPath(Constants::k_GBCD_Name);

  SECTION("Invalid output file")
  {
    // Create default Parameters for the filter.
    args.insertOrAssign(ExportGBCDTriangleDataFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path("/Path/To/Output/File/To/Write.dat")));
    args.insertOrAssign(ExportGBCDTriangleDataFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(faceLabels));
    args.insertOrAssign(ExportGBCDTriangleDataFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(faceNormals));
    args.insertOrAssign(ExportGBCDTriangleDataFilter::k_SurfaceMeshFaceAreasArrayPath_Key, std::make_any<DataPath>(faceAreas));
    args.insertOrAssign(ExportGBCDTriangleDataFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(avgEulerAnglesPath));
  }

  SECTION("Inconsistent Tuple Dimensions")
  {
    auto outputFile = fs::path(fmt::format("{}/TriangleDumperTest.ph", unit_test::k_BinaryTestOutputDir));
    // Create default Parameters for the filter.
    args.insertOrAssign(ExportGBCDTriangleDataFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(outputFile));
    args.insertOrAssign(ExportGBCDTriangleDataFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(gbcd));
    args.insertOrAssign(ExportGBCDTriangleDataFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(faceNormals));
    args.insertOrAssign(ExportGBCDTriangleDataFilter::k_SurfaceMeshFaceAreasArrayPath_Key, std::make_any<DataPath>(faceAreas));
    args.insertOrAssign(ExportGBCDTriangleDataFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(avgEulerAnglesPath));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(executeResult.result);
}
