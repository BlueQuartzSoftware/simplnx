#include <catch2/catch.hpp>

#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/FindGBPDMetricBasedFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;
using namespace complex::Constants;

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

TEST_CASE("OrientationAnalysis::FindGBPDMetricBasedFilter: Valid Filter Execution", "[OrientationAnalysis][FindGBPDMetricBasedFilter]")
{
  const std::string kDataInputArchive = "6_6_find_gbcd_metric_based.tar.gz";
  const std::string kExpectedOutputTopLevel = "6_6_find_gbcd_metric_based";
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, kDataInputArchive, kExpectedOutputTopLevel,
                                                             complex::unit_test::k_BinaryTestOutputDir);

  std::shared_ptr<UnitTest::make_shared_enabler> app = std::make_shared<UnitTest::make_shared_enabler>();
  app->loadPlugins(unit_test::k_BuildDir.view(), true);
  auto* filterList = Application::Instance()->getFilterList();

  // Read Exemplar DREAM3D File Input
  auto exemplarInputFilePath = fs::path(fmt::format("{}/6_6_find_gbcd_metric_based/6_6_find_gbcd_metric_based.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarInputFilePath);

  fs::path exemplarDistOutput(fmt::format("{}/6_6_find_gbcd_metric_based/6_6_gbpd_distribution_1.dat", unit_test::k_TestFilesDir));
  fs::path exemplarErrorsOutput(fmt::format("{}/6_6_find_gbcd_metric_based/6_6_gbpd_distribution_errors_1.dat", unit_test::k_TestFilesDir));
  fs::path computedDistOutput(fmt::format("{}/computed_gbpd_distribution_1.dat", unit_test::k_BinaryTestOutputDir));
  fs::path computedErrorsOutput(fmt::format("{}/computed_gbpd_distribution_errors_1.dat", unit_test::k_BinaryTestOutputDir));

  // Run the FindGBCDMetricBased filter
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    FindGBPDMetricBasedFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(FindGBPDMetricBasedFilter::k_PhaseOfInterest_Key, std::make_any<int32>(1));
    args.insertOrAssign(FindGBPDMetricBasedFilter::k_LimitDist_Key, std::make_any<float32>(7.0f));
    args.insertOrAssign(FindGBPDMetricBasedFilter::k_NumSamplPts_Key, std::make_any<int32>(3000));
    args.insertOrAssign(FindGBPDMetricBasedFilter::k_ExcludeTripleLines_Key, std::make_any<bool>(false));
    args.insertOrAssign(FindGBPDMetricBasedFilter::k_DistOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedDistOutput));
    args.insertOrAssign(FindGBPDMetricBasedFilter::k_ErrOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedErrorsOutput));
    args.insertOrAssign(FindGBPDMetricBasedFilter::k_SaveRelativeErr_Key, std::make_any<bool>(false));
    args.insertOrAssign(FindGBPDMetricBasedFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleDataContainerPath));
    args.insertOrAssign(FindGBPDMetricBasedFilter::k_NodeTypesArrayPath_Key, std::make_any<DataPath>(k_NodeTypesPath));
    args.insertOrAssign(FindGBPDMetricBasedFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FaceLabelsPath));
    args.insertOrAssign(FindGBPDMetricBasedFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(k_FaceNormalsPath));
    args.insertOrAssign(FindGBPDMetricBasedFilter::k_SurfaceMeshFaceAreasArrayPath_Key, std::make_any<DataPath>(k_FaceAreasPath));
    args.insertOrAssign(FindGBPDMetricBasedFilter::k_SurfaceMeshFeatureFaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FeatureFaceLabelsPath));
    args.insertOrAssign(FindGBPDMetricBasedFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(k_AvgEulerAnglesPath));
    args.insertOrAssign(FindGBPDMetricBasedFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));
    args.insertOrAssign(FindGBPDMetricBasedFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // read in exemplar and computed data files for comparison
  {
    static constexpr StringLiteral k_InputFileKey = "input_file";
    static constexpr StringLiteral k_ScalarTypeKey = "scalar_type";
    static constexpr StringLiteral k_NTuplesKey = "n_tuples";
    static constexpr StringLiteral k_NCompKey = "n_comp";
    static constexpr StringLiteral k_NSkipLinesKey = "n_skip_lines";
    static constexpr StringLiteral k_DelimiterChoiceKey = "delimiter_choice";
    static constexpr StringLiteral k_DataArrayKey = "output_data_array";
    auto filter = filterList->createFilter(k_ImportTextFilterHandle);
    REQUIRE(nullptr != filter);
    // exemplar distribution
    {
      Arguments args;
      args.insertOrAssign(k_InputFileKey, std::make_any<FileSystemPathParameter::ValueType>(exemplarDistOutput));
      args.insertOrAssign(k_ScalarTypeKey, std::make_any<NumericTypeParameter::ValueType>(complex::NumericType::float32));
      args.insertOrAssign(k_NTuplesKey, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{static_cast<double>(3624)}}));
      args.insertOrAssign(k_NCompKey, std::make_any<uint64>(3));
      args.insertOrAssign(k_NSkipLinesKey, std::make_any<uint64>(1));
      args.insertOrAssign(k_DelimiterChoiceKey, std::make_any<ChoicesParameter::ValueType>(2));
      args.insertOrAssign(k_DataArrayKey, std::make_any<DataPath>(k_ExemplarDistributionPath));

      auto executeResult = filter->execute(dataStructure, args);
      COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
    }
    // exemplar errors
    {
      Arguments args;
      args.insertOrAssign(k_InputFileKey, std::make_any<FileSystemPathParameter::ValueType>(exemplarErrorsOutput));
      args.insertOrAssign(k_ScalarTypeKey, std::make_any<NumericTypeParameter::ValueType>(complex::NumericType::float32));
      args.insertOrAssign(k_NTuplesKey, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{static_cast<double>(3624)}}));
      args.insertOrAssign(k_NCompKey, std::make_any<uint64>(3));
      args.insertOrAssign(k_NSkipLinesKey, std::make_any<uint64>(1));
      args.insertOrAssign(k_DelimiterChoiceKey, std::make_any<ChoicesParameter::ValueType>(2));
      args.insertOrAssign(k_DataArrayKey, std::make_any<DataPath>(k_ExemplarErrorPath));

      auto executeResult = filter->execute(dataStructure, args);
      COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
    }
    // computed distribution
    {
      Arguments args;
      args.insertOrAssign(k_InputFileKey, std::make_any<FileSystemPathParameter::ValueType>(computedDistOutput));
      args.insertOrAssign(k_ScalarTypeKey, std::make_any<NumericTypeParameter::ValueType>(complex::NumericType::float32));
      args.insertOrAssign(k_NTuplesKey, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{static_cast<double>(3624)}}));
      args.insertOrAssign(k_NCompKey, std::make_any<uint64>(3));
      args.insertOrAssign(k_NSkipLinesKey, std::make_any<uint64>(1));
      args.insertOrAssign(k_DelimiterChoiceKey, std::make_any<ChoicesParameter::ValueType>(2));
      args.insertOrAssign(k_DataArrayKey, std::make_any<DataPath>(k_ComputedDistributionPath));

      auto executeResult = filter->execute(dataStructure, args);
      COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
    }
    // computed errors
    {
      Arguments args;
      args.insertOrAssign(k_InputFileKey, std::make_any<FileSystemPathParameter::ValueType>(computedErrorsOutput));
      args.insertOrAssign(k_ScalarTypeKey, std::make_any<NumericTypeParameter::ValueType>(complex::NumericType::float32));
      args.insertOrAssign(k_NTuplesKey, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{static_cast<double>(3624)}}));
      args.insertOrAssign(k_NCompKey, std::make_any<uint64>(3));
      args.insertOrAssign(k_NSkipLinesKey, std::make_any<uint64>(1));
      args.insertOrAssign(k_DelimiterChoiceKey, std::make_any<ChoicesParameter::ValueType>(2));
      args.insertOrAssign(k_DataArrayKey, std::make_any<DataPath>(k_ComputedErrorPath));

      auto executeResult = filter->execute(dataStructure, args);
      COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)
    }
  }

  // compare results
  UnitTest::CompareArrays<float32>(dataStructure, k_ExemplarDistributionPath, k_ComputedDistributionPath);
  UnitTest::CompareArrays<float32>(dataStructure, k_ExemplarErrorPath, k_ComputedErrorPath);
}

TEST_CASE("OrientationAnalysis::FindGBPDMetricBasedFilter: InValid Filter Execution")
{
  // Read Exemplar DREAM3D File Input
  auto exemplarInputFilePath = fs::path(fmt::format("{}/6_6_find_gbcd_metric_based/6_6_find_gbcd_metric_based.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarInputFilePath);

  fs::path computedDistOutput(fmt::format("{}/computed_gbcd_distribution_1.dat", unit_test::k_BinaryTestOutputDir));
  fs::path computedErrorsOutput(fmt::format("{}/computed_gbcd_distribution_errors_1.dat", unit_test::k_BinaryTestOutputDir));

  // Instantiate the filter, a DataStructure object and an Arguments Object
  FindGBPDMetricBasedFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(FindGBPDMetricBasedFilter::k_LimitDist_Key, std::make_any<float32>(7.0f));
  args.insertOrAssign(FindGBPDMetricBasedFilter::k_ExcludeTripleLines_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindGBPDMetricBasedFilter::k_DistOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedDistOutput));
  args.insertOrAssign(FindGBPDMetricBasedFilter::k_SaveRelativeErr_Key, std::make_any<bool>(true));
  args.insertOrAssign(FindGBPDMetricBasedFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleDataContainerPath));
  args.insertOrAssign(FindGBPDMetricBasedFilter::k_NodeTypesArrayPath_Key, std::make_any<DataPath>(k_NodeTypesPath));
  args.insertOrAssign(FindGBPDMetricBasedFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FaceLabelsPath));
  args.insertOrAssign(FindGBPDMetricBasedFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(k_FaceNormalsPath));
  args.insertOrAssign(FindGBPDMetricBasedFilter::k_SurfaceMeshFaceAreasArrayPath_Key, std::make_any<DataPath>(k_FaceAreasPath));
  args.insertOrAssign(FindGBPDMetricBasedFilter::k_SurfaceMeshFeatureFaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FeatureFaceLabelsPath));
  args.insertOrAssign(FindGBPDMetricBasedFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(k_AvgEulerAnglesPath));
  args.insertOrAssign(FindGBPDMetricBasedFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));
  args.insertOrAssign(FindGBPDMetricBasedFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresPath));

  SECTION("Invalid Number of Sample Points")
  {
    args.insertOrAssign(FindGBPDMetricBasedFilter::k_PhaseOfInterest_Key, std::make_any<int32>(1));
    args.insertOrAssign(FindGBPDMetricBasedFilter::k_NumSamplPts_Key, std::make_any<int32>(0));
    args.insertOrAssign(FindGBPDMetricBasedFilter::k_ErrOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedErrorsOutput));
  }
  SECTION("Invalid Phase Of Interest Value (must be > 0)")
  {
    args.insertOrAssign(FindGBPDMetricBasedFilter::k_PhaseOfInterest_Key, std::make_any<int32>(0));
    args.insertOrAssign(FindGBPDMetricBasedFilter::k_NumSamplPts_Key, std::make_any<int32>(3000));
    args.insertOrAssign(FindGBPDMetricBasedFilter::k_ErrOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedErrorsOutput));
  }
  SECTION("Invalid Phase Of Interest Value (cannot be > number of ensembles)")
  {
    args.insertOrAssign(FindGBPDMetricBasedFilter::k_PhaseOfInterest_Key, std::make_any<int32>(2));
    args.insertOrAssign(FindGBPDMetricBasedFilter::k_NumSamplPts_Key, std::make_any<int32>(3000));
    args.insertOrAssign(FindGBPDMetricBasedFilter::k_ErrOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedErrorsOutput));
  }
  SECTION("Invalid Output File Names")
  {
    args.insertOrAssign(FindGBPDMetricBasedFilter::k_PhaseOfInterest_Key, std::make_any<int32>(1));
    args.insertOrAssign(FindGBPDMetricBasedFilter::k_NumSamplPts_Key, std::make_any<int32>(3000));
    args.insertOrAssign(FindGBPDMetricBasedFilter::k_ErrOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedDistOutput));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(executeResult.result)
}
