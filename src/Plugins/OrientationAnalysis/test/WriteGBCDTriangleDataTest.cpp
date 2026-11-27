#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/WriteGBCDTriangleDataFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/util/ReadCSVData.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <atomic>
#include <filesystem>
#include <fstream>
#include <optional>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::unit_test;

namespace
{
constexpr StringLiteralType k_ReadCSVData_Key = "read_csv_data_object";
constexpr StringLiteralType k_TupleDims_Key = "tuple_dimensions";
constexpr StringLiteralType k_UseExistingGroup_Key = "use_existing_group";
constexpr StringLiteralType k_SelectedDataGroup_Key = "selected_attribute_matrix_path";
constexpr StringLiteralType k_CreatedDataGroup_Key = "created_data_group_path";

constexpr StringLiteralType k_Phi1Right("Phi1Right");
constexpr StringLiteralType k_PhiRight("PhiRight");
constexpr StringLiteralType k_Phi2Right("Phi2Right");
constexpr StringLiteralType k_Phi1Left("Phi1Left");
constexpr StringLiteralType k_PhiLeft("PhiLeft");
constexpr StringLiteralType k_Phi2Left("Phi2Left");
constexpr StringLiteralType k_TriangleNormal0("TriangleNormal0");
constexpr StringLiteralType k_TriangleNormal1("TriangleNormal1");
constexpr StringLiteralType k_TriangleNormal2("TriangleNormal2");
constexpr StringLiteralType k_SurfaceArea("SurfaceArea");

constexpr StringLiteralType k_ExemplarTriangleDumperResults("ExemplarTriangleDumperResults");
constexpr StringLiteralType k_NXTriangleDumperResults("NXTriangleDumperResults");

constexpr float32 k_EPSILON = 0.001;

template <typename T>
class CancelAfterReadDataStore : public DataStore<T>
{
public:
  CancelAfterReadDataStore(const ShapeType& tupleShape, const ShapeType& componentShape, std::optional<T> initValue, std::atomic_bool& shouldCancel)
  : DataStore<T>(tupleShape, componentShape, initValue)
  , m_ShouldCancel(shouldCancel)
  {
  }

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    Result<> result = DataStore<T>::copyIntoBuffer(startIndex, buffer);
    if(result.valid() && !m_DidCancel)
    {
      m_DidCancel = true;
      m_ShouldCancel.store(true);
    }
    return result;
  }

private:
  std::atomic_bool& m_ShouldCancel;
  mutable bool m_DidCancel = false;
};

} // namespace

TEST_CASE("OrientationAnalysis::WriteGBCDTriangleDataFilter: Valid filter execution")
{
  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_Small_IN100_GBCD.tar.gz", "6_6_Small_IN100_GBCD");

  unit_test::LoadPlugins();
  auto* filterListPtr = Application::Instance()->getFilterList();

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_Small_IN100_GBCD/6_6_Small_IN100_GBCD.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  DataPath smallIn100Group({constants::k_SmallIN100});
  DataPath featureDataPath = smallIn100Group.createChildPath(constants::k_GrainDataWithSpaces);
  DataPath avgEulerAnglesPath = featureDataPath.createChildPath(constants::k_AvgEulerAngles);
  DataPath triangleDataContainerPath({constants::k_TriangleDataContainerName});
  DataPath faceDataGroup = triangleDataContainerPath.createChildPath(constants::k_FaceData);
  DataPath faceLabels = faceDataGroup.createChildPath(constants::k_FaceLabels);
  DataPath faceNormals = faceDataGroup.createChildPath(constants::k_FaceNormals);
  DataPath faceAreas = faceDataGroup.createChildPath(constants::k_FaceAreas);

  DataPath exemplarResultsGroupPath({k_ExemplarTriangleDumperResults});
  DataPath generatedResultsGroupPath({k_NXTriangleDumperResults});

  // Run WriteGBCDTriangleData
  auto outputFile = fs::path(fmt::format("{}/small_in100_gbcd_triangles.ph", unit_test::k_BinaryTestOutputDir));
  {
    WriteGBCDTriangleDataFilter filter;
    Arguments args;
    args.insertOrAssign(WriteGBCDTriangleDataFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(outputFile));
    args.insertOrAssign(WriteGBCDTriangleDataFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(faceLabels));
    args.insertOrAssign(WriteGBCDTriangleDataFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(faceNormals));
    args.insertOrAssign(WriteGBCDTriangleDataFilter::k_SurfaceMeshFaceAreasArrayPath_Key, std::make_any<DataPath>(faceAreas));
    args.insertOrAssign(WriteGBCDTriangleDataFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(avgEulerAnglesPath));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare the Output triangles files
  auto importDataFilter = filterListPtr->createFilter(k_ReadCSVFileFilterHandle);
  REQUIRE(nullptr != importDataFilter);

  // read in exemplar
  {
    Arguments args;
    ReadCSVData data;
    data.input_file_path = fmt::format("{}/6_6_Small_IN100_GBCD/6_6_Small_IN100_GBCD_Triangles.ph", unit_test::k_TestFilesDir);
    data.custom_headers = {k_Phi1Right, k_PhiRight, k_Phi2Right, k_Phi1Left, k_PhiLeft, k_Phi2Left, k_TriangleNormal0, k_TriangleNormal1, k_TriangleNormal2, k_SurfaceArea};
    data.data_types = {CSVType::float32, CSVType::float32, CSVType::float32, CSVType::float32, CSVType::float32,
                      CSVType::float32, CSVType::float64, CSVType::float64, CSVType::float64, CSVType::float64};
    data.skipped_array_mask = {false, false, false, false, false, false, false, false, false, false};
    data.start_import_row = 6;
    data.delimiters = {' '};
    data.tuple_dims = {636474};

    args.insertOrAssign(k_ReadCSVData_Key, std::make_any<ReadCSVData>(data));
    args.insertOrAssign(k_UseExistingGroup_Key, std::make_any<bool>(false));
    args.insertOrAssign(k_CreatedDataGroup_Key, std::make_any<DataPath>(exemplarResultsGroupPath));
    args.insertOrAssign(k_SelectedDataGroup_Key, std::make_any<DataPath>(DataPath{}));

    auto executeResult = importDataFilter->execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // read in generated
  {
    Arguments args;
    ReadCSVData data;
    data.input_file_path = outputFile.string();
    data.custom_headers = {k_Phi1Right, k_PhiRight, k_Phi2Right, k_Phi1Left, k_PhiLeft, k_Phi2Left, k_TriangleNormal0, k_TriangleNormal1, k_TriangleNormal2, k_SurfaceArea};
    data.data_types = {CSVType::float32, CSVType::float32, CSVType::float32, CSVType::float32, CSVType::float32,
                      CSVType::float32, CSVType::float64, CSVType::float64, CSVType::float64, CSVType::float64};
    data.skipped_array_mask = {false, false, false, false, false, false, false, false, false, false};
    data.start_import_row = 5;
    data.delimiters = {' '};
    data.tuple_dims = {636474};

    args.insertOrAssign(k_ReadCSVData_Key, std::make_any<ReadCSVData>(data));
    args.insertOrAssign(k_UseExistingGroup_Key, std::make_any<bool>(false));
    args.insertOrAssign(k_CreatedDataGroup_Key, std::make_any<DataPath>(generatedResultsGroupPath));
    args.insertOrAssign(k_SelectedDataGroup_Key, std::make_any<DataPath>(generatedResultsGroupPath));

    auto executeResult = importDataFilter->execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
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

    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(phi1RightArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(phiRightArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(phi2RightArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(phi1LeftArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(phiLeftArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(phi2LeftArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float64ArrayType>(triNormal0ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float64ArrayType>(triNormal1ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float64ArrayType>(triNormal2ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float64ArrayType>(surfaceAreaArrayPath) != nullptr);

    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(exemplarPhi1RightArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(exemplarPhiRightArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(exemplarPhi2RightArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(exemplarPhi1LeftArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(exemplarPhiLeftArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(exemplarPhi2LeftArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float64ArrayType>(exemplarTriNormal0ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float64ArrayType>(exemplarTriNormal1ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float64ArrayType>(exemplarTriNormal2ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float64ArrayType>(exemplarSurfaceAreaArrayPath) != nullptr);

    unit_test::CompareFloatArraysWithNans<float32>(dataStructure, exemplarPhi1RightArrayPath, phi1RightArrayPath, k_EPSILON);
    unit_test::CompareFloatArraysWithNans<float32>(dataStructure, exemplarPhiRightArrayPath, phiRightArrayPath, k_EPSILON);
    unit_test::CompareFloatArraysWithNans<float32>(dataStructure, exemplarPhi2RightArrayPath, phi2RightArrayPath, k_EPSILON);
    unit_test::CompareFloatArraysWithNans<float32>(dataStructure, exemplarPhi1LeftArrayPath, phi1LeftArrayPath, k_EPSILON);
    unit_test::CompareFloatArraysWithNans<float32>(dataStructure, exemplarPhiLeftArrayPath, phiLeftArrayPath, k_EPSILON);
    unit_test::CompareFloatArraysWithNans<float32>(dataStructure, exemplarPhi2LeftArrayPath, phi2LeftArrayPath, k_EPSILON);
    unit_test::CompareFloatArraysWithNans<float64>(dataStructure, exemplarTriNormal0ArrayPath, triNormal0ArrayPath, k_EPSILON);
    unit_test::CompareFloatArraysWithNans<float64>(dataStructure, exemplarTriNormal1ArrayPath, triNormal1ArrayPath, k_EPSILON);
    unit_test::CompareFloatArraysWithNans<float64>(dataStructure, exemplarTriNormal2ArrayPath, triNormal2ArrayPath, k_EPSILON);
    unit_test::CompareFloatArraysWithNans<float64>(dataStructure, exemplarSurfaceAreaArrayPath, surfaceAreaArrayPath, k_EPSILON);
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::WriteGBCDTriangleDataFilter: Cancellation before publication preserves the destination", "[OrientationAnalysis][WriteGBCDTriangleDataFilter]")
{
  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_Small_IN100_GBCD.tar.gz", "6_6_Small_IN100_GBCD");
  unit_test::LoadPlugins();

  const fs::path sourcePath = fs::path(fmt::format("{}/6_6_Small_IN100_GBCD/6_6_Small_IN100_GBCD.dream3d", unit_test::k_TestFilesDir));
  const DataPath smallIn100Group({constants::k_SmallIN100});
  const DataPath featureEulerAnglesPath = smallIn100Group.createChildPath(constants::k_GrainDataWithSpaces).createChildPath(constants::k_AvgEulerAngles);
  const DataPath faceDataPath = DataPath({constants::k_TriangleDataContainerName}).createChildPath(constants::k_FaceData);
  const DataPath faceLabelsPath = faceDataPath.createChildPath(constants::k_FaceLabels);
  const DataPath faceNormalsPath = faceDataPath.createChildPath(constants::k_FaceNormals);
  const DataPath faceAreasPath = faceDataPath.createChildPath(constants::k_FaceAreas);
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "gbcd_triangle_cancellation.ph";
  const std::string sentinel = "existing triangle bytes";

  for(const bool destinationExists : {true, false})
  {
    DYNAMIC_SECTION((destinationExists ? "Existing destination" : "Absent destination"))
    {
      std::error_code cleanupError;
      fs::remove(outputPath, cleanupError);
      REQUIRE_FALSE(cleanupError);
      if(destinationExists)
      {
        std::ofstream outputStream(outputPath, std::ios::binary);
        REQUIRE(outputStream.is_open());
        outputStream << sentinel;
      }

      DataStructure dataStructure = unit_test::LoadDataStructure(sourcePath);
      auto& eulerAnglesArray = dataStructure.getDataRefAs<Float32ArrayType>(featureEulerAnglesPath);
      const auto& sourceStore = eulerAnglesArray.getDataStoreRef();
      std::atomic_bool shouldCancel = false;
      auto cancelStore = std::make_shared<CancelAfterReadDataStore<float32>>(eulerAnglesArray.getTupleShape(), eulerAnglesArray.getComponentShape(), 0.0F, shouldCancel);
      std::vector<float32> eulerAngles(eulerAnglesArray.getSize());
      auto copyIntoBufferResult = sourceStore.copyIntoBuffer(0, nonstd::span<float32>(eulerAngles.data(), eulerAngles.size()));
      SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
      auto copyFromBufferResult = cancelStore->copyFromBuffer(0, nonstd::span<const float32>(eulerAngles.data(), eulerAngles.size()));
      SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult);
      auto setStoreResult = eulerAnglesArray.setDataStore(cancelStore);
      SIMPLNX_RESULT_REQUIRE_VALID(setStoreResult);

      WriteGBCDTriangleDataFilter filter;
      Arguments args;
      args.insertOrAssign(WriteGBCDTriangleDataFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(outputPath));
      args.insertOrAssign(WriteGBCDTriangleDataFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(faceLabelsPath));
      args.insertOrAssign(WriteGBCDTriangleDataFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(faceNormalsPath));
      args.insertOrAssign(WriteGBCDTriangleDataFilter::k_SurfaceMeshFaceAreasArrayPath_Key, std::make_any<DataPath>(faceAreasPath));
      args.insertOrAssign(WriteGBCDTriangleDataFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(featureEulerAnglesPath));

      auto preflightResult2 = filter.preflight(dataStructure, args).output_actions;
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult2);
      const auto executeResult = filter.execute(dataStructure, args, nullptr, {}, shouldCancel);

      REQUIRE(shouldCancel.load());
      SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
      REQUIRE_FALSE(executeResult.result.errors().empty());
      REQUIRE(executeResult.result.errors().front().code == -1);
      if(destinationExists)
      {
        std::ifstream inputStream(outputPath, std::ios::binary);
        REQUIRE(inputStream.is_open());
        const std::string contents((std::istreambuf_iterator<char>(inputStream)), std::istreambuf_iterator<char>());
        REQUIRE(contents == sentinel);
      }
      else
      {
        REQUIRE_FALSE(fs::exists(outputPath));
      }

      unit_test::CheckArraysInheritTupleDims(dataStructure);
    }
  }
}

TEST_CASE("OrientationAnalysis::WriteGBCDTriangleDataFilter: InValid filter execution")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_Small_IN100_GBCD.tar.gz", "6_6_Small_IN100_GBCD");

  // Instantiate the filter and an Arguments Object
  WriteGBCDTriangleDataFilter filter;
  Arguments args;

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_Small_IN100_GBCD/6_6_Small_IN100_GBCD.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);

  DataPath smallIn100Group({constants::k_SmallIN100});
  DataPath featureDataPath = smallIn100Group.createChildPath(constants::k_GrainDataWithSpaces);
  DataPath avgEulerAnglesPath = featureDataPath.createChildPath(constants::k_AvgEulerAngles);
  DataPath triangleDataContainerPath({constants::k_TriangleDataContainerName});
  DataPath faceDataGroup = triangleDataContainerPath.createChildPath(constants::k_FaceData);
  DataPath faceLabels = faceDataGroup.createChildPath(constants::k_FaceLabels);
  DataPath faceNormals = faceDataGroup.createChildPath(constants::k_FaceNormals);
  DataPath faceAreas = faceDataGroup.createChildPath(constants::k_FaceAreas);

  DataPath gbcd = faceDataGroup.createChildPath(constants::k_GbcdName);

  SECTION("Invalid output file")
  {
    // Create default Parameters for the filter.
    args.insertOrAssign(WriteGBCDTriangleDataFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path("/Path/To/Output/File/To/Write.dat")));
    args.insertOrAssign(WriteGBCDTriangleDataFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(faceLabels));
    args.insertOrAssign(WriteGBCDTriangleDataFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(faceNormals));
    args.insertOrAssign(WriteGBCDTriangleDataFilter::k_SurfaceMeshFaceAreasArrayPath_Key, std::make_any<DataPath>(faceAreas));
    args.insertOrAssign(WriteGBCDTriangleDataFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(avgEulerAnglesPath));
  }

  SECTION("Inconsistent Tuple Dimensions")
  {
    auto outputFile = fs::path(fmt::format("{}/TriangleDumperTest.ph", unit_test::k_BinaryTestOutputDir));
    // Create default Parameters for the filter.
    args.insertOrAssign(WriteGBCDTriangleDataFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(outputFile));
    args.insertOrAssign(WriteGBCDTriangleDataFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(gbcd));
    args.insertOrAssign(WriteGBCDTriangleDataFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(faceNormals));
    args.insertOrAssign(WriteGBCDTriangleDataFilter::k_SurfaceMeshFaceAreasArrayPath_Key, std::make_any<DataPath>(faceAreas));
    args.insertOrAssign(WriteGBCDTriangleDataFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(avgEulerAnglesPath));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::WriteGBCDTriangleDataFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][WriteGBCDTriangleDataFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "WriteGBCDTriangleDataFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "WriteGBCDTriangleDataFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterListPtr);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilterPtr = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilterPtr != nullptr);

      const IFilter* filterPtr = pipelineFilterPtr->getFilter();
      REQUIRE(filterPtr != nullptr);
      REQUIRE(filterPtr->uuid() == FilterTraits<WriteGBCDTriangleDataFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
      CHECK(args.value<FileSystemPathParameter::ValueType>(WriteGBCDTriangleDataFilter::k_OutputFile_Key) == fs::path("/test/path/file.txt"));
      CHECK(args.value<DataPath>(WriteGBCDTriangleDataFilter::k_SurfaceMeshFaceLabelsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(WriteGBCDTriangleDataFilter::k_SurfaceMeshFaceNormalsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(WriteGBCDTriangleDataFilter::k_SurfaceMeshFaceAreasArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(WriteGBCDTriangleDataFilter::k_FeatureEulerAnglesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}
