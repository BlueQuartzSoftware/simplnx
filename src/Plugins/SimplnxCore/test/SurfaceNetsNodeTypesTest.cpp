#include "SimplnxCore/Filters/QuickSurfaceMeshFilter.hpp"
#include "SimplnxCore/Filters/SurfaceNetsFilter.hpp"
#include "SurfaceMeshingTestUtils.hpp"

#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <map>
#include <set>
#include <sstream>
#include <tuple>

using namespace nx::core;

namespace
{
const DataPath k_SurfaceNetsTriangleGeomPath({"SurfaceNets"});
const DataPath k_QuickMeshTriangleGeomPath({"QuickMesh"});

// Runs SurfaceNets on the flush cylinder, using the shared mesher-agnostic RunMesher helper (see
// SurfaceMeshingTestUtils.hpp) so the argument list is defined in one place instead of duplicated
// per test file.
SurfaceMeshingTest::MeshResult RunSurfaceNetsForNodeTypes()
{
  return SurfaceMeshingTest::RunMesher<SurfaceNetsFilter>(SurfaceMeshingTest::CreateCylinderInBox(true), k_SurfaceNetsTriangleGeomPath, false, [](Arguments& args) {
    args.insertOrAssign(SurfaceNetsFilter::k_ApplySmoothing_Key, std::make_any<bool>(false));
    args.insertOrAssign(SurfaceNetsFilter::k_SmoothingIterations_Key, std::make_any<int32>(20));
    args.insertOrAssign(SurfaceNetsFilter::k_MaxDistanceFromVoxelCenter_Key, std::make_any<float32>(1.0F));
    args.insertOrAssign(SurfaceNetsFilter::k_RelaxationFactor_Key, std::make_any<float32>(0.5F));
  });
}

// Returns the set of distinct Node Type values present in meshResult's Vertex Data/NodeTypes array.
std::set<int8> CollectNodeTypeValues(const SurfaceMeshingTest::MeshResult& meshResult)
{
  const DataPath nodeTypesPath = meshResult.TriangleGeomPath.createChildPath("Vertex Data").createChildPath("NodeTypes");
  REQUIRE_NOTHROW(meshResult.Structure.getDataRefAs<Int8Array>(nodeTypesPath));
  const auto& nodeTypesRef = meshResult.Structure.getDataRefAs<Int8Array>(nodeTypesPath).getDataStoreRef();

  std::set<int8> distinctValues;
  for(usize i = 0; i < nodeTypesRef.getNumberOfTuples(); i++)
  {
    distinctValues.insert(nodeTypesRef[i]);
  }
  return distinctValues;
}
} // namespace

TEST_CASE("SimplnxCore::SurfaceNetsFilter: Node Types follow the NodeType convention", "[SimplnxCore][SurfaceNetsFilter]")
{
  UnitTest::LoadPlugins();

  SurfaceMeshingTest::MeshResult meshResult = RunSurfaceNetsForNodeTypes();
  const std::set<int8> distinctValues = CollectNodeTypeValues(meshResult);

  // Only the shared convention values may appear: 2/3/4 interior, 12/13/14 on the box wall.
  const std::set<int8> allowedValues = {NodeType::Default, NodeType::TriplePoint, NodeType::QuadPoint, NodeType::SurfaceDefault, NodeType::SurfaceTriplePoint, NodeType::SurfaceQuadPoint};
  for(const int8 value : distinctValues)
  {
    INFO("Unexpected Node Type value: " << static_cast<int32>(value));
    REQUIRE(allowedValues.count(value) == 1);
  }

  // The flush cylinder guarantees both an interior node and a box-wall node exist.
  REQUIRE(distinctValues.count(NodeType::Default) == 1);
  REQUIRE(distinctValues.count(NodeType::SurfaceDefault) == 1);

  // 0 must never appear -- it means NodeType::Unused.
  REQUIRE(distinctValues.count(0) == 0);

  UnitTest::CheckArraysInheritTupleDims(meshResult.Structure);
}

TEST_CASE("SimplnxCore::SurfaceNetsFilter: Node Types agree with QuickSurfaceMesh", "[SimplnxCore][SurfaceNetsFilter]")
{
  UnitTest::LoadPlugins();

  // Both meshers place a node near each grid corner where two or more Features meet, and
  // both derive the node type from the same 8 surrounding voxels. With smoothing disabled
  // the values must agree corner-for-corner. This is the check that proves the equivalence
  // argument in the design spec rather than trusting it.
  SurfaceMeshingTest::MeshResult surfaceNetsResult = RunSurfaceNetsForNodeTypes();
  SurfaceMeshingTest::MeshResult quickMeshResult =
      SurfaceMeshingTest::RunMesher<QuickSurfaceMeshFilter>(SurfaceMeshingTest::CreateCylinderInBox(true), k_QuickMeshTriangleGeomPath, false,
                                                            [](Arguments& args) { args.insertOrAssign(QuickSurfaceMeshFilter::k_FixProblemVoxels_Key, std::make_any<bool>(false)); });

  // Both meshers should produce the same number of nodes, since both emit one per grid
  // corner where two or more Features meet.
  REQUIRE_NOTHROW(surfaceNetsResult.Structure.getDataRefAs<TriangleGeom>(k_SurfaceNetsTriangleGeomPath));
  REQUIRE_NOTHROW(quickMeshResult.Structure.getDataRefAs<TriangleGeom>(k_QuickMeshTriangleGeomPath));
  const auto& snGeom = surfaceNetsResult.Structure.getDataRefAs<TriangleGeom>(k_SurfaceNetsTriangleGeomPath);
  const auto& qsmGeom = quickMeshResult.Structure.getDataRefAs<TriangleGeom>(k_QuickMeshTriangleGeomPath);
  REQUIRE(snGeom.getNumberOfVertices() == qsmGeom.getNumberOfVertices());

  const auto& snVertsRef = snGeom.getVertices()->getDataStoreRef();
  const auto& qsmVertsRef = qsmGeom.getVertices()->getDataStoreRef();
  const DataPath snNodeTypesPath = k_SurfaceNetsTriangleGeomPath.createChildPath("Vertex Data").createChildPath("NodeTypes");
  const DataPath qsmNodeTypesPath = k_QuickMeshTriangleGeomPath.createChildPath("Vertex Data").createChildPath("NodeTypes");
  REQUIRE_NOTHROW(surfaceNetsResult.Structure.getDataRefAs<Int8Array>(snNodeTypesPath));
  REQUIRE_NOTHROW(quickMeshResult.Structure.getDataRefAs<Int8Array>(qsmNodeTypesPath));
  const auto& snTypesRef = surfaceNetsResult.Structure.getDataRefAs<Int8Array>(snNodeTypesPath).getDataStoreRef();
  const auto& qsmTypesRef = quickMeshResult.Structure.getDataRefAs<Int8Array>(qsmNodeTypesPath).getDataStoreRef();

  // Exact, corner-for-corner comparison: the two meshers number their nodes in different orders
  // (a histogram comparison alone would pass under a permutation of values across corners), so key
  // QuickSurfaceMesh's vertices by coordinate and require every SurfaceNets vertex at the same
  // coordinate to carry the identical Node Type value. This is the check that proves the
  // equivalence argument in the design spec rather than trusting it.
  std::map<std::tuple<float32, float32, float32>, int8> qsmTypeByCoord;
  for(usize i = 0; i < qsmTypesRef.getNumberOfTuples(); i++)
  {
    qsmTypeByCoord[{qsmVertsRef[i * 3], qsmVertsRef[i * 3 + 1], qsmVertsRef[i * 3 + 2]}] = qsmTypesRef[i];
  }

  usize mismatchCount = 0;
  std::ostringstream firstMismatch;
  for(usize i = 0; i < snTypesRef.getNumberOfTuples(); i++)
  {
    const std::tuple<float32, float32, float32> coord = {snVertsRef[i * 3], snVertsRef[i * 3 + 1], snVertsRef[i * 3 + 2]};
    REQUIRE(qsmTypeByCoord.count(coord) == 1);
    if(qsmTypeByCoord[coord] != snTypesRef[i])
    {
      if(mismatchCount == 0)
      {
        firstMismatch << "(" << std::get<0>(coord) << ", " << std::get<1>(coord) << ", " << std::get<2>(coord) << "): QuickSurfaceMesh=" << static_cast<int32>(qsmTypeByCoord[coord])
                      << " SurfaceNets=" << static_cast<int32>(snTypesRef[i]);
      }
      mismatchCount++;
    }
  }
  INFO("Total mismatching vertices: " << mismatchCount << " / " << snTypesRef.getNumberOfTuples() << (mismatchCount > 0 ? (" -- first mismatch at " + firstMismatch.str()) : std::string{}));
  REQUIRE(mismatchCount == 0);

  UnitTest::CheckArraysInheritTupleDims(surfaceNetsResult.Structure);
  UnitTest::CheckArraysInheritTupleDims(quickMeshResult.Structure);
}
