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
const DataPath k_ImageGeomPath({"ImageGeom"});
const DataPath k_FeatureIdsPath({"ImageGeom", "CellData", "FeatureIds"});

// Returns the set of distinct Node Type values produced by SurfaceNets on the flush cylinder.
std::set<int8> RunSurfaceNetsNodeTypes()
{
  DataStructure dataStructure = SurfaceMeshingTest::CreateCylinderInBox(true);
  const DataPath triangleGeomPath({"SurfaceNets"});

  SurfaceNetsFilter filter;
  Arguments args;
  args.insertOrAssign(SurfaceNetsFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
  args.insertOrAssign(SurfaceNetsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
  args.insertOrAssign(SurfaceNetsFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType()));
  args.insertOrAssign(SurfaceNetsFilter::k_SelectedFeatureDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType()));
  args.insertOrAssign(SurfaceNetsFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(triangleGeomPath));
  args.insertOrAssign(SurfaceNetsFilter::k_VertexDataGroupName_Key, std::make_any<std::string>("Vertex Data"));
  args.insertOrAssign(SurfaceNetsFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>("NodeTypes"));
  args.insertOrAssign(SurfaceNetsFilter::k_FaceDataGroupName_Key, std::make_any<std::string>("Face Data"));
  args.insertOrAssign(SurfaceNetsFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>("FaceLabels"));
  args.insertOrAssign(SurfaceNetsFilter::k_ApplySmoothing_Key, std::make_any<bool>(false));
  args.insertOrAssign(SurfaceNetsFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(false));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath nodeTypesPath = triangleGeomPath.createChildPath("Vertex Data").createChildPath("NodeTypes");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int8Array>(nodeTypesPath));
  const auto& nodeTypesRef = dataStructure.getDataRefAs<Int8Array>(nodeTypesPath).getDataStoreRef();

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

  const std::set<int8> distinctValues = RunSurfaceNetsNodeTypes();

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
}

TEST_CASE("SimplnxCore::SurfaceNetsFilter: Node Types agree with QuickSurfaceMesh", "[SimplnxCore][SurfaceNetsFilter]")
{
  UnitTest::LoadPlugins();

  // Both meshers place a node near each grid corner where two or more Features meet, and
  // both derive the node type from the same 8 surrounding voxels. With smoothing disabled
  // the values must agree corner-for-corner. This is the check that proves the equivalence
  // argument in the design spec rather than trusting it.
  DataStructure surfaceNetsStructure = SurfaceMeshingTest::CreateCylinderInBox(true);
  DataStructure quickMeshStructure = SurfaceMeshingTest::CreateCylinderInBox(true);

  const DataPath surfaceNetsPath({"SurfaceNets"});
  const DataPath quickMeshPath({"QuickMesh"});

  {
    SurfaceNetsFilter filter;
    Arguments args;
    args.insertOrAssign(SurfaceNetsFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
    args.insertOrAssign(SurfaceNetsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
    args.insertOrAssign(SurfaceNetsFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType()));
    args.insertOrAssign(SurfaceNetsFilter::k_SelectedFeatureDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType()));
    args.insertOrAssign(SurfaceNetsFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(surfaceNetsPath));
    args.insertOrAssign(SurfaceNetsFilter::k_VertexDataGroupName_Key, std::make_any<std::string>("Vertex Data"));
    args.insertOrAssign(SurfaceNetsFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>("NodeTypes"));
    args.insertOrAssign(SurfaceNetsFilter::k_FaceDataGroupName_Key, std::make_any<std::string>("Face Data"));
    args.insertOrAssign(SurfaceNetsFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>("FaceLabels"));
    args.insertOrAssign(SurfaceNetsFilter::k_ApplySmoothing_Key, std::make_any<bool>(false));
    args.insertOrAssign(SurfaceNetsFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(false));

    auto preflightResult = filter.preflight(surfaceNetsStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(surfaceNetsStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    QuickSurfaceMeshFilter filter;
    Arguments args;
    args.insertOrAssign(QuickSurfaceMeshFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(k_ImageGeomPath));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType()));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_SelectedFeatureDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType()));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(quickMeshPath));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_VertexDataGroupName_Key, std::make_any<std::string>("Vertex Data"));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>("NodeTypes"));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_FaceDataGroupName_Key, std::make_any<std::string>("Face Data"));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>("FaceLabels"));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_FixProblemVoxels_Key, std::make_any<bool>(false));
    args.insertOrAssign(QuickSurfaceMeshFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(false));

    auto preflightResult = filter.preflight(quickMeshStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(quickMeshStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Both meshers should produce the same number of nodes, since both emit one per grid
  // corner where two or more Features meet.
  const auto& snGeom = surfaceNetsStructure.getDataRefAs<TriangleGeom>(surfaceNetsPath);
  const auto& qsmGeom = quickMeshStructure.getDataRefAs<TriangleGeom>(quickMeshPath);
  REQUIRE(snGeom.getNumberOfVertices() == qsmGeom.getNumberOfVertices());

  const auto& snVertsRef = snGeom.getVertices()->getDataStoreRef();
  const auto& qsmVertsRef = qsmGeom.getVertices()->getDataStoreRef();
  const auto& snTypesRef = surfaceNetsStructure.getDataRefAs<Int8Array>(surfaceNetsPath.createChildPath("Vertex Data").createChildPath("NodeTypes")).getDataStoreRef();
  const auto& qsmTypesRef = quickMeshStructure.getDataRefAs<Int8Array>(quickMeshPath.createChildPath("Vertex Data").createChildPath("NodeTypes")).getDataStoreRef();

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
}
