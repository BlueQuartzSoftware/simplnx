#include <iostream>
#include <string>

#include <catch2/catch.hpp>

#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/IterativeClosestPointFilter.hpp"

#include <filesystem>
#include <limits>

namespace fs = std::filesystem;

using namespace complex;
using namespace complex::Constants;

TEST_CASE("IterativeClosestPointFilter: Create Filter", "[DREAM3DReview][IterativeClosestPointFilter]")
{
  IterativeClosestPointFilter filter;
  DataStructure dataGraph;
  Arguments args;

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.invalid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.invalid());
}

TEST_CASE("IterativeClosestPointFilter: Test Algorithm", "[DREAM3DReview][IterativeClosestPointFilter]")
{
  IterativeClosestPointFilter filter;
  DataStructure dataGraph = UnitTest::CreateDataStructure();
  Arguments args;

  DataPath movingVertexPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_VertexGeometry});
  DataPath targetVertexPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "Vertex Geometry 2"});
  uint64 numIterations = 1;
  bool applyTransformation = true;
  DataPath transformArrayPath({Constants::k_SmallIN100, "Transform Array"});

  args.insertOrAssign(IterativeClosestPointFilter::k_MovingVertexPath_Key, std::make_any<DataPath>(movingVertexPath));
  args.insertOrAssign(IterativeClosestPointFilter::k_TargetVertexPath_Key, std::make_any<DataPath>(targetVertexPath));
  args.insertOrAssign(IterativeClosestPointFilter::k_NumIterations_Key, std::make_any<uint64>(numIterations));
  args.insertOrAssign(IterativeClosestPointFilter::k_ApplyTransformation_Key, std::make_any<bool>(applyTransformation));
  args.insertOrAssign(IterativeClosestPointFilter::k_TransformArrayPath_Key, std::make_any<DataPath>(transformArrayPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  REQUIRE(executeResult.result.valid());
}
