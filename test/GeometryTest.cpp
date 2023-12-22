#include "GeometryTestUtilities.hpp"

#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/HexahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

////////////////////////////////////
// Begin generic geometry testing //
////////////////////////////////////
void testAbstractGeometry(IGeometry* geom)
{
  SECTION("abstract geometry")
  {
    SECTION("units")
    {
      const auto units = IGeometry::LengthUnit::Fathom;
      geom->setUnits(units);
      REQUIRE(geom->getUnits() == units);
    }
    SECTION("dimensionality")
    {
      const uint32 uDims = 20;
      geom->setUnitDimensionality(uDims);
      REQUIRE(geom->getUnitDimensionality() == uDims);

      const uint32 sDims = 14;
      geom->setSpatialDimensionality(sDims);
      REQUIRE(geom->getSpatialDimensionality() == sDims);
    }
  }
}

void testGeom2D(INodeGeometry2D* geom)
{
  SECTION("abstract geometry 2D")
  {
    const usize vertId = 2;
    const Point3D<float32> coord = {0.5f, 0.0f, 2.0f};

    // Vertices
    {
      auto vertices = createVertexList(geom);
      REQUIRE(vertices != nullptr);
      geom->setVertices(*vertices);
      REQUIRE(geom->getVertices() == vertices);
      const usize numVertices = 10;
      geom->resizeVertexList(numVertices);
      REQUIRE(geom->getNumberOfVertices() == numVertices);

      geom->setVertexCoordinate(vertId, coord);
      REQUIRE(geom->getVertexCoordinate(vertId) == coord);
    }

    // edges
    {
      auto edges = createEdgeList(geom);
      geom->setEdgeList(*edges);
      REQUIRE(geom->getEdges() == edges);
      const usize numEdges = 5;
      geom->resizeEdgeList(numEdges);
      REQUIRE(geom->getNumberOfEdges() == numEdges);
      const usize edgeId = 3;
      std::array<usize, 2> verts = {vertId, vertId + 1};
      geom->setEdgePointIds(edgeId, verts);

      std::array<Point3Df, 2> edge_verts;
      std::array<usize, 2> vertsOut = {0, 0};
      geom->getEdgePointIds(edgeId, vertsOut);
      for(usize i = 0; i < 2; i++)
      {
        REQUIRE(verts[i] == vertsOut[i]);
      }
      geom->getEdgeCoordinates(edgeId, edge_verts);
      REQUIRE(edge_verts[0] == coord);
    }
  }
}

void testGeom3D(INodeGeometry3D* geom)
{
  SECTION("abstract geometry 3D")
  {
    const usize vertId = 2;
    const Point3D<float32> coord = {0.5f, 0.0f, 2.0f};

    // vertices
    {
      auto vertices = createVertexList(geom);
      geom->setVertices(*vertices);
      REQUIRE(geom->getVertices() == vertices);
      const usize numVertices = 10;
      geom->resizeVertexList(numVertices);
      REQUIRE(geom->getNumberOfVertices() == numVertices);

      geom->setVertexCoordinate(vertId, coord);
      REQUIRE(geom->getVertexCoordinate(vertId) == coord);
    }
    // edges
    {
      auto edges = createEdgeList(geom);
      geom->setEdgeList(*edges);
      REQUIRE(geom->getEdges() == edges);
      const usize numEdges = 5;
      geom->resizeEdgeList(numEdges);
      REQUIRE(geom->getNumberOfEdges() == numEdges);
      const usize edgeId = 3;
      std::array<usize, 2> verts = {vertId, vertId + 1};
      geom->setEdgePointIds(edgeId, verts);
      usize vertsOut[2];
      geom->getEdgePointIds(edgeId, vertsOut);
      for(usize i = 0; i < 2; i++)
      {
        REQUIRE(verts[i] == vertsOut[i]);
      }
      std::array<Point3Df, 2> edge_verts;

      geom->getEdgeCoordinates(edgeId, edge_verts);
      REQUIRE(edge_verts[0] == coord);
    }
    // faces
    {
    }
  }
}

void testGeomGrid(IGridGeometry* geom)
{
  SECTION("abstract geometry grid")
  {
    const usize xDim = 10;
    const usize yDim = 150;
    const usize zDim = 50;
    const SizeVec3 dims = {xDim, yDim, zDim};
    geom->setDimensions(dims);
    REQUIRE(geom->getDimensions() == dims);

    REQUIRE(geom->getNumXCells() == xDim);
    REQUIRE(geom->getNumYCells() == yDim);
    REQUIRE(geom->getNumZCells() == zDim);
  }
}

/////////////////////////////////////
// Begin geometry-specific testing //
/////////////////////////////////////
TEST_CASE("EdgeGeomTest")
{
  DataStructure dataStructure;
  auto geom = createGeom<EdgeGeom>(dataStructure);

  testAbstractGeometry(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getTypeName() == "EdgeGeom");
  }
}

TEST_CASE("HexahedralGeomTest")
{
  DataStructure dataStructure;
  auto geom = createGeom<HexahedralGeom>(dataStructure);

  testGeom3D(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getTypeName() == "HexahedralGeom");
  }
}

TEST_CASE("ImageGeomTest")
{
  DataStructure dataStructure;
  auto geom = createGeom<ImageGeom>(dataStructure);

  testGeomGrid(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getTypeName() == "ImageGeom");
  }
}

TEST_CASE("QuadGeomTest")
{
  DataStructure dataStructure;
  auto geom = createGeom<QuadGeom>(dataStructure);

  testGeom2D(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getTypeName() == "QuadGeom");
  }
}

TEST_CASE("RectGridGeomTest")
{
  DataStructure dataStructure;
  auto geom = createGeom<RectGridGeom>(dataStructure);

  testGeomGrid(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getTypeName() == "RectGridGeom");
  }
}

TEST_CASE("TetrahedralGeomTest")
{
  DataStructure dataStructure;
  auto geom = createGeom<TetrahedralGeom>(dataStructure);

  testGeom3D(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getTypeName() == "TetrahedralGeom");
  }
}

TEST_CASE("TriangleGeomTest")
{
  DataStructure dataStructure;
  auto geom = createGeom<TriangleGeom>(dataStructure);

  testGeom2D(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getTypeName() == "TriangleGeom");
  }
}

TEST_CASE("VertexGeomTest")
{
  DataStructure dataStructure;
  auto geom = createGeom<VertexGeom>(dataStructure);

  testAbstractGeometry(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getTypeName() == "VertexGeom");
  }
}
