#include "simplnx/Utilities/Meshing/TripleLineUtilities.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"

#include <catch2/catch.hpp>

#include <set>

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
 * label lists. Returns the geometry; the FaceLabels array is created as its child.
 */
TriangleGeom* CreateTriangleMesh(DataStructure& dataStructure, const std::vector<std::array<float32, 3>>& vertices, const std::vector<std::array<usize, 3>>& triangles,
                                 const std::vector<std::array<int32, 2>>& faceLabels)
{
  REQUIRE(triangles.size() == faceLabels.size());

  auto* triangleGeom = TriangleGeom::Create(dataStructure, k_TriangleGeomName);
  REQUIRE(triangleGeom != nullptr);

  auto vertexStore = std::make_unique<DataStore<float32>>(std::vector<usize>{vertices.size()}, std::vector<usize>{3}, 0.0f);
  auto* vertexArray = IGeometry::SharedVertexList::Create(dataStructure, "SharedVertexList", std::move(vertexStore), triangleGeom->getId());
  REQUIRE(vertexArray != nullptr);
  auto& verticesRef = vertexArray->getDataStoreRef();
  for(usize i = 0; i < vertices.size(); i++)
  {
    verticesRef[i * 3 + 0] = vertices[i][0];
    verticesRef[i * 3 + 1] = vertices[i][1];
    verticesRef[i * 3 + 2] = vertices[i][2];
  }
  triangleGeom->setVertices(*vertexArray);

  auto faceStore = std::make_unique<DataStore<IGeometry::MeshIndexType>>(std::vector<usize>{triangles.size()}, std::vector<usize>{3}, 0);
  auto* faceArray = IGeometry::SharedFaceList::Create(dataStructure, "SharedTriList", std::move(faceStore), triangleGeom->getId());
  REQUIRE(faceArray != nullptr);
  auto& facesRef = faceArray->getDataStoreRef();
  for(usize i = 0; i < triangles.size(); i++)
  {
    facesRef[i * 3 + 0] = triangles[i][0];
    facesRef[i * 3 + 1] = triangles[i][1];
    facesRef[i * 3 + 2] = triangles[i][2];
  }
  triangleGeom->setFaceList(*faceArray);

  // Every source vertex gets a NodeTypes value. GenerateTripleLines copies these through to the
  // output vertices; it never reads them to decide which edges are triple lines.
  auto nodeTypeStore = std::make_unique<DataStore<int8>>(std::vector<usize>{vertices.size()}, std::vector<usize>{1}, 0);
  auto* nodeTypeArray = Int8Array::Create(dataStructure, k_NodeTypesName, std::move(nodeTypeStore), triangleGeom->getId());
  REQUIRE(nodeTypeArray != nullptr);
  auto& nodeTypesRef = nodeTypeArray->getDataStoreRef();
  for(usize i = 0; i < vertices.size(); i++)
  {
    // Distinct, recognisable values so a copy-through bug is visible rather than masked by zeros.
    nodeTypesRef[i] = static_cast<int8>(2 + (i % 3));
  }

  auto labelStore = std::make_unique<DataStore<int32>>(std::vector<usize>{faceLabels.size()}, std::vector<usize>{2}, 0);
  auto* labelArray = Int32Array::Create(dataStructure, k_FaceLabelsName, std::move(labelStore), triangleGeom->getId());
  REQUIRE(labelArray != nullptr);
  auto& labelsRef = labelArray->getDataStoreRef();
  for(usize i = 0; i < faceLabels.size(); i++)
  {
    labelsRef[i * 2 + 0] = faceLabels[i][0];
    labelsRef[i * 2 + 1] = faceLabels[i][1];
  }

  return triangleGeom;
}

/**
 * @brief Creates an empty EdgeGeom with its attribute matrices, plus a NumFeatures array.
 * GenerateTripleLines resizes all of them.
 */
std::tuple<EdgeGeom*, Int8Array*, Int8Array*> CreateEmptyTripleLineGeom(DataStructure& dataStructure)
{
  auto* edgeGeom = EdgeGeom::Create(dataStructure, k_TripleLineGeomName);
  REQUIRE(edgeGeom != nullptr);

  auto vertexStore = std::make_unique<DataStore<float32>>(std::vector<usize>{0}, std::vector<usize>{3}, 0.0f);
  auto* vertexArray = IGeometry::SharedVertexList::Create(dataStructure, "SharedVertexList", std::move(vertexStore), edgeGeom->getId());
  REQUIRE(vertexArray != nullptr);
  edgeGeom->setVertices(*vertexArray);

  auto edgeStore = std::make_unique<DataStore<IGeometry::MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{2}, 0);
  auto* edgeArray = IGeometry::SharedEdgeList::Create(dataStructure, "SharedEdgeList", std::move(edgeStore), edgeGeom->getId());
  REQUIRE(edgeArray != nullptr);
  edgeGeom->setEdgeList(*edgeArray);

  auto* vertexAM = AttributeMatrix::Create(dataStructure, "Vertex Data", ShapeType{0}, edgeGeom->getId());
  REQUIRE(vertexAM != nullptr);
  edgeGeom->setVertexAttributeMatrix(*vertexAM);

  auto* edgeAM = AttributeMatrix::Create(dataStructure, "Edge Data", ShapeType{0}, edgeGeom->getId());
  REQUIRE(edgeAM != nullptr);
  edgeGeom->setEdgeAttributeMatrix(*edgeAM);

  auto numFeaturesStore = std::make_unique<DataStore<int8>>(std::vector<usize>{0}, std::vector<usize>{1}, 0);
  auto* numFeaturesArray = Int8Array::Create(dataStructure, k_NumFeaturesName, std::move(numFeaturesStore), edgeAM->getId());
  REQUIRE(numFeaturesArray != nullptr);

  auto outNodeTypeStore = std::make_unique<DataStore<int8>>(std::vector<usize>{0}, std::vector<usize>{1}, 0);
  auto* outNodeTypeArray = Int8Array::Create(dataStructure, k_NodeTypesName, std::move(outNodeTypeStore), vertexAM->getId());
  REQUIRE(outNodeTypeArray != nullptr);

  return {edgeGeom, numFeaturesArray, outNodeTypeArray};
}
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

  auto* triangleGeom = CreateTriangleMesh(dataStructure, vertices, triangles, faceLabels);
  auto [edgeGeom, numFeaturesArray, tripleLineNodeTypes] = CreateEmptyTripleLineGeom(dataStructure);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({k_TriangleGeomName, k_FaceLabelsName})));
  const auto& faceLabelsRef = dataStructure.getDataRefAs<Int32Array>(DataPath({k_TriangleGeomName, k_FaceLabelsName})).getDataStoreRef();
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int8Array>(DataPath({k_TriangleGeomName, k_NodeTypesName})));
  const auto& sourceNodeTypesRef = dataStructure.getDataRefAs<Int8Array>(DataPath({k_TriangleGeomName, k_NodeTypesName})).getDataStoreRef();

  const std::atomic_bool shouldCancel{false};
  MeshingUtilities::TripleLineOptions options;

  Result<> result = MeshingUtilities::GenerateTripleLines(*triangleGeom, faceLabelsRef, sourceNodeTypesRef, *edgeGeom, numFeaturesArray->getDataStoreRef(), tripleLineNodeTypes->getDataStoreRef(),
                                                          options, shouldCancel, {});

  REQUIRE(result.valid());
  REQUIRE(edgeGeom->getNumberOfEdges() == 0);
  REQUIRE(edgeGeom->getNumberOfVertices() == 0);
  REQUIRE(numFeaturesArray->getNumberOfTuples() == 0);
}

TEST_CASE("MeshingUtilities::GenerateTripleLines: Interior triple junction", "[Core][MeshingUtilities]")
{
  DataStructure dataStructure;

  // Three quad sheets meeting along the shared edge v0->v1. Each sheet is split so that
  // exactly one of its two triangles contains both v0 and v1, so the shared edge is
  // touched by exactly 3 triangles carrying labels {1,2}, {2,3} and {1,3} => 3 unique.
  const std::vector<std::array<float32, 3>> vertices = {
      {0.0f, 0.0f, 0.0f},    {0.0f, 0.0f, 1.0f},    // v0, v1 : the shared edge
      {1.0f, 0.0f, 0.0f},    {1.0f, 0.0f, 1.0f},    // v2, v3 : sheet A rim
      {-0.5f, 0.87f, 0.0f},  {-0.5f, 0.87f, 1.0f},  // v4, v5 : sheet B rim
      {-0.5f, -0.87f, 0.0f}, {-0.5f, -0.87f, 1.0f}, // v6, v7 : sheet C rim
  };
  const std::vector<std::array<usize, 3>> triangles = {
      {0, 1, 3}, {0, 3, 2}, // sheet A
      {0, 1, 5}, {0, 5, 4}, // sheet B
      {0, 1, 7}, {0, 7, 6}, // sheet C
  };
  const std::vector<std::array<int32, 2>> faceLabels = {
      {1, 2}, {1, 2}, // sheet A
      {2, 3}, {2, 3}, // sheet B
      {1, 3}, {1, 3}, // sheet C
  };

  auto* triangleGeom = CreateTriangleMesh(dataStructure, vertices, triangles, faceLabels);
  auto [edgeGeom, numFeaturesArray, tripleLineNodeTypes] = CreateEmptyTripleLineGeom(dataStructure);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({k_TriangleGeomName, k_FaceLabelsName})));
  const auto& faceLabelsRef = dataStructure.getDataRefAs<Int32Array>(DataPath({k_TriangleGeomName, k_FaceLabelsName})).getDataStoreRef();
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int8Array>(DataPath({k_TriangleGeomName, k_NodeTypesName})));
  const auto& sourceNodeTypesRef = dataStructure.getDataRefAs<Int8Array>(DataPath({k_TriangleGeomName, k_NodeTypesName})).getDataStoreRef();

  const std::atomic_bool shouldCancel{false};
  MeshingUtilities::TripleLineOptions options;

  Result<> result = MeshingUtilities::GenerateTripleLines(*triangleGeom, faceLabelsRef, sourceNodeTypesRef, *edgeGeom, numFeaturesArray->getDataStoreRef(), tripleLineNodeTypes->getDataStoreRef(),
                                                          options, shouldCancel, {});

  REQUIRE(result.valid());
  REQUIRE(edgeGeom->getNumberOfEdges() == 1);
  REQUIRE(edgeGeom->getNumberOfVertices() == 2);
  REQUIRE(numFeaturesArray->getNumberOfTuples() == 1);
  REQUIRE((*numFeaturesArray)[0] == 3);

  // The one emitted edge must join the two ends of the shared edge, which sit at
  // z = 0 and z = 1 with x = y = 0. Vertex order within the edge is unspecified.
  const auto& edgesRef = edgeGeom->getEdges()->getDataStoreRef();
  const auto& vertsRef = edgeGeom->getVertices()->getDataStoreRef();
  std::set<float32> zCoords;
  for(usize i = 0; i < 2; i++)
  {
    const usize vertIndex = edgesRef[i];
    REQUIRE(vertsRef[vertIndex * 3 + 0] == Approx(0.0f));
    REQUIRE(vertsRef[vertIndex * 3 + 1] == Approx(0.0f));
    zCoords.insert(vertsRef[vertIndex * 3 + 2]);
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

  auto* triangleGeom = CreateTriangleMesh(dataStructure, vertices, triangles, faceLabels);
  auto [edgeGeom, numFeaturesArray, tripleLineNodeTypes] = CreateEmptyTripleLineGeom(dataStructure);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({k_TriangleGeomName, k_FaceLabelsName})));
  const auto& faceLabelsRef = dataStructure.getDataRefAs<Int32Array>(DataPath({k_TriangleGeomName, k_FaceLabelsName})).getDataStoreRef();
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int8Array>(DataPath({k_TriangleGeomName, k_NodeTypesName})));
  const auto& sourceNodeTypesRef = dataStructure.getDataRefAs<Int8Array>(DataPath({k_TriangleGeomName, k_NodeTypesName})).getDataStoreRef();

  const std::atomic_bool shouldCancel{false};
  MeshingUtilities::TripleLineOptions options;

  Result<> result = MeshingUtilities::GenerateTripleLines(*triangleGeom, faceLabelsRef, sourceNodeTypesRef, *edgeGeom, numFeaturesArray->getDataStoreRef(), tripleLineNodeTypes->getDataStoreRef(),
                                                          options, shouldCancel, {});

  REQUIRE(result.valid());
  REQUIRE(edgeGeom->getNumberOfEdges() == 1);
  REQUIRE(numFeaturesArray->getNumberOfTuples() == 1);
  REQUIRE((*numFeaturesArray)[0] == 4);
}

TEST_CASE("MeshingUtilities::GenerateTripleLines: Vertex list is compacted", "[Core][MeshingUtilities]")
{
  DataStructure dataStructure;

  // Same triple junction as Task 2, which has 8 source vertices but only 2 lie on a
  // triple line. The output must carry exactly those 2, and every edge index must be
  // in range - i.e. the remap really happened rather than passing indices through.
  const std::vector<std::array<float32, 3>> vertices = {
      {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f}, {-0.5f, 0.87f, 0.0f}, {-0.5f, 0.87f, 1.0f}, {-0.5f, -0.87f, 0.0f}, {-0.5f, -0.87f, 1.0f},
  };
  const std::vector<std::array<usize, 3>> triangles = {{0, 1, 3}, {0, 3, 2}, {0, 1, 5}, {0, 5, 4}, {0, 1, 7}, {0, 7, 6}};
  const std::vector<std::array<int32, 2>> faceLabels = {{1, 2}, {1, 2}, {2, 3}, {2, 3}, {1, 3}, {1, 3}};

  auto* triangleGeom = CreateTriangleMesh(dataStructure, vertices, triangles, faceLabels);
  auto [edgeGeom, numFeaturesArray, tripleLineNodeTypes] = CreateEmptyTripleLineGeom(dataStructure);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({k_TriangleGeomName, k_FaceLabelsName})));
  const auto& faceLabelsRef = dataStructure.getDataRefAs<Int32Array>(DataPath({k_TriangleGeomName, k_FaceLabelsName})).getDataStoreRef();
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int8Array>(DataPath({k_TriangleGeomName, k_NodeTypesName})));
  const auto& sourceNodeTypesRef = dataStructure.getDataRefAs<Int8Array>(DataPath({k_TriangleGeomName, k_NodeTypesName})).getDataStoreRef();

  const std::atomic_bool shouldCancel{false};
  MeshingUtilities::TripleLineOptions options;

  Result<> result = MeshingUtilities::GenerateTripleLines(*triangleGeom, faceLabelsRef, sourceNodeTypesRef, *edgeGeom, numFeaturesArray->getDataStoreRef(), tripleLineNodeTypes->getDataStoreRef(),
                                                          options, shouldCancel, {});

  REQUIRE(result.valid());
  REQUIRE(triangleGeom->getNumberOfVertices() == 8);
  REQUIRE(edgeGeom->getNumberOfVertices() == 2);

  const auto& edgesRef = edgeGeom->getEdges()->getDataStoreRef();
  for(usize i = 0; i < edgeGeom->getNumberOfEdges() * 2; i++)
  {
    REQUIRE(edgesRef[i] < edgeGeom->getNumberOfVertices());
  }
}

TEST_CASE("MeshingUtilities::GenerateTripleLines: IncludeExteriorLines toggles surface lines", "[Core][MeshingUtilities]")
{
  // A grain boundary between grains 1 and 2 reaching the free surface of the volume.
  // Three sheets meet along the shared edge: the interior 1|2 boundary, and the two
  // exposed outer faces of grains 1 and 2, which carry the -1 "outside" label.
  const std::vector<std::array<float32, 3>> vertices = {
      {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f}, {-0.5f, 0.87f, 0.0f}, {-0.5f, 0.87f, 1.0f}, {-0.5f, -0.87f, 0.0f}, {-0.5f, -0.87f, 1.0f},
  };
  const std::vector<std::array<usize, 3>> triangles = {{0, 1, 3}, {0, 3, 2}, {0, 1, 5}, {0, 5, 4}, {0, 1, 7}, {0, 7, 6}};
  const std::vector<std::array<int32, 2>> faceLabels = {{1, 2}, {1, 2}, {-1, 2}, {-1, 2}, {-1, 1}, {-1, 1}};

  SECTION("Interior only (the default) rejects it")
  {
    DataStructure dataStructure;
    auto* triangleGeom = CreateTriangleMesh(dataStructure, vertices, triangles, faceLabels);
    auto [edgeGeom, numFeaturesArray, tripleLineNodeTypes] = CreateEmptyTripleLineGeom(dataStructure);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({k_TriangleGeomName, k_FaceLabelsName})));
    const auto& faceLabelsRef = dataStructure.getDataRefAs<Int32Array>(DataPath({k_TriangleGeomName, k_FaceLabelsName})).getDataStoreRef();
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int8Array>(DataPath({k_TriangleGeomName, k_NodeTypesName})));
    const auto& sourceNodeTypesRef = dataStructure.getDataRefAs<Int8Array>(DataPath({k_TriangleGeomName, k_NodeTypesName})).getDataStoreRef();

    const std::atomic_bool shouldCancel{false};
    MeshingUtilities::TripleLineOptions options;
    options.IncludeExteriorLines = false;

    Result<> result = MeshingUtilities::GenerateTripleLines(*triangleGeom, faceLabelsRef, sourceNodeTypesRef, *edgeGeom, numFeaturesArray->getDataStoreRef(), tripleLineNodeTypes->getDataStoreRef(),
                                                            options, shouldCancel, {});

    // Discounting -1, the shared edge borders only grains 1 and 2.
    REQUIRE(result.valid());
    REQUIRE(edgeGeom->getNumberOfEdges() == 0);
  }

  SECTION("IncludeExteriorLines accepts it")
  {
    DataStructure dataStructure;
    auto* triangleGeom = CreateTriangleMesh(dataStructure, vertices, triangles, faceLabels);
    auto [edgeGeom, numFeaturesArray, tripleLineNodeTypes] = CreateEmptyTripleLineGeom(dataStructure);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({k_TriangleGeomName, k_FaceLabelsName})));
    const auto& faceLabelsRef = dataStructure.getDataRefAs<Int32Array>(DataPath({k_TriangleGeomName, k_FaceLabelsName})).getDataStoreRef();
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int8Array>(DataPath({k_TriangleGeomName, k_NodeTypesName})));
    const auto& sourceNodeTypesRef = dataStructure.getDataRefAs<Int8Array>(DataPath({k_TriangleGeomName, k_NodeTypesName})).getDataStoreRef();

    const std::atomic_bool shouldCancel{false};
    MeshingUtilities::TripleLineOptions options;
    options.IncludeExteriorLines = true;

    Result<> result = MeshingUtilities::GenerateTripleLines(*triangleGeom, faceLabelsRef, sourceNodeTypesRef, *edgeGeom, numFeaturesArray->getDataStoreRef(), tripleLineNodeTypes->getDataStoreRef(),
                                                            options, shouldCancel, {});

    // Counting -1 as a region, the shared edge borders {1, 2, -1} => 3 unique.
    REQUIRE(result.valid());
    REQUIRE(edgeGeom->getNumberOfEdges() == 1);
    REQUIRE((*numFeaturesArray)[0] == 3);
  }
}

TEST_CASE("MeshingUtilities::GenerateTripleLines: NodeTypes are copied through to the output vertices", "[Core][MeshingUtilities]")
{
  DataStructure dataStructure;

  // The Task 2 triple junction: 8 source vertices, of which only v0 and v1 lie on the triple line.
  const std::vector<std::array<float32, 3>> vertices = {
      {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f}, {-0.5f, 0.87f, 0.0f}, {-0.5f, 0.87f, 1.0f}, {-0.5f, -0.87f, 0.0f}, {-0.5f, -0.87f, 1.0f},
  };
  const std::vector<std::array<usize, 3>> triangles = {{0, 1, 3}, {0, 3, 2}, {0, 1, 5}, {0, 5, 4}, {0, 1, 7}, {0, 7, 6}};
  const std::vector<std::array<int32, 2>> faceLabels = {{1, 2}, {1, 2}, {2, 3}, {2, 3}, {1, 3}, {1, 3}};

  auto* triangleGeom = CreateTriangleMesh(dataStructure, vertices, triangles, faceLabels);
  auto [edgeGeom, numFeaturesArray, tripleLineNodeTypes] = CreateEmptyTripleLineGeom(dataStructure);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(DataPath({k_TriangleGeomName, k_FaceLabelsName})));
  const auto& faceLabelsRef = dataStructure.getDataRefAs<Int32Array>(DataPath({k_TriangleGeomName, k_FaceLabelsName})).getDataStoreRef();
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int8Array>(DataPath({k_TriangleGeomName, k_NodeTypesName})));
  const auto& sourceNodeTypesRef = dataStructure.getDataRefAs<Int8Array>(DataPath({k_TriangleGeomName, k_NodeTypesName})).getDataStoreRef();

  const std::atomic_bool shouldCancel{false};
  MeshingUtilities::TripleLineOptions options;

  Result<> result = MeshingUtilities::GenerateTripleLines(*triangleGeom, faceLabelsRef, sourceNodeTypesRef, *edgeGeom, numFeaturesArray->getDataStoreRef(), tripleLineNodeTypes->getDataStoreRef(),
                                                          options, shouldCancel, {});

  REQUIRE(result.valid());
  REQUIRE(edgeGeom->getNumberOfVertices() == 2);
  REQUIRE(tripleLineNodeTypes->getNumberOfTuples() == 2);

  // Each output vertex must carry the NodeTypes value of the SOURCE vertex it was copied from.
  // The fixture assigns nodeTypes[i] = 2 + (i % 3), and only source vertices 0 and 1 survive
  // compaction, so the two output values must be exactly {2, 3} — matched by coordinate, since
  // compaction does not preserve index order.
  const auto& vertsRef = edgeGeom->getVertices()->getDataStoreRef();
  const auto& outNodeTypesRef = tripleLineNodeTypes->getDataStoreRef();
  for(usize i = 0; i < 2; i++)
  {
    const float32 z = vertsRef[i * 3 + 2];
    const usize originalIndex = (z == Approx(0.0f)) ? 0 : 1;
    INFO("output vertex " << i << " at z=" << z << " maps to source vertex " << originalIndex);
    REQUIRE(outNodeTypesRef[i] == sourceNodeTypesRef[originalIndex]);
  }
}
