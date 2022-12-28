#include <catch2/catch.hpp>

#include <string>

#include "ComplexCore/Filters/ExtractInternalSurfacesFromTriangleGeometry.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

using namespace complex;

namespace
{
constexpr StringLiteral k_TriangleGeomName = "TriangleGeom";
constexpr StringLiteral k_InternalTriangleGeomName = "Internal TriangleGeom";
constexpr StringLiteral k_NodeTypesName = "Node Types";
constexpr StringLiteral k_VertsListName = "Vert List";
constexpr StringLiteral k_TriListName = "Tri List";
const DataPath k_TriangleGeomPath({k_TriangleGeomName});
const DataPath k_InternalTrianglePath({k_InternalTriangleGeomName});
const DataPath k_NodeTypesPath = k_TriangleGeomPath.createChildPath(k_NodeTypesName);
const std::vector<DataPath> k_CopyVertexPaths = {k_TriangleGeomPath.createChildPath(k_VertsListName)};
const std::vector<DataPath> k_CopyTrianglePaths = {k_TriangleGeomPath.createChildPath(k_TriListName)};

DataStructure createTestData(const std::string& triangleGeomName, const std::string& nodeTypesName)
{
  DataStructure dataGraph;
  auto* triangleGeom = TriangleGeom::Create(dataGraph, triangleGeomName);

  Float32Array* vertListdata = UnitTest::CreateTestDataArray<float>(dataGraph, k_VertsListName, {80, 60, 40}, {1}, triangleGeom->getId());
  auto* triangleList = UInt64Array::CreateWithStore<UInt64DataStore>(dataGraph, k_TriListName, {500}, {3}, triangleGeom->getId());
  auto& triangles = triangleList->getDataStoreRef();
  triangles.fill(0);
  triangles[3] = 2;
  triangles[4] = 3;
  triangles[5] = 4;
  triangles[6] = 5;
  triangles[7] = 6;
  triangles[8] = 7;
  triangles[9] = 2;
  triangles[10] = 7;
  triangles[11] = 3;

  triangleGeom->setVertices(*vertListdata);
  triangleGeom->setFaceList(*triangleList);

  auto* nodeArray = Int8Array::CreateWithStore<Int8DataStore>(dataGraph, nodeTypesName, {triangleGeom->getNumberOfFaces()}, {1}, triangleGeom->getId());
  auto& nodeData = nodeArray->getDataStoreRef();
  nodeData.fill(2);
  nodeData[0] = 12;

  return dataGraph;
}
} // namespace

TEST_CASE("ComplexCore::ExtractInternalSurfacesFromTriangleGeometry(Instantiate)", "[ComplexCore][ExtractInternalSurfacesFromTriangleGeometry]")
{
  ExtractInternalSurfacesFromTriangleGeometry filter;
  DataStructure dataGraph = createTestData(k_TriangleGeomName, k_NodeTypesName);
  Arguments args;

  args.insert(ExtractInternalSurfacesFromTriangleGeometry::k_TriangleGeom_Key, std::make_any<DataPath>(k_TriangleGeomPath));
  args.insert(ExtractInternalSurfacesFromTriangleGeometry::k_InternalTriangleGeom_Key, std::make_any<DataPath>(k_InternalTrianglePath));
  args.insert(ExtractInternalSurfacesFromTriangleGeometry::k_NodeTypesPath_Key, std::make_any<DataPath>(k_NodeTypesPath));
  args.insert(ExtractInternalSurfacesFromTriangleGeometry::k_CopyVertexPaths_Key, std::make_any<std::vector<DataPath>>(k_CopyVertexPaths));
  args.insert(ExtractInternalSurfacesFromTriangleGeometry::k_CopyTrianglePaths_Key, std::make_any<std::vector<DataPath>>(k_CopyTrianglePaths));
  args.insert(ExtractInternalSurfacesFromTriangleGeometry::k_VertexDataName_Key, std::make_any<std::string>("Vertex Data"));
  args.insert(ExtractInternalSurfacesFromTriangleGeometry::k_FaceDataName_Key, std::make_any<std::string>("Face Data"));

  auto preflight = filter.preflight(dataGraph, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflight.outputActions);
}

TEST_CASE("ComplexCore::ExtractInternalSurfacesFromTriangleGeometry(Data)", "[ComplexCore][ExtractInternalSurfacesFromTriangleGeometry]")
{
  ExtractInternalSurfacesFromTriangleGeometry filter;
  DataStructure dataGraph = createTestData(k_TriangleGeomName, k_NodeTypesName);
  Arguments args;

  args.insert(ExtractInternalSurfacesFromTriangleGeometry::k_TriangleGeom_Key, std::make_any<DataPath>(k_TriangleGeomPath));
  args.insert(ExtractInternalSurfacesFromTriangleGeometry::k_InternalTriangleGeom_Key, std::make_any<DataPath>(k_InternalTrianglePath));
  args.insert(ExtractInternalSurfacesFromTriangleGeometry::k_NodeTypesPath_Key, std::make_any<DataPath>(k_NodeTypesPath));
  args.insert(ExtractInternalSurfacesFromTriangleGeometry::k_CopyVertexPaths_Key, std::make_any<std::vector<DataPath>>(k_CopyVertexPaths));
  args.insert(ExtractInternalSurfacesFromTriangleGeometry::k_CopyTrianglePaths_Key, std::make_any<std::vector<DataPath>>(k_CopyTrianglePaths));
  args.insert(ExtractInternalSurfacesFromTriangleGeometry::k_VertexDataName_Key, std::make_any<std::string>("Vertex Data"));
  args.insert(ExtractInternalSurfacesFromTriangleGeometry::k_FaceDataName_Key, std::make_any<std::string>("Face Data"));

  auto preflight = filter.preflight(dataGraph, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflight.outputActions);

  auto result = filter.execute(dataGraph, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  auto* newTrianglesGeom = dataGraph.getDataAs<TriangleGeom>(k_InternalTrianglePath);
  auto* oldTrianglesGeom = dataGraph.getDataAs<TriangleGeom>(k_TriangleGeomPath);

  REQUIRE(newTrianglesGeom != nullptr);
  REQUIRE(oldTrianglesGeom != nullptr);

  {
    auto* newVerticesArray = newTrianglesGeom->getVertices();
    auto* oldVerticesArray = oldTrianglesGeom->getVertices();

    REQUIRE(newVerticesArray != nullptr);
    REQUIRE(oldVerticesArray != nullptr);

    REQUIRE(newVerticesArray->getSize() == 18);
  }

  {
    auto* newTrianglesArray = dataGraph.getDataAs<IDataArray>(newTrianglesGeom->getFaceListDataArrayId());
    auto* oldTrianglesArray = dataGraph.getDataAs<IDataArray>(oldTrianglesGeom->getFaceListDataArrayId());

    REQUIRE(newTrianglesArray != nullptr);
    REQUIRE(oldTrianglesArray != nullptr);

    REQUIRE(newTrianglesArray->getNumberOfTuples() == 3);
  }
}
