#include <catch2/catch.hpp>

#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumericTypeParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/ComputeGBCDMetricBasedFilter.hpp"
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

TEST_CASE("OrientationAnalysis::ComputeGBCDMetricBasedFilter: Valid Filter Execution", "[OrientationAnalysis][ComputeGBCDMetricBasedFilter]")
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
}

TEST_CASE("OrientationAnalysis::ComputeGBCDMetricBasedFilter: InValid Filter Execution")
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
}
