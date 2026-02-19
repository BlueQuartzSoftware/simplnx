#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/PointSampleEdgeGeometryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

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

TEST_CASE("SimplnxCore::PointSampleEdgeGeometryFilter: Valid Filter Execution", "[SimplnxCore][PointSampleEdgeGeometryFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "point_sample_edge_geometry.tar.gz", "point_sample_edge_geometry");

  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/point_sample_edge_geometry/point_sample_edge_geometry.dream3d", unit_test::k_TestFilesDir)));
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/point_sample_edge_geometry/point_sample_edge_geometry.dream3d", unit_test::k_TestFilesDir)));

  // Instantiate the filter and an Arguments Object
  PointSampleEdgeGeometryFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_ScanVectorSamplingRes_Key, std::make_any<float32>(0.1f));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_ScanVectorGeometryPath_Key, std::make_any<DataPath>(k_ScanVectorGeometryPath));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_CalculateCumulativeSampleDistance_Key, std::make_any<bool>(true));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_SampledVertexGeometryPath_Key, std::make_any<DataPath>(k_SampledVertexGeomPath));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_EdgeIdsArrayName_Key, std::make_any<std::string>("Edge Ids"));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_CumulativeSampleDistanceArrayName_Key, std::make_any<std::string>("Cumulative Sample Distance"));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>({k_PowerArrayPath, k_SliceIdArrayPath}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  UnitTest::CompareExemplarToGeneratedData(dataStructure, exemplarDataStructure, k_SampledVertexAMPath, k_ExemplarySampledVertexGeomPath.getTargetName());

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::PointSampleEdgeGeometryFilter: Invalid Filter Execution", "[SimplnxCore][PointSampleEdgeGeometryFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "point_sample_edge_geometry.tar.gz", "point_sample_edge_geometry");

  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/point_sample_edge_geometry/point_sample_edge_geometry.dream3d", unit_test::k_TestFilesDir)));

  // Instantiate the filter, a DataStructure object and an Arguments Object
  PointSampleEdgeGeometryFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_ScanVectorSamplingRes_Key, std::make_any<float32>(0.1f));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_ScanVectorGeometryPath_Key, std::make_any<DataPath>(k_ScanVectorGeometryPath));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_CalculateCumulativeSampleDistance_Key, std::make_any<bool>(true));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_SampledVertexGeometryPath_Key, std::make_any<DataPath>(k_SampledVertexGeomPath));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_EdgeIdsArrayName_Key, std::make_any<std::string>("Edge Ids"));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_CumulativeSampleDistanceArrayName_Key, std::make_any<std::string>(""));
  args.insertOrAssign(PointSampleEdgeGeometryFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>({k_PowerArrayPath, k_SliceIdArrayPath}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
