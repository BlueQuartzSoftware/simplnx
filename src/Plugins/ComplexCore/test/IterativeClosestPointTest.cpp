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

TEST_CASE("ComplexCore::IterativeClosestPointFilter: Create Filter", "[DREAM3DReview][IterativeClosestPointFilter]")
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

TEST_CASE("ComplexCore::IterativeClosestPointFilter: Test Algorithm", "[DREAM3DReview][IterativeClosestPointFilter]")
{
  IterativeClosestPointFilter filter;
  DataStructure dataGraph = UnitTest::CreateDataStructure();
  Arguments args;

  DataPath movingVertexPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_VertexGeometry});
  auto movingVertexGeom = dataGraph.getDataAs<VertexGeom>(movingVertexPath);

  uint64 numIterations = 1;
  bool applyTransformation = true;
  DataPath transformArrayPath({Constants::k_SmallIN100, "Transform Array"});

  auto* targetVertexGeom = VertexGeom::Create(dataGraph, "Vertex Geometry 2", movingVertexGeom->getParentIds().front());
  auto* euler_data_2 = Float32Array::CreateWithStore<DataStore<float>>(dataGraph, "Euler 2", {movingVertexGeom->getVertices()->getNumberOfTuples()}, {3}, targetVertexGeom->getParentIds().front());
  targetVertexGeom->setVertices(*euler_data_2);

  DataPath targetVertexPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, targetVertexGeom->getName()});

  auto vertices1 = movingVertexGeom->getVertices();
  auto vertices2 = targetVertexGeom->getVertices();
  usize numVertices = vertices1->getSize();
  for(usize i = 0; i < numVertices; i++)
  {
    (*vertices1)[i] = i;
    (*vertices2)[i] = i;
  }

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
