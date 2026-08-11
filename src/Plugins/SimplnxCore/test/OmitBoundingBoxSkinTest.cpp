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

namespace
{
// Raw (unchecked-result) runners built on the shared RunMesherRaw helper, reusing the same
// Triangle Geometry paths and mesher-specific extra args as the RunQuickSurfaceMesh /
// RunSurfaceNets / RunM3C wrappers above, so degenerate cases can inspect the execute Result<>
// (e.g. warnings) instead of asserting validity and aborting.
Result<> RunQuickSurfaceMeshRaw(DataStructure& dataStructure, bool omitSkin, bool repairWinding = false)
{
  return SurfaceMeshingTest::RunMesherRaw<QuickSurfaceMeshFilter>(
      dataStructure, k_TriangleGeomPath, omitSkin, [](Arguments& args) { args.insertOrAssign(QuickSurfaceMeshFilter::k_FixProblemVoxels_Key, std::make_any<bool>(false)); }, repairWinding);
}

Result<> RunSurfaceNetsRaw(DataStructure& dataStructure, bool omitSkin, bool repairWinding = false)
{
  return SurfaceMeshingTest::RunMesherRaw<SurfaceNetsFilter>(
      dataStructure, k_SurfaceNetsTriangleGeomPath, omitSkin,
      [](Arguments& args) {
        args.insertOrAssign(SurfaceNetsFilter::k_ApplySmoothing_Key, std::make_any<bool>(false));
        args.insertOrAssign(SurfaceNetsFilter::k_SmoothingIterations_Key, std::make_any<int32>(20));
        args.insertOrAssign(SurfaceNetsFilter::k_MaxDistanceFromVoxelCenter_Key, std::make_any<float32>(1.0F));
        args.insertOrAssign(SurfaceNetsFilter::k_RelaxationFactor_Key, std::make_any<float32>(0.5F));
      },
      repairWinding);
}

Result<> RunM3CRaw(DataStructure& dataStructure, bool omitSkin, bool repairWinding = false)
{
  return SurfaceMeshingTest::RunMesherRaw<M3CSurfaceMeshingFilter>(
      dataStructure, k_M3CTriangleGeomPath, omitSkin, [](Arguments&) {}, repairWinding);
}
} // namespace

TEST_CASE("SimplnxCore::Omit Bounding Box Skin is a no-op without background", "[SimplnxCore][QuickSurfaceMeshFilter][SurfaceNetsFilter][M3CSurfaceMeshingFilter]")
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
    const Result<> offResult = RunQuickSurfaceMeshRaw(offStructure, false);
    SIMPLNX_RESULT_REQUIRE_VALID(offResult);
    const Result<> onResult = RunQuickSurfaceMeshRaw(onStructure, true);
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

    // Node Types are untouched by M3C's orphan-cleanup discontinuity, but this is the only no-op
    // test that checks all three meshers, so it is the natural place to cover the array for all of
    // them rather than just Face Labels/Faces/Vertices.
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
    const Result<> offResult = RunSurfaceNetsRaw(offStructure, false);
    SIMPLNX_RESULT_REQUIRE_VALID(offResult);
    const Result<> onResult = RunSurfaceNetsRaw(onStructure, true);
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
    const Result<> offResult = RunM3CRaw(offStructure, false);
    SIMPLNX_RESULT_REQUIRE_VALID(offResult);
    const Result<> onResult = RunM3CRaw(onStructure, true);
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

    // M3C's orphan-cleanup is the one code path in the whole feature that has a discontinuity
    // (see M3CSurfaceMeshing.cpp:2566): it is gated on whether the prune actually dropped anything,
    // not on the option flag alone. This fully-indexed input drops nothing, so Node Types must
    // still match exactly here.
    const DataPath nodeTypesPath = k_M3CTriangleGeomPath.createChildPath("Vertex Data").createChildPath("NodeTypes");
    REQUIRE_NOTHROW(offStructure.getDataRefAs<Int8Array>(nodeTypesPath));
    REQUIRE_NOTHROW(onStructure.getDataRefAs<Int8Array>(nodeTypesPath));
    UnitTest::CompareDataArrays<int8>(offStructure.getDataRefAs<Int8Array>(nodeTypesPath), onStructure.getDataRefAs<Int8Array>(nodeTypesPath));

    UnitTest::CheckArraysInheritTupleDims(offStructure);
    UnitTest::CheckArraysInheritTupleDims(onStructure);
  }
}

TEST_CASE("SimplnxCore::Omit Bounding Box Skin warns on an all-background volume", "[SimplnxCore][QuickSurfaceMeshFilter]")
{
  UnitTest::LoadPlugins();

  SECTION("Repair Triangle Winding off")
  {
    DataStructure dataStructure = SurfaceMeshingTest::CreateAllBackground();
    const Result<> executeResult = RunQuickSurfaceMeshRaw(dataStructure, true);

    // Success with a warning, not an error: the data is legal, just entirely background.
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
    REQUIRE(executeResult.warnings().size() == 1);
    REQUIRE(executeResult.warnings()[0].code == -56340);

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
    const Result<> executeResult = RunQuickSurfaceMeshRaw(dataStructure, true, /*repairWinding*/ true);

    SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
    REQUIRE(executeResult.warnings().size() == 1);
    REQUIRE(executeResult.warnings()[0].code == -56340);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath));
    const auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(k_TriangleGeomPath);
    REQUIRE(triangleGeom.getNumberOfFaces() == 0);
    REQUIRE(triangleGeom.getNumberOfVertices() == 0);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::Omit Bounding Box Skin warns on an all-background volume (SurfaceNets)", "[SimplnxCore][SurfaceNetsFilter]")
{
  UnitTest::LoadPlugins();

  SECTION("Repair Triangle Winding off")
  {
    DataStructure dataStructure = SurfaceMeshingTest::CreateAllBackground();
    const Result<> executeResult = RunSurfaceNetsRaw(dataStructure, true);

    SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
    REQUIRE(executeResult.warnings().size() == 1);
    REQUIRE(executeResult.warnings()[0].code == -56340);

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
    const Result<> executeResult = RunSurfaceNetsRaw(dataStructure, true, /*repairWinding*/ true);

    SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
    REQUIRE(executeResult.warnings().size() == 1);
    REQUIRE(executeResult.warnings()[0].code == -56340);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(k_SurfaceNetsTriangleGeomPath));
    const auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(k_SurfaceNetsTriangleGeomPath);
    REQUIRE(triangleGeom.getNumberOfFaces() == 0);
    REQUIRE(triangleGeom.getNumberOfVertices() == 0);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::Omit Bounding Box Skin warns on an all-background volume (M3CSurfaceMeshing)", "[SimplnxCore][M3CSurfaceMeshingFilter]")
{
  UnitTest::LoadPlugins();

  SECTION("Repair Triangle Winding off")
  {
    DataStructure dataStructure = SurfaceMeshingTest::CreateAllBackground();
    const Result<> executeResult = RunM3CRaw(dataStructure, true);

    SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
    REQUIRE(executeResult.warnings().size() == 1);
    REQUIRE(executeResult.warnings()[0].code == -56340);

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
    const Result<> executeResult = RunM3CRaw(dataStructure, true, /*repairWinding*/ true);

    SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
    REQUIRE(executeResult.warnings().size() == 1);
    REQUIRE(executeResult.warnings()[0].code == -56340);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(k_M3CTriangleGeomPath));
    const auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(k_M3CTriangleGeomPath);
    REQUIRE(triangleGeom.getNumberOfFaces() == 0);
    REQUIRE(triangleGeom.getNumberOfVertices() == 0);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}
