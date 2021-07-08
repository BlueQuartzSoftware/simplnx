#include <catch2/catch.hpp>

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/EdgeGeom.hpp"
#include "complex/DataStructure/Geometry/HexahedralGeom.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Geometry/QuadGeom.hpp"
#include "complex/DataStructure/Geometry/RectGridGeom.hpp"
#include "complex/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"

using namespace complex;

template <typename T>
T* createGeom(DataStructure& ds)
{
  auto geom = ds.createGeometry<T>("Geom");
  T* output = dynamic_cast<T*>(geom);
  REQUIRE(nullptr != output);
  return output;
}

const AbstractGeometry::SharedVertexList* createVertexList(AbstractGeometry* geom)
{
  auto ds = geom->getDataStructure();
  auto dataStore = new DataStore<float>(3, 0);
  auto dataArr = ds->createDataArray("Vertices", dataStore, geom->getId());
  return dynamic_cast<const AbstractGeometry::SharedVertexList*>(dataArr);
}

const AbstractGeometry::SharedEdgeList* createEdgeList(AbstractGeometry* geom)
{
  auto ds = geom->getDataStructure();
  auto dataStore = new DataStore<size_t>(2, 0);
  auto dataArr = ds->createDataArray("Edges", dataStore, geom->getId());
  return dynamic_cast<const AbstractGeometry::SharedEdgeList*>(dataArr);
}

const AbstractGeometry::SharedFaceList* createFaceList(AbstractGeometry* geom)
{
  auto ds = geom->getDataStructure();
  auto dataStore = new DataStore<size_t>(4, 0);
  auto dataArr = ds->createDataArray("Faces", dataStore, geom->getId());
  return dynamic_cast<const AbstractGeometry::SharedFaceList*>(dataArr);
}

////////////////////////////////////
// Begin generic geometry testing //
////////////////////////////////////
void testAbstractGeometry(AbstractGeometry* geom)
{
  SECTION("abstract geometry")
  {
    SECTION("units")
    {
      const auto units = AbstractGeometry::LengthUnit::Fathom;
      geom->setUnits(units);
      REQUIRE(geom->getUnits() == units);
    }
    SECTION("time series")
    {
      geom->setTimeSeriesEnabled(false);
      REQUIRE(geom->getTimeSeriesEnabled() == false);
      geom->setTimeSeriesEnabled(true);
      REQUIRE(geom->getTimeSeriesEnabled() == true);
    }
    SECTION("time value")
    {
      const float timeValue = 6.4f;
      geom->setTimeValue(timeValue);
      REQUIRE(geom->getTimeValue() == timeValue);
    }
    SECTION("dimensionality")
    {
      const uint32_t uDims = 20;
      geom->setUnitDimensionality(uDims);
      REQUIRE(geom->getUnitDimensionality() == uDims);

      const uint32_t sDims = 14;
      geom->setSpatialDimensionality(sDims);
      REQUIRE(geom->getSpatialDimensionality() == sDims);
    }

    SECTION("info string")
    {
      REQUIRE(geom->getInfoString(InfoStringFormat::HtmlFormat) == geom->getTooltipGenerator().generateHTML());
    }
  }
}

void testGeom2D(AbstractGeometry2D* geom)
{
  SECTION("abstract geometry 2D")
  {
    const size_t vertId = 2;
    const Point3D<float> coord = {0.5f, 0.0f, 2.0f};

    // Vertices
    {
      auto vertices = createVertexList(geom);
      geom->setVertices(vertices);
      REQUIRE(geom->getVertices() == vertices);
      const size_t numVertices = 10;
      geom->resizeVertexList(numVertices);
      REQUIRE(geom->getNumberOfVertices() == numVertices);

      geom->setCoords(vertId, coord);
      REQUIRE(geom->getCoords(vertId) == coord);
    }

    // edges
    {
      auto edges = createEdgeList(geom);
      geom->setEdges(edges);
      REQUIRE(geom->getEdges() == edges);
      const size_t numEdges = 5;
      geom->resizeEdgeList(numEdges);
      REQUIRE(geom->getNumberOfEdges() == numEdges);
      const size_t edgeId = 3;
      const size_t verts[2] = {vertId, vertId + 1};
      geom->setVertsAtEdge(edgeId, verts);
      Point3D<float> vert1, vert2;
      size_t vertsOut[2];
      geom->getVertsAtEdge(edgeId, vertsOut);
      for(size_t i = 0; i < 2; i++)
      {
        REQUIRE(verts[i] == vertsOut[i]);
      }
      geom->getVertCoordsAtEdge(edgeId, vert1, vert2);
      REQUIRE(vert1 == coord);
    }
  }
}

void testGeom3D(AbstractGeometry3D* geom)
{
  SECTION("abstract geometry 3D")
  {
    const size_t vertId = 2;
    const Point3D<float> coord = {0.5f, 0.0f, 2.0f};

    // vertices
    {
      auto vertices = createVertexList(geom);
      geom->setVertices(vertices);
      REQUIRE(geom->getVertices() == vertices);
      const size_t numVertices = 10;
      geom->resizeVertexList(numVertices);
      REQUIRE(geom->getNumberOfVertices() == numVertices);

      geom->setCoords(vertId, coord);
      REQUIRE(geom->getCoords(vertId) == coord);
    }
    // edges
    {
      auto edges = createEdgeList(geom);
      geom->setEdges(edges);
      REQUIRE(geom->getEdges() == edges);
      const size_t numEdges = 5;
      geom->resizeEdgeList(numEdges);
      REQUIRE(geom->getNumberOfEdges() == numEdges);
      const size_t edgeId = 3;
      const size_t verts[2] = {vertId, vertId + 1};
      geom->setVertsAtEdge(edgeId, verts);
      Point3D<float> vert1, vert2;
      size_t vertsOut[2];
      geom->getVertsAtEdge(edgeId, vertsOut);
      for(size_t i = 0; i < 2; i++)
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

void testGeomGrid(AbstractGeometryGrid* geom)
{
  SECTION("abstract geometry grid")
  {
    const size_t xDim = 10;
    const size_t yDim = 150;
    const size_t zDim = 50;
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
    REQUIRE(geom->getGeometryTypeAsString() == "EdgeGeom");
  }
}

TEST_CASE("HexahedralGeomTest")
{
  DataStructure ds;
  auto geom = createGeom<HexahedralGeom>(ds);

  testGeom3D(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getGeometryTypeAsString() == "HexahedralGeom");
  }
}

TEST_CASE("ImageGeomTest")
{
  DataStructure ds;
  auto geom = createGeom<ImageGeom>(ds);

  testGeomGrid(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getGeometryTypeAsString() == "ImageGeom");
  }
}

TEST_CASE("QuadGeomTest")
{
  DataStructure ds;
  auto geom = createGeom<QuadGeom>(ds);

  testGeom2D(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getGeometryTypeAsString() == "QuadGeom");
  }
}

TEST_CASE("RectGridGeomTest")
{
  DataStructure ds;
  auto geom = createGeom<RectGridGeom>(ds);

  testGeomGrid(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getGeometryTypeAsString() == "RectGridGeom");
  }
}

TEST_CASE("TetrahedralGeomTest")
{
  DataStructure ds;
  auto geom = createGeom<TetrahedralGeom>(ds);

  testGeom3D(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getGeometryTypeAsString() == "TetrahedralGeom");
  }
}

TEST_CASE("TriangleGeomTest")
{
  DataStructure ds;
  auto geom = createGeom<TriangleGeom>(ds);

  testGeom2D(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getGeometryTypeAsString() == "TriangleGeom");
  }
}

TEST_CASE("VertexGeomTest")
{
  DataStructure ds;
  auto geom = createGeom<VertexGeom>(ds);

  testAbstractGeometry(geom);

  SECTION("type as string")
  {
    REQUIRE(geom->getGeometryTypeAsString() == "VertexGeom");
  }
}
