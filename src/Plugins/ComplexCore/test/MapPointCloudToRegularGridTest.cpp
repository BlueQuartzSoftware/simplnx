#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/MapPointCloudToRegularGridFilter.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <string>

using namespace complex;
using namespace complex::Constants;

TEST_CASE("ComplexCore::MapPointCloudToRegularGridFilter: Instantiate Filter", "[MapPointCloudToRegularGridFilter]")
{
  MapPointCloudToRegularGridFilter filter;
  DataStructure dataStructure;
  Arguments args;

  uint64 samplingGridType = 0;
  std::vector<int32> gridDimensions;
  DataPath vertexGeomPath;
  DataPath imageGeomPath;
  std::vector<DataPath> arraysToMap;
  bool useMask = true;
  DataPath maskPath;
  DataPath voxelIndicesPath;

  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_SamplingGridType_Key, std::make_any<uint64>(samplingGridType));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_GridDimensions_Key, std::make_any<std::vector<int32>>(gridDimensions));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_VertexGeometry_Key, std::make_any<DataPath>(vertexGeomPath));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_NewImageGeometry_Key, std::make_any<DataPath>(imageGeomPath));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_ExistingImageGeometry_Key, std::make_any<DataPath>(imageGeomPath));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_ArraysToMap_Key, std::make_any<std::vector<DataPath>>(arraysToMap));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_UseMask_Key, std::make_any<bool>(useMask));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_MaskPath_Key, std::make_any<DataPath>(maskPath));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_VoxelIndices_Key, std::make_any<DataPath>(voxelIndicesPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.invalid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.invalid());
}

TEST_CASE("ComplexCore::MapPointCloudToRegularGridFilter: Test Algorithm 1", "[MapPointCloudToRegularGridFilter]")
{
  MapPointCloudToRegularGridFilter filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  uint64 samplingGridType = 0;
  std::vector<int32> gridDimensions{1, 1, 1};
  DataPath vertexGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_VertexGeometry});
  DataPath newImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "Image Geometry (New)"});
  DataPath existingImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "Image Geometry"});
  std::vector<DataPath> arraysToMap = std::vector<DataPath>{DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ConfidenceIndex})};
  bool useMask = false;
  DataPath maskPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, k_ConditionalArray});
  DataPath voxelIndicesPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "Voxel Indices"});

  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_SamplingGridType_Key, std::make_any<uint64>(samplingGridType));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_GridDimensions_Key, std::make_any<std::vector<int32>>(gridDimensions));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_VertexGeometry_Key, std::make_any<DataPath>(vertexGeomPath));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_NewImageGeometry_Key, std::make_any<DataPath>(newImageGeomPath));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_ExistingImageGeometry_Key, std::make_any<DataPath>(existingImageGeomPath));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_ArraysToMap_Key, std::make_any<std::vector<DataPath>>(arraysToMap));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_UseMask_Key, std::make_any<bool>(useMask));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_MaskPath_Key, std::make_any<DataPath>(maskPath));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_VoxelIndices_Key, std::make_any<DataPath>(voxelIndicesPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
}

TEST_CASE("ComplexCore::MapPointCloudToRegularGridFilter: Test Algorithm 2", "[MapPointCloudToRegularGridFilter]")
{
  MapPointCloudToRegularGridFilter filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  uint64 samplingGridType = 1;
  std::vector<int32> gridDimensions{1, 1, 1};
  DataPath vertexGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_VertexGeometry});
  DataPath newImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "Image Geometry (New)"});
  DataPath existingImageGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "Image Geometry"});
  std::vector<DataPath> arraysToMap = std::vector<DataPath>{DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ConfidenceIndex})};
  bool useMask = false;
  DataPath maskPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, k_ConditionalArray});
  DataPath voxelIndicesPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "Voxel Indices"});

  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_SamplingGridType_Key, std::make_any<uint64>(samplingGridType));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_GridDimensions_Key, std::make_any<std::vector<int32>>(gridDimensions));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_VertexGeometry_Key, std::make_any<DataPath>(vertexGeomPath));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_NewImageGeometry_Key, std::make_any<DataPath>(newImageGeomPath));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_ExistingImageGeometry_Key, std::make_any<DataPath>(existingImageGeomPath));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_ArraysToMap_Key, std::make_any<std::vector<DataPath>>(arraysToMap));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_UseMask_Key, std::make_any<bool>(useMask));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_MaskPath_Key, std::make_any<DataPath>(maskPath));
  args.insertOrAssign(MapPointCloudToRegularGridFilter::k_VoxelIndices_Key, std::make_any<DataPath>(voxelIndicesPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
}
