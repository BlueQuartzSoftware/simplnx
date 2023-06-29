#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/MapPointCloudToRegularGridFilter.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <string>

using namespace complex;
using namespace complex::Constants;

namespace
{
const std::string k_Existing = "[Existing]";
const std::string k_Masked = "[Mask]";
const std::string k_Computed = "[Computed]";

const DataPath k_ExistingImageGeomPath({k_ImageGeometry.str() + " " + k_Existing});
const DataPath k_VertexGeometryPath({k_PointCloudContainerName});
const DataPath k_VertexDataPath = k_VertexGeometryPath.createChildPath(k_VertexData);
const DataPath k_MaskPath = k_VertexDataPath.createChildPath(k_Mask);

const DataPath k_ManualImageGeomPathExemplar({k_ImageGeometry});
const DataPath k_ManualMaskImageGeomPathExemplar({k_ImageGeometry.str() + " " + k_Masked});

const DataPath k_ManualImageGeomPathComputed({k_ImageGeometry.str() + " " + k_Computed});
const DataPath k_ManualMaskImageGeomPathComputed({k_ImageGeometry.str() + " " + k_Masked + " " + k_Computed});

const DataPath k_VoxelIndicesManualExemplar = k_VertexDataPath.createChildPath(k_VoxelIndices);
const DataPath k_VoxelIndicesManualMaskExemplar = k_VertexDataPath.createChildPath(k_VoxelIndices.str() + " " + k_Masked);
const DataPath k_VoxelIndicesExistingMaskExemplar = k_VertexDataPath.createChildPath(k_VoxelIndices.str() + " " + k_Existing);

const DataPath k_VoxelIndicesManualComputed = k_VertexDataPath.createChildPath(k_VoxelIndices.str() + " " + k_Computed);
const DataPath k_VoxelIndicesManualMaskComputed = k_VertexDataPath.createChildPath(k_VoxelIndices.str() + " " + k_Masked + " " + k_Computed);
const DataPath k_VoxelIndicesExistingMaskComputed = k_VertexDataPath.createChildPath(k_VoxelIndices.str() + " " + k_Existing + " " + k_Computed);
} // namespace

TEST_CASE("ComplexCore::MapPointCloudToRegularGridFilter: Valid Filter Execution - Manual Geometry", "[MapPointCloudToRegularGridFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_map_point_cloud_to_regular_grid.tar.gz",
                                                             "6_6_map_point_cloud_to_regular_grid", complex::unit_test::k_BinaryTestOutputDir);

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_map_point_cloud_to_regular_grid/6_6_map_point_cloud_to_regular_grid.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  MapPointCloudToRegularGridFilter filter;
  Arguments args;

  uint64 samplingGridType = 0;
  std::vector<int32> gridDimensions = {5, 5, 10};

  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_SamplingGridType_Key, std::make_any<uint64>(samplingGridType));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_GridDimensions_Key, std::make_any<std::vector<int32>>(gridDimensions));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_VertexGeometry_Key, std::make_any<DataPath>(k_VertexGeometryPath));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_NewImageGeometry_Key, std::make_any<DataPath>(k_ManualImageGeomPathComputed));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_VoxelIndices_Key, std::make_any<std::string>(k_VoxelIndicesManualComputed.getTargetName()));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_CellDataName_Key, std::make_any<std::string>(k_CellData));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareImageGeometry(dataStructure, k_ManualImageGeomPathExemplar, k_ManualImageGeomPathComputed);
  UnitTest::CompareArrays<uint64>(dataStructure, k_VoxelIndicesManualExemplar, k_VoxelIndicesManualComputed);
}

TEST_CASE("ComplexCore::MapPointCloudToRegularGridFilter: Valid Filter Execution - Manual Geometry with Mask", "[MapPointCloudToRegularGridFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_map_point_cloud_to_regular_grid.tar.gz",
                                                             "6_6_map_point_cloud_to_regular_grid", complex::unit_test::k_BinaryTestOutputDir);

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_map_point_cloud_to_regular_grid/6_6_map_point_cloud_to_regular_grid.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  MapPointCloudToRegularGridFilter filter;
  Arguments args;

  uint64 samplingGridType = 0;
  std::vector<int32> gridDimensions{11, 11, 26};

  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_SamplingGridType_Key, std::make_any<uint64>(samplingGridType));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_GridDimensions_Key, std::make_any<std::vector<int32>>(gridDimensions));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_VertexGeometry_Key, std::make_any<DataPath>(k_VertexGeometryPath));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_NewImageGeometry_Key, std::make_any<DataPath>(k_ManualMaskImageGeomPathComputed));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_MaskPath_Key, std::make_any<DataPath>(k_MaskPath));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_VoxelIndices_Key, std::make_any<std::string>(k_VoxelIndicesManualMaskComputed.getTargetName()));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_CellDataName_Key, std::make_any<std::string>(k_CellData));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareImageGeometry(dataStructure, k_ManualMaskImageGeomPathExemplar, k_ManualMaskImageGeomPathComputed);
  UnitTest::CompareArrays<uint64>(dataStructure, k_VoxelIndicesManualMaskExemplar, k_VoxelIndicesManualMaskComputed);
}

TEST_CASE("ComplexCore::MapPointCloudToRegularGridFilter: Valid Filter Execution - Existing Geometry with Mask", "[MapPointCloudToRegularGridFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_map_point_cloud_to_regular_grid.tar.gz",
                                                             "6_6_map_point_cloud_to_regular_grid", complex::unit_test::k_BinaryTestOutputDir);

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_map_point_cloud_to_regular_grid/6_6_map_point_cloud_to_regular_grid.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  MapPointCloudToRegularGridFilter filter;
  Arguments args;

  uint64 samplingGridType = 1;

  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_SamplingGridType_Key, std::make_any<uint64>(samplingGridType));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_VertexGeometry_Key, std::make_any<DataPath>(k_VertexGeometryPath));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_ExistingImageGeometry_Key, std::make_any<DataPath>(k_ExistingImageGeomPath));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_MaskPath_Key, std::make_any<DataPath>(k_MaskPath));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_VoxelIndices_Key, std::make_any<std::string>(k_VoxelIndicesExistingMaskComputed.getTargetName()));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  UnitTest::CompareArrays<uint64>(dataStructure, k_VoxelIndicesExistingMaskExemplar, k_VoxelIndicesExistingMaskComputed);
}

TEST_CASE("ComplexCore::MapPointCloudToRegularGridFilter: Invalid Filter Execution", "[MapPointCloudToRegularGridFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_map_point_cloud_to_regular_grid.tar.gz",
                                                             "6_6_map_point_cloud_to_regular_grid", complex::unit_test::k_BinaryTestOutputDir);

  // Read Exemplar DREAM3D File Filter
  auto exemplarFilePath = fs::path(fmt::format("{}/6_6_map_point_cloud_to_regular_grid/6_6_map_point_cloud_to_regular_grid.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(exemplarFilePath);

  MapPointCloudToRegularGridFilter filter;
  Arguments args;

  uint64 samplingGridType = 0;
  std::vector<int32> gridDimensions{11, 11, 26};

  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_SamplingGridType_Key, std::make_any<uint64>(samplingGridType));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_VertexGeometry_Key, std::make_any<DataPath>(k_VertexGeometryPath));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_NewImageGeometry_Key, std::make_any<DataPath>(k_ManualMaskImageGeomPathComputed));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_VoxelIndices_Key, std::make_any<std::string>(k_VoxelIndicesManualMaskComputed.getTargetName()));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_CellDataName_Key, std::make_any<std::string>(k_CellData));

  SECTION("Invalid Grid Dimensions")
  {
    gridDimensions[2] = 0;
    args.insertOrAssign(MapPointCloudToRegularGridFilter::k_GridDimensions_Key, std::make_any<std::vector<int32>>(gridDimensions));
    args.insertOrAssign(MapPointCloudToRegularGridFilter::k_MaskPath_Key, std::make_any<DataPath>(k_MaskPath));
  }
  SECTION("Mismatching mask & voxel indices array tuples")
  {
    const std::string invalidMask = "Invalid Mask Array";
    UnitTest::CreateTestDataArray<bool>(dataStructure, invalidMask, std::vector<usize>{100}, std::vector<usize>{1});
    args.insertOrAssign(MapPointCloudToRegularGridFilter::k_GridDimensions_Key, std::make_any<std::vector<int32>>(gridDimensions));
    args.insertOrAssign(MapPointCloudToRegularGridFilter::k_MaskPath_Key, std::make_any<DataPath>(DataPath({invalidMask})));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(executeResult.result)
}
