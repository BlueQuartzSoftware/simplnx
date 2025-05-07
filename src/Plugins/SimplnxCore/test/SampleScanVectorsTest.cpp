#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/SampleScanVectorsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const DataPath k_ScanVectorGeometryPath = DataPath({"ALSAM Scan"});
const DataPath k_PowerArrayPath = k_ScanVectorGeometryPath.createChildPath("Segments").createChildPath("Traveler - Power");
const DataPath k_SliceIdArrayPath = k_ScanVectorGeometryPath.createChildPath("Segments").createChildPath("LayerNumber");
const DataPath k_SampledVertexGeomPath = DataPath({"Sampled Vertex Geometry"});
const DataPath k_SampledVertexAMPath = k_SampledVertexGeomPath.createChildPath(VertexGeom::k_VertexAttributeMatrixName);
const DataPath k_ExemplarySampledVertexGeomPath = DataPath({"Exemplary Sampled Vertex Geometry"});
} // namespace

TEST_CASE("SimplnxCore::SampleScanVectorsFilter: Valid Filter Execution", "[SimplnxCore][SampleScanVectorsFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "sample_scan_vectors_test.tar.gz",
                                                              "sample_scan_vectors_test");

  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/sample_scan_vectors_test/sample_scan_vectors_test.dream3d", unit_test::k_TestFilesDir)));
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/sample_scan_vectors_test/sample_scan_vectors_test.dream3d", unit_test::k_TestFilesDir)));

  // Instantiate the filter and an Arguments Object
  SampleScanVectorsFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(SampleScanVectorsFilter::k_ScanVectorSamplingRes_Key, std::make_any<float32>(0.1f));
  args.insertOrAssign(SampleScanVectorsFilter::k_ScanVectorGeometryPath_Key, std::make_any<DataPath>(k_ScanVectorGeometryPath));
  args.insertOrAssign(SampleScanVectorsFilter::k_CopyPowerData_Key, std::make_any<bool>(true));
  args.insertOrAssign(SampleScanVectorsFilter::k_CopySliceIdData_Key, std::make_any<bool>(true));
  args.insertOrAssign(SampleScanVectorsFilter::k_CopyEdgeIdData_Key, std::make_any<bool>(true));
  args.insertOrAssign(SampleScanVectorsFilter::k_CalculateCumulativeSampleDistance_Key, std::make_any<bool>(true));
  args.insertOrAssign(SampleScanVectorsFilter::k_PowerArrayPath_Key, std::make_any<DataPath>(k_PowerArrayPath));
  args.insertOrAssign(SampleScanVectorsFilter::k_SliceIdArrayPath_Key, std::make_any<DataPath>(k_SliceIdArrayPath));
  args.insertOrAssign(SampleScanVectorsFilter::k_SampledVertexGeometryPath_Key, std::make_any<DataPath>(k_SampledVertexGeomPath));
  args.insertOrAssign(SampleScanVectorsFilter::k_EdgeIdsArrayName_Key, std::make_any<std::string>("Edge Ids"));
  args.insertOrAssign(SampleScanVectorsFilter::k_CumulativeSampleDistanceArrayName_Key, std::make_any<std::string>("Cumulative Sample Distance"));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  UnitTest::CompareExemplarToGeneratedData(dataStructure, exemplarDataStructure, k_SampledVertexAMPath, k_ExemplarySampledVertexGeomPath.getTargetName());
}

TEST_CASE("SimplnxCore::SampleScanVectorsFilter: Invalid Filter Execution", "[SimplnxCore][SampleScanVectorsFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "sample_scan_vectors_test.tar.gz",
                                                              "sample_scan_vectors_test");

  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/sample_scan_vectors_test/sample_scan_vectors_test.dream3d", unit_test::k_TestFilesDir)));

  // Instantiate the filter, a DataStructure object and an Arguments Object
  SampleScanVectorsFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(SampleScanVectorsFilter::k_ScanVectorSamplingRes_Key, std::make_any<float32>(0.1f));
  args.insertOrAssign(SampleScanVectorsFilter::k_ScanVectorGeometryPath_Key, std::make_any<DataPath>(k_ScanVectorGeometryPath));
  args.insertOrAssign(SampleScanVectorsFilter::k_CopyPowerData_Key, std::make_any<bool>(false));
  args.insertOrAssign(SampleScanVectorsFilter::k_CopySliceIdData_Key, std::make_any<bool>(false));
  args.insertOrAssign(SampleScanVectorsFilter::k_CopyEdgeIdData_Key, std::make_any<bool>(false));
  args.insertOrAssign(SampleScanVectorsFilter::k_CalculateCumulativeSampleDistance_Key, std::make_any<bool>(false));
  args.insertOrAssign(SampleScanVectorsFilter::k_PowerArrayPath_Key, std::make_any<DataPath>());
  args.insertOrAssign(SampleScanVectorsFilter::k_SliceIdArrayPath_Key, std::make_any<DataPath>());
  args.insertOrAssign(SampleScanVectorsFilter::k_SampledVertexGeometryPath_Key, std::make_any<DataPath>(k_SampledVertexGeomPath));
  args.insertOrAssign(SampleScanVectorsFilter::k_EdgeIdsArrayName_Key, std::make_any<std::string>("Edge Ids"));
  args.insertOrAssign(SampleScanVectorsFilter::k_CumulativeSampleDistanceArrayName_Key, std::make_any<std::string>("Cumulative Sample Distance"));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -5320);
}

TEST_CASE("SimplnxCore::SampleScanVectorsFilter: Valid Filter Execution-2", "[SimplnxCore][SampleScanVectorsFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "sample_scan_vectors_test.tar.gz",
                                                              "sample_scan_vectors_test");

  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/sample_scan_vectors_test/sample_scan_vectors_test.dream3d", unit_test::k_TestFilesDir)));
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/sample_scan_vectors_test/sample_scan_vectors_test.dream3d", unit_test::k_TestFilesDir)));

  // Instantiate the filter and an Arguments Object
  SampleScanVectorsFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(SampleScanVectorsFilter::k_ScanVectorSamplingRes_Key, std::make_any<float32>(0.1f));
  args.insertOrAssign(SampleScanVectorsFilter::k_ScanVectorGeometryPath_Key, std::make_any<DataPath>(k_ScanVectorGeometryPath));
  args.insertOrAssign(SampleScanVectorsFilter::k_CopyPowerData_Key, std::make_any<bool>(false));
  args.insertOrAssign(SampleScanVectorsFilter::k_CopySliceIdData_Key, std::make_any<bool>(false));
  args.insertOrAssign(SampleScanVectorsFilter::k_CopyEdgeIdData_Key, std::make_any<bool>(false));
  args.insertOrAssign(SampleScanVectorsFilter::k_CalculateCumulativeSampleDistance_Key, std::make_any<bool>(true));
  args.insertOrAssign(SampleScanVectorsFilter::k_PowerArrayPath_Key, std::make_any<DataPath>());
  args.insertOrAssign(SampleScanVectorsFilter::k_SliceIdArrayPath_Key, std::make_any<DataPath>());
  args.insertOrAssign(SampleScanVectorsFilter::k_SampledVertexGeometryPath_Key, std::make_any<DataPath>(k_SampledVertexGeomPath));
  args.insertOrAssign(SampleScanVectorsFilter::k_EdgeIdsArrayName_Key, std::make_any<std::string>("Edge Ids"));
  args.insertOrAssign(SampleScanVectorsFilter::k_CumulativeSampleDistanceArrayName_Key, std::make_any<std::string>("Cumulative Sample Distance"));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  UnitTest::CompareExemplarToGeneratedData(dataStructure, exemplarDataStructure, k_SampledVertexAMPath, k_ExemplarySampledVertexGeomPath.getTargetName());
}

TEST_CASE("SimplnxCore::SampleScanVectorsFilter: Valid Filter Execution-3", "[SimplnxCore][SampleScanVectorsFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "sample_scan_vectors_test.tar.gz",
                                                              "sample_scan_vectors_test");

  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/sample_scan_vectors_test/sample_scan_vectors_test.dream3d", unit_test::k_TestFilesDir)));
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/sample_scan_vectors_test/sample_scan_vectors_test.dream3d", unit_test::k_TestFilesDir)));

  // Instantiate the filter and an Arguments Object
  SampleScanVectorsFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(SampleScanVectorsFilter::k_ScanVectorSamplingRes_Key, std::make_any<float32>(0.1f));
  args.insertOrAssign(SampleScanVectorsFilter::k_ScanVectorGeometryPath_Key, std::make_any<DataPath>(k_ScanVectorGeometryPath));
  args.insertOrAssign(SampleScanVectorsFilter::k_CopyPowerData_Key, std::make_any<bool>(true));
  args.insertOrAssign(SampleScanVectorsFilter::k_CopySliceIdData_Key, std::make_any<bool>(false));
  args.insertOrAssign(SampleScanVectorsFilter::k_CopyEdgeIdData_Key, std::make_any<bool>(false));
  args.insertOrAssign(SampleScanVectorsFilter::k_CalculateCumulativeSampleDistance_Key, std::make_any<bool>(false));
  args.insertOrAssign(SampleScanVectorsFilter::k_PowerArrayPath_Key, std::make_any<DataPath>(k_PowerArrayPath));
  args.insertOrAssign(SampleScanVectorsFilter::k_SliceIdArrayPath_Key, std::make_any<DataPath>());
  args.insertOrAssign(SampleScanVectorsFilter::k_SampledVertexGeometryPath_Key, std::make_any<DataPath>(k_SampledVertexGeomPath));
  args.insertOrAssign(SampleScanVectorsFilter::k_EdgeIdsArrayName_Key, std::make_any<std::string>("Edge Ids"));
  args.insertOrAssign(SampleScanVectorsFilter::k_CumulativeSampleDistanceArrayName_Key, std::make_any<std::string>("Cumulative Sample Distance"));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  UnitTest::CompareExemplarToGeneratedData(dataStructure, exemplarDataStructure, k_SampledVertexAMPath, k_ExemplarySampledVertexGeomPath.getTargetName());
}
