#include "SimplnxCore/Filters/SurfaceNetsFilter.hpp"
#include "SimplnxCore/SurfaceNets/MMCellMap.h"
#include "SurfaceMeshingTestUtils.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/Meshing/TriangleUtilities.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <set>
#include <utility>

using namespace nx::core;

namespace
{
const DataPath k_TriangleGeomPath({"SurfaceNets"});

void CheckCellMapEndpoints(MMCellFlag::Edge edge, std::array<usize, 3> dimensions, const std::array<int32, 3>& cellPosition, bool requireRealStores)
{
  DataStructure dataStructure;
  const DataPath labelsPath({"Labels"});
  const DataPath verticesPath({"Vertices"});
  const ShapeType tupleShape = {dimensions[2], dimensions[1], dimensions[0]};
  auto* labelsPtr = Int32Array::Create(dataStructure, "Labels", DataStoreUtilities::CreateDataStore<int32>(dataStructure, labelsPath, tupleShape, {1}));
  auto* verticesPtr = Float32Array::Create(dataStructure, "Vertices", DataStoreUtilities::CreateDataStore<float32>(dataStructure, verticesPath, {1}, {3}));
  REQUIRE(labelsPtr != nullptr);
  REQUIRE(verticesPtr != nullptr);
  if(requireRealStores)
  {
    REQUIRE(labelsPtr->getDataStoreRef().getDataFormat() == "HDF5-OOC");
    REQUIRE(verticesPtr->getDataStoreRef().getDataFormat() == "HDF5-OOC");
  }
  const std::array<int32, 2> labels = {11, 13};
  auto writeResult = labelsPtr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(labels));
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  const std::array<float32, 3> voxelSizes = {1, 1, 1};
  MMCellMap cellMap(verticesPtr->getDataStoreRef(), labelsPtr, dimensions.data(), voxelSizes.data());
  REQUIRE(cellMap.valid());
  auto initResult = cellMap.init();
  SIMPLNX_RESULT_REQUIRE_VALID(initResult);
  const auto* cellPtr = cellMap.getCell(cellPosition[0], cellPosition[1], cellPosition[2]);
  REQUIRE(cellPtr != nullptr);
  REQUIRE(cellPtr->vertexIndex < cellMap.numVertices());

  const std::array<int32, 2> expectedLabels = {11, 13};
  const std::array<usize, 2> expectedIndices = {0, 1};
  SECTION("coordinate overload")
  {
    std::array<float32, 12> corners = {};
    std::array<int32, 2> actualLabels = {};
    std::array<usize, 2> actualIndices = {};
    REQUIRE(cellMap.getEdgeQuad(cellPtr->vertexIndex, edge, corners.data(), actualLabels.data(), actualIndices.data()));
    CHECK(actualLabels == expectedLabels);
    CHECK(actualIndices == expectedIndices);
  }
  SECTION("vertex-index overload")
  {
    std::array<usize, 4> vertexIndices = {};
    std::array<int32, 2> actualLabels = {};
    std::array<usize, 2> actualIndices = {};
    const MMCellMap& cellMapRef = cellMap;
    REQUIRE(cellMapRef.getEdgeQuad(cellPtr->vertexIndex, edge, vertexIndices.data(), actualLabels.data(), actualIndices.data()));
    CHECK(actualLabels == expectedLabels);
    CHECK(actualIndices == expectedIndices);
  }
}

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

TEST_CASE("SimplnxCore::MMCellMap: BackTopEdge endpoints", "[SimplnxCore][MMCellMap]")
{
  CheckCellMapEndpoints(MMCellFlag::Edge::BackTopEdge, {2, 1, 1}, {1, 1, 0}, false);
}

TEST_CASE("SimplnxCore::MMCellMap: RightBackEdge endpoints", "[SimplnxCore][MMCellMap]")
{
  CheckCellMapEndpoints(MMCellFlag::Edge::RightBackEdge, {1, 1, 2}, {0, 1, 1}, false);
}

TEST_CASE("SimplnxCore::MMCellMap: real-store endpoints", "[SimplnxCore][.OocStoreContract]")
{
  REQUIRE(Application::GetOrCreateInstance()->getIOManager("HDF5-OOC") != nullptr);
  const UnitTest::PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);
  SECTION("BackTopEdge")
  {
    CheckCellMapEndpoints(MMCellFlag::Edge::BackTopEdge, {2, 1, 1}, {1, 1, 0}, true);
  }
  SECTION("RightBackEdge")
  {
    CheckCellMapEndpoints(MMCellFlag::Edge::RightBackEdge, {1, 1, 2}, {0, 1, 1}, true);
  }
}

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
