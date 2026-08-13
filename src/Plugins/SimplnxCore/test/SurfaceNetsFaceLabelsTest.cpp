#include "SimplnxCore/Filters/SurfaceNetsFilter.hpp"
#include "SurfaceMeshingTestUtils.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Meshing/TriangleUtilities.hpp"

#include <catch2/catch.hpp>

#include <set>
#include <utility>

using namespace nx::core;

namespace
{
const DataPath k_TriangleGeomPath({"SurfaceNets"});

// Runs SurfaceNets on the flush cylinder, using the shared mesher-agnostic RunMesher helper (see
// SurfaceMeshingTestUtils.hpp) so the argument list is defined in one place instead of duplicated
// per test file.
SurfaceMeshingTest::MeshResult RunSurfaceNetsForFaceLabels()
{
  return SurfaceMeshingTest::RunMesher<SurfaceNetsFilter>(SurfaceMeshingTest::CreateCylinderInBox(true), k_TriangleGeomPath, BoundingBoxSkinMode::k_Off, [](Arguments& args) {
    args.insertOrAssign(SurfaceNetsFilter::k_ApplySmoothing_Key, std::make_any<bool>(false));
    args.insertOrAssign(SurfaceNetsFilter::k_SmoothingIterations_Key, std::make_any<int32>(20));
    args.insertOrAssign(SurfaceNetsFilter::k_MaxDistanceFromVoxelCenter_Key, std::make_any<float32>(1.0F));
    args.insertOrAssign(SurfaceNetsFilter::k_RelaxationFactor_Key, std::make_any<float32>(0.5F));
  });
}
} // namespace

TEST_CASE("SimplnxCore::SurfaceNetsFilter: Face Label conventions", "[SimplnxCore][SurfaceNetsFilter]")
{
  UnitTest::LoadPlugins();

  SurfaceMeshingTest::MeshResult meshResult = RunSurfaceNetsForFaceLabels();
  const std::set<std::pair<int32, int32>> labelPairs = SurfaceMeshingTest::CollectLabelPairs(meshResult);

  // The cylinder wall separates background (0) from the cylinder (1).
  CHECK(labelPairs.count({0, 1}) == 1);
  // The box wall backed by background must be distinguishable from a feature cap.
  CHECK(labelPairs.count({-1, 0}) == 1);
  // The cylinder's bottom cap: box wall backed by the cylinder.
  CHECK(labelPairs.count({-1, 1}) == 1);
  // The buggy {-1,-1} pair must not appear.
  CHECK(labelPairs.count({-1, -1}) == 0);

  UnitTest::CheckArraysInheritTupleDims(meshResult.Structure);
}
