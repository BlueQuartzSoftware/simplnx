#include "SimplnxCore/Filters/M3CSurfaceMeshingFilter.hpp"
#include "SimplnxCore/Filters/QuickSurfaceMeshFilter.hpp"
#include "SimplnxCore/Filters/SurfaceNetsFilter.hpp"
#include "SurfaceMeshingTestUtils.hpp"

#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Meshing/TriangleUtilities.hpp"

#include <catch2/catch.hpp>

#include <limits>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <utility>

using namespace nx::core;

namespace
{
const DataPath k_TriangleGeomPath({"QuickMesh"});

SurfaceMeshingTest::MeshResult RunQuickSurfaceMesh(bool flushWithBottom, ChoicesParameter::ValueType boundingBoxSkinMode)
{
  return SurfaceMeshingTest::RunMesher<QuickSurfaceMeshFilter>(SurfaceMeshingTest::CreateCylinderInBox(flushWithBottom), k_TriangleGeomPath, boundingBoxSkinMode,
                                                               [](Arguments& args) { args.insertOrAssign(QuickSurfaceMeshFilter::k_FixProblemVoxels_Key, std::make_any<bool>(false)); });
}
} // namespace

TEST_CASE("SimplnxCore::QuickSurfaceMeshFilter: Bounding Box Skin", "[SimplnxCore][QuickSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();

  SECTION("Option off leaves the box skin over background in place")
  {
    SurfaceMeshingTest::MeshResult meshResult = RunQuickSurfaceMesh(true, BoundingBoxSkinMode::k_Off);
    const auto labelPairs = SurfaceMeshingTest::CollectLabelPairs(meshResult);
    REQUIRE(labelPairs.count({-1, 0}) == 1);
    UnitTest::CheckArraysInheritTupleDims(meshResult.Structure);
  }

  SECTION("Option on removes only the background skin and keeps the cylinder closed")
  {
    SurfaceMeshingTest::MeshResult meshResult = RunQuickSurfaceMesh(true, BoundingBoxSkinMode::k_BackgroundBackedWallsOnly);
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
    SurfaceMeshingTest::MeshResult meshResult = RunQuickSurfaceMesh(false, BoundingBoxSkinMode::k_BackgroundBackedWallsOnly);
    const auto labelPairs = SurfaceMeshingTest::CollectLabelPairs(meshResult);
    REQUIRE(labelPairs.count({-1, 0}) == 0);
    REQUIRE(labelPairs.count({-1, 1}) == 0);
    REQUIRE(labelPairs.count({0, 1}) == 1);

    REQUIRE_NOTHROW(meshResult.Structure.getDataRefAs<TriangleGeom>(meshResult.TriangleGeomPath));
    const auto& triangleGeom = meshResult.Structure.getDataRefAs<TriangleGeom>(meshResult.TriangleGeomPath);
    REQUIRE(SurfaceMeshingTest::IsWatertight(triangleGeom));

    UnitTest::CheckArraysInheritTupleDims(meshResult.Structure);
  }

  SECTION("Node Types of surviving nodes match the full mesh")
  {
    SurfaceMeshingTest::MeshResult fullMesh = RunQuickSurfaceMesh(true, BoundingBoxSkinMode::k_Off);
    SurfaceMeshingTest::MeshResult prunedMesh = RunQuickSurfaceMesh(true, BoundingBoxSkinMode::k_BackgroundBackedWallsOnly);

    const DataPath nodeTypesPath = k_TriangleGeomPath.createChildPath("Vertex Data").createChildPath("NodeTypes");
    REQUIRE_NOTHROW(fullMesh.Structure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath));
    const auto& fullVertsRef = fullMesh.Structure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath).getVertices()->getDataStoreRef();
    REQUIRE_NOTHROW(fullMesh.Structure.getDataRefAs<Int8Array>(nodeTypesPath));
    const auto& fullTypesRef = fullMesh.Structure.getDataRefAs<Int8Array>(nodeTypesPath).getDataStoreRef();
    REQUIRE_NOTHROW(prunedMesh.Structure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath));
    const auto& prunedVertsRef = prunedMesh.Structure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath).getVertices()->getDataStoreRef();
    REQUIRE_NOTHROW(prunedMesh.Structure.getDataRefAs<Int8Array>(nodeTypesPath));
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

    UnitTest::CheckArraysInheritTupleDims(fullMesh.Structure);
    UnitTest::CheckArraysInheritTupleDims(prunedMesh.Structure);
  }
}

namespace
{
const DataPath k_SurfaceNetsTriangleGeomPath({"SurfaceNets"});

SurfaceMeshingTest::MeshResult RunSurfaceNets(bool flushWithBottom, ChoicesParameter::ValueType boundingBoxSkinMode)
{
  return SurfaceMeshingTest::RunMesher<SurfaceNetsFilter>(SurfaceMeshingTest::CreateCylinderInBox(flushWithBottom), k_SurfaceNetsTriangleGeomPath, boundingBoxSkinMode, [](Arguments& args) {
    args.insertOrAssign(SurfaceNetsFilter::k_ApplySmoothing_Key, std::make_any<bool>(false));
    args.insertOrAssign(SurfaceNetsFilter::k_SmoothingIterations_Key, std::make_any<int32>(20));
    args.insertOrAssign(SurfaceNetsFilter::k_MaxDistanceFromVoxelCenter_Key, std::make_any<float32>(1.0F));
    args.insertOrAssign(SurfaceNetsFilter::k_RelaxationFactor_Key, std::make_any<float32>(0.5F));
  });
}
} // namespace

TEST_CASE("SimplnxCore::SurfaceNetsFilter: Bounding Box Skin", "[SimplnxCore][SurfaceNetsFilter]")
{
  UnitTest::LoadPlugins();

  SECTION("Option off leaves the box skin over background in place")
  {
    SurfaceMeshingTest::MeshResult meshResult = RunSurfaceNets(true, BoundingBoxSkinMode::k_Off);
    REQUIRE(SurfaceMeshingTest::CollectLabelPairs(meshResult).count({-1, 0}) == 1);
    UnitTest::CheckArraysInheritTupleDims(meshResult.Structure);
  }

  SECTION("Option on removes only the background skin and keeps the cylinder closed")
  {
    SurfaceMeshingTest::MeshResult meshResult = RunSurfaceNets(true, BoundingBoxSkinMode::k_BackgroundBackedWallsOnly);
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
    SurfaceMeshingTest::MeshResult meshResult = RunSurfaceNets(true, BoundingBoxSkinMode::k_BackgroundBackedWallsOnly);
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

SurfaceMeshingTest::MeshResult RunM3C(bool flushWithBottom, ChoicesParameter::ValueType boundingBoxSkinMode)
{
  return SurfaceMeshingTest::RunMesher<M3CSurfaceMeshingFilter>(SurfaceMeshingTest::CreateCylinderInBox(flushWithBottom), k_M3CTriangleGeomPath, boundingBoxSkinMode, [](Arguments&) {});
}
} // namespace

TEST_CASE("SimplnxCore::M3CSurfaceMeshingFilter: Bounding Box Skin", "[SimplnxCore][M3CSurfaceMeshingFilter]")
{
  UnitTest::LoadPlugins();

  SECTION("Option off leaves the box skin over background in place")
  {
    SurfaceMeshingTest::MeshResult meshResult = RunM3C(true, BoundingBoxSkinMode::k_Off);
    REQUIRE(SurfaceMeshingTest::CollectLabelPairs(meshResult).count({-1, 0}) == 1);
    UnitTest::CheckArraysInheritTupleDims(meshResult.Structure);
  }

  SECTION("Option on removes only the background skin and keeps the cylinder closed")
  {
    SurfaceMeshingTest::MeshResult meshResult = RunM3C(true, BoundingBoxSkinMode::k_BackgroundBackedWallsOnly);
    const auto labelPairs = SurfaceMeshingTest::CollectLabelPairs(meshResult);
    REQUIRE(labelPairs.count({-1, 0}) == 0);
    REQUIRE(labelPairs.count({-1, 1}) == 1);
    REQUIRE(labelPairs.count({0, 1}) == 1);

    REQUIRE_NOTHROW(meshResult.Structure.getDataRefAs<TriangleGeom>(meshResult.TriangleGeomPath));
    const auto& triangleGeom = meshResult.Structure.getDataRefAs<TriangleGeom>(meshResult.TriangleGeomPath);
    REQUIRE(SurfaceMeshingTest::IsWatertight(triangleGeom));

    UnitTest::CheckArraysInheritTupleDims(meshResult.Structure);
  }

  // The option must never leave a vertex behind that only dropped faces referenced. Verified by
  // coordinate: a vertex unreferenced in the pruned mesh would have to be unreferenced in the full
  // (un-pruned) mesh at the same coordinate as well -- and since M3C no longer emits orphan vertices
  // at all (issue #1706 was resolved by the single ghost sentinel), no vertex is unreferenced in either.
  SECTION("Option on leaves no vertex newly orphaned by the prune")
  {
    SurfaceMeshingTest::MeshResult fullMesh = RunM3C(true, BoundingBoxSkinMode::k_Off);
    SurfaceMeshingTest::MeshResult prunedMesh = RunM3C(true, BoundingBoxSkinMode::k_BackgroundBackedWallsOnly);

    const auto isReferencedByCoord = [](const TriangleGeom& triangleGeom) {
      const auto& vertsRef = triangleGeom.getVertices()->getDataStoreRef();
      const auto& facesRef = triangleGeom.getFaces()->getDataStoreRef();

      std::set<usize> referencedIndices;
      for(usize i = 0; i < triangleGeom.getNumberOfFaces() * 3; i++)
      {
        referencedIndices.insert(static_cast<usize>(facesRef[i]));
      }

      std::map<std::tuple<float32, float32, float32>, bool> result;
      for(usize v = 0; v < triangleGeom.getNumberOfVertices(); v++)
      {
        const std::tuple<float32, float32, float32> coord = {vertsRef[v * 3], vertsRef[v * 3 + 1], vertsRef[v * 3 + 2]};
        result[coord] = referencedIndices.count(v) > 0;
      }
      return result;
    };

    REQUIRE_NOTHROW(fullMesh.Structure.getDataRefAs<TriangleGeom>(k_M3CTriangleGeomPath));
    REQUIRE_NOTHROW(prunedMesh.Structure.getDataRefAs<TriangleGeom>(k_M3CTriangleGeomPath));
    const auto fullReferenced = isReferencedByCoord(fullMesh.Structure.getDataRefAs<TriangleGeom>(k_M3CTriangleGeomPath));
    const auto prunedReferenced = isReferencedByCoord(prunedMesh.Structure.getDataRefAs<TriangleGeom>(k_M3CTriangleGeomPath));

    usize newlyOrphanedCount = 0;
    usize unreferencedCount = 0;
    for(const auto& [coord, isReferencedInPruned] : prunedReferenced)
    {
      if(isReferencedInPruned)
      {
        continue;
      }
      unreferencedCount++;
      // A vertex that survives the prune unreferenced must have also been unreferenced pre-prune.
      REQUIRE(fullReferenced.count(coord) == 1);
      if(fullReferenced.at(coord))
      {
        newlyOrphanedCount++;
      }
    }

    INFO("Vertices newly orphaned by the prune (must be 0): " << newlyOrphanedCount);
    REQUIRE(newlyOrphanedCount == 0);
    INFO("Unreferenced vertices in the pruned mesh (must be 0): " << unreferencedCount);
    REQUIRE(unreferencedCount == 0);

    UnitTest::CheckArraysInheritTupleDims(fullMesh.Structure);
    UnitTest::CheckArraysInheritTupleDims(prunedMesh.Structure);
  }
}

TEST_CASE("SimplnxCore::M3CSurfaceMeshingFilter: Node Types after omitting skin", "[SimplnxCore][M3CSurfaceMeshingFilter]")
{
  UnitTest::LoadPlugins();

  SurfaceMeshingTest::MeshResult fullMesh = RunM3C(true, BoundingBoxSkinMode::k_Off);
  SurfaceMeshingTest::MeshResult prunedMesh = RunM3C(true, BoundingBoxSkinMode::k_BackgroundBackedWallsOnly);

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

namespace
{
// Raw (unchecked-result) runners built on the shared RunMesherRaw helper, reusing the same
// Triangle Geometry paths and mesher-specific extra args as the RunQuickSurfaceMesh /
// RunSurfaceNets / RunM3C wrappers above, so degenerate cases can inspect the execute Result<>
// (e.g. warnings) instead of asserting validity and aborting.
Result<> RunQuickSurfaceMeshRaw(DataStructure& dataStructure, ChoicesParameter::ValueType boundingBoxSkinMode, bool repairWinding = false)
{
  return SurfaceMeshingTest::RunMesherRaw<QuickSurfaceMeshFilter>(
      dataStructure, k_TriangleGeomPath, boundingBoxSkinMode, [](Arguments& args) { args.insertOrAssign(QuickSurfaceMeshFilter::k_FixProblemVoxels_Key, std::make_any<bool>(false)); }, repairWinding);
}

Result<> RunSurfaceNetsRaw(DataStructure& dataStructure, ChoicesParameter::ValueType boundingBoxSkinMode, bool repairWinding = false)
{
  return SurfaceMeshingTest::RunMesherRaw<SurfaceNetsFilter>(
      dataStructure, k_SurfaceNetsTriangleGeomPath, boundingBoxSkinMode,
      [](Arguments& args) {
        args.insertOrAssign(SurfaceNetsFilter::k_ApplySmoothing_Key, std::make_any<bool>(false));
        args.insertOrAssign(SurfaceNetsFilter::k_SmoothingIterations_Key, std::make_any<int32>(20));
        args.insertOrAssign(SurfaceNetsFilter::k_MaxDistanceFromVoxelCenter_Key, std::make_any<float32>(1.0F));
        args.insertOrAssign(SurfaceNetsFilter::k_RelaxationFactor_Key, std::make_any<float32>(0.5F));
      },
      repairWinding);
}

Result<> RunM3CRaw(DataStructure& dataStructure, ChoicesParameter::ValueType boundingBoxSkinMode, bool repairWinding = false)
{
  return SurfaceMeshingTest::RunMesherRaw<M3CSurfaceMeshingFilter>(dataStructure, k_M3CTriangleGeomPath, boundingBoxSkinMode, [](Arguments&) {}, repairWinding);
}
} // namespace

TEST_CASE("SimplnxCore::Bounding Box Skin is a no-op without background", "[SimplnxCore][QuickSurfaceMeshFilter][SurfaceNetsFilter][M3CSurfaceMeshingFilter]")
{
  UnitTest::LoadPlugins();

  // A fully-indexed volume has no Feature Id 0, so no face can match {-1, 0} and the
  // option must change nothing -- every boundary Feature keeps its wall cap.
  SECTION("QuickSurfaceMesh")
  {
    DataStructure offStructure = SurfaceMeshingTest::CreateFullyIndexedPolycrystal();
    DataStructure onStructure = SurfaceMeshingTest::CreateFullyIndexedPolycrystal();
    // Store each Result<> before handing it to SIMPLNX_RESULT_REQUIRE_VALID: that macro expands
    // its argument multiple times (once per accessor), so passing the RunXRaw(...) call directly
    // would run the filter more than once against the same DataStructure.
    const Result<> offResult = RunQuickSurfaceMeshRaw(offStructure, BoundingBoxSkinMode::k_Off);
    SIMPLNX_RESULT_REQUIRE_VALID(offResult);
    const Result<> onResult = RunQuickSurfaceMeshRaw(onStructure, BoundingBoxSkinMode::k_BackgroundBackedWallsOnly);
    SIMPLNX_RESULT_REQUIRE_VALID(onResult);

    REQUIRE_NOTHROW(offStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath));
    REQUIRE_NOTHROW(onStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath));
    const auto& offGeom = offStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath);
    const auto& onGeom = onStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath);
    REQUIRE(onGeom.getNumberOfFaces() == offGeom.getNumberOfFaces());
    REQUIRE(onGeom.getNumberOfVertices() == offGeom.getNumberOfVertices());
    UnitTest::CompareDataArrays<uint64>(*offGeom.getFaces(), *onGeom.getFaces());
    UnitTest::CompareDataArrays<float32>(*offGeom.getVertices(), *onGeom.getVertices());

    const DataPath faceLabelsPath = k_TriangleGeomPath.createChildPath("Face Data").createChildPath("FaceLabels");
    REQUIRE_NOTHROW(offStructure.getDataRefAs<Int32Array>(faceLabelsPath));
    REQUIRE_NOTHROW(onStructure.getDataRefAs<Int32Array>(faceLabelsPath));
    UnitTest::CompareDataArrays<int32>(offStructure.getDataRefAs<Int32Array>(faceLabelsPath), onStructure.getDataRefAs<Int32Array>(faceLabelsPath));

    // This is the only no-op test that checks all three meshers, so it is the natural place to cover
    // Node Types for all of them rather than just Face Labels/Faces/Vertices.
    const DataPath nodeTypesPath = k_TriangleGeomPath.createChildPath("Vertex Data").createChildPath("NodeTypes");
    REQUIRE_NOTHROW(offStructure.getDataRefAs<Int8Array>(nodeTypesPath));
    REQUIRE_NOTHROW(onStructure.getDataRefAs<Int8Array>(nodeTypesPath));
    UnitTest::CompareDataArrays<int8>(offStructure.getDataRefAs<Int8Array>(nodeTypesPath), onStructure.getDataRefAs<Int8Array>(nodeTypesPath));

    UnitTest::CheckArraysInheritTupleDims(offStructure);
    UnitTest::CheckArraysInheritTupleDims(onStructure);
  }

  SECTION("SurfaceNets")
  {
    DataStructure offStructure = SurfaceMeshingTest::CreateFullyIndexedPolycrystal();
    DataStructure onStructure = SurfaceMeshingTest::CreateFullyIndexedPolycrystal();
    const Result<> offResult = RunSurfaceNetsRaw(offStructure, BoundingBoxSkinMode::k_Off);
    SIMPLNX_RESULT_REQUIRE_VALID(offResult);
    const Result<> onResult = RunSurfaceNetsRaw(onStructure, BoundingBoxSkinMode::k_BackgroundBackedWallsOnly);
    SIMPLNX_RESULT_REQUIRE_VALID(onResult);

    REQUIRE_NOTHROW(offStructure.getDataRefAs<TriangleGeom>(k_SurfaceNetsTriangleGeomPath));
    REQUIRE_NOTHROW(onStructure.getDataRefAs<TriangleGeom>(k_SurfaceNetsTriangleGeomPath));
    const auto& offGeom = offStructure.getDataRefAs<TriangleGeom>(k_SurfaceNetsTriangleGeomPath);
    const auto& onGeom = onStructure.getDataRefAs<TriangleGeom>(k_SurfaceNetsTriangleGeomPath);
    REQUIRE(onGeom.getNumberOfFaces() == offGeom.getNumberOfFaces());
    REQUIRE(onGeom.getNumberOfVertices() == offGeom.getNumberOfVertices());
    UnitTest::CompareDataArrays<uint64>(*offGeom.getFaces(), *onGeom.getFaces());
    UnitTest::CompareDataArrays<float32>(*offGeom.getVertices(), *onGeom.getVertices());

    const DataPath faceLabelsPath = k_SurfaceNetsTriangleGeomPath.createChildPath("Face Data").createChildPath("FaceLabels");
    REQUIRE_NOTHROW(offStructure.getDataRefAs<Int32Array>(faceLabelsPath));
    REQUIRE_NOTHROW(onStructure.getDataRefAs<Int32Array>(faceLabelsPath));
    UnitTest::CompareDataArrays<int32>(offStructure.getDataRefAs<Int32Array>(faceLabelsPath), onStructure.getDataRefAs<Int32Array>(faceLabelsPath));

    const DataPath nodeTypesPath = k_SurfaceNetsTriangleGeomPath.createChildPath("Vertex Data").createChildPath("NodeTypes");
    REQUIRE_NOTHROW(offStructure.getDataRefAs<Int8Array>(nodeTypesPath));
    REQUIRE_NOTHROW(onStructure.getDataRefAs<Int8Array>(nodeTypesPath));
    UnitTest::CompareDataArrays<int8>(offStructure.getDataRefAs<Int8Array>(nodeTypesPath), onStructure.getDataRefAs<Int8Array>(nodeTypesPath));

    UnitTest::CheckArraysInheritTupleDims(offStructure);
    UnitTest::CheckArraysInheritTupleDims(onStructure);
  }

  SECTION("M3CSurfaceMeshing")
  {
    DataStructure offStructure = SurfaceMeshingTest::CreateFullyIndexedPolycrystal();
    DataStructure onStructure = SurfaceMeshingTest::CreateFullyIndexedPolycrystal();
    const Result<> offResult = RunM3CRaw(offStructure, BoundingBoxSkinMode::k_Off);
    SIMPLNX_RESULT_REQUIRE_VALID(offResult);
    const Result<> onResult = RunM3CRaw(onStructure, BoundingBoxSkinMode::k_BackgroundBackedWallsOnly);
    SIMPLNX_RESULT_REQUIRE_VALID(onResult);

    REQUIRE_NOTHROW(offStructure.getDataRefAs<TriangleGeom>(k_M3CTriangleGeomPath));
    REQUIRE_NOTHROW(onStructure.getDataRefAs<TriangleGeom>(k_M3CTriangleGeomPath));
    const auto& offGeom = offStructure.getDataRefAs<TriangleGeom>(k_M3CTriangleGeomPath);
    const auto& onGeom = onStructure.getDataRefAs<TriangleGeom>(k_M3CTriangleGeomPath);
    REQUIRE(onGeom.getNumberOfFaces() == offGeom.getNumberOfFaces());
    REQUIRE(onGeom.getNumberOfVertices() == offGeom.getNumberOfVertices());
    UnitTest::CompareDataArrays<uint64>(*offGeom.getFaces(), *onGeom.getFaces());
    UnitTest::CompareDataArrays<float32>(*offGeom.getVertices(), *onGeom.getVertices());

    const DataPath faceLabelsPath = k_M3CTriangleGeomPath.createChildPath("Face Data").createChildPath("FaceLabels");
    REQUIRE_NOTHROW(offStructure.getDataRefAs<Int32Array>(faceLabelsPath));
    REQUIRE_NOTHROW(onStructure.getDataRefAs<Int32Array>(faceLabelsPath));
    UnitTest::CompareDataArrays<int32>(offStructure.getDataRefAs<Int32Array>(faceLabelsPath), onStructure.getDataRefAs<Int32Array>(faceLabelsPath));

    // M3C's orphan-node clearing after the prune is the one code path in the whole feature that is
    // gated on whether the prune actually dropped anything, not on the option flag alone. This
    // fully-indexed input drops nothing, so Node Types must still match exactly here.
    const DataPath nodeTypesPath = k_M3CTriangleGeomPath.createChildPath("Vertex Data").createChildPath("NodeTypes");
    REQUIRE_NOTHROW(offStructure.getDataRefAs<Int8Array>(nodeTypesPath));
    REQUIRE_NOTHROW(onStructure.getDataRefAs<Int8Array>(nodeTypesPath));
    UnitTest::CompareDataArrays<int8>(offStructure.getDataRefAs<Int8Array>(nodeTypesPath), onStructure.getDataRefAs<Int8Array>(nodeTypesPath));

    UnitTest::CheckArraysInheritTupleDims(offStructure);
    UnitTest::CheckArraysInheritTupleDims(onStructure);
  }
}

TEST_CASE("SimplnxCore::Bounding Box Skin warns when nothing is pruned", "[SimplnxCore][QuickSurfaceMeshFilter][SurfaceNetsFilter][M3CSurfaceMeshingFilter]")
{
  UnitTest::LoadPlugins();

  // A fully-indexed volume has no Feature Id 0, so the option suppresses zero faces. That is the
  // most common dataset shape in practice, and byte-identical output with no feedback is not
  // acceptable -- the user cannot tell the option had no effect. This is the companion to the
  // all-background case (which suppresses every face); the two warnings must never fire together.
  SECTION("QuickSurfaceMesh")
  {
    DataStructure dataStructure = SurfaceMeshingTest::CreateFullyIndexedPolycrystal();
    const Result<> executeResult = RunQuickSurfaceMeshRaw(dataStructure, BoundingBoxSkinMode::k_BackgroundBackedWallsOnly);

    SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
    REQUIRE(executeResult.warnings().size() == 1);
    REQUIRE(executeResult.warnings()[0].code == MeshingUtilities::k_NoFacesPrunedWarning);
  }

  SECTION("SurfaceNets")
  {
    DataStructure dataStructure = SurfaceMeshingTest::CreateFullyIndexedPolycrystal();
    const Result<> executeResult = RunSurfaceNetsRaw(dataStructure, BoundingBoxSkinMode::k_BackgroundBackedWallsOnly);

    SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
    REQUIRE(executeResult.warnings().size() == 1);
    REQUIRE(executeResult.warnings()[0].code == MeshingUtilities::k_NoFacesPrunedWarning);
  }

  SECTION("M3CSurfaceMeshing")
  {
    DataStructure dataStructure = SurfaceMeshingTest::CreateFullyIndexedPolycrystal();
    const Result<> executeResult = RunM3CRaw(dataStructure, BoundingBoxSkinMode::k_BackgroundBackedWallsOnly);

    SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
    REQUIRE(executeResult.warnings().size() == 1);
    REQUIRE(executeResult.warnings()[0].code == MeshingUtilities::k_NoFacesPrunedWarning);
  }
}

TEST_CASE("SimplnxCore::Bounding Box Skin warns when background is fully enclosed", "[SimplnxCore][QuickSurfaceMeshFilter][SurfaceNetsFilter][M3CSurfaceMeshingFilter]")
{
  UnitTest::LoadPlugins();

  // CreateEnclosedPorosity() is full of Feature Id 0 internally, but every wall voxel is Feature 1 --
  // the interior pocket never touches a bounding-box wall. k_NoFacesPrunedWarning's text says only
  // that no WALL face is background-backed, not that the volume has no background; this is the input
  // that would falsify the old (incorrect) wording, which claimed the latter. The option must still
  // prune zero faces and warn, and its output must be byte-identical to leaving the option off --
  // exactly the same no-op contract as the fully-indexed case above, but for a different reason.
  SECTION("QuickSurfaceMesh")
  {
    DataStructure offStructure = SurfaceMeshingTest::CreateEnclosedPorosity();
    DataStructure onStructure = SurfaceMeshingTest::CreateEnclosedPorosity();
    const Result<> offResult = RunQuickSurfaceMeshRaw(offStructure, BoundingBoxSkinMode::k_Off);
    SIMPLNX_RESULT_REQUIRE_VALID(offResult);
    const Result<> onResult = RunQuickSurfaceMeshRaw(onStructure, BoundingBoxSkinMode::k_BackgroundBackedWallsOnly);
    SIMPLNX_RESULT_REQUIRE_VALID(onResult);
    REQUIRE(onResult.warnings().size() == 1);
    REQUIRE(onResult.warnings()[0].code == MeshingUtilities::k_NoFacesPrunedWarning);

    REQUIRE_NOTHROW(offStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath));
    REQUIRE_NOTHROW(onStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath));
    const auto& offGeom = offStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath);
    const auto& onGeom = onStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath);
    REQUIRE(onGeom.getNumberOfFaces() == offGeom.getNumberOfFaces());
    REQUIRE(onGeom.getNumberOfVertices() == offGeom.getNumberOfVertices());
    UnitTest::CompareDataArrays<uint64>(*offGeom.getFaces(), *onGeom.getFaces());
    UnitTest::CompareDataArrays<float32>(*offGeom.getVertices(), *onGeom.getVertices());

    const DataPath faceLabelsPath = k_TriangleGeomPath.createChildPath("Face Data").createChildPath("FaceLabels");
    REQUIRE_NOTHROW(offStructure.getDataRefAs<Int32Array>(faceLabelsPath));
    REQUIRE_NOTHROW(onStructure.getDataRefAs<Int32Array>(faceLabelsPath));
    UnitTest::CompareDataArrays<int32>(offStructure.getDataRefAs<Int32Array>(faceLabelsPath), onStructure.getDataRefAs<Int32Array>(faceLabelsPath));

    UnitTest::CheckArraysInheritTupleDims(offStructure);
    UnitTest::CheckArraysInheritTupleDims(onStructure);
  }

  SECTION("SurfaceNets")
  {
    DataStructure offStructure = SurfaceMeshingTest::CreateEnclosedPorosity();
    DataStructure onStructure = SurfaceMeshingTest::CreateEnclosedPorosity();
    const Result<> offResult = RunSurfaceNetsRaw(offStructure, BoundingBoxSkinMode::k_Off);
    SIMPLNX_RESULT_REQUIRE_VALID(offResult);
    const Result<> onResult = RunSurfaceNetsRaw(onStructure, BoundingBoxSkinMode::k_BackgroundBackedWallsOnly);
    SIMPLNX_RESULT_REQUIRE_VALID(onResult);
    REQUIRE(onResult.warnings().size() == 1);
    REQUIRE(onResult.warnings()[0].code == MeshingUtilities::k_NoFacesPrunedWarning);

    REQUIRE_NOTHROW(offStructure.getDataRefAs<TriangleGeom>(k_SurfaceNetsTriangleGeomPath));
    REQUIRE_NOTHROW(onStructure.getDataRefAs<TriangleGeom>(k_SurfaceNetsTriangleGeomPath));
    const auto& offGeom = offStructure.getDataRefAs<TriangleGeom>(k_SurfaceNetsTriangleGeomPath);
    const auto& onGeom = onStructure.getDataRefAs<TriangleGeom>(k_SurfaceNetsTriangleGeomPath);
    REQUIRE(onGeom.getNumberOfFaces() == offGeom.getNumberOfFaces());
    REQUIRE(onGeom.getNumberOfVertices() == offGeom.getNumberOfVertices());
    UnitTest::CompareDataArrays<uint64>(*offGeom.getFaces(), *onGeom.getFaces());
    UnitTest::CompareDataArrays<float32>(*offGeom.getVertices(), *onGeom.getVertices());

    const DataPath faceLabelsPath = k_SurfaceNetsTriangleGeomPath.createChildPath("Face Data").createChildPath("FaceLabels");
    REQUIRE_NOTHROW(offStructure.getDataRefAs<Int32Array>(faceLabelsPath));
    REQUIRE_NOTHROW(onStructure.getDataRefAs<Int32Array>(faceLabelsPath));
    UnitTest::CompareDataArrays<int32>(offStructure.getDataRefAs<Int32Array>(faceLabelsPath), onStructure.getDataRefAs<Int32Array>(faceLabelsPath));

    UnitTest::CheckArraysInheritTupleDims(offStructure);
    UnitTest::CheckArraysInheritTupleDims(onStructure);
  }

  SECTION("M3CSurfaceMeshing")
  {
    DataStructure offStructure = SurfaceMeshingTest::CreateEnclosedPorosity();
    DataStructure onStructure = SurfaceMeshingTest::CreateEnclosedPorosity();
    const Result<> offResult = RunM3CRaw(offStructure, BoundingBoxSkinMode::k_Off);
    SIMPLNX_RESULT_REQUIRE_VALID(offResult);
    const Result<> onResult = RunM3CRaw(onStructure, BoundingBoxSkinMode::k_BackgroundBackedWallsOnly);
    SIMPLNX_RESULT_REQUIRE_VALID(onResult);
    REQUIRE(onResult.warnings().size() == 1);
    REQUIRE(onResult.warnings()[0].code == MeshingUtilities::k_NoFacesPrunedWarning);

    REQUIRE_NOTHROW(offStructure.getDataRefAs<TriangleGeom>(k_M3CTriangleGeomPath));
    REQUIRE_NOTHROW(onStructure.getDataRefAs<TriangleGeom>(k_M3CTriangleGeomPath));
    const auto& offGeom = offStructure.getDataRefAs<TriangleGeom>(k_M3CTriangleGeomPath);
    const auto& onGeom = onStructure.getDataRefAs<TriangleGeom>(k_M3CTriangleGeomPath);
    REQUIRE(onGeom.getNumberOfFaces() == offGeom.getNumberOfFaces());
    REQUIRE(onGeom.getNumberOfVertices() == offGeom.getNumberOfVertices());
    UnitTest::CompareDataArrays<uint64>(*offGeom.getFaces(), *onGeom.getFaces());
    UnitTest::CompareDataArrays<float32>(*offGeom.getVertices(), *onGeom.getVertices());

    const DataPath faceLabelsPath = k_M3CTriangleGeomPath.createChildPath("Face Data").createChildPath("FaceLabels");
    REQUIRE_NOTHROW(offStructure.getDataRefAs<Int32Array>(faceLabelsPath));
    REQUIRE_NOTHROW(onStructure.getDataRefAs<Int32Array>(faceLabelsPath));
    UnitTest::CompareDataArrays<int32>(offStructure.getDataRefAs<Int32Array>(faceLabelsPath), onStructure.getDataRefAs<Int32Array>(faceLabelsPath));

    UnitTest::CheckArraysInheritTupleDims(offStructure);
    UnitTest::CheckArraysInheritTupleDims(onStructure);
  }
}

TEST_CASE("SimplnxCore::Bounding Box Skin warns on an all-background volume", "[SimplnxCore][QuickSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();

  SECTION("Repair Triangle Winding off")
  {
    DataStructure dataStructure = SurfaceMeshingTest::CreateAllBackground();
    const Result<> executeResult = RunQuickSurfaceMeshRaw(dataStructure, BoundingBoxSkinMode::k_BackgroundBackedWallsOnly);

    // Success with a warning, not an error: the data is legal, just entirely background.
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
    REQUIRE(executeResult.warnings().size() == 1);
    REQUIRE(executeResult.warnings()[0].code == MeshingUtilities::k_EmptyMeshAfterSkinRemovalWarning);

    // The geometry still exists, just empty.
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath));
    const auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath);
    REQUIRE(triangleGeom.getNumberOfFaces() == 0);
    REQUIRE(triangleGeom.getNumberOfVertices() == 0);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  // Repair Triangle Winding defaults to true on all three meshers, so this is the configuration
  // shipped users actually get. It exercises the windingResult.valid() guard (QuickSurfaceMesh.cpp)
  // and proves findElementNeighbors()/RepairTriangleWinding() are safe to run on a 0-face/0-vertex
  // geometry: the contract (success, one warning, zero faces, zero vertices) must still hold.
  SECTION("Repair Triangle Winding on (shipped default)")
  {
    DataStructure dataStructure = SurfaceMeshingTest::CreateAllBackground();
    const Result<> executeResult = RunQuickSurfaceMeshRaw(dataStructure, BoundingBoxSkinMode::k_BackgroundBackedWallsOnly, /*repairWinding*/ true);

    SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
    REQUIRE(executeResult.warnings().size() == 1);
    REQUIRE(executeResult.warnings()[0].code == MeshingUtilities::k_EmptyMeshAfterSkinRemovalWarning);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath));
    const auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath);
    REQUIRE(triangleGeom.getNumberOfFaces() == 0);
    REQUIRE(triangleGeom.getNumberOfVertices() == 0);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::Bounding Box Skin warns on an all-background volume (SurfaceNets)", "[SimplnxCore][SurfaceNetsFilter]")
{
  UnitTest::LoadPlugins();

  SECTION("Repair Triangle Winding off")
  {
    DataStructure dataStructure = SurfaceMeshingTest::CreateAllBackground();
    const Result<> executeResult = RunSurfaceNetsRaw(dataStructure, BoundingBoxSkinMode::k_BackgroundBackedWallsOnly);

    SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
    REQUIRE(executeResult.warnings().size() == 1);
    REQUIRE(executeResult.warnings()[0].code == MeshingUtilities::k_EmptyMeshAfterSkinRemovalWarning);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(k_SurfaceNetsTriangleGeomPath));
    const auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(k_SurfaceNetsTriangleGeomPath);
    REQUIRE(triangleGeom.getNumberOfFaces() == 0);
    REQUIRE(triangleGeom.getNumberOfVertices() == 0);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  // See the QuickSurfaceMesh test case above for why this configuration matters: Repair Triangle
  // Winding defaults to true, and this proves the warning contract holds there too (guard at
  // SurfaceNets.cpp).
  SECTION("Repair Triangle Winding on (shipped default)")
  {
    DataStructure dataStructure = SurfaceMeshingTest::CreateAllBackground();
    const Result<> executeResult = RunSurfaceNetsRaw(dataStructure, BoundingBoxSkinMode::k_BackgroundBackedWallsOnly, /*repairWinding*/ true);

    SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
    REQUIRE(executeResult.warnings().size() == 1);
    REQUIRE(executeResult.warnings()[0].code == MeshingUtilities::k_EmptyMeshAfterSkinRemovalWarning);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(k_SurfaceNetsTriangleGeomPath));
    const auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(k_SurfaceNetsTriangleGeomPath);
    REQUIRE(triangleGeom.getNumberOfFaces() == 0);
    REQUIRE(triangleGeom.getNumberOfVertices() == 0);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::Bounding Box Skin warns on an all-background volume (M3CSurfaceMeshing)", "[SimplnxCore][M3CSurfaceMeshingFilter]")
{
  UnitTest::LoadPlugins();

  // Like QuickSurfaceMesh/SurfaceNets, M3C must reach zero vertices here: pruning every face orphans
  // every node, and the option clears the nodes its prune orphans. (M3C once also carried orphan
  // vertices of its own that survived this, issue #1706; the single ghost sentinel removed them.)

  SECTION("Repair Triangle Winding off")
  {
    DataStructure dataStructure = SurfaceMeshingTest::CreateAllBackground();
    const Result<> executeResult = RunM3CRaw(dataStructure, BoundingBoxSkinMode::k_BackgroundBackedWallsOnly);

    SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
    REQUIRE(executeResult.warnings().size() == 1);
    REQUIRE(executeResult.warnings()[0].code == MeshingUtilities::k_EmptyMeshAfterSkinRemovalWarning);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(k_M3CTriangleGeomPath));
    const auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(k_M3CTriangleGeomPath);
    REQUIRE(triangleGeom.getNumberOfFaces() == 0);
    REQUIRE(triangleGeom.getNumberOfVertices() == 0);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  // See the QuickSurfaceMesh test case above for why this configuration matters. M3C does not
  // route through the shared windingResult.valid() guard the same way, but it still must honor
  // the same warning contract when Repair Triangle Winding is left at its shipped default of true.
  SECTION("Repair Triangle Winding on (shipped default)")
  {
    DataStructure dataStructure = SurfaceMeshingTest::CreateAllBackground();
    const Result<> executeResult = RunM3CRaw(dataStructure, BoundingBoxSkinMode::k_BackgroundBackedWallsOnly, /*repairWinding*/ true);

    SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
    REQUIRE(executeResult.warnings().size() == 1);
    REQUIRE(executeResult.warnings()[0].code == MeshingUtilities::k_EmptyMeshAfterSkinRemovalWarning);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(k_M3CTriangleGeomPath));
    const auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(k_M3CTriangleGeomPath);
    REQUIRE(triangleGeom.getNumberOfFaces() == 0);
    REQUIRE(triangleGeom.getNumberOfVertices() == 0);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::Bounding Box Skin mesher inputs reject Feature Id sentinel collisions", "[SimplnxCore][QuickSurfaceMeshFilter][SurfaceNetsFilter][M3CSurfaceMeshingFilter]")
{
  UnitTest::LoadPlugins();

  // These validations are unconditional (not gated on the Bounding Box Skin mode), so each SECTION
  // below leaves the option off to prove that. This is a mitigation for the sentinel-collision
  // design described in simplnx#1705, not a fix for it.
  SECTION("QuickSurfaceMesh rejects a negative Feature Id")
  {
    DataStructure dataStructure = SurfaceMeshingTest::CreateFullyIndexedPolycrystal();
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(SurfaceMeshingTest::k_FeatureIdsPath));
    auto& featureIdsRef = dataStructure.getDataRefAs<Int32Array>(SurfaceMeshingTest::k_FeatureIdsPath).getDataStoreRef();
    featureIdsRef[0] = -7;

    const Result<> executeResult = RunQuickSurfaceMeshRaw(dataStructure, BoundingBoxSkinMode::k_Off);
    REQUIRE(executeResult.invalid());
    REQUIRE(executeResult.errors().size() == 1);
    REQUIRE(executeResult.errors()[0].code == MeshingUtilities::k_InvalidFeatureIdError);
    INFO(executeResult.errors()[0].message);
    REQUIRE(executeResult.errors()[0].message.find("-7") != std::string::npos);
    REQUIRE(executeResult.errors()[0].message.find(SurfaceMeshingTest::k_FeatureIdsPath.toString()) != std::string::npos);
  }

  SECTION("SurfaceNets rejects a negative Feature Id")
  {
    DataStructure dataStructure = SurfaceMeshingTest::CreateFullyIndexedPolycrystal();
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(SurfaceMeshingTest::k_FeatureIdsPath));
    auto& featureIdsRef = dataStructure.getDataRefAs<Int32Array>(SurfaceMeshingTest::k_FeatureIdsPath).getDataStoreRef();
    featureIdsRef[0] = -7;

    const Result<> executeResult = RunSurfaceNetsRaw(dataStructure, BoundingBoxSkinMode::k_Off);
    REQUIRE(executeResult.invalid());
    REQUIRE(executeResult.errors().size() == 1);
    REQUIRE(executeResult.errors()[0].code == MeshingUtilities::k_InvalidFeatureIdError);
    INFO(executeResult.errors()[0].message);
    REQUIRE(executeResult.errors()[0].message.find("-7") != std::string::npos);
    REQUIRE(executeResult.errors()[0].message.find(SurfaceMeshingTest::k_FeatureIdsPath.toString()) != std::string::npos);
  }

  SECTION("SurfaceNets rejects a Feature Id of INT32_MAX")
  {
    DataStructure dataStructure = SurfaceMeshingTest::CreateFullyIndexedPolycrystal();
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(SurfaceMeshingTest::k_FeatureIdsPath));
    auto& featureIdsRef = dataStructure.getDataRefAs<Int32Array>(SurfaceMeshingTest::k_FeatureIdsPath).getDataStoreRef();
    featureIdsRef[0] = std::numeric_limits<int32>::max();

    const Result<> executeResult = RunSurfaceNetsRaw(dataStructure, BoundingBoxSkinMode::k_Off);
    REQUIRE(executeResult.invalid());
    REQUIRE(executeResult.errors().size() == 1);
    REQUIRE(executeResult.errors()[0].code == MeshingUtilities::k_InvalidFeatureIdError);
    INFO(executeResult.errors()[0].message);
    REQUIRE(executeResult.errors()[0].message.find(std::to_string(std::numeric_limits<int32>::max())) != std::string::npos);
    REQUIRE(executeResult.errors()[0].message.find(SurfaceMeshingTest::k_FeatureIdsPath.toString()) != std::string::npos);
  }

  SECTION("M3CSurfaceMeshing rejects a negative Feature Id")
  {
    DataStructure dataStructure = SurfaceMeshingTest::CreateFullyIndexedPolycrystal();
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(SurfaceMeshingTest::k_FeatureIdsPath));
    auto& featureIdsRef = dataStructure.getDataRefAs<Int32Array>(SurfaceMeshingTest::k_FeatureIdsPath).getDataStoreRef();
    featureIdsRef[0] = -7;

    const Result<> executeResult = RunM3CRaw(dataStructure, BoundingBoxSkinMode::k_Off);
    REQUIRE(executeResult.invalid());
    REQUIRE(executeResult.errors().size() == 1);
    REQUIRE(executeResult.errors()[0].code == MeshingUtilities::k_InvalidFeatureIdError);
    INFO(executeResult.errors()[0].message);
    REQUIRE(executeResult.errors()[0].message.find("-7") != std::string::npos);
    REQUIRE(executeResult.errors()[0].message.find(SurfaceMeshingTest::k_FeatureIdsPath.toString()) != std::string::npos);
  }

  SECTION("M3CSurfaceMeshing rejects a Feature Id of INT32_MAX")
  {
    DataStructure dataStructure = SurfaceMeshingTest::CreateFullyIndexedPolycrystal();
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(SurfaceMeshingTest::k_FeatureIdsPath));
    auto& featureIdsRef = dataStructure.getDataRefAs<Int32Array>(SurfaceMeshingTest::k_FeatureIdsPath).getDataStoreRef();
    featureIdsRef[0] = std::numeric_limits<int32>::max();

    const Result<> executeResult = RunM3CRaw(dataStructure, BoundingBoxSkinMode::k_Off);
    REQUIRE(executeResult.invalid());
    REQUIRE(executeResult.errors().size() == 1);
    REQUIRE(executeResult.errors()[0].code == MeshingUtilities::k_InvalidFeatureIdError);
    INFO(executeResult.errors()[0].message);
    REQUIRE(executeResult.errors()[0].message.find(std::to_string(std::numeric_limits<int32>::max())) != std::string::npos);
    REQUIRE(executeResult.errors()[0].message.find(SurfaceMeshingTest::k_FeatureIdsPath.toString()) != std::string::npos);
  }
}

TEST_CASE("SimplnxCore::Bounding Box Skin: all three meshers agree on Face Label pairs", "[SimplnxCore][QuickSurfaceMeshFilter][SurfaceNetsFilter][M3CSurfaceMeshingFilter]")
{
  UnitTest::LoadPlugins();

  // The three meshers legitimately produce different geometry -- different vertex counts,
  // triangle counts, and tessellations -- so those are NOT compared here. What must agree is the
  // *semantic* result: which Feature-pair interfaces exist. CollectLabelPairs gives exactly that
  // set, independent of mesh topology, which is why it (rather than raw array/geometry equality)
  // is the right comparison across three independently-implemented meshers. CreateCylinderInBox(true)
  // (via flushWithBottom=true below) is used so both the {-1, 1} cap (cylinder flush with the box
  // floor) and the {0, 1} interior interface (cylinder side away from the floor) are present to compare,
  // alongside the {-1, 0} background skin.
  //
  // Watertightness is only asserted for the "on" mode. With the mode off, a per-voxel-face
  // boundary quad and the vertical wall quad next to it can share an edge with a THIRD quad from
  // the neighboring (differently-labeled) voxel's own boundary quad -- a pre-existing T-junction
  // at any point an internal boundary meets the box wall, independent of this option. "On" mode
  // drops that neighboring background-backed quad, leaving exactly 2 quads at the edge, which is
  // precisely the closure this option exists to provide (see the per-mesher "Option on removes
  // only the background skin and keeps the cylinder closed" cases above).
  // Mutual agreement alone would pass on an identical regression shared by all three meshers (e.g.
  // all three suddenly dropping the {0, 1} interface), so each mode is also checked against the
  // absolute set of label pairs it must contain/omit, independent of what the other two meshers did.
  auto checkAgreement = [](ChoicesParameter::ValueType boundingBoxSkinMode) {
    SurfaceMeshingTest::MeshResult qsmResult = RunQuickSurfaceMesh(true, boundingBoxSkinMode);
    SurfaceMeshingTest::MeshResult snResult = RunSurfaceNets(true, boundingBoxSkinMode);
    SurfaceMeshingTest::MeshResult m3cResult = RunM3C(true, boundingBoxSkinMode);

    const auto qsmLabelPairs = SurfaceMeshingTest::CollectLabelPairs(qsmResult);
    const auto snLabelPairs = SurfaceMeshingTest::CollectLabelPairs(snResult);
    const auto m3cLabelPairs = SurfaceMeshingTest::CollectLabelPairs(m3cResult);

    CHECK(qsmLabelPairs == snLabelPairs);
    CHECK(qsmLabelPairs == m3cLabelPairs);

    // The {0, 1} interior interface (cylinder side away from the floor) is untouched by the option
    // in either mode, so it must always be present.
    CHECK(qsmLabelPairs.count({0, 1}) == 1);
    // The cylinder's bottom cap ({-1, 1}) is the closure this option exists to preserve; it must
    // survive regardless of mode.
    CHECK(qsmLabelPairs.count({-1, 1}) == 1);
    if(boundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly)
    {
      // The background-backed box skin must be gone.
      CHECK(qsmLabelPairs.count({-1, 0}) == 0);
    }
    else
    {
      // With the option off, the artificial background skin is left in place.
      CHECK(qsmLabelPairs.count({-1, 0}) == 1);
    }

    REQUIRE_NOTHROW(qsmResult.Structure.getDataRefAs<TriangleGeom>(qsmResult.TriangleGeomPath));
    REQUIRE_NOTHROW(snResult.Structure.getDataRefAs<TriangleGeom>(snResult.TriangleGeomPath));
    REQUIRE_NOTHROW(m3cResult.Structure.getDataRefAs<TriangleGeom>(m3cResult.TriangleGeomPath));
    if(boundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly)
    {
      CHECK(SurfaceMeshingTest::IsWatertight(qsmResult.Structure.getDataRefAs<TriangleGeom>(qsmResult.TriangleGeomPath)));
      CHECK(SurfaceMeshingTest::IsWatertight(snResult.Structure.getDataRefAs<TriangleGeom>(snResult.TriangleGeomPath)));
      CHECK(SurfaceMeshingTest::IsWatertight(m3cResult.Structure.getDataRefAs<TriangleGeom>(m3cResult.TriangleGeomPath)));
    }

    UnitTest::CheckArraysInheritTupleDims(qsmResult.Structure);
    UnitTest::CheckArraysInheritTupleDims(snResult.Structure);
    UnitTest::CheckArraysInheritTupleDims(m3cResult.Structure);
  };

  SECTION("Bounding Box Skin mode off")
  {
    checkAgreement(BoundingBoxSkinMode::k_Off);
  }

  SECTION("Bounding Box Skin mode on (Background-Backed Walls Only)")
  {
    checkAgreement(BoundingBoxSkinMode::k_BackgroundBackedWallsOnly);
  }
}

namespace
{
// Builds a 12x12x12 ImageGeom where a single Feature (Id 1) occupies a corner block touching the
// x==0, y==0, AND z==0 walls simultaneously, so three suppressed wall planes meet at a right-angle
// corner. CreateCylinderInBox only ever touches one wall (the floor) at a time, so this is built
// directly rather than forcing that helper into a shape it was not designed for.
DataStructure CreateCornerFeatureInBox()
{
  DataStructure dataStructure;
  constexpr usize k_CornerExtent = 3;

  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "ImageGeom");
  const std::vector<usize> dims = {SurfaceMeshingTest::k_BoxDim, SurfaceMeshingTest::k_BoxDim, SurfaceMeshingTest::k_BoxDim};
  imageGeomPtr->setDimensions(dims);
  imageGeomPtr->setSpacing({1.0F, 1.0F, 1.0F});
  imageGeomPtr->setOrigin({0.0F, 0.0F, 0.0F});

  auto* cellAMPtr = AttributeMatrix::Create(dataStructure, "CellData", {dims[2], dims[1], dims[0]}, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAMPtr);
  auto* featureIdsPtr = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "FeatureIds", {dims[2], dims[1], dims[0]}, {1}, cellAMPtr->getId());
  featureIdsPtr->fill(0);

  auto& featureIdsRef = featureIdsPtr->getDataStoreRef();
  for(usize z = 0; z < k_CornerExtent; z++)
  {
    for(usize y = 0; y < k_CornerExtent; y++)
    {
      for(usize x = 0; x < k_CornerExtent; x++)
      {
        featureIdsRef[(z * SurfaceMeshingTest::k_BoxDim * SurfaceMeshingTest::k_BoxDim) + (y * SurfaceMeshingTest::k_BoxDim) + x] = 1;
      }
    }
  }

  return dataStructure;
}
} // namespace

TEST_CASE("SimplnxCore::Bounding Box Skin: corner Feature stays watertight", "[SimplnxCore][QuickSurfaceMeshFilter][SurfaceNetsFilter][M3CSurfaceMeshingFilter]")
{
  UnitTest::LoadPlugins();

  // The feature's whole justification is that a Feature flush with the box stays closed. Every
  // other test in this file only exercises a single flush face (CreateCylinderInBox's floor). A
  // per-face suppression rule could still fail where two or three suppressed wall planes meet at a
  // right angle -- which only happens at a corner -- so this is the test that would catch it.
  SECTION("QuickSurfaceMesh")
  {
    SurfaceMeshingTest::MeshResult meshResult =
        SurfaceMeshingTest::RunMesher<QuickSurfaceMeshFilter>(CreateCornerFeatureInBox(), k_TriangleGeomPath, BoundingBoxSkinMode::k_BackgroundBackedWallsOnly,
                                                              [](Arguments& args) { args.insertOrAssign(QuickSurfaceMeshFilter::k_FixProblemVoxels_Key, std::make_any<bool>(false)); });
    const auto labelPairs = SurfaceMeshingTest::CollectLabelPairs(meshResult);
    CHECK(labelPairs.count({-1, 1}) == 1);
    CHECK(labelPairs.count({0, 1}) == 1);
    CHECK(labelPairs.count({-1, 0}) == 0);

    REQUIRE_NOTHROW(meshResult.Structure.getDataRefAs<TriangleGeom>(meshResult.TriangleGeomPath));
    const auto& triangleGeom = meshResult.Structure.getDataRefAs<TriangleGeom>(meshResult.TriangleGeomPath);
    const auto counts = SurfaceMeshingTest::CountEdgeUses(triangleGeom);
    INFO("QuickSurfaceMesh corner-Feature edge use counts -- Total: " << counts.TotalEdges << " UsedOnce: " << counts.EdgesUsedOnce << " UsedTwice: " << counts.EdgesUsedTwice
                                                                      << " UsedMoreThanTwice: " << counts.EdgesUsedMoreThanTwice);
    REQUIRE(SurfaceMeshingTest::IsWatertight(triangleGeom));

    UnitTest::CheckArraysInheritTupleDims(meshResult.Structure);
  }

  SECTION("SurfaceNets")
  {
    SurfaceMeshingTest::MeshResult meshResult =
        SurfaceMeshingTest::RunMesher<SurfaceNetsFilter>(CreateCornerFeatureInBox(), k_SurfaceNetsTriangleGeomPath, BoundingBoxSkinMode::k_BackgroundBackedWallsOnly, [](Arguments& args) {
          args.insertOrAssign(SurfaceNetsFilter::k_ApplySmoothing_Key, std::make_any<bool>(false));
          args.insertOrAssign(SurfaceNetsFilter::k_SmoothingIterations_Key, std::make_any<int32>(20));
          args.insertOrAssign(SurfaceNetsFilter::k_MaxDistanceFromVoxelCenter_Key, std::make_any<float32>(1.0F));
          args.insertOrAssign(SurfaceNetsFilter::k_RelaxationFactor_Key, std::make_any<float32>(0.5F));
        });
    const auto labelPairs = SurfaceMeshingTest::CollectLabelPairs(meshResult);
    CHECK(labelPairs.count({-1, 1}) == 1);
    CHECK(labelPairs.count({0, 1}) == 1);
    CHECK(labelPairs.count({-1, 0}) == 0);

    REQUIRE_NOTHROW(meshResult.Structure.getDataRefAs<TriangleGeom>(meshResult.TriangleGeomPath));
    const auto& triangleGeom = meshResult.Structure.getDataRefAs<TriangleGeom>(meshResult.TriangleGeomPath);
    const auto counts = SurfaceMeshingTest::CountEdgeUses(triangleGeom);
    INFO("SurfaceNets corner-Feature edge use counts -- Total: " << counts.TotalEdges << " UsedOnce: " << counts.EdgesUsedOnce << " UsedTwice: " << counts.EdgesUsedTwice
                                                                 << " UsedMoreThanTwice: " << counts.EdgesUsedMoreThanTwice);
    REQUIRE(SurfaceMeshingTest::IsWatertight(triangleGeom));

    UnitTest::CheckArraysInheritTupleDims(meshResult.Structure);
  }

  SECTION("M3CSurfaceMeshing")
  {
    SurfaceMeshingTest::MeshResult meshResult =
        SurfaceMeshingTest::RunMesher<M3CSurfaceMeshingFilter>(CreateCornerFeatureInBox(), k_M3CTriangleGeomPath, BoundingBoxSkinMode::k_BackgroundBackedWallsOnly, [](Arguments&) {});
    const auto labelPairs = SurfaceMeshingTest::CollectLabelPairs(meshResult);
    CHECK(labelPairs.count({-1, 1}) == 1);
    CHECK(labelPairs.count({0, 1}) == 1);
    CHECK(labelPairs.count({-1, 0}) == 0);

    REQUIRE_NOTHROW(meshResult.Structure.getDataRefAs<TriangleGeom>(meshResult.TriangleGeomPath));
    const auto& triangleGeom = meshResult.Structure.getDataRefAs<TriangleGeom>(meshResult.TriangleGeomPath);
    const auto counts = SurfaceMeshingTest::CountEdgeUses(triangleGeom);
    INFO("M3CSurfaceMeshing corner-Feature edge use counts -- Total: " << counts.TotalEdges << " UsedOnce: " << counts.EdgesUsedOnce << " UsedTwice: " << counts.EdgesUsedTwice
                                                                       << " UsedMoreThanTwice: " << counts.EdgesUsedMoreThanTwice);
    REQUIRE(SurfaceMeshingTest::IsWatertight(triangleGeom));

    UnitTest::CheckArraysInheritTupleDims(meshResult.Structure);
  }
}
