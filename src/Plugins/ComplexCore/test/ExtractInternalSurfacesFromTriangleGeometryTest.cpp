#include <catch2/catch.hpp>

#include "ComplexCore/Filters/ExtractInternalSurfacesFromTriangleGeometry.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

using namespace complex;

TEST_CASE("ExtractInternalSurfacesFromTriangleGeometry(Instantiate)", "[ComplexCore][ExtractInternalSurfacesFromTriangleGeometry]")
{
  constexpr StringLiteral k_TriangleGeomName = "TriangleGeom";
  constexpr StringLiteral k_InternalTriangleGeomName = "Internal TriangleGeom";
  constexpr StringLiteral k_NodeTypesName = "Node Types";

  const DataPath k_TriangleGeomPath({k_TriangleGeomName});
  const DataPath k_InternalTrianglePath({k_InternalTriangleGeomName});
  const DataPath k_NodeTypesPath({k_NodeTypesName});
  const std::vector<DataPath> k_CopyVertexPaths({DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ConfidenceIndex})});
  const std::vector<DataPath> k_CopyTrianglePaths({DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "Phases"})});

  ExtractInternalSurfacesFromTriangleGeometry filter;
  DataStructure ds = UnitTest::CreateDataStructure();
  Arguments args;

  const DataPath vertexListPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_ConfidenceIndex});
  auto* triangleList = UInt64Array::CreateWithStore<UInt64DataStore>(ds, "Tri List", {500}, {3});
  triangleList->fill(1);
  auto* triangleGeom = TriangleGeom::Create(ds, k_TriangleGeomName);
  triangleGeom->setVertices(ds.getDataAs<AbstractGeometry::SharedVertexList>(vertexListPath));
  triangleGeom->setFaces(triangleList);
  Int8Array::CreateWithStore<Int8DataStore>(ds, k_NodeTypesName, {triangleGeom->getNumberOfFaces()}, {3});

  args.insert(ExtractInternalSurfacesFromTriangleGeometry::k_TriangleGeom_Key, std::make_any<DataPath>(k_TriangleGeomPath));
  args.insert(ExtractInternalSurfacesFromTriangleGeometry::k_InternalTriangleGeom_Key, std::make_any<DataPath>(k_InternalTrianglePath));
  args.insert(ExtractInternalSurfacesFromTriangleGeometry::k_NodeTypesPath_Key, std::make_any<DataPath>(k_NodeTypesPath));
  args.insert(ExtractInternalSurfacesFromTriangleGeometry::k_CopyVertexPaths_Key, std::make_any<std::vector<DataPath>>(k_CopyVertexPaths));
  args.insert(ExtractInternalSurfacesFromTriangleGeometry::k_CopyTrianglePaths_Key, std::make_any<std::vector<DataPath>>(k_CopyTrianglePaths));

  auto preflight = filter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflight.outputActions);

  auto result = filter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);
}