#include "SimplnxCore/Filters/M3CSurfaceMeshingFilter.hpp"
#include "SimplnxCore/Filters/QuickSurfaceMeshFilter.hpp"
#include "SimplnxCore/Filters/SurfaceNetsFilter.hpp"
#include "SurfaceMeshingTestUtils.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <map>
#include <set>
#include <tuple>
#include <utility>

using namespace nx::core;

namespace
{
const DataPath k_TriangleGeomPath({"QuickMesh"});

SurfaceMeshingTest::MeshResult RunQuickSurfaceMesh(bool flushWithBottom, bool omitSkin)
{
  return SurfaceMeshingTest::RunMesher<QuickSurfaceMeshFilter>(SurfaceMeshingTest::CreateCylinderInBox(flushWithBottom), k_TriangleGeomPath, omitSkin,
                                                               [](Arguments& args) { args.insertOrAssign(QuickSurfaceMeshFilter::k_FixProblemVoxels_Key, std::make_any<bool>(false)); });
}
} // namespace

TEST_CASE("SimplnxCore::QuickSurfaceMeshFilter: Omit Bounding Box Skin", "[SimplnxCore][QuickSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();

  SECTION("Option off leaves the box skin over background in place")
  {
    SurfaceMeshingTest::MeshResult meshResult = RunQuickSurfaceMesh(true, false);
    const auto labelPairs = SurfaceMeshingTest::CollectLabelPairs(meshResult);
    REQUIRE(labelPairs.count({-1, 0}) == 1);
    UnitTest::CheckArraysInheritTupleDims(meshResult.Structure);
  }

  SECTION("Option on removes only the background skin and keeps the cylinder closed")
  {
    SurfaceMeshingTest::MeshResult meshResult = RunQuickSurfaceMesh(true, true);
    const auto labelPairs = SurfaceMeshingTest::CollectLabelPairs(meshResult);

    // The artificial background skin is gone.
    REQUIRE(labelPairs.count({-1, 0}) == 0);
    // The cylinder's bottom cap survives -- this is the issue-2 fix.
    REQUIRE(labelPairs.count({-1, 1}) == 1);
    // The cylinder wall is untouched.
    REQUIRE(labelPairs.count({0, 1}) == 1);

    // With the cap kept, the cylinder is a closed surface.
    REQUIRE_NOTHROW(meshResult.Structure.getDataRefAs<TriangleGeom>(meshResult.TriangleGeomPath));
    const auto& triangleGeom = meshResult.Structure.getDataRefAs<TriangleGeom>(meshResult.TriangleGeomPath);
    REQUIRE(SurfaceMeshingTest::IsWatertight(triangleGeom));

    UnitTest::CheckArraysInheritTupleDims(meshResult.Structure);
  }

  SECTION("Option on is a no-op when nothing touches the wall except background")
  {
    // Inset cylinder: the whole box wall is background, so ALL skin is dropped and only
    // the cylinder surface remains -- which is closed on its own.
    SurfaceMeshingTest::MeshResult meshResult = RunQuickSurfaceMesh(false, true);
    const auto labelPairs = SurfaceMeshingTest::CollectLabelPairs(meshResult);
    REQUIRE(labelPairs.count({-1, 0}) == 0);
    REQUIRE(labelPairs.count({-1, 1}) == 0);
    REQUIRE(labelPairs.count({0, 1}) == 1);

    const auto& triangleGeom = meshResult.Structure.getDataRefAs<TriangleGeom>(meshResult.TriangleGeomPath);
    REQUIRE(SurfaceMeshingTest::IsWatertight(triangleGeom));

    UnitTest::CheckArraysInheritTupleDims(meshResult.Structure);
  }

  SECTION("Node Types of surviving nodes match the full mesh")
  {
    SurfaceMeshingTest::MeshResult fullMesh = RunQuickSurfaceMesh(true, false);
    SurfaceMeshingTest::MeshResult prunedMesh = RunQuickSurfaceMesh(true, true);

    const DataPath nodeTypesPath = k_TriangleGeomPath.createChildPath("Vertex Data").createChildPath("NodeTypes");
    const auto& fullVertsRef = fullMesh.Structure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath).getVertices()->getDataStoreRef();
    const auto& fullTypesRef = fullMesh.Structure.getDataRefAs<Int8Array>(nodeTypesPath).getDataStoreRef();
    const auto& prunedVertsRef = prunedMesh.Structure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath).getVertices()->getDataStoreRef();
    const auto& prunedTypesRef = prunedMesh.Structure.getDataRefAs<Int8Array>(nodeTypesPath).getDataStoreRef();

    // Map full-mesh vertex coordinate -> node type, then check every surviving vertex agrees.
    std::map<std::tuple<float32, float32, float32>, int8> fullTypeByCoord;
    for(usize i = 0; i < fullTypesRef.getNumberOfTuples(); i++)
    {
      fullTypeByCoord[{fullVertsRef[i * 3], fullVertsRef[i * 3 + 1], fullVertsRef[i * 3 + 2]}] = fullTypesRef[i];
    }

    for(usize i = 0; i < prunedTypesRef.getNumberOfTuples(); i++)
    {
      const std::tuple<float32, float32, float32> coord = {prunedVertsRef[i * 3], prunedVertsRef[i * 3 + 1], prunedVertsRef[i * 3 + 2]};
      REQUIRE(fullTypeByCoord.count(coord) == 1);
      REQUIRE(fullTypeByCoord[coord] == prunedTypesRef[i]);
    }
  }
}

namespace
{
const DataPath k_SurfaceNetsTriangleGeomPath({"SurfaceNets"});

SurfaceMeshingTest::MeshResult RunSurfaceNets(bool flushWithBottom, bool omitSkin)
{
  return SurfaceMeshingTest::RunMesher<SurfaceNetsFilter>(SurfaceMeshingTest::CreateCylinderInBox(flushWithBottom), k_SurfaceNetsTriangleGeomPath, omitSkin, [](Arguments& args) {
    args.insertOrAssign(SurfaceNetsFilter::k_ApplySmoothing_Key, std::make_any<bool>(false));
    args.insertOrAssign(SurfaceNetsFilter::k_SmoothingIterations_Key, std::make_any<int32>(20));
    args.insertOrAssign(SurfaceNetsFilter::k_MaxDistanceFromVoxelCenter_Key, std::make_any<float32>(1.0F));
    args.insertOrAssign(SurfaceNetsFilter::k_RelaxationFactor_Key, std::make_any<float32>(0.5F));
  });
}
} // namespace

TEST_CASE("SimplnxCore::SurfaceNetsFilter: Omit Bounding Box Skin", "[SimplnxCore][SurfaceNetsFilter]")
{
  UnitTest::LoadPlugins();

  SECTION("Option off leaves the box skin over background in place")
  {
    SurfaceMeshingTest::MeshResult meshResult = RunSurfaceNets(true, false);
    REQUIRE(SurfaceMeshingTest::CollectLabelPairs(meshResult).count({-1, 0}) == 1);
    UnitTest::CheckArraysInheritTupleDims(meshResult.Structure);
  }

  SECTION("Option on removes only the background skin and keeps the cylinder closed")
  {
    SurfaceMeshingTest::MeshResult meshResult = RunSurfaceNets(true, true);
    const auto labelPairs = SurfaceMeshingTest::CollectLabelPairs(meshResult);
    REQUIRE(labelPairs.count({-1, 0}) == 0);
    REQUIRE(labelPairs.count({-1, 1}) == 1);
    REQUIRE(labelPairs.count({0, 1}) == 1);

    REQUIRE_NOTHROW(meshResult.Structure.getDataRefAs<TriangleGeom>(meshResult.TriangleGeomPath));
    const auto& triangleGeom = meshResult.Structure.getDataRefAs<TriangleGeom>(meshResult.TriangleGeomPath);
    REQUIRE(SurfaceMeshingTest::IsWatertight(triangleGeom));

    UnitTest::CheckArraysInheritTupleDims(meshResult.Structure);
  }

  SECTION("Option on leaves no orphan vertices")
  {
    SurfaceMeshingTest::MeshResult meshResult = RunSurfaceNets(true, true);
    REQUIRE_NOTHROW(meshResult.Structure.getDataRefAs<TriangleGeom>(meshResult.TriangleGeomPath));
    const auto& triangleGeom = meshResult.Structure.getDataRefAs<TriangleGeom>(meshResult.TriangleGeomPath);
    const auto& facesRef = triangleGeom.getFaces()->getDataStoreRef();

    std::set<usize> referencedVertices;
    for(usize i = 0; i < triangleGeom.getNumberOfFaces() * 3; i++)
    {
      referencedVertices.insert(static_cast<usize>(facesRef[i]));
    }
    REQUIRE(referencedVertices.size() == triangleGeom.getNumberOfVertices());

    const DataPath nodeTypesPath = meshResult.TriangleGeomPath.createChildPath("Vertex Data").createChildPath("NodeTypes");
    REQUIRE_NOTHROW(meshResult.Structure.getDataRefAs<Int8Array>(nodeTypesPath));
    const auto& nodeTypesRef = meshResult.Structure.getDataRefAs<Int8Array>(nodeTypesPath).getDataStoreRef();
    REQUIRE(nodeTypesRef.getNumberOfTuples() == triangleGeom.getNumberOfVertices());

    UnitTest::CheckArraysInheritTupleDims(meshResult.Structure);
  }
}

namespace
{
const DataPath k_M3CTriangleGeomPath({"M3CMesh"});

SurfaceMeshingTest::MeshResult RunM3C(bool flushWithBottom, bool omitSkin)
{
  return SurfaceMeshingTest::RunMesher<M3CSurfaceMeshingFilter>(SurfaceMeshingTest::CreateCylinderInBox(flushWithBottom), k_M3CTriangleGeomPath, omitSkin, [](Arguments&) {});
}
} // namespace

TEST_CASE("SimplnxCore::M3CSurfaceMeshingFilter: Omit Bounding Box Skin", "[SimplnxCore][M3CSurfaceMeshingFilter]")
{
  UnitTest::LoadPlugins();

  SECTION("Option off leaves the box skin over background in place")
  {
    SurfaceMeshingTest::MeshResult meshResult = RunM3C(true, false);
    REQUIRE(SurfaceMeshingTest::CollectLabelPairs(meshResult).count({-1, 0}) == 1);
    UnitTest::CheckArraysInheritTupleDims(meshResult.Structure);
  }

  SECTION("Option on removes only the background skin and keeps the cylinder closed")
  {
    SurfaceMeshingTest::MeshResult meshResult = RunM3C(true, true);
    const auto labelPairs = SurfaceMeshingTest::CollectLabelPairs(meshResult);
    REQUIRE(labelPairs.count({-1, 0}) == 0);
    REQUIRE(labelPairs.count({-1, 1}) == 1);
    REQUIRE(labelPairs.count({0, 1}) == 1);

    REQUIRE_NOTHROW(meshResult.Structure.getDataRefAs<TriangleGeom>(meshResult.TriangleGeomPath));
    const auto& triangleGeom = meshResult.Structure.getDataRefAs<TriangleGeom>(meshResult.TriangleGeomPath);
    REQUIRE(SurfaceMeshingTest::IsWatertight(triangleGeom));

    UnitTest::CheckArraysInheritTupleDims(meshResult.Structure);
  }

  SECTION("Option on leaves no orphan vertices")
  {
    SurfaceMeshingTest::MeshResult meshResult = RunM3C(true, true);
    REQUIRE_NOTHROW(meshResult.Structure.getDataRefAs<TriangleGeom>(meshResult.TriangleGeomPath));
    const auto& triangleGeom = meshResult.Structure.getDataRefAs<TriangleGeom>(meshResult.TriangleGeomPath);
    const auto& facesRef = triangleGeom.getFaces()->getDataStoreRef();

    std::set<usize> referenced;
    for(usize i = 0; i < triangleGeom.getNumberOfFaces() * 3; i++)
    {
      referenced.insert(static_cast<usize>(facesRef[i]));
    }
    REQUIRE(referenced.size() == triangleGeom.getNumberOfVertices());

    UnitTest::CheckArraysInheritTupleDims(meshResult.Structure);
  }
}

TEST_CASE("SimplnxCore::M3CSurfaceMeshingFilter: Node Types after omitting skin", "[SimplnxCore][M3CSurfaceMeshingFilter]")
{
  UnitTest::LoadPlugins();

  SurfaceMeshingTest::MeshResult fullMesh = RunM3C(true, false);
  SurfaceMeshingTest::MeshResult prunedMesh = RunM3C(true, true);

  const DataPath nodeTypesPath = k_M3CTriangleGeomPath.createChildPath("Vertex Data").createChildPath("NodeTypes");
  REQUIRE_NOTHROW(fullMesh.Structure.getDataRefAs<TriangleGeom>(k_M3CTriangleGeomPath));
  const auto& fullVertsRef = fullMesh.Structure.getDataRefAs<TriangleGeom>(k_M3CTriangleGeomPath).getVertices()->getDataStoreRef();
  REQUIRE_NOTHROW(fullMesh.Structure.getDataRefAs<Int8Array>(nodeTypesPath));
  const auto& fullTypesRef = fullMesh.Structure.getDataRefAs<Int8Array>(nodeTypesPath).getDataStoreRef();
  REQUIRE_NOTHROW(prunedMesh.Structure.getDataRefAs<TriangleGeom>(k_M3CTriangleGeomPath));
  const auto& prunedVertsRef = prunedMesh.Structure.getDataRefAs<TriangleGeom>(k_M3CTriangleGeomPath).getVertices()->getDataStoreRef();
  REQUIRE_NOTHROW(prunedMesh.Structure.getDataRefAs<Int8Array>(nodeTypesPath));
  const auto& prunedTypesRef = prunedMesh.Structure.getDataRefAs<Int8Array>(nodeTypesPath).getDataStoreRef();

  // Map full-mesh vertex coordinate -> node type, then check every surviving vertex against it.
  std::map<std::tuple<float32, float32, float32>, int8> fullTypeByCoord;
  for(usize i = 0; i < fullTypesRef.getNumberOfTuples(); i++)
  {
    fullTypeByCoord[{fullVertsRef[i * 3], fullVertsRef[i * 3 + 1], fullVertsRef[i * 3 + 2]}] = fullTypesRef[i];
  }

  usize divergentCount = 0;
  for(usize i = 0; i < prunedTypesRef.getNumberOfTuples(); i++)
  {
    const std::tuple<float32, float32, float32> coord = {prunedVertsRef[i * 3], prunedVertsRef[i * 3 + 1], prunedVertsRef[i * 3 + 2]};
    REQUIRE(fullTypeByCoord.count(coord) == 1);
    if(fullTypeByCoord[coord] != prunedTypesRef[i])
    {
      divergentCount++;
    }
  }

  // The spec deliberately makes no preservation claim for M3C. Record the finding: if this
  // is non-zero, characterize which nodes diverge and document it in the filter docs.
  INFO("M3C nodes whose Node Type changed when the skin was omitted: " << divergentCount);
  REQUIRE(divergentCount == 0);
}
