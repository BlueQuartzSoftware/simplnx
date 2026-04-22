#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumericTypeParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "OrientationAnalysis/Filters/ComputeGBPDMetricBasedFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysisTestUtils.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

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

/**
 * @brief Compares two float32 DataArrays after sorting their tuples, so that row ordering does not affect the comparison.
 * This is needed because the GBPD output row order depends on EbsdLib symmetry operator ordering, which may change between versions.
 */
void CompareSortedArrays(const DataStructure& dataStructure, const DataPath& exemplarPath, const DataPath& computedPath)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<float32>>(exemplarPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<float32>>(computedPath));
  const auto& exemplarArray = dataStructure.getDataRefAs<DataArray<float32>>(exemplarPath);
  const auto& computedArray = dataStructure.getDataRefAs<DataArray<float32>>(computedPath);
  REQUIRE(exemplarArray.getNumberOfTuples() == computedArray.getNumberOfTuples());
  REQUIRE(exemplarArray.getNumberOfComponents() == computedArray.getNumberOfComponents());

  const usize numTuples = exemplarArray.getNumberOfTuples();
  const usize numComps = exemplarArray.getNumberOfComponents();

  auto extractAndSort = [numTuples, numComps](const DataArray<float32>& arr) {
    std::vector<std::vector<float32>> rows(numTuples, std::vector<float32>(numComps));
    for(usize i = 0; i < numTuples; i++)
    {
      for(usize c = 0; c < numComps; c++)
      {
        rows[i][c] = arr[i * numComps + c];
      }
    }
    std::sort(rows.begin(), rows.end());
    return rows;
  };

  auto exemplarRows = extractAndSort(exemplarArray);
  auto computedRows = extractAndSort(computedArray);

  for(usize i = 0; i < numTuples; i++)
  {
    for(usize c = 0; c < numComps; c++)
    {
      float32 diff = std::fabs(exemplarRows[i][c] - computedRows[i][c]);
      INFO(fmt::format("Sorted row {}, comp {}: exemplar={}, computed={}", i, c, exemplarRows[i][c], computedRows[i][c]));
      REQUIRE(diff < UnitTest::EPSILON);
    }
  }
}

const DataPath k_ExemplarDistributionPath({"6_6_distribution"});
const DataPath k_ExemplarErrorPath({"6_6_errors"});
const DataPath k_ComputedDistributionPath({"NX_distribution"});
const DataPath k_ComputedErrorPath({"NX_errors"});
} // namespace

TEST_CASE("OrientationAnalysis::ComputeGBPDMetricBasedFilter: Valid Filter Execution", "[OrientationAnalysis][ComputeGBPDMetricBasedFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_find_gbpd_metric_based.tar.gz", "6_6_find_gbpd_metric_based");

  auto* filterList = Application::Instance()->getFilterList();

  // Read Exemplar DREAM3D File Input
  auto exemplarInputFilePath = fs::path(fmt::format("{}/6_6_find_gbpd_metric_based/6_6_find_gbpd_metric_based.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarInputFilePath);

  fs::path exemplarDistOutput(fmt::format("{}/6_6_find_gbpd_metric_based/6_6_gbpd_distribution_1.dat", unit_test::k_TestFilesDir));
  fs::path exemplarErrorsOutput(fmt::format("{}/6_6_find_gbpd_metric_based/6_6_gbpd_distribution_errors_1.dat", unit_test::k_TestFilesDir));
  fs::path computedDistOutput(fmt::format("{}/computed_gbpd_distribution_1.dat", unit_test::k_BinaryTestOutputDir));
  fs::path computedErrorsOutput(fmt::format("{}/computed_gbpd_distribution_errors_1.dat", unit_test::k_BinaryTestOutputDir));

  // Run the ComputeGBCDMetricBased filter
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    ComputeGBPDMetricBasedFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_PhaseOfInterest_Key, std::make_any<int32>(1));
    args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_LimitDist_Key, std::make_any<float32>(7.0f));
    args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_NumSamplPts_Key, std::make_any<int32>(3000));
    args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_ExcludeTripleLines_Key, std::make_any<bool>(false));
    args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_DistOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedDistOutput));
    args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_ErrOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedErrorsOutput));
    args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_SaveRelativeErr_Key, std::make_any<bool>(false));
    args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleDataContainerPath));
    args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_NodeTypesArrayPath_Key, std::make_any<DataPath>(k_NodeTypesPath));
    args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FaceLabelsPath));
    args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(k_FaceNormalsPath));
    args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_SurfaceMeshFaceAreasArrayPath_Key, std::make_any<DataPath>(k_FaceAreasPath));
    args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_SurfaceMeshFeatureFaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FeatureFaceLabelsPath));
    args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(k_AvgEulerAnglesPath));
    args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));
    args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
  }

  // read in exemplar and computed data files for comparison
  {
    auto filter = filterList->createFilter(k_ReadTextDataArrayFilterHandle);
    REQUIRE(nullptr != filter);
    // exemplar distribution
    {
      Arguments args;
      args.insertOrAssign(ReadTextDataArrayFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(exemplarDistOutput));
      args.insertOrAssign(ReadTextDataArrayFilter::k_ScalarType_Key, std::make_any<NumericTypeParameter::ValueType>(nx::core::NumericType::float32));
      args.insertOrAssign(ReadTextDataArrayFilter::k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{static_cast<double>(3624)}}));
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
      args.insertOrAssign(ReadTextDataArrayFilter::k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{static_cast<double>(3624)}}));
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
      args.insertOrAssign(ReadTextDataArrayFilter::k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{static_cast<double>(3624)}}));
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
      args.insertOrAssign(ReadTextDataArrayFilter::k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{static_cast<double>(3624)}}));
      args.insertOrAssign(ReadTextDataArrayFilter::k_NComp_Key, std::make_any<uint64>(3));
      args.insertOrAssign(ReadTextDataArrayFilter::k_NSkipLines_Key, std::make_any<uint64>(1));
      args.insertOrAssign(ReadTextDataArrayFilter::k_DelimiterChoice_Key, std::make_any<ChoicesParameter::ValueType>(2));
      args.insertOrAssign(ReadTextDataArrayFilter::k_DataArrayPath_Key, std::make_any<DataPath>(k_ComputedErrorPath));

      auto executeResult = filter->execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    }
  }

// Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/Compute_GBPD_Metric_Based.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  // compare results (sorted to be independent of symmetry operator ordering)
  CompareSortedArrays(dataStructure, k_ExemplarDistributionPath, k_ComputedDistributionPath);
  CompareSortedArrays(dataStructure, k_ExemplarErrorPath, k_ComputedErrorPath);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeGBPDMetricBasedFilter: InValid Filter Execution")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_find_gbcd_metric_based.tar.gz", "6_6_find_gbcd_metric_based");

  // Read Exemplar DREAM3D File Input
  auto exemplarInputFilePath = fs::path(fmt::format("{}/6_6_find_gbcd_metric_based/6_6_find_gbcd_metric_based.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarInputFilePath);

  fs::path computedDistOutput(fmt::format("{}/computed_gbcd_distribution_1.dat", unit_test::k_BinaryTestOutputDir));
  fs::path computedErrorsOutput(fmt::format("{}/computed_gbcd_distribution_errors_1.dat", unit_test::k_BinaryTestOutputDir));

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ComputeGBPDMetricBasedFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_LimitDist_Key, std::make_any<float32>(7.0f));
  args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_ExcludeTripleLines_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_DistOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedDistOutput));
  args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_SaveRelativeErr_Key, std::make_any<bool>(true));
  args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_TriangleGeometryPath_Key, std::make_any<DataPath>(k_TriangleDataContainerPath));
  args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_NodeTypesArrayPath_Key, std::make_any<DataPath>(k_NodeTypesPath));
  args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_SurfaceMeshFaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FaceLabelsPath));
  args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_SurfaceMeshFaceNormalsArrayPath_Key, std::make_any<DataPath>(k_FaceNormalsPath));
  args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_SurfaceMeshFaceAreasArrayPath_Key, std::make_any<DataPath>(k_FaceAreasPath));
  args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_SurfaceMeshFeatureFaceLabelsArrayPath_Key, std::make_any<DataPath>(k_FeatureFaceLabelsPath));
  args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_FeatureEulerAnglesArrayPath_Key, std::make_any<DataPath>(k_AvgEulerAnglesPath));
  args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_PhasesPath));
  args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructuresPath));

  SECTION("Invalid Number of Sample Points")
  {
    args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_PhaseOfInterest_Key, std::make_any<int32>(1));
    args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_NumSamplPts_Key, std::make_any<int32>(0));
    args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_ErrOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedErrorsOutput));
  }
  SECTION("Invalid Phase Of Interest Value (must be > 0)")
  {
    args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_PhaseOfInterest_Key, std::make_any<int32>(0));
    args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_NumSamplPts_Key, std::make_any<int32>(3000));
    args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_ErrOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedErrorsOutput));
  }
  SECTION("Invalid Phase Of Interest Value (cannot be > number of ensembles)")
  {
    args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_PhaseOfInterest_Key, std::make_any<int32>(2));
    args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_NumSamplPts_Key, std::make_any<int32>(3000));
    args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_ErrOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedErrorsOutput));
  }
  SECTION("Invalid Output File Names")
  {
    args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_PhaseOfInterest_Key, std::make_any<int32>(1));
    args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_NumSamplPts_Key, std::make_any<int32>(3000));
    args.insertOrAssign(ComputeGBPDMetricBasedFilter::k_ErrOutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(computedDistOutput));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeGBPDMetricBasedFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeGBPDMetricBasedFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeGBPDMetricBasedFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeGBPDMetricBasedFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeGBPDMetricBasedFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<int32>(ComputeGBPDMetricBasedFilter::k_PhaseOfInterest_Key) == 5);
      CHECK(args.value<float32>(ComputeGBPDMetricBasedFilter::k_LimitDist_Key) == 2.5f);
      CHECK(args.value<int32>(ComputeGBPDMetricBasedFilter::k_NumSamplPts_Key) == 5);
      CHECK(args.value<bool>(ComputeGBPDMetricBasedFilter::k_ExcludeTripleLines_Key) == true);
      CHECK(args.value<FileSystemPathParameter::ValueType>(ComputeGBPDMetricBasedFilter::k_DistOutputFile_Key) == fs::path("/test/path/file.txt"));
      CHECK(args.value<FileSystemPathParameter::ValueType>(ComputeGBPDMetricBasedFilter::k_ErrOutputFile_Key) == fs::path("/test/path/file.txt"));
      CHECK(args.value<bool>(ComputeGBPDMetricBasedFilter::k_SaveRelativeErr_Key) == true);
      CHECK(args.value<DataPath>(ComputeGBPDMetricBasedFilter::k_NodeTypesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeGBPDMetricBasedFilter::k_SurfaceMeshFaceLabelsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeGBPDMetricBasedFilter::k_SurfaceMeshFaceNormalsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeGBPDMetricBasedFilter::k_SurfaceMeshFaceAreasArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeGBPDMetricBasedFilter::k_SurfaceMeshFeatureFaceLabelsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeGBPDMetricBasedFilter::k_FeatureEulerAnglesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeGBPDMetricBasedFilter::k_FeaturePhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeGBPDMetricBasedFilter::k_TriangleGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ComputeGBPDMetricBasedFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}
