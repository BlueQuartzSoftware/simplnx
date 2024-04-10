#include <catch2/catch.hpp>

#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumericTypeParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/FindGBCDMetricBasedFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include <filesystem>
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

TEST_CASE("OrientationAnalysis::FindGBCDMetricBasedFilter: Valid Filter Execution", "[OrientationAnalysis][FindGBCDMetricBasedFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_find_gbcd_metric_based.tar.gz",
                                                              "6_6_find_gbcd_metric_based");

  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);
  const auto* filterListPtr = Application::Instance()->getFilterList();

  // Read Exemplar DREAM3D File Input
  auto exemplarInputFilePath = fs::path(fmt::format("{}/6_6_find_gbcd_metric_based/6_6_find_gbcd_metric_based.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarInputFilePath);

  const fs::path exemplarDistOutput(fmt::format("{}/6_6_find_gbcd_metric_based/6_6_gbcd_distribution_1.dat", unit_test::k_TestFilesDir));
  const fs::path exemplarErrorsOutput(fmt::format("{}/6_6_find_gbcd_metric_based/6_6_gbcd_distribution_errors_1.dat", unit_test::k_TestFilesDir));
  const fs::path computedDistOutput(fmt::format("{}/6_6_find_gbcd_metric_based/7_0_computed_gbcd_distribution_1.dat", unit_test::k_TestFilesDir));
  const fs::path computedErrorsOutput(fmt::format("{}/6_6_find_gbcd_metric_based/7_0_computed_gbcd_distribution_errors_1.dat", unit_test::k_TestFilesDir));

  // Run the FindGBCDMetricBased filter
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    const FindGBCDMetricBasedFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_PhaseOfInterest_Key, std::make_any<int32>(1));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1.0f, 1.0f, 1.0f, 60.0f}));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_ChosenLimitDists_Key, std::make_any<ChoicesParameter::ValueType>(0));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_NumSamplPts_Key, std::make_any<int32>(3000));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_ExcludeTripleLines_Key, std::make_any<bool>(false));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_DistOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedDistOutput));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_ErrOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedErrorsOutput));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_SaveRelativeErr_Key, std::make_any<bool>(false));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleDataContainerPath));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_NodeTypesArrayPath_Key, std::make_any<DataPath>(k_NodeTypesPath));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FaceLabelsPath));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(k_FaceNormalsPath));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_SurfaceMeshFaceAreasArrayPath_Key, std::make_any<DataPath>(k_FaceAreasPath));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_SurfaceMeshFeatureFaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FeatureFaceLabelsPath));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(k_AvgEulerAnglesPath));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // read in exemplar and computed data files for comparison
  {
    constexpr StringLiteral k_InputFile_Key = "input_file";
    constexpr StringLiteral k_ScalarType_Key = "scalar_type";
    constexpr StringLiteral k_NTuples_Key = "number_tuples";
    constexpr StringLiteral k_NComp_Key = "number_comp";
    constexpr StringLiteral k_NSkipLines_Key = "skip_line_count";
    constexpr StringLiteral k_DelimiterChoice_Key = "delimiter_choice";
    constexpr StringLiteral k_DataArrayPath_Key = "output_data_array_path";
    constexpr StringLiteral k_DataFormat_Key = "data_format";
    constexpr StringLiteral k_AdvancedOptions_Key = "set_tuple_dimensions";

    auto filter = filterListPtr->createFilter(k_ReadTextDataArrayFilterHandle);
    REQUIRE(nullptr != filter);
    // exemplar distribution
    {
      Arguments args;
      args.insertOrAssign(k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(exemplarDistOutput));
      args.insertOrAssign(k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(nx::core::NumericType::float32));
      args.insertOrAssign(k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{static_cast<double>(3052)}}));
      args.insertOrAssign(k_NComp_Key, std::make_any<uint64>(3));
      args.insertOrAssign(k_NSkipLines_Key, std::make_any<uint64>(1));
      args.insertOrAssign(k_DelimiterChoice_Key, std::make_any<ChoicesParameter::ValueType>(2));
      args.insertOrAssign(k_DataArrayPath_Key, std::make_any<DataPath>(k_ExemplarDistributionPath));

      auto executeResult = filter->execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    }
    // exemplar errors
    {
      Arguments args;
      args.insertOrAssign(k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(exemplarErrorsOutput));
      args.insertOrAssign(k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(nx::core::NumericType::float32));
      args.insertOrAssign(k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{static_cast<double>(3052)}}));
      args.insertOrAssign(k_NComp_Key, std::make_any<uint64>(3));
      args.insertOrAssign(k_NSkipLines_Key, std::make_any<uint64>(1));
      args.insertOrAssign(k_DelimiterChoice_Key, std::make_any<ChoicesParameter::ValueType>(2));
      args.insertOrAssign(k_DataArrayPath_Key, std::make_any<DataPath>(k_ExemplarErrorPath));

      auto executeResult = filter->execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    }
    // computed distribution
    {
      Arguments args;
      args.insertOrAssign(k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedDistOutput));
      args.insertOrAssign(k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(nx::core::NumericType::float32));
      args.insertOrAssign(k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{static_cast<double>(3052)}}));
      args.insertOrAssign(k_NComp_Key, std::make_any<uint64>(3));
      args.insertOrAssign(k_NSkipLines_Key, std::make_any<uint64>(1));
      args.insertOrAssign(k_DelimiterChoice_Key, std::make_any<ChoicesParameter::ValueType>(2));
      args.insertOrAssign(k_DataArrayPath_Key, std::make_any<DataPath>(k_ComputedDistributionPath));

      auto executeResult = filter->execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    }
    // computed errors
    {
      Arguments args;
      args.insertOrAssign(k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedErrorsOutput));
      args.insertOrAssign(k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(nx::core::NumericType::float32));
      args.insertOrAssign(k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{static_cast<double>(3052)}}));
      args.insertOrAssign(k_NComp_Key, std::make_any<uint64>(3));
      args.insertOrAssign(k_NSkipLines_Key, std::make_any<uint64>(1));
      args.insertOrAssign(k_DelimiterChoice_Key, std::make_any<ChoicesParameter::ValueType>(2));
      args.insertOrAssign(k_DataArrayPath_Key, std::make_any<DataPath>(k_ComputedErrorPath));

      auto executeResult = filter->execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    }
  }

  // compare results
  UnitTest::CompareArrays<float32>(dataStructure, k_ExemplarDistributionPath, k_ComputedDistributionPath);
  UnitTest::CompareArrays<float32>(dataStructure, k_ExemplarErrorPath, k_ComputedErrorPath);
}

TEST_CASE("OrientationAnalysis::FindGBCDMetricBasedFilter: InValid Filter Execution")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_find_gbcd_metric_based.tar.gz",
                                                              "6_6_find_gbcd_metric_based");

  // Read Exemplar DREAM3D File Input
  auto exemplarInputFilePath = fs::path(fmt::format("{}/6_6_find_gbcd_metric_based/6_6_find_gbcd_metric_based.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarInputFilePath);

  const fs::path computedDistOutput(fmt::format("{}/computed_gbcd_distribution_1.dat", unit_test::k_BinaryTestOutputDir));
  const fs::path computedErrorsOutput(fmt::format("{}/computed_gbcd_distribution_errors_1.dat", unit_test::k_BinaryTestOutputDir));

  // Instantiate the filter, a DataStructure object and an Arguments Object
  const FindGBCDMetricBasedFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(FindGBCDMetricBasedFilter::k_ChosenLimitDists_Key, std::make_any<ChoicesParameter::ValueType>(2));
  args.insertOrAssign(FindGBCDMetricBasedFilter::k_ExcludeTripleLines_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindGBCDMetricBasedFilter::k_DistOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedDistOutput));
  args.insertOrAssign(FindGBCDMetricBasedFilter::k_SaveRelativeErr_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindGBCDMetricBasedFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleDataContainerPath));
  args.insertOrAssign(FindGBCDMetricBasedFilter::k_NodeTypesArrayPath_Key, std::make_any<DataPath>(k_NodeTypesPath));
  args.insertOrAssign(FindGBCDMetricBasedFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FaceLabelsPath));
  args.insertOrAssign(FindGBCDMetricBasedFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(k_FaceNormalsPath));
  args.insertOrAssign(FindGBCDMetricBasedFilter::k_SurfaceMeshFaceAreasArrayPath_Key, std::make_any<DataPath>(k_FaceAreasPath));
  args.insertOrAssign(FindGBCDMetricBasedFilter::k_SurfaceMeshFeatureFaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FeatureFaceLabelsPath));
  args.insertOrAssign(FindGBCDMetricBasedFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(k_AvgEulerAnglesPath));
  args.insertOrAssign(FindGBCDMetricBasedFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));
  args.insertOrAssign(FindGBCDMetricBasedFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresPath));

  SECTION("Invalid Misorientation Rotation Angle")
  {
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_PhaseOfInterest_Key, std::make_any<int32>(1));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_NumSamplPts_Key, std::make_any<int32>(3000));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1, 1, 1, -90.0f}));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_ErrOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedErrorsOutput));
  }
  SECTION("Invalid Misorientation Rotation Axis")
  {
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_PhaseOfInterest_Key, std::make_any<int32>(1));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_NumSamplPts_Key, std::make_any<int32>(3000));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{0.0f, 0.0f, 0.0f, 17.9f}));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_ErrOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedErrorsOutput));
  }
  SECTION("Invalid Number of Sample Points")
  {
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_PhaseOfInterest_Key, std::make_any<int32>(1));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_NumSamplPts_Key, std::make_any<int32>(0));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1, 1, 1, 17.9f}));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_ErrOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedErrorsOutput));
  }
  SECTION("Invalid Phase Of Interest Value (must be > 0)")
  {
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_PhaseOfInterest_Key, std::make_any<int32>(0));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_NumSamplPts_Key, std::make_any<int32>(3000));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1, 1, 1, 17.9f}));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_ErrOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedErrorsOutput));
  }
  SECTION("Invalid Phase Of Interest Value (cannot be > number of ensembles)")
  {
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_PhaseOfInterest_Key, std::make_any<int32>(2));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_NumSamplPts_Key, std::make_any<int32>(3000));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1, 1, 1, 17.9f}));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_ErrOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedErrorsOutput));
  }
  SECTION("Invalid Output File Names")
  {
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_PhaseOfInterest_Key, std::make_any<int32>(1));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_NumSamplPts_Key, std::make_any<int32>(3000));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1, 1, 1, 17.9f}));
    args.insertOrAssign(FindGBCDMetricBasedFilter::k_ErrOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedDistOutput));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
}
