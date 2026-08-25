#include "simplnx/Utilities/Meshing/TripleLineUtilities.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using namespace nx::core;

namespace
{
const std::string k_TriangleGeomName = "TriangleGeom";
const std::string k_FaceLabelsName = "FaceLabels";
const std::string k_TripleLineGeomName = "TripleLines";
const std::string k_NumFeaturesName = "NumFeatures";
const std::string k_NodeTypesName = "NodeTypes";

/**
 * @brief Builds a TriangleGeom plus its FaceLabels array from explicit vertex, triangle and
 * label lists. The FaceLabels array is a child of the geometry.
 * @param dataStructure Receives the geometry and its arrays.
 * @param vertices Source vertex coordinates.
 * @param triangles Source vertex indices for each triangle.
 * @param faceLabels Feature Id pair for each triangle.
 * @return Created source geometry.
 */
TriangleGeom* CreateTriangleMesh(DataStructure& dataStructure, const std::vector<std::array<float32, 3>>& vertices, const std::vector<std::array<usize, 3>>& triangles,
                                 const std::vector<std::array<int32, 2>>& faceLabels)
{
  REQUIRE(triangles.size() == faceLabels.size());

  auto* triangleGeomPtr = TriangleGeom::Create(dataStructure, k_TriangleGeomName);
  REQUIRE(triangleGeomPtr != nullptr);

  auto vertexStore = std::make_unique<DataStore<float32>>(std::vector<usize>{vertices.size()}, std::vector<usize>{3}, 0.0f);
  auto* vertexArrayPtr = IGeometry::SharedVertexList::Create(dataStructure, "SharedVertexList", std::move(vertexStore), triangleGeomPtr->getId());
  REQUIRE(vertexArrayPtr != nullptr);
  auto& verticesRef = vertexArrayPtr->getDataStoreRef();
  for(usize i = 0; i < vertices.size(); i++)
  {
    verticesRef[(i * 3) + 0] = vertices[i][0];
    verticesRef[(i * 3) + 1] = vertices[i][1];
    verticesRef[(i * 3) + 2] = vertices[i][2];
  }
  triangleGeomPtr->setVertices(*vertexArrayPtr);

  auto faceStore = std::make_unique<DataStore<IGeometry::MeshIndexType>>(std::vector<usize>{triangles.size()}, std::vector<usize>{3}, 0);
  auto* faceArrayPtr = IGeometry::SharedFaceList::Create(dataStructure, "SharedTriList", std::move(faceStore), triangleGeomPtr->getId());
  REQUIRE(faceArrayPtr != nullptr);
  auto& facesRef = faceArrayPtr->getDataStoreRef();
  for(usize i = 0; i < triangles.size(); i++)
  {
    facesRef[(i * 3) + 0] = triangles[i][0];
    facesRef[(i * 3) + 1] = triangles[i][1];
    facesRef[(i * 3) + 2] = triangles[i][2];
  }
  triangleGeomPtr->setFaceList(*faceArrayPtr);

  // Every source vertex gets a NodeTypes value. GenerateTripleLines copies these through to the
  // output vertices; it never reads them to decide which edges are triple lines.
  auto nodeTypeStore = std::make_unique<DataStore<int8>>(std::vector<usize>{vertices.size()}, std::vector<usize>{1}, 0);
  auto* nodeTypeArrayPtr = Int8Array::Create(dataStructure, k_NodeTypesName, std::move(nodeTypeStore), triangleGeomPtr->getId());
  REQUIRE(nodeTypeArrayPtr != nullptr);
  auto& nodeTypesRef = nodeTypeArrayPtr->getDataStoreRef();
  for(usize i = 0; i < vertices.size(); i++)
  {
    // Distinct, recognisable values so a copy-through bug is visible rather than masked by zeros.
    nodeTypesRef[i] = static_cast<int8>(2 + (i % 3));
  }

  auto labelStore = std::make_unique<DataStore<int32>>(std::vector<usize>{faceLabels.size()}, std::vector<usize>{2}, 0);
  auto* labelArrayPtr = Int32Array::Create(dataStructure, k_FaceLabelsName, std::move(labelStore), triangleGeomPtr->getId());
  REQUIRE(labelArrayPtr != nullptr);
  auto& labelsRef = labelArrayPtr->getDataStoreRef();
  for(usize i = 0; i < faceLabels.size(); i++)
  {
    labelsRef[(i * 2) + 0] = faceLabels[i][0];
    labelsRef[(i * 2) + 1] = faceLabels[i][1];
  }

  return triangleGeomPtr;
}

/**
 * @brief Creates an empty EdgeGeom with its attribute matrices, plus a NumFeatures array.
 * GenerateTripleLines resizes all of them.
 * @param dataStructure Receives the geometry and output arrays.
 * @return Geometry, NumFeatures array, and NodeTypes array.
 */
std::tuple<EdgeGeom*, Int8Array*, Int8Array*> CreateEmptyTripleLineGeom(DataStructure& dataStructure)
{
  auto* edgeGeomPtr = EdgeGeom::Create(dataStructure, k_TripleLineGeomName);
  REQUIRE(edgeGeomPtr != nullptr);

  auto vertexStore = std::make_unique<DataStore<float32>>(std::vector<usize>{0}, std::vector<usize>{3}, 0.0f);
  auto* vertexArrayPtr = IGeometry::SharedVertexList::Create(dataStructure, "SharedVertexList", std::move(vertexStore), edgeGeomPtr->getId());
  REQUIRE(vertexArrayPtr != nullptr);
  edgeGeomPtr->setVertices(*vertexArrayPtr);

  auto edgeStore = std::make_unique<DataStore<IGeometry::MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{2}, 0);
  auto* edgeArrayPtr = IGeometry::SharedEdgeList::Create(dataStructure, "SharedEdgeList", std::move(edgeStore), edgeGeomPtr->getId());
  REQUIRE(edgeArrayPtr != nullptr);
  edgeGeomPtr->setEdgeList(*edgeArrayPtr);

  auto* vertexAmPtr = AttributeMatrix::Create(dataStructure, "Vertex Data", ShapeType{0}, edgeGeomPtr->getId());
  REQUIRE(vertexAmPtr != nullptr);
  edgeGeomPtr->setVertexAttributeMatrix(*vertexAmPtr);

  auto* edgeAmPtr = AttributeMatrix::Create(dataStructure, "Edge Data", ShapeType{0}, edgeGeomPtr->getId());
  REQUIRE(edgeAmPtr != nullptr);
  edgeGeomPtr->setEdgeAttributeMatrix(*edgeAmPtr);

  auto numFeaturesStore = std::make_unique<DataStore<int8>>(std::vector<usize>{0}, std::vector<usize>{1}, 0);
  auto* numFeaturesArrayPtr = Int8Array::Create(dataStructure, k_NumFeaturesName, std::move(numFeaturesStore), edgeAmPtr->getId());
  REQUIRE(numFeaturesArrayPtr != nullptr);

  auto outNodeTypeStore = std::make_unique<DataStore<int8>>(std::vector<usize>{0}, std::vector<usize>{1}, 0);
  auto* outNodeTypeArrayPtr = Int8Array::Create(dataStructure, k_NodeTypesName, std::move(outNodeTypeStore), vertexAmPtr->getId());
  REQUIRE(outNodeTypeArrayPtr != nullptr);

  return {edgeGeomPtr, numFeaturesArrayPtr, outNodeTypeArrayPtr};
}

std::tuple<EdgeGeom*, Int8Array*, Int8Array*> GetOutputs(DataStructure& dataStructure)
{
  auto* edgeGeomPtr = dataStructure.getDataAs<EdgeGeom>(DataPath({k_TripleLineGeomName}));
  auto* numFeaturesPtr = dataStructure.getDataAs<Int8Array>(DataPath({k_TripleLineGeomName, "Edge Data", k_NumFeaturesName}));
  auto* nodeTypesPtr = dataStructure.getDataAs<Int8Array>(DataPath({k_TripleLineGeomName, "Vertex Data", k_NodeTypesName}));
  REQUIRE(edgeGeomPtr != nullptr);
  REQUIRE(numFeaturesPtr != nullptr);
  REQUIRE(nodeTypesPtr != nullptr);
  return {edgeGeomPtr, numFeaturesPtr, nodeTypesPtr};
}

enum class NodeTypePointers : uint8
{
  Both,
  Neither,
  InputOnly,
  OutputOnly
};

Result<> RunGenerateTripleLines(DataStructure& dataStructure, TriangleGeom& triangleGeom, bool includeExterior = false, NodeTypePointers nodeTypePointers = NodeTypePointers::Both,
                                const std::atomic_bool& shouldCancel = std::atomic_bool{false})
{
  auto [edgeGeomPtr, numFeaturesPtr, nodeTypesPtr] = CreateEmptyTripleLineGeom(dataStructure);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({k_TriangleGeomName, k_FaceLabelsName})));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int8Array>(DataPath({k_TriangleGeomName, k_NodeTypesName})));
  const auto& faceLabels = dataStructure.getDataRefAs<Int32Array>(DataPath({k_TriangleGeomName, k_FaceLabelsName})).getDataStoreRef();
  const auto& sourceNodeTypes = dataStructure.getDataRefAs<Int8Array>(DataPath({k_TriangleGeomName, k_NodeTypesName})).getDataStoreRef();
  const auto* inputNodeTypesPtr = nodeTypePointers == NodeTypePointers::Both || nodeTypePointers == NodeTypePointers::InputOnly ? &sourceNodeTypes : nullptr;
  auto* outputNodeTypesPtr = nodeTypePointers == NodeTypePointers::Both || nodeTypePointers == NodeTypePointers::OutputOnly ? &nodeTypesPtr->getDataStoreRef() : nullptr;
  const MeshingUtilities::TripleLineInputs inputs{triangleGeom, faceLabels, inputNodeTypesPtr, includeExterior};
  const MeshingUtilities::TripleLineOutputs outputs{*edgeGeomPtr, numFeaturesPtr->getDataStoreRef(), outputNodeTypesPtr};
  return MeshingUtilities::GenerateTripleLines(inputs, outputs, shouldCancel, {});
}

const std::vector<std::array<float32, 3>> k_TripleJunctionVertices = {
    {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f}, {-0.5f, 0.87f, 0.0f}, {-0.5f, 0.87f, 1.0f}, {-0.5f, -0.87f, 0.0f}, {-0.5f, -0.87f, 1.0f},
};
const std::vector<std::array<usize, 3>> k_TripleJunctionTriangles = {
    {0, 1, 3}, {0, 3, 2}, {0, 1, 5}, {0, 5, 4}, {0, 1, 7}, {0, 7, 6},
};
const std::vector<std::array<int32, 2>> k_TripleJunctionLabels = {
    {1, 2}, {1, 2}, {2, 3}, {2, 3}, {1, 3}, {1, 3},
};

} // namespace

TEST_CASE("MeshingUtilities::GenerateTripleLines: Flat boundary produces no triple lines", "[Core][MeshingUtilities]")
{
  DataStructure dataStructure;

  // A single quad between grains 1 and 2, split into two triangles. Every edge is
  // shared by at most two triangles, and both carry the same labels, so no edge
  // borders 3 or more unique Feature Ids.
  const std::vector<std::array<float32, 3>> vertices = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}};
  const std::vector<std::array<usize, 3>> triangles = {{0, 1, 2}, {0, 2, 3}};
  const std::vector<std::array<int32, 2>> faceLabels = {{1, 2}, {1, 2}};

  auto* triangleGeomPtr = CreateTriangleMesh(dataStructure, vertices, triangles, faceLabels);

  Result<> result = RunGenerateTripleLines(dataStructure, *triangleGeomPtr, false);

  REQUIRE(result.valid());
  const auto [edgeGeomPtr, numFeaturesArrayPtr, tripleLineNodeTypesPtr] = GetOutputs(dataStructure);
  REQUIRE(edgeGeomPtr->getNumberOfEdges() == 0);
  REQUIRE(edgeGeomPtr->getNumberOfVertices() == 0);
  REQUIRE(numFeaturesArrayPtr->getNumberOfTuples() == 0);
}

TEST_CASE("MeshingUtilities::GenerateTripleLines: Interior triple junction", "[Core][MeshingUtilities]")
{
  DataStructure dataStructure;

  // Three quad sheets meeting along the shared edge v0->v1. Each sheet is split so that
  // exactly one of its two triangles contains both v0 and v1, so the shared edge is
  // touched by exactly 3 triangles carrying labels {1,2}, {2,3} and {1,3} => 3 unique.
  const auto& vertices = k_TripleJunctionVertices;
  const auto& triangles = k_TripleJunctionTriangles;
  const auto& faceLabels = k_TripleJunctionLabels;

  auto* triangleGeomPtr = CreateTriangleMesh(dataStructure, vertices, triangles, faceLabels);

  Result<> result = RunGenerateTripleLines(dataStructure, *triangleGeomPtr, false);

  REQUIRE(result.valid());
  const auto [edgeGeomPtr, numFeaturesArrayPtr, tripleLineNodeTypesPtr] = GetOutputs(dataStructure);
  REQUIRE(edgeGeomPtr->getNumberOfEdges() == 1);
  REQUIRE(edgeGeomPtr->getNumberOfVertices() == 2);
  REQUIRE(numFeaturesArrayPtr->getNumberOfTuples() == 1);
  REQUIRE((*numFeaturesArrayPtr)[0] == 3);

  // The one emitted edge must join the two ends of the shared edge, which sit at
  // z = 0 and z = 1 with x = y = 0. Vertices follow ascending source index order.
  const auto& edgesRef = edgeGeomPtr->getEdges()->getDataStoreRef();
  const auto& vertsRef = edgeGeomPtr->getVertices()->getDataStoreRef();
  std::set<float32> zCoords;
  for(usize i = 0; i < 2; i++)
  {
    const usize vertIndex = edgesRef[i];
    REQUIRE(vertsRef[(vertIndex * 3) + 0] == Approx(0.0f));
    REQUIRE(vertsRef[(vertIndex * 3) + 1] == Approx(0.0f));
    zCoords.insert(vertsRef[(vertIndex * 3) + 2]);
  }
  REQUIRE(zCoords == std::set<float32>{0.0f, 1.0f});
}

TEST_CASE("MeshingUtilities::GenerateTripleLines: Quadruple point line", "[Core][MeshingUtilities]")
{
  DataStructure dataStructure;

  // Four sheets around the shared edge, labels {1,2}, {2,3}, {3,4} and {1,4} => 4 unique.
  const std::vector<std::array<float32, 3>> vertices = {
      {0.0f, 0.0f, 0.0f},  {0.0f, 0.0f, 1.0f},  // v0, v1 : the shared edge
      {1.0f, 0.0f, 0.0f},  {1.0f, 0.0f, 1.0f},  // sheet A rim
      {0.0f, 1.0f, 0.0f},  {0.0f, 1.0f, 1.0f},  // sheet B rim
      {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 1.0f}, // sheet C rim
      {0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 1.0f}, // sheet D rim
  };
  const std::vector<std::array<usize, 3>> triangles = {
      {0, 1, 3}, {0, 3, 2}, {0, 1, 5}, {0, 5, 4}, {0, 1, 7}, {0, 7, 6}, {0, 1, 9}, {0, 9, 8},
  };
  const std::vector<std::array<int32, 2>> faceLabels = {
      {1, 2}, {1, 2}, {2, 3}, {2, 3}, {3, 4}, {3, 4}, {1, 4}, {1, 4},
  };

  auto* triangleGeomPtr = CreateTriangleMesh(dataStructure, vertices, triangles, faceLabels);

  Result<> result = RunGenerateTripleLines(dataStructure, *triangleGeomPtr, false);

  REQUIRE(result.valid());
  const auto [edgeGeomPtr, numFeaturesArrayPtr, tripleLineNodeTypesPtr] = GetOutputs(dataStructure);
  REQUIRE(edgeGeomPtr->getNumberOfEdges() == 1);
  REQUIRE(numFeaturesArrayPtr->getNumberOfTuples() == 1);
  REQUIRE((*numFeaturesArrayPtr)[0] == 4);
}

TEST_CASE("MeshingUtilities::GenerateTripleLines: Vertex list is compacted", "[Core][MeshingUtilities]")
{
  DataStructure dataStructure;

  // Same fixture as Interior triple junction, which has 8 source vertices but only 2 lie on a
  // triple line. The output must carry exactly those 2, and every edge index must be
  // in range - i.e. the remap really happened rather than passing indices through.
  const auto& vertices = k_TripleJunctionVertices;
  const auto& triangles = k_TripleJunctionTriangles;
  const auto& faceLabels = k_TripleJunctionLabels;

  auto* triangleGeomPtr = CreateTriangleMesh(dataStructure, vertices, triangles, faceLabels);

  Result<> result = RunGenerateTripleLines(dataStructure, *triangleGeomPtr, false);

  REQUIRE(result.valid());
  const auto [edgeGeomPtr, numFeaturesArrayPtr, tripleLineNodeTypesPtr] = GetOutputs(dataStructure);
  REQUIRE(triangleGeomPtr->getNumberOfVertices() == 8);
  REQUIRE(edgeGeomPtr->getNumberOfVertices() == 2);

  const auto& edgesRef = edgeGeomPtr->getEdges()->getDataStoreRef();
  for(usize i = 0; i < edgeGeomPtr->getNumberOfEdges() * 2; i++)
  {
    REQUIRE(edgesRef[i] < edgeGeomPtr->getNumberOfVertices());
  }
}

TEST_CASE("MeshingUtilities::GenerateTripleLines: IncludeExteriorLines toggles surface lines", "[Core][MeshingUtilities]")
{
  // A grain boundary between grains 1 and 2 reaching the free surface of the volume.
  // Three sheets meet along the shared edge: the interior 1|2 boundary and two exposed outer faces.
  // The outer faces carry the -1 "outside" label.
  const auto& vertices = k_TripleJunctionVertices;
  const auto& triangles = k_TripleJunctionTriangles;
  const std::vector<std::array<int32, 2>> faceLabels = {{1, 2}, {1, 2}, {-1, 2}, {-1, 2}, {-1, 1}, {-1, 1}};

  SECTION("Interior only (the default) rejects it")
  {
    DataStructure dataStructure;
    auto* triangleGeomPtr = CreateTriangleMesh(dataStructure, vertices, triangles, faceLabels);

    Result<> result = RunGenerateTripleLines(dataStructure, *triangleGeomPtr, false);

    // Discounting -1, the shared edge borders only grains 1 and 2.
    REQUIRE(result.valid());
    const auto [edgeGeomPtr, numFeaturesArrayPtr, tripleLineNodeTypesPtr] = GetOutputs(dataStructure);
    REQUIRE(edgeGeomPtr->getNumberOfEdges() == 0);
  }

  SECTION("IncludeExteriorLines accepts it")
  {
    DataStructure dataStructure;
    auto* triangleGeomPtr = CreateTriangleMesh(dataStructure, vertices, triangles, faceLabels);

    Result<> result = RunGenerateTripleLines(dataStructure, *triangleGeomPtr, true);

    // Counting -1 as a region, the shared edge borders {1, 2, -1} => 3 unique.
    REQUIRE(result.valid());
    const auto [edgeGeomPtr, numFeaturesArrayPtr, tripleLineNodeTypesPtr] = GetOutputs(dataStructure);
    REQUIRE(edgeGeomPtr->getNumberOfEdges() == 1);
    REQUIRE((*numFeaturesArrayPtr)[0] == 3);
  }
}

TEST_CASE("MeshingUtilities::GenerateTripleLines: NodeTypes are copied through to the output vertices", "[Core][MeshingUtilities]")
{
  DataStructure dataStructure;

  // The Interior triple junction fixture: 8 source vertices, of which only v0 and v1 lie on the triple line.
  const auto& vertices = k_TripleJunctionVertices;
  const auto& triangles = k_TripleJunctionTriangles;
  const auto& faceLabels = k_TripleJunctionLabels;

  auto* triangleGeomPtr = CreateTriangleMesh(dataStructure, vertices, triangles, faceLabels);

  Result<> result = RunGenerateTripleLines(dataStructure, *triangleGeomPtr, false);

  REQUIRE(result.valid());
  const auto [edgeGeomPtr, numFeaturesArrayPtr, tripleLineNodeTypesPtr] = GetOutputs(dataStructure);
  REQUIRE(edgeGeomPtr->getNumberOfVertices() == 2);
  REQUIRE(tripleLineNodeTypesPtr->getNumberOfTuples() == 2);

  // The Interior triple junction fixture retains source indices 0 and 1, in that order.
  REQUIRE((*tripleLineNodeTypesPtr)[0] == 2);
  REQUIRE((*tripleLineNodeTypesPtr)[1] == 3);
}

TEST_CASE("MeshingUtilities::GenerateTripleLines: Five sheets saturate at four Features", "[Core][MeshingUtilities]")
{
  DataStructure dataStructure;
  const std::vector<std::array<float32, 3>> vertices = {{0, 0, 0}, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}, {-1, 1, 0}, {-1, -1, 0}, {1, -1, 0}};
  const std::vector<std::array<usize, 3>> triangles = {{0, 1, 2}, {0, 1, 3}, {0, 1, 4}, {0, 1, 5}, {0, 1, 6}};
  const std::vector<std::array<int32, 2>> labels = {{1, 2}, {2, 3}, {3, 4}, {4, 5}, {1, 5}};
  auto* triangleGeomPtr = CreateTriangleMesh(dataStructure, vertices, triangles, labels);
  auto result = RunGenerateTripleLines(dataStructure, *triangleGeomPtr);
  REQUIRE(result.valid());
  const auto [edgeGeomPtr, numFeaturesPtr, nodeTypesPtr] = GetOutputs(dataStructure);
  REQUIRE(edgeGeomPtr->getNumberOfEdges() == 1);
  REQUIRE((*numFeaturesPtr)[0] == 4); // A value of 4 means four or more unique Feature Ids.
}

TEST_CASE("MeshingUtilities::GenerateTripleLines: Feature zero is an ordinary Feature", "[Core][MeshingUtilities]")
{
  DataStructure dataStructure;
  const std::vector<std::array<int32, 2>> labels = {{0, 1}, {0, 1}, {1, 2}, {1, 2}, {0, 2}, {0, 2}};
  auto* triangleGeomPtr = CreateTriangleMesh(dataStructure, k_TripleJunctionVertices, k_TripleJunctionTriangles, labels);
  auto result = RunGenerateTripleLines(dataStructure, *triangleGeomPtr, false);
  REQUIRE(result.valid());
  const auto [edgeGeomPtr, numFeaturesPtr, nodeTypesPtr] = GetOutputs(dataStructure);
  REQUIRE(edgeGeomPtr->getNumberOfEdges() == 1);
  REQUIRE((*numFeaturesPtr)[0] == 3);
}

TEST_CASE("MeshingUtilities::GenerateTripleLines: Checkerboard counts Features instead of triangles", "[Core][MeshingUtilities]")
{
  // This fixture separates unique Feature Id count from triangle count per edge.
  // Four triangles share edge (0,1), but only two distinct Features border that edge.
  DataStructure dataStructure;
  const std::vector<std::array<float32, 3>> vertices = {{0, 0, 0}, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}, {-1, 0, 0}, {0, -1, 0}};
  const std::vector<std::array<usize, 3>> triangles = {{0, 1, 2}, {0, 1, 3}, {0, 1, 4}, {0, 1, 5}};
  const std::vector<std::array<int32, 2>> labels(4, {1, 2});
  auto* triangleGeomPtr = CreateTriangleMesh(dataStructure, vertices, triangles, labels);
  auto result = RunGenerateTripleLines(dataStructure, *triangleGeomPtr);
  REQUIRE(result.valid());
  const auto [edgeGeomPtr, numFeaturesPtr, nodeTypesPtr] = GetOutputs(dataStructure);
  REQUIRE(edgeGeomPtr->getNumberOfEdges() == 0);
  REQUIRE(edgeGeomPtr->getNumberOfVertices() == 0);
}

TEST_CASE("MeshingUtilities::GenerateTripleLines: Cancellation leaves empty output", "[Core][MeshingUtilities]")
{
  DataStructure dataStructure;
  auto* triangleGeomPtr = CreateTriangleMesh(dataStructure, k_TripleJunctionVertices, k_TripleJunctionTriangles, k_TripleJunctionLabels);
  const std::atomic_bool shouldCancel{true};
  auto result = RunGenerateTripleLines(dataStructure, *triangleGeomPtr, false, NodeTypePointers::Both, shouldCancel);
  REQUIRE(result.valid());
  const auto [edgeGeomPtr, numFeaturesPtr, nodeTypesPtr] = GetOutputs(dataStructure);
  REQUIRE(edgeGeomPtr->getNumberOfEdges() == 0);
  REQUIRE(edgeGeomPtr->getNumberOfVertices() == 0);
}

TEST_CASE("MeshingUtilities::GenerateTripleLines: Output follows sorted source indices", "[Core][MeshingUtilities]")
{
  // Unused source indices force compaction. Triangles for the larger edge key come first,
  // and each shared edge has its endpoints reversed to check canonical key ordering.
  const std::vector<std::array<float32, 3>> vertices = {{9, 9, 9}, {2, 0, 0}, {0, 0, 0}, {9, 9, 8}, {2, 0, 1}, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}, {-1, 0, 0}, {3, 0, 0}, {2, 1, 0}, {1, 1, 0}};
  const std::vector<std::array<usize, 3>> triangles = {{5, 2, 6}, {5, 2, 7}, {5, 2, 8}, {4, 1, 9}, {4, 1, 10}, {4, 1, 11}};
  const std::vector<std::array<int32, 2>> labels = {{1, 2}, {2, 3}, {1, 3}, {1, 2}, {2, 3}, {1, 3}};
  for(const bool reverseTriangles : {false, true})
  {
    CAPTURE(reverseTriangles);
    auto orderedTriangles = triangles;
    auto orderedLabels = labels;
    if(reverseTriangles)
    {
      std::reverse(orderedTriangles.begin(), orderedTriangles.end());
      std::reverse(orderedLabels.begin(), orderedLabels.end());
    }
    DataStructure dataStructure;
    auto* triangleGeomPtr = CreateTriangleMesh(dataStructure, vertices, orderedTriangles, orderedLabels);
    auto result = RunGenerateTripleLines(dataStructure, *triangleGeomPtr);
    REQUIRE(result.valid());
    const auto [edgeGeomPtr, numFeaturesPtr, nodeTypesPtr] = GetOutputs(dataStructure);
    REQUIRE(edgeGeomPtr->getNumberOfEdges() == 2);
    REQUIRE(edgeGeomPtr->getNumberOfVertices() == 4);
    const auto& outputVertices = edgeGeomPtr->getVertices()->getDataStoreRef();
    const std::array<usize, 4> sourceIndices = {1, 2, 4, 5};
    for(usize vertexIdx = 0; vertexIdx < sourceIndices.size(); vertexIdx++)
    {
      for(usize compIdx = 0; compIdx < 3; compIdx++)
      {
        REQUIRE(outputVertices[(3 * vertexIdx) + compIdx] == vertices[sourceIndices[vertexIdx]][compIdx]);
      }
    }
    const auto& edges = edgeGeomPtr->getEdges()->getDataStoreRef();
    REQUIRE(edges[0] == 0);
    REQUIRE(edges[1] == 2);
    REQUIRE(edges[2] == 1);
    REQUIRE(edges[3] == 3);
  }
}

TEST_CASE("MeshingUtilities::GenerateTripleLines: NodeTypes pointers must be paired", "[Core][MeshingUtilities]")
{
  const auto pointers = GENERATE(NodeTypePointers::Neither, NodeTypePointers::InputOnly, NodeTypePointers::OutputOnly);
  DataStructure dataStructure;
  auto* triangleGeomPtr = CreateTriangleMesh(dataStructure, k_TripleJunctionVertices, k_TripleJunctionTriangles, k_TripleJunctionLabels);
  auto result = RunGenerateTripleLines(dataStructure, *triangleGeomPtr, false, pointers);
  const auto [edgeGeomPtr, numFeaturesPtr, nodeTypesPtr] = GetOutputs(dataStructure);
  if(pointers == NodeTypePointers::Neither)
  {
    REQUIRE(result.valid());
    REQUIRE(edgeGeomPtr->getNumberOfEdges() == 1);
    REQUIRE((*numFeaturesPtr)[0] == 3);
    // The AM resizes this optional test array, but extraction must not copy into it.
    REQUIRE((*nodeTypesPtr)[0] == 0);
    REQUIRE((*nodeTypesPtr)[1] == 0);
  }
  else
  {
    REQUIRE(result.invalid());
    REQUIRE(result.errors()[0].code == -57401);
    REQUIRE(edgeGeomPtr->getNumberOfEdges() == 0);
    REQUIRE(edgeGeomPtr->getNumberOfVertices() == 0);
  }
}
