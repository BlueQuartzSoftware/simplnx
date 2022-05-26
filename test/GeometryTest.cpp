#include <catch2/catch.hpp>

#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/EdgeGeom.hpp"
#include "complex/DataStructure/Geometry/HexahedralGeom.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Geometry/QuadGeom.hpp"
#include "complex/DataStructure/Geometry/RectGridGeom.hpp"
#include "complex/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"

#include "GeometryTestUtilities.hpp"

using namespace complex;

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

      geom->setCoords(vertId, coord);
      REQUIRE(geom->getCoords(vertId) == coord);
    }

    // edges
    {
      auto edges = createEdgeList(geom);
      geom->setEdges(*edges);
      REQUIRE(geom->getEdges() == edges);
      const usize numEdges = 5;
      geom->resizeEdgeList(numEdges);
      REQUIRE(geom->getNumberOfEdges() == numEdges);
      const usize edgeId = 3;
      const usize verts[2] = {vertId, vertId + 1};
      geom->setVertsAtEdge(edgeId, verts);
      Point3D<float32> vert1, vert2;
      usize vertsOut[2];
      geom->getVertsAtEdge(edgeId, vertsOut);
      for(usize i = 0; i < 2; i++)
      {
        REQUIRE(verts[i] == vertsOut[i]);
      }
      geom->getVertCoordsAtEdge(edgeId, vert1, vert2);
      REQUIRE(vert1 == coord);
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

      geom->setCoords(vertId, coord);
      REQUIRE(geom->getCoords(vertId) == coord);
    }
    // edges
    {
      auto edges = createEdgeList(geom);
      geom->setEdges(*edges);
      REQUIRE(geom->getEdges() == edges);
      const usize numEdges = 5;
      geom->resizeEdgeList(numEdges);
      REQUIRE(geom->getNumberOfEdges() == numEdges);
      const usize edgeId = 3;
      const usize verts[2] = {vertId, vertId + 1};
      geom->setVertsAtEdge(edgeId, verts);
      Point3D<float32> vert1, vert2;
      usize vertsOut[2];
      geom->getVertsAtEdge(edgeId, vertsOut);
      for(usize i = 0; i < 2; i++)
      {
        REQUIRE(verts[i] == vertsOut[i]);
      }
      geom->getVertCoordsAtEdge(edgeId, vert1, vert2);
      REQUIRE(vert1 == coord);
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

    REQUIRE(geom->getNumXPoints() == xDim);
    REQUIRE(geom->getNumYPoints() == yDim);
    REQUIRE(geom->getNumZPoints() == zDim);
  }
}

/////////////////////////////////////
// Begin geometry-specific testing //
/////////////////////////////////////
TEST_CASE("EdgeGeomTest")
{
  DataStructure ds;
  auto geom = createGeom<EdgeGeom>(ds);

  testAbstractGeometry(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getTypeName() == "EdgeGeom");
  }
}

TEST_CASE("HexahedralGeomTest")
{
  DataStructure ds;
  auto geom = createGeom<HexahedralGeom>(ds);

  testGeom3D(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getTypeName() == "HexahedralGeom");
  }
}

TEST_CASE("ImageGeomTest")
{
  DataStructure ds;
  auto geom = createGeom<ImageGeom>(ds);

  testGeomGrid(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getTypeName() == "ImageGeom");
  }
}

TEST_CASE("QuadGeomTest")
{
  DataStructure ds;
  auto geom = createGeom<QuadGeom>(ds);

  testGeom2D(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getTypeName() == "QuadGeom");
  }
}

TEST_CASE("RectGridGeomTest")
{
  DataStructure ds;
  auto geom = createGeom<RectGridGeom>(ds);

  testGeomGrid(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getTypeName() == "RectGridGeom");
  }
}

TEST_CASE("TetrahedralGeomTest")
{
  DataStructure ds;
  auto geom = createGeom<TetrahedralGeom>(ds);

  testGeom3D(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getTypeName() == "TetrahedralGeom");
  }
}

TEST_CASE("TriangleGeomTest")
{
  DataStructure ds;
  auto geom = createGeom<TriangleGeom>(ds);

  testGeom2D(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getTypeName() == "TriangleGeom");
  }
}

TEST_CASE("VertexGeomTest")
{
  DataStructure ds;
  auto geom = createGeom<VertexGeom>(ds);

  testAbstractGeometry(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getTypeName() == "VertexGeom");
  }
}
