#include "SimplnxCore/Filters/IterativeClosestPointFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <limits>
#include <string>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;

TEST_CASE("SimplnxCore::IterativeClosestPointFilter: Create Filter", "[DREAM3DReview][IterativeClosestPointFilter]")
{
  IterativeClosestPointFilter filter;
  DataStructure dataStructure;
  Arguments args;

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.invalid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.invalid());
}

TEST_CASE("SimplnxCore::IterativeClosestPointFilter: Test Algorithm", "[DREAM3DReview][IterativeClosestPointFilter]")
{
  IterativeClosestPointFilter filter;
  DataStructure dataStructure = UnitTest::CreateDataStructure();
  Arguments args;

  DataPath movingVertexPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_VertexGeometry});
  auto movingVertexGeom = dataStructure.getDataAs<VertexGeom>(movingVertexPath);

  uint64 numIterations = 1;
  bool applyTransformation = true;
  DataPath transformArrayPath({Constants::k_SmallIN100, "Transform Array"});

  auto* targetVertexGeom = VertexGeom::Create(dataStructure, "Vertex Geometry 2", movingVertexGeom->getParentIds().front());
  auto* euler_data_2 = Float32Array::CreateWithStore<DataStore<float>>(dataStructure, "Euler 2", {movingVertexGeom->getVertices()->getNumberOfTuples()}, {3}, targetVertexGeom->getParentIds().front());
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
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());
}
