#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumericTypeParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/RecordingFileReplaceHandler.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/ComputeGBCDMetricBasedFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
const DataPath k_TriangleDataContainerPath({k_TriangleDataContainerName});
const DataPath k_SmallIN100Path({k_SmallIN100});

const DataPath k_NodeTypesPath = k_TriangleDataContainerPath.createChildPath(k_VertexData).createChildPath(k_NodeType);

const DataPath k_FaceDataPath = k_TriangleDataContainerPath.createChildPath(k_FaceData);
const DataPath k_FaceLabelsPath = k_FaceDataPath.createChildPath(k_FaceLabels);
const DataPath k_FaceNormalsPath = k_FaceDataPath.createChildPath(k_FaceNormals);
const DataPath k_FaceAreasPath = k_FaceDataPath.createChildPath(k_FaceAreas);

const DataPath k_FeatureFaceLabelsPath = k_TriangleDataContainerPath.createChildPath(k_FaceFeatureData).createChildPath(k_FaceLabels);

const DataPath k_AvgEulerAnglesPath = k_SmallIN100Path.createChildPath(k_Grain_Data).createChildPath(k_AvgEulerAngles);
const DataPath k_PhasesPath = k_SmallIN100Path.createChildPath(k_Grain_Data).createChildPath(k_Phases);
const DataPath k_CrystalStructuresPath = k_SmallIN100Path.createChildPath(k_Phase_Data).createChildPath(k_CrystalStructures);

const DataPath k_ExemplarDistributionPath({"6_6_distribution"});
const DataPath k_ExemplarErrorPath({"6_6_errors"});
const DataPath k_ComputedDistributionPath({"NX_distribution"});
const DataPath k_ComputedErrorPath({"NX_errors"});
} // namespace

TEST_CASE("OrientationAnalysis::ComputeGBCDMetricBasedFilter: Valid Filter Execution", "[OrientationAnalysis][ComputeGBCDMetricBasedFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "compute_gbcd_metric_based.tar.gz", "compute_gbcd_metric_based");

  UnitTest::LoadPlugins();
  const auto* filterListPtr = Application::Instance()->getFilterList();

  // Read Exemplar DREAM3D File Input
  auto exemplarInputFilePath = fs::path(fmt::format("{}/compute_gbcd_metric_based/compute_gbcd_metric_based.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarInputFilePath);

  const fs::path exemplarDistOutput(fmt::format("{}/compute_gbcd_metric_based/gbcd_distribution_1.dat", unit_test::k_TestFilesDir));
  const fs::path exemplarErrorsOutput(fmt::format("{}/compute_gbcd_metric_based/gbcd_distribution_errors_1.dat", unit_test::k_TestFilesDir));
  const fs::path computedDistOutput(fmt::format("{}/compute_gbcd_metric_based/7_0_computed_gbcd_distribution_1.dat", unit_test::k_TestFilesDir));
  const fs::path computedErrorsOutput(fmt::format("{}/compute_gbcd_metric_based/7_0_computed_gbcd_distribution_errors_1.dat", unit_test::k_TestFilesDir));

  // Run the ComputeGBCDMetricBased filter
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    const ComputeGBCDMetricBasedFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_PhaseOfInterest_Key, std::make_any<int32>(1));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1.0f, 1.0f, 1.0f, 60.0f}));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_ChosenLimitDists_Key, std::make_any<ChoicesParameter::ValueType>(0));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_NumSamplPts_Key, std::make_any<int32>(3000));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_ExcludeTripleLines_Key, std::make_any<bool>(false));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_DistOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedDistOutput));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_ErrOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedErrorsOutput));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_SaveRelativeErr_Key, std::make_any<bool>(false));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleDataContainerPath));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_NodeTypesArrayPath_Key, std::make_any<DataPath>(k_NodeTypesPath));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FaceLabelsPath));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(k_FaceNormalsPath));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_SurfaceMeshFaceAreasArrayPath_Key, std::make_any<DataPath>(k_FaceAreasPath));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_SurfaceMeshFeatureFaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FeatureFaceLabelsPath));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(k_AvgEulerAnglesPath));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // read in exemplar and computed data files for comparison
  {

    auto filter = filterListPtr->createFilter(k_ReadTextDataArrayFilterHandle);
    REQUIRE(nullptr != filter);
    // exemplar distribution
    {
      Arguments args;
      args.insertOrAssign(ReadTextDataArrayFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(exemplarDistOutput));
      args.insertOrAssign(ReadTextDataArrayFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(nx::core::NumericType::float32));
      args.insertOrAssign(ReadTextDataArrayFilter::k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{static_cast<double>(3052)}}));
      args.insertOrAssign(ReadTextDataArrayFilter::k_NComp_Key, std::make_any<uint64>(3));
      args.insertOrAssign(ReadTextDataArrayFilter::k_NSkipLines_Key, std::make_any<uint64>(1));
      args.insertOrAssign(ReadTextDataArrayFilter::k_DelimiterChoice_Key, std::make_any<ChoicesParameter::ValueType>(2));
      args.insertOrAssign(ReadTextDataArrayFilter::k_DataArrayPath_Key, std::make_any<DataPath>(k_ExemplarDistributionPath));

      auto executeResult = filter->execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    }
    // exemplar errors
    {
      Arguments args;
      args.insertOrAssign(ReadTextDataArrayFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(exemplarErrorsOutput));
      args.insertOrAssign(ReadTextDataArrayFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(nx::core::NumericType::float32));
      args.insertOrAssign(ReadTextDataArrayFilter::k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{static_cast<double>(3052)}}));
      args.insertOrAssign(ReadTextDataArrayFilter::k_NComp_Key, std::make_any<uint64>(3));
      args.insertOrAssign(ReadTextDataArrayFilter::k_NSkipLines_Key, std::make_any<uint64>(1));
      args.insertOrAssign(ReadTextDataArrayFilter::k_DelimiterChoice_Key, std::make_any<ChoicesParameter::ValueType>(2));
      args.insertOrAssign(ReadTextDataArrayFilter::k_DataArrayPath_Key, std::make_any<DataPath>(k_ExemplarErrorPath));

      auto executeResult = filter->execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    }
    // computed distribution
    {
      Arguments args;
      args.insertOrAssign(ReadTextDataArrayFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedDistOutput));
      args.insertOrAssign(ReadTextDataArrayFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(nx::core::NumericType::float32));
      args.insertOrAssign(ReadTextDataArrayFilter::k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{static_cast<double>(3052)}}));
      args.insertOrAssign(ReadTextDataArrayFilter::k_NComp_Key, std::make_any<uint64>(3));
      args.insertOrAssign(ReadTextDataArrayFilter::k_NSkipLines_Key, std::make_any<uint64>(1));
      args.insertOrAssign(ReadTextDataArrayFilter::k_DelimiterChoice_Key, std::make_any<ChoicesParameter::ValueType>(2));
      args.insertOrAssign(ReadTextDataArrayFilter::k_DataArrayPath_Key, std::make_any<DataPath>(k_ComputedDistributionPath));

      auto executeResult = filter->execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    }
    // computed errors
    {
      Arguments args;
      args.insertOrAssign(ReadTextDataArrayFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedErrorsOutput));
      args.insertOrAssign(ReadTextDataArrayFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(nx::core::NumericType::float32));
      args.insertOrAssign(ReadTextDataArrayFilter::k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{static_cast<double>(3052)}}));
      args.insertOrAssign(ReadTextDataArrayFilter::k_NComp_Key, std::make_any<uint64>(3));
      args.insertOrAssign(ReadTextDataArrayFilter::k_NSkipLines_Key, std::make_any<uint64>(1));
      args.insertOrAssign(ReadTextDataArrayFilter::k_DelimiterChoice_Key, std::make_any<ChoicesParameter::ValueType>(2));
      args.insertOrAssign(ReadTextDataArrayFilter::k_DataArrayPath_Key, std::make_any<DataPath>(k_ComputedErrorPath));

      auto executeResult = filter->execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    }
  }

  // compare results
  UnitTest::CompareArrays<float32>(dataStructure, k_ExemplarDistributionPath, k_ComputedDistributionPath);
  UnitTest::CompareArrays<float32>(dataStructure, k_ExemplarErrorPath, k_ComputedErrorPath);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeGBCDMetricBasedFilter: InValid Filter Execution")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "compute_gbcd_metric_based.tar.gz", "compute_gbcd_metric_based");

  // Read Exemplar DREAM3D File Input
  auto exemplarInputFilePath = fs::path(fmt::format("{}/compute_gbcd_metric_based/compute_gbcd_metric_based.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarInputFilePath);

  const fs::path computedDistOutput(fmt::format("{}/computed_gbcd_distribution_1.dat", unit_test::k_BinaryTestOutputDir));
  const fs::path computedErrorsOutput(fmt::format("{}/computed_gbcd_distribution_errors_1.dat", unit_test::k_BinaryTestOutputDir));

  // Instantiate the filter, a DataStructure object and an Arguments Object
  const ComputeGBCDMetricBasedFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_ChosenLimitDists_Key, std::make_any<ChoicesParameter::ValueType>(2));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_ExcludeTripleLines_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_DistOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedDistOutput));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_SaveRelativeErr_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleDataContainerPath));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_NodeTypesArrayPath_Key, std::make_any<DataPath>(k_NodeTypesPath));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FaceLabelsPath));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(k_FaceNormalsPath));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_SurfaceMeshFaceAreasArrayPath_Key, std::make_any<DataPath>(k_FaceAreasPath));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_SurfaceMeshFeatureFaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FeatureFaceLabelsPath));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(k_AvgEulerAnglesPath));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresPath));

  SECTION("Invalid Misorientation Rotation Angle")
  {
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_PhaseOfInterest_Key, std::make_any<int32>(1));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_NumSamplPts_Key, std::make_any<int32>(3000));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1, 1, 1, -90.0f}));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_ErrOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedErrorsOutput));
  }
  SECTION("Invalid Misorientation Rotation Axis")
  {
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_PhaseOfInterest_Key, std::make_any<int32>(1));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_NumSamplPts_Key, std::make_any<int32>(3000));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{0.0f, 0.0f, 0.0f, 17.9f}));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_ErrOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedErrorsOutput));
  }
  SECTION("Invalid Number of Sample Points")
  {
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_PhaseOfInterest_Key, std::make_any<int32>(1));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_NumSamplPts_Key, std::make_any<int32>(0));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1, 1, 1, 17.9f}));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_ErrOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedErrorsOutput));
  }
  SECTION("Invalid Phase Of Interest Value (must be > 0)")
  {
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_PhaseOfInterest_Key, std::make_any<int32>(0));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_NumSamplPts_Key, std::make_any<int32>(3000));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1, 1, 1, 17.9f}));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_ErrOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedErrorsOutput));
  }
  SECTION("Invalid Phase Of Interest Value (cannot be > number of ensembles)")
  {
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_PhaseOfInterest_Key, std::make_any<int32>(2));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_NumSamplPts_Key, std::make_any<int32>(3000));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1, 1, 1, 17.9f}));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_ErrOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedErrorsOutput));
  }
  SECTION("Invalid Output File Names")
  {
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_PhaseOfInterest_Key, std::make_any<int32>(1));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_NumSamplPts_Key, std::make_any<int32>(3000));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1, 1, 1, 17.9f}));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_ErrOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedDistOutput));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeGBCDMetricBasedFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeGBCDMetricBasedFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeGBCDMetricBasedFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeGBCDMetricBasedFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeGBCDMetricBasedFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<int32>(ComputeGBCDMetricBasedFilter::k_PhaseOfInterest_Key) == 5);
      // Complex type (AxisAngleFilterParameterConverter<float32>) - verified by successful pipeline loading
      CHECK(args.value<ChoicesParameter::ValueType>(ComputeGBCDMetricBasedFilter::k_ChosenLimitDists_Key) == 0);
      CHECK(args.value<int32>(ComputeGBCDMetricBasedFilter::k_NumSamplPts_Key) == 5);
      CHECK(args.value<bool>(ComputeGBCDMetricBasedFilter::k_ExcludeTripleLines_Key) == true);
      CHECK(args.value<FileSystemPathParameter::ValueType>(ComputeGBCDMetricBasedFilter::k_DistOutputFile_Key) == fs::path("/test/path/file.txt"));
      CHECK(args.value<FileSystemPathParameter::ValueType>(ComputeGBCDMetricBasedFilter::k_ErrOutputFile_Key) == fs::path("/test/path/file.txt"));
      CHECK(args.value<bool>(ComputeGBCDMetricBasedFilter::k_SaveRelativeErr_Key) == true);
      CHECK(args.value<DataPath>(ComputeGBCDMetricBasedFilter::k_TriangleGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ComputeGBCDMetricBasedFilter::k_NodeTypesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeGBCDMetricBasedFilter::k_SurfaceMeshFaceLabelsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeGBCDMetricBasedFilter::k_SurfaceMeshFaceNormalsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeGBCDMetricBasedFilter::k_SurfaceMeshFaceAreasArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeGBCDMetricBasedFilter::k_SurfaceMeshFeatureFaceLabelsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeGBCDMetricBasedFilter::k_FeatureEulerAnglesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeGBCDMetricBasedFilter::k_FeaturePhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeGBCDMetricBasedFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}

TEST_CASE("OrientationAnalysis::ComputeGBCDMetricBasedFilter: Laue Index Bounds", "[OrientationAnalysis][ComputeGBCDMetricBasedFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);
  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "compute_gbcd_metric_based.tar.gz", "compute_gbcd_metric_based");

  const fs::path inputFile = fs::path(unit_test::k_TestFilesDir.view()) / "compute_gbcd_metric_based" / "compute_gbcd_metric_based.dream3d";
  DataStructure dataStructure = UnitTest::LoadDataStructure(inputFile);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>(k_CrystalStructuresPath));
  auto& crystalStructuresArrayRef = dataStructure.getDataRefAs<UInt32Array>(k_CrystalStructuresPath);
  crystalStructuresArrayRef.getDataStoreRef()[1] = 999U;

  const fs::path distributionOutput = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "gbcd_metric_bounds_distribution.dat";
  const fs::path errorsOutput = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "gbcd_metric_bounds_errors.dat";

  ComputeGBCDMetricBasedFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_PhaseOfInterest_Key, std::make_any<int32>(1));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1.0F, 1.0F, 1.0F, 60.0F}));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_NumSamplPts_Key, std::make_any<int32>(1));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_DistOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(distributionOutput));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_ErrOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(errorsOutput));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleDataContainerPath));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_NodeTypesArrayPath_Key, std::make_any<DataPath>(k_NodeTypesPath));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FaceLabelsPath));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(k_FaceNormalsPath));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_SurfaceMeshFaceAreasArrayPath_Key, std::make_any<DataPath>(k_FaceAreasPath));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_SurfaceMeshFeatureFaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FeatureFaceLabelsPath));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(k_AvgEulerAnglesPath));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresPath));

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -7240);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeGBCDMetricBasedFilter: the file-replace hook runs on both output files before either opens",
          "[OrientationAnalysis][ComputeGBCDMetricBasedFilter][FileWriteHook]")
{
  UnitTest::LoadPlugins();
  constexpr int32 k_RecorderWarningCode = 99221;
  constexpr int32 k_RecorderErrorCode = -99222;
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "compute_gbcd_metric_based.tar.gz", "compute_gbcd_metric_based");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/compute_gbcd_metric_based/compute_gbcd_metric_based.dream3d", unit_test::k_TestFilesDir)));

  const fs::path outputDirectory = fs::temp_directory_path() / "simplnx_compute_gbcd_metric_based_hook";
  fs::remove_all(outputDirectory);
  fs::create_directories(outputDirectory);
  const fs::path distOutput = outputDirectory / "hook_distribution_1.dat";
  const fs::path errorOutput = outputDirectory / "hook_distribution_errors_1.dat";

  const ComputeGBCDMetricBasedFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_PhaseOfInterest_Key, std::make_any<int32>(1));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1.0f, 1.0f, 1.0f, 60.0f}));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_ChosenLimitDists_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_NumSamplPts_Key, std::make_any<int32>(3000));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_ExcludeTripleLines_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_DistOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(distOutput));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_ErrOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(errorOutput));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_SaveRelativeErr_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleDataContainerPath));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_NodeTypesArrayPath_Key, std::make_any<DataPath>(k_NodeTypesPath));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FaceLabelsPath));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(k_FaceNormalsPath));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_SurfaceMeshFaceAreasArrayPath_Key, std::make_any<DataPath>(k_FaceAreasPath));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_SurfaceMeshFeatureFaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FeatureFaceLabelsPath));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(k_AvgEulerAnglesPath));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));
  args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresPath));

  static const std::shared_ptr<UnitTest::RecordingFileReplaceHandler> s_ErrorFileRecorder = std::make_shared<UnitTest::RecordingFileReplaceHandler>();
  static const std::shared_ptr<void> s_ErrorFileRegistration = UnitTest::RegisterRecorder(s_ErrorFileRecorder);
  UnitTest::RecordingFileReplaceHandler& distRecorder = UnitTest::ProcessRecorder();
  UnitTest::RecordingFileReplaceHandler& errorRecorder = *s_ErrorFileRecorder;
  const UnitTest::RecorderDisarmGuard disarmGuard({&distRecorder, &errorRecorder});
  std::vector<std::string> calls;
  std::vector<bool> areFilesAbsentAtCall;
  const auto observe = [&](const std::string& name) {
    return [&, name](const fs::path&, FileWriteHookMode) {
      calls.push_back(name);
      areFilesAbsentAtCall.push_back(!fs::exists(distOutput) && !fs::exists(errorOutput));
    };
  };

  SECTION("a refused error file prevents the distribution file")
  {
    distRecorder.arm(distOutput, observe("distribution"), MakeWarningVoidResult(k_RecorderWarningCode, "recorder warning"), false);
    errorRecorder.arm(errorOutput, observe("errors"), MakeErrorResult(k_RecorderErrorCode, "recorder error"), false);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors().size() == 1);
    CHECK(executeResult.result.errors().front().code == k_RecorderErrorCode);
    const bool hasRecorderWarning =
        std::any_of(executeResult.result.warnings().begin(), executeResult.result.warnings().end(), [](const Warning& warning) { return warning.code == k_RecorderWarningCode; });
    CHECK(hasRecorderWarning);
    CHECK(calls == std::vector<std::string>{"distribution", "errors"});
    CHECK(areFilesAbsentAtCall == std::vector<bool>{true, true});
    CHECK_FALSE(fs::exists(distOutput));
    CHECK_FALSE(fs::exists(errorOutput));
  }
  SECTION("outputs that resolve to the same file are rejected before any hook")
  {
    // The distribution file gets the _1 suffix, so it resolves to the error file name.
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_DistOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(outputDirectory / "hook_distribution_errors.dat"));
    distRecorder.arm(errorOutput, observe("distribution"), {}, false);
    errorRecorder.arm(errorOutput, observe("errors"), {}, false);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors().size() == 1);
    CHECK(executeResult.result.errors().front().code == -7239);
    CHECK(calls.empty());
    CHECK_FALSE(fs::exists(errorOutput));
  }
  SECTION("outputs reached through symbolic links are compared by the files they name")
  {
    // Creating a symbolic link can need a privilege on Windows; the section is skipped when the link cannot be created.
    fs::create_directories(outputDirectory / "real" / "sub");
    std::error_code linkError;
    fs::create_directory_symlink(outputDirectory / "real" / "sub", outputDirectory / "link", linkError);
    if(!linkError)
    {
      fs::create_directory_symlink(outputDirectory / "real", outputDirectory / "alias", linkError);
    }
    if(linkError)
    {
      WARN("symbolic-link capability unavailable: " << linkError.message());
      return;
    }
    // Two absent files reached through a linked folder and its real folder are one file.
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_DistOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(outputDirectory / "real" / "same_1.dat"));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_ErrOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(outputDirectory / "alias" / "same_1.dat"));
    auto sameResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(sameResult.result);
    REQUIRE(sameResult.result.errors().size() == 1);
    CHECK(sameResult.result.errors().front().code == -7239);

    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_DistOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(outputDirectory / "link" / ".." / "other_1.dat"));
    args.insertOrAssign(ComputeGBCDMetricBasedFilter::k_ErrOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(outputDirectory / "other_1.dat"));
    auto linkParentResult = filter.execute(dataStructure, args);
#ifdef _WIN32
    // Windows path handling removes ".." before it resolves "link", so "link/../other_1.dat" names the same file.
    SIMPLNX_RESULT_REQUIRE_INVALID(linkParentResult.result);
    REQUIRE(linkParentResult.result.errors().size() == 1);
    CHECK(linkParentResult.result.errors().front().code == -7239);
#else
    // POSIX resolves "link" first, so "link/../other_1.dat" names real/other_1.dat, a different file.
    SIMPLNX_RESULT_REQUIRE_VALID(linkParentResult.result);
    CHECK(fs::is_regular_file(outputDirectory / "real" / "other_1.dat"));
    CHECK(fs::is_regular_file(outputDirectory / "other_1.dat"));
#endif
  }
  SECTION("success calls each hook once before the files exist")
  {
    distRecorder.arm(distOutput, observe("distribution"), {}, false);
    errorRecorder.arm(errorOutput, observe("errors"), {}, false);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    CHECK(calls == std::vector<std::string>{"distribution", "errors"});
    CHECK(areFilesAbsentAtCall == std::vector<bool>{true, true});
    CHECK(fs::exists(distOutput));
    CHECK(fs::exists(errorOutput));
  }
  std::error_code ignored;
  fs::remove_all(outputDirectory, ignored);
}
