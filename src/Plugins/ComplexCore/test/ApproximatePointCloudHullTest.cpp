#include <iostream>
#include <string>

#include <catch2/catch.hpp>

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/ApproximatePointCloudHull.hpp"

using namespace complex;
using namespace complex::Constants;

TEST_CASE("ApproximatePointCloudHull: Instantiate Filter", "[ApproximatePointCloudHull]")
{
  ApproximatePointCloudHull filter;
  DataStructure dataGraph;
  Arguments args;

  std::vector<float32> gridResolution;
  uint64 minEmptyNeighbors = 0;
  DataPath vertexGeomPath;
  DataPath hullVertexGeomPath;

  args.insertOrAssign(ApproximatePointCloudHull::k_GridResolution_Key, std::make_any<std::vector<float32>>(gridResolution));
  args.insertOrAssign(ApproximatePointCloudHull::k_MinEmptyNeighbors_Key, std::make_any<uint64>(minEmptyNeighbors));
  args.insertOrAssign(ApproximatePointCloudHull::k_VertexGeomPath_Key, std::make_any<DataPath>(vertexGeomPath));
  args.insertOrAssign(ApproximatePointCloudHull::k_HullVertexGeomPath_Key, std::make_any<DataPath>(hullVertexGeomPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.invalid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.invalid());
}

TEST_CASE("ApproximatePointCloudHull: Test Filter", "[ApproximatePointCloudHull]")
{
  ApproximatePointCloudHull filter;
  DataStructure dataGraph = UnitTest::CreateDataStructure();
  Arguments args;

  std::vector<float32> gridResolution = {1,1,1};
  uint64 minEmptyNeighbors = 0;
  DataPath vertexGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_VertexGeometry});
  DataPath hullVertexGeomPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "Hull Vertex Geometry"});

  args.insertOrAssign(ApproximatePointCloudHull::k_GridResolution_Key, std::make_any<std::vector<float32>>(gridResolution));
  args.insertOrAssign(ApproximatePointCloudHull::k_MinEmptyNeighbors_Key, std::make_any<uint64>(minEmptyNeighbors));
  args.insertOrAssign(ApproximatePointCloudHull::k_VertexGeomPath_Key, std::make_any<DataPath>(vertexGeomPath));
  args.insertOrAssign(ApproximatePointCloudHull::k_HullVertexGeomPath_Key, std::make_any<DataPath>(hullVertexGeomPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.valid());
}
