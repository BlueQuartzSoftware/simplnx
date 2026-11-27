#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/WriteGBCDGMTFileFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
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
constexpr StringLiteralType k_FaceEnsembleDataPath("FaceEnsembleData [NX]");

constexpr StringLiteralType k_ReadCSVData_Key = "read_csv_data_object";
constexpr StringLiteralType k_TupleDims_Key = "tuple_dimensions";
constexpr StringLiteralType k_UseExistingGroup_Key = "use_existing_group";
constexpr StringLiteralType k_SelectedDataGroup_Key = "selected_attribute_matrix_path";
constexpr StringLiteralType k_CreatedDataGroup_Key = "created_data_group_path";

constexpr StringLiteralType k_ExemplarGMT1("ExemplarGMT1");
constexpr StringLiteralType k_ExemplarGMT2("ExemplarGMT2");
constexpr StringLiteralType k_ExemplarGMT3("ExemplarGMT3");
constexpr StringLiteralType k_GMT1("GMT1");
constexpr StringLiteralType k_GMT2("GMT2");
constexpr StringLiteralType k_GMT3("GMT3");

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

TEST_CASE("OrientationAnalysis::WriteGBCDGMTFileFilter", "[OrientationAnalysis][WriteGBCDGMTFile]")
{
  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_Small_IN100_GBCD.tar.gz", "6_6_Small_IN100_GBCD");

  unit_test::LoadPlugins();
  auto* filterListPtr = Application::Instance()->getFilterList();

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_Small_IN100_GBCD/6_6_Small_IN100_GBCD.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);
  const DataPath smallIn100Group({nx::core::constants::k_SmallIN100});
  const DataPath featureDataPath = smallIn100Group.createChildPath(constants::k_GrainDataWithSpaces);
  const DataPath avgEulerAnglesPath = featureDataPath.createChildPath(constants::k_AvgEulerAngles);
  const DataPath featurePhasesPath = featureDataPath.createChildPath(constants::k_Phases);

  const DataPath ensembleDataPath = smallIn100Group.createChildPath(constants::k_PhaseData);
  const DataPath crystalStructurePath = ensembleDataPath.createChildPath(constants::k_CrystalStructures);

  const DataPath triangleDataContainerPath({constants::k_TriangleDataContainerName});
  const DataPath faceDataGroup = triangleDataContainerPath.createChildPath(constants::k_FaceData);
  const DataPath faceEnsemblePath = triangleDataContainerPath.createChildPath(k_FaceEnsembleDataPath);

  const DataPath faceLabels = faceDataGroup.createChildPath(constants::k_FaceLabels);
  const DataPath faceNormals = faceDataGroup.createChildPath(constants::k_FaceNormals);
  const DataPath faceAreas = faceDataGroup.createChildPath(constants::k_FaceAreas);

  const DataPath gbcdArrayPath = triangleDataContainerPath.createChildPath("FaceEnsembleData").createChildPath(constants::k_GbcdName);

  const DataPath gmtGroupPath = triangleDataContainerPath.createChildPath("GMTResults");

  SECTION("Pole Figures for Sigma 3  60@[111]")
  {
    // Create Pole Figure
    auto outputFile = fs::path(fmt::format("{}/small_in100_sigma_3_1.dat", unit_test::k_BinaryTestOutputDir));
    {
      const WriteGBCDGMTFileFilter gmtFilter;
      Arguments args;

      args.insertOrAssign(WriteGBCDGMTFileFilter::k_PhaseOfInterest_Key, std::make_any<Int32ParameterType::ValueType>(1));
      args.insertOrAssign(WriteGBCDGMTFileFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32ParameterType::ValueType>({60.0F, 1.0F, 1.0F, 1.0F}));
      args.insertOrAssign(WriteGBCDGMTFileFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(outputFile));
      args.insertOrAssign(WriteGBCDGMTFileFilter::k_GBCDArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(gbcdArrayPath));
      args.insertOrAssign(WriteGBCDGMTFileFilter::k_CrystalStructuresArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(crystalStructurePath));

      // Preflight the filter and check result
      auto preflightResult = gmtFilter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

      // Execute the filter and check the result
      auto executeResult = gmtFilter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }

    // Compare the Output Pole Figure
    auto importDataFilter = filterListPtr->createFilter(k_ReadCSVFileFilterHandle);
    REQUIRE(nullptr != importDataFilter);

    {
      Arguments args;
      ReadCSVData data;
      data.input_file_path = fmt::format("{}/6_6_Small_IN100_GBCD/small_in100_sigma_3_1.dat", unit_test::k_TestFilesDir);
      data.custom_headers = {k_ExemplarGMT1, k_ExemplarGMT2, k_ExemplarGMT3};
      data.data_types = {CSVType::float32, CSVType::float32, CSVType::float32};
      data.skipped_array_mask = {false, false, false};
      data.start_import_row = 2;
      data.delimiters = {' '};
      data.tuple_dims = {3751};

      args.insertOrAssign(k_ReadCSVData_Key, std::make_any<ReadCSVData>(data));
      args.insertOrAssign(k_UseExistingGroup_Key, std::make_any<bool>(false));
      args.insertOrAssign(k_CreatedDataGroup_Key, std::make_any<DataPath>(gmtGroupPath));
      args.insertOrAssign(k_SelectedDataGroup_Key, std::make_any<DataPath>(faceEnsemblePath));

      auto executeResult = importDataFilter->execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }

    {
      Arguments args;
      ReadCSVData data;
      data.input_file_path = outputFile.string();
      data.custom_headers = {k_GMT1, k_GMT2, k_GMT3};
      data.data_types = {CSVType::float32, CSVType::float32, CSVType::float32};
      data.skipped_array_mask = {false, false, false};
      data.start_import_row = 2;
      data.delimiters = {' '};
      data.tuple_dims = {3751};

      args.insertOrAssign(k_ReadCSVData_Key, std::make_any<ReadCSVData>(data));
      args.insertOrAssign(k_UseExistingGroup_Key, std::make_any<bool>(true));
      args.insertOrAssign(k_CreatedDataGroup_Key, std::make_any<DataPath>(gmtGroupPath));
      args.insertOrAssign(k_SelectedDataGroup_Key, std::make_any<DataPath>(gmtGroupPath));

      auto executeResult = importDataFilter->execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }

    const DataPath gmt1ArrayPath = gmtGroupPath.createChildPath(k_GMT1);
    const DataPath gmt2ArrayPath = gmtGroupPath.createChildPath(k_GMT2);
    const DataPath gmt3ArrayPath = gmtGroupPath.createChildPath(k_GMT3);
    const DataPath exemplarGmt1ArrayPath = gmtGroupPath.createChildPath(k_ExemplarGMT1);
    const DataPath exemplarGmt2ArrayPath = gmtGroupPath.createChildPath(k_ExemplarGMT2);
    const DataPath exemplarGmt3ArrayPath = gmtGroupPath.createChildPath(k_ExemplarGMT3);
    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(gmt1ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(gmt2ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(gmt3ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(exemplarGmt1ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(exemplarGmt2ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(exemplarGmt3ArrayPath) != nullptr);

    unit_test::CompareFloatArraysWithNans<float32>(dataStructure, exemplarGmt1ArrayPath, gmt1ArrayPath);
    unit_test::CompareFloatArraysWithNans<float32>(dataStructure, exemplarGmt2ArrayPath, gmt2ArrayPath);
    unit_test::CompareFloatArraysWithNans<float32>(dataStructure, exemplarGmt3ArrayPath, gmt3ArrayPath);
  }

  SECTION("Pole Figures for Sigma 9  39@[110]")
  {
    // Create Pole Figure
    auto outputFile = fs::path(fmt::format("{}/small_in100_sigma_9_1.dat", unit_test::k_BinaryTestOutputDir));

    {
      const WriteGBCDGMTFileFilter filter;
      Arguments args;

      args.insertOrAssign(WriteGBCDGMTFileFilter::k_PhaseOfInterest_Key, std::make_any<Int32ParameterType::ValueType>(1));
      args.insertOrAssign(WriteGBCDGMTFileFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32ParameterType::ValueType>({39.0F, 1.0F, 1.0F, 0.0F}));
      args.insertOrAssign(WriteGBCDGMTFileFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(outputFile));
      args.insertOrAssign(WriteGBCDGMTFileFilter::k_GBCDArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(gbcdArrayPath));
      args.insertOrAssign(WriteGBCDGMTFileFilter::k_CrystalStructuresArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(crystalStructurePath));

      // Preflight the filter and check result
      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

      // Execute the filter and check the result
      auto executeResult = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }

    // Compare the Output Pole Figure
    auto importDataFilter = filterListPtr->createFilter(k_ReadCSVFileFilterHandle);
    REQUIRE(nullptr != importDataFilter);

    {
      Arguments args;
      ReadCSVData data;
      data.input_file_path = fmt::format("{}/6_6_Small_IN100_GBCD/small_in100_sigma_9_1.dat", unit_test::k_TestFilesDir);
      data.custom_headers = {k_ExemplarGMT1, k_ExemplarGMT2, k_ExemplarGMT3};
      data.data_types = {CSVType::float32, CSVType::float32, CSVType::float32};
      data.skipped_array_mask = {false, false, false};
      data.start_import_row = 2;
      data.delimiters = {' '};
      data.tuple_dims = {3751};

      args.insertOrAssign(k_ReadCSVData_Key, std::make_any<ReadCSVData>(data));
      args.insertOrAssign(k_UseExistingGroup_Key, std::make_any<bool>(false));
      args.insertOrAssign(k_CreatedDataGroup_Key, std::make_any<DataPath>(gmtGroupPath));
      args.insertOrAssign(k_SelectedDataGroup_Key, std::make_any<DataPath>(faceEnsemblePath));

      auto executeResult = importDataFilter->execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }

    {
      Arguments args;
      ReadCSVData data;
      data.input_file_path = outputFile.string();
      data.custom_headers = {k_GMT1, k_GMT2, k_GMT3};
      data.data_types = {CSVType::float32, CSVType::float32, CSVType::float32};
      data.skipped_array_mask = {false, false, false};
      data.start_import_row = 2;
      data.delimiters = {' '};
      data.tuple_dims = {3751};

      args.insertOrAssign(k_ReadCSVData_Key, std::make_any<ReadCSVData>(data));
      args.insertOrAssign(k_UseExistingGroup_Key, std::make_any<bool>(true));
      args.insertOrAssign(k_CreatedDataGroup_Key, std::make_any<DataPath>(gmtGroupPath));
      args.insertOrAssign(k_SelectedDataGroup_Key, std::make_any<DataPath>(gmtGroupPath));

      auto executeResult = importDataFilter->execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }

    const DataPath gmt1ArrayPath = gmtGroupPath.createChildPath(k_GMT1);
    const DataPath gmt2ArrayPath = gmtGroupPath.createChildPath(k_GMT2);
    const DataPath gmt3ArrayPath = gmtGroupPath.createChildPath(k_GMT3);
    const DataPath exemplarGmt1ArrayPath = gmtGroupPath.createChildPath(k_ExemplarGMT1);
    const DataPath exemplarGmt2ArrayPath = gmtGroupPath.createChildPath(k_ExemplarGMT2);
    const DataPath exemplarGmt3ArrayPath = gmtGroupPath.createChildPath(k_ExemplarGMT3);
    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(gmt1ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(gmt2ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(gmt3ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(exemplarGmt1ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(exemplarGmt2ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(exemplarGmt3ArrayPath) != nullptr);

    unit_test::CompareFloatArraysWithNans<float32>(dataStructure, exemplarGmt1ArrayPath, gmt1ArrayPath);
    unit_test::CompareFloatArraysWithNans<float32>(dataStructure, exemplarGmt2ArrayPath, gmt2ArrayPath);
    unit_test::CompareFloatArraysWithNans<float32>(dataStructure, exemplarGmt3ArrayPath, gmt3ArrayPath);
  }

  SECTION("Pole Figures for Sigma 11  50.5@[110]")
  {
    // Create Pole Figure
    auto outputFile = fs::path(fmt::format("{}/small_in100_sigma_11_1.dat", unit_test::k_BinaryTestOutputDir));

    {
      const WriteGBCDGMTFileFilter filter;
      Arguments args;

      args.insertOrAssign(WriteGBCDGMTFileFilter::k_PhaseOfInterest_Key, std::make_any<Int32ParameterType::ValueType>(1));
      args.insertOrAssign(WriteGBCDGMTFileFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32ParameterType::ValueType>({50.5F, 1.0F, 1.0F, 0.0F}));
      args.insertOrAssign(WriteGBCDGMTFileFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(outputFile));
      args.insertOrAssign(WriteGBCDGMTFileFilter::k_GBCDArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(gbcdArrayPath));
      args.insertOrAssign(WriteGBCDGMTFileFilter::k_CrystalStructuresArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(crystalStructurePath));

      // Preflight the filter and check result
      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

      // Execute the filter and check the result
      auto executeResult = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }

    // Compare the Output Pole Figure
    auto importDataFilter = filterListPtr->createFilter(k_ReadCSVFileFilterHandle);
    REQUIRE(nullptr != importDataFilter);

    {
      Arguments args;
      ReadCSVData data;
      data.input_file_path = fmt::format("{}/6_6_Small_IN100_GBCD/small_in100_sigma_11_1.dat", unit_test::k_TestFilesDir);
      data.custom_headers = {k_ExemplarGMT1, k_ExemplarGMT2, k_ExemplarGMT3};
      data.data_types = {CSVType::float32, CSVType::float32, CSVType::float32};
      data.skipped_array_mask = {false, false, false};
      data.start_import_row = 2;
      data.delimiters = {' '};
      data.tuple_dims = {3751};

      args.insertOrAssign(k_ReadCSVData_Key, std::make_any<ReadCSVData>(data));
      args.insertOrAssign(k_UseExistingGroup_Key, std::make_any<bool>(false));
      args.insertOrAssign(k_CreatedDataGroup_Key, std::make_any<DataPath>(gmtGroupPath));
      args.insertOrAssign(k_SelectedDataGroup_Key, std::make_any<DataPath>(faceEnsemblePath));

      auto executeResult = importDataFilter->execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }

    {
      Arguments args;
      ReadCSVData data;
      data.input_file_path = outputFile.string();
      data.custom_headers = {k_GMT1, k_GMT2, k_GMT3};
      data.data_types = {CSVType::float32, CSVType::float32, CSVType::float32};
      data.skipped_array_mask = {false, false, false};
      data.start_import_row = 2;
      data.delimiters = {' '};
      data.tuple_dims = {3751};

      args.insertOrAssign(k_ReadCSVData_Key, std::make_any<ReadCSVData>(data));
      args.insertOrAssign(k_UseExistingGroup_Key, std::make_any<bool>(true));
      args.insertOrAssign(k_CreatedDataGroup_Key, std::make_any<DataPath>(gmtGroupPath));
      args.insertOrAssign(k_SelectedDataGroup_Key, std::make_any<DataPath>(gmtGroupPath));

      auto executeResult = importDataFilter->execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    }

    const DataPath gmt1ArrayPath = gmtGroupPath.createChildPath(k_GMT1);
    const DataPath gmt2ArrayPath = gmtGroupPath.createChildPath(k_GMT2);
    const DataPath gmt3ArrayPath = gmtGroupPath.createChildPath(k_GMT3);
    const DataPath exemplarGmt1ArrayPath = gmtGroupPath.createChildPath(k_ExemplarGMT1);
    const DataPath exemplarGmt2ArrayPath = gmtGroupPath.createChildPath(k_ExemplarGMT2);
    const DataPath exemplarGmt3ArrayPath = gmtGroupPath.createChildPath(k_ExemplarGMT3);
    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(gmt1ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(gmt2ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(gmt3ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(exemplarGmt1ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(exemplarGmt2ArrayPath) != nullptr);
    REQUIRE(dataStructure.getDataAs<Float32ArrayType>(exemplarGmt3ArrayPath) != nullptr);

    unit_test::CompareFloatArraysWithNans<float32>(dataStructure, exemplarGmt1ArrayPath, gmt1ArrayPath);
    unit_test::CompareFloatArraysWithNans<float32>(dataStructure, exemplarGmt2ArrayPath, gmt2ArrayPath);
    unit_test::CompareFloatArraysWithNans<float32>(dataStructure, exemplarGmt3ArrayPath, gmt3ArrayPath);
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/gbcd_gmt.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::WriteGBCDGMTFileFilter: Cancellation before publication preserves the destination", "[OrientationAnalysis][WriteGBCDGMTFile]")
{
  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_Small_IN100_GBCD.tar.gz", "6_6_Small_IN100_GBCD");
  unit_test::LoadPlugins();

  const fs::path sourcePath = fs::path(fmt::format("{}/6_6_Small_IN100_GBCD/6_6_Small_IN100_GBCD.dream3d", unit_test::k_TestFilesDir));
  const DataPath smallIn100Group({nx::core::constants::k_SmallIN100});
  const DataPath crystalStructurePath = smallIn100Group.createChildPath(constants::k_PhaseData).createChildPath(constants::k_CrystalStructures);
  const DataPath gbcdArrayPath = DataPath({constants::k_TriangleDataContainerName}).createChildPath("FaceEnsembleData").createChildPath(constants::k_GbcdName);
  const fs::path outputPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "gbcd_gmt_cancellation.dat";
  const std::string sentinel = "existing GMT bytes";

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
      auto& gbcdArray = dataStructure.getDataRefAs<Float64ArrayType>(gbcdArrayPath);
      const auto& sourceStore = gbcdArray.getDataStoreRef();
      std::atomic_bool shouldCancel = false;
      auto cancelStore = std::make_shared<CancelAfterReadDataStore<float64>>(gbcdArray.getTupleShape(), gbcdArray.getComponentShape(), 0.0, shouldCancel);
      std::vector<float64> gbcdValues(gbcdArray.getSize());
      auto copyIntoBufferResult = sourceStore.copyIntoBuffer(0, nonstd::span<float64>(gbcdValues.data(), gbcdValues.size()));
      SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
      auto copyFromBufferResult = cancelStore->copyFromBuffer(0, nonstd::span<const float64>(gbcdValues.data(), gbcdValues.size()));
      SIMPLNX_RESULT_REQUIRE_VALID(copyFromBufferResult);
      auto setStoreResult = gbcdArray.setDataStore(cancelStore);
      SIMPLNX_RESULT_REQUIRE_VALID(setStoreResult);

      WriteGBCDGMTFileFilter filter;
      Arguments args;
      args.insertOrAssign(WriteGBCDGMTFileFilter::k_PhaseOfInterest_Key, std::make_any<Int32ParameterType::ValueType>(1));
      args.insertOrAssign(WriteGBCDGMTFileFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32ParameterType::ValueType>({60.0F, 1.0F, 1.0F, 1.0F}));
      args.insertOrAssign(WriteGBCDGMTFileFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(outputPath));
      args.insertOrAssign(WriteGBCDGMTFileFilter::k_GBCDArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(gbcdArrayPath));
      args.insertOrAssign(WriteGBCDGMTFileFilter::k_CrystalStructuresArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(crystalStructurePath));

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

TEST_CASE("OrientationAnalysis::WriteGBCDGMTFileFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][WriteGBCDGMTFileFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "WriteGBCDGMTFileFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "WriteGBCDGMTFileFilter.json"},
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
      REQUIRE(filterPtr->uuid() == FilterTraits<WriteGBCDGMTFileFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
      CHECK(args.value<int32>(WriteGBCDGMTFileFilter::k_PhaseOfInterest_Key) == 5);
      // Complex type (AxisAngleFilterParameterConverter<float32>) - verified by successful pipeline loading
      CHECK(args.value<FileSystemPathParameter::ValueType>(WriteGBCDGMTFileFilter::k_OutputFile_Key) == fs::path("/test/path/file.txt"));
      CHECK(args.value<DataPath>(WriteGBCDGMTFileFilter::k_GBCDArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(WriteGBCDGMTFileFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}

TEST_CASE("OrientationAnalysis::WriteGBCDGMTFileFilter: Invalid GBCD Component Shape", "[OrientationAnalysis][WriteGBCDGMTFile][ComponentShape]")
{
  const auto componentShape = GENERATE(ShapeType{1}, ShapeType{2, 2, 2, 2, 2, 1}, ShapeType{0, 2, 2, 2, 2, 2}, ShapeType{2, 0, 2, 2, 2, 2}, ShapeType{2, 2, 0, 2, 2, 2}, ShapeType{2, 2, 2, 0, 2, 2},
                                       ShapeType{2, 2, 2, 2, 0, 2});
  CAPTURE(componentShape);

  DataStructure dataStructure;
  const DataPath gbcdPath({"Malformed GBCD"});
  const DataPath crystalStructuresPath({"Crystal Structures"});
  auto* gbcdPtr = unit_test::CreateTestDataArray<float64>(dataStructure, gbcdPath.getTargetName(), {2}, componentShape);
  REQUIRE(gbcdPtr != nullptr);
  auto* crystalStructuresPtr = unit_test::CreateTestDataArray<uint32>(dataStructure, crystalStructuresPath.getTargetName(), {2}, {1});
  REQUIRE(crystalStructuresPtr != nullptr);

  WriteGBCDGMTFileFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(WriteGBCDGMTFileFilter::k_PhaseOfInterest_Key, std::make_any<int32>(1));
  args.insertOrAssign(WriteGBCDGMTFileFilter::k_GBCDArrayPath_Key, std::make_any<DataPath>(gbcdPath));
  args.insertOrAssign(WriteGBCDGMTFileFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructuresPath));
  args.insertOrAssign(WriteGBCDGMTFileFilter::k_OutputFile_Key, std::make_any<fs::path>(fs::path(unit_test::k_BinaryTestOutputDir.view()) / "invalid_gbcd_shape.dat"));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions);
  REQUIRE(preflightResult.output_actions.errors().size() == 1);
  REQUIRE(preflightResult.output_actions.errors()[0].code == -96713);
}

TEST_CASE("OrientationAnalysis::WriteGBCDGMTFileFilter: Invalid selected phase metadata", "[OrientationAnalysis][WriteGBCDGMTFile]")
{
  unit_test::LoadPlugins();

  const uint32 crystalStructure = GENERATE(11U, 999U);
  const bool missingPhase = GENERATE(false, true);
  CAPTURE(crystalStructure, missingPhase);
  DataStructure dataStructure;
  const DataPath gbcdPath({"GBCD"});
  const DataPath crystalStructuresPath({"Crystal Structures"});
  REQUIRE(unit_test::CreateTestDataArray<float64>(dataStructure, gbcdPath.getTargetName(), {2}, {1, 1, 1, 1, 1, 2}) != nullptr);
  REQUIRE(unit_test::CreateTestDataArray<uint32>(dataStructure, crystalStructuresPath.getTargetName(), {missingPhase ? 1ULL : 2ULL}, {1}) != nullptr);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32ArrayType>(crystalStructuresPath));
  auto& crystalStructures = dataStructure.getDataRefAs<UInt32ArrayType>(crystalStructuresPath);
  if(!missingPhase)
  {
    crystalStructures[1] = crystalStructure;
  }

  WriteGBCDGMTFileFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(WriteGBCDGMTFileFilter::k_PhaseOfInterest_Key, std::make_any<int32>(1));
  args.insertOrAssign(WriteGBCDGMTFileFilter::k_GBCDArrayPath_Key, std::make_any<DataPath>(gbcdPath));
  args.insertOrAssign(WriteGBCDGMTFileFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructuresPath));
  args.insertOrAssign(WriteGBCDGMTFileFilter::k_OutputFile_Key, std::make_any<fs::path>(fs::path(unit_test::k_BinaryTestOutputDir.view()) / "invalid_gbcd_crystal_structure.dat"));

  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().size() == 1);
  REQUIRE(executeResult.result.errors()[0].code == (missingPhase ? -96715 : -96714));
  const auto& message = executeResult.result.errors()[0].message;
  REQUIRE(message.find("phase index 1") != std::string::npos);
  REQUIRE(message.find(crystalStructuresPath.toString()) != std::string::npos);
  if(!missingPhase)
  {
    REQUIRE(message.find(std::to_string(crystalStructure)) != std::string::npos);
    REQUIRE(message.find("0-10") != std::string::npos);
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
