#include <exception>
#include <iostream>

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

  if(!dynamic_cast<T*>(geom))
  {
    throw std::exception();
  }
  return dynamic_cast<T*>(geom);
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
  std::cout << "  Testing units..." << std::endl;
  auto units = AbstractGeometry::LengthUnit::Fathom;
  geom->setUnits(units);
  if(geom->getUnits() != units)
  {
    throw std::exception();
  }

  std::cout << "  Testing time series..." << std::endl;
  geom->setTimeSeriesEnabled(false);
  if(geom->getTimeSeriesEnabled() != false)
  {
    throw std::exception();
  }
  geom->setTimeSeriesEnabled(true);
  if(geom->getTimeSeriesEnabled() != true)
  {
    throw std::exception();
  }

  std::cout << "  Testing time values..." << std::endl;
  const float timeValue = 6.4f;
  geom->setTimeValue(timeValue);
  if(geom->getTimeValue() != timeValue)
  {
    throw std::exception();
  }

  std::cout << "  Testing dimensionality..." << std::endl;
  const uint32_t uDims = 20;
  geom->setUnitDimensionality(uDims);
  if(geom->getUnitDimensionality() != uDims)
  {
    throw std::exception();
  }

  const uint32_t sDims = 14;
  geom->setSpatialDimensionality(sDims);
  if(geom->getSpatialDimensionality() != sDims)
  {
    throw std::exception();
  }

  if(geom->getInfoString(InfoStringFormat::HtmlFormat) != geom->getTooltipGenerator().generateHTML())
  {
    throw std::exception();
  }
}

void testGeom2D(AbstractGeometry2D* geom)
{
  std::cout << "  Testing vertices..." << std::endl;
  auto vertices = createVertexList(geom);
  geom->setVertices(vertices);
  if(geom->getVertices() != vertices)
  {
    throw std::exception();
  }
  const size_t numVertices = 10;
  geom->resizeVertexList(numVertices);
  if(geom->getNumberOfVertices() != numVertices)
  {
    throw std::exception();
  }

  const size_t vertId = 2;
  const Point3D<float> coord = {0.5f, 0.0f, 2.0f};
  geom->setCoords(vertId, coord);
  if(geom->getCoords(vertId) != coord)
  {
    throw std::exception();
  }

  std::cout << "  Testing edges..." << std::endl;
  auto edges = createEdgeList(geom);
  geom->setEdges(edges);
  if(geom->getEdges() != edges)
  {
    throw std::exception();
  }
  const size_t numEdges = 5;
  geom->resizeEdgeList(numEdges);
  if(geom->getNumberOfEdges() != numEdges)
  {
    throw std::exception();
  }
  const size_t edgeId = 3;
  const size_t verts[2] = {vertId, vertId + 1};
  geom->setVertsAtEdge(edgeId, verts);
  Point3D<float> vert1, vert2;
  size_t vertsOut[2];
  geom->getVertsAtEdge(edgeId, vertsOut);
  for(size_t i = 0; i < 2; i++)
  {
    if(verts[i] != vertsOut[i])
    {
      throw std::exception();
    }
  }
  geom->getVertCoordsAtEdge(edgeId, vert1, vert2);
  if(vert1 != coord)
  {
    throw std::exception();
  }
}

void testGeom3D(AbstractGeometry3D* geom)
{
  std::cout << "  Testing vertices..." << std::endl;
  auto vertices = createVertexList(geom);
  geom->setVertices(vertices);
  if(geom->getVertices() != vertices)
  {
    throw std::exception();
  }
  const size_t numVertices = 10;
  geom->resizeVertexList(numVertices);
  if(geom->getNumberOfVertices() != numVertices)
  {
    throw std::exception();
  }

  const size_t vertId = 2;
  const Point3D<float> coord = {0.5f, 0.0f, 2.0f};
  geom->setCoords(vertId, coord);
  if(geom->getCoords(vertId) != coord)
  {
    throw std::exception();
  }

  std::cout << "  Testing edges..." << std::endl;
  auto edges = createEdgeList(geom);
  geom->setEdges(edges);
  if(geom->getEdges() != edges)
  {
    throw std::exception();
  }
  const size_t numEdges = 5;
  geom->resizeEdgeList(numEdges);
  if(geom->getNumberOfEdges() != numEdges)
  {
    throw std::exception();
  }
  const size_t edgeId = 3;
  const size_t verts[2] = {vertId, vertId + 1};
  geom->setVertsAtEdge(edgeId, verts);
  Point3D<float> vert1, vert2;
  size_t vertsOut[2];
  geom->getVertsAtEdge(edgeId, vertsOut);
  for(size_t i = 0; i < 2; i++)
  {
    if(verts[i] != vertsOut[i])
    {
      throw std::exception();
    }
  }
  geom->getVertCoordsAtEdge(edgeId, vert1, vert2);
  if(vert1 != coord)
  {
    throw std::exception();
  }

  // Faces
}

void testGeomGrid(AbstractGeometryGrid* geom)
{
  std::cout << "  Testing dimensions..." << std::endl;
  const size_t xDim = 10;
  const size_t yDim = 150;
  const size_t zDim = 50;
  const SizeVec3 dims = {xDim, yDim, zDim};
  geom->setDimensions(dims);
  if(geom->getDimensions() != dims)
  {
    throw std::exception();
  }

  if(geom->getNumXPoints() != xDim)
  {
    throw std::exception();
  }
  if(geom->getNumYPoints() != yDim)
  {
    throw std::exception();
  }
  if(geom->getNumZPoints() != zDim)
  {
    throw std::exception();
  }
}

/////////////////////////////////////
// Begin geometry-specific testing //
/////////////////////////////////////
void edgeGeomTests()
{
  std::cout << "edgeGeomTests()..." << std::endl;
  std::cout << "  Creating EdgeGeom..." << std::endl;

  DataStructure ds;
  auto geom = createGeom<EdgeGeom>(ds);

  testAbstractGeometry(geom);

  std::cout << "  Checking Type as String..." << std::endl;

  if(geom->getGeometryTypeAsString() != "EdgeGeom")
  {
    throw std::exception();
  }
}

void hexahedralGeomTests()
{
  std::cout << "hexahedralGeomTests()..." << std::endl;
  std::cout << "  Creating HexahedralGeom..." << std::endl;

  DataStructure ds;
  auto geom = createGeom<HexahedralGeom>(ds);

  std::cout << "  Checking Type as String..." << std::endl;

  if(geom->getGeometryTypeAsString() != "HexahedralGeom")
  {
    throw std::exception();
  }

  testGeom3D(geom);
}

void imageGeomTests()
{
  std::cout << "imageGeomTests()..." << std::endl;
  std::cout << "  Creating ImageGeom..." << std::endl;

  DataStructure ds;
  auto geom = createGeom<ImageGeom>(ds);

  std::cout << "  Checking Type as String..." << std::endl;

  if(geom->getGeometryTypeAsString() != "ImageGeom")
  {
    throw std::exception();
  }

  testGeomGrid(geom);
}

void quadGeomTests()
{
  std::cout << "quadGeomTests()..." << std::endl;
  std::cout << "  Creating QuadGeom..." << std::endl;

  DataStructure ds;
  auto geom = createGeom<QuadGeom>(ds);

  std::cout << "  Checking Type as String..." << std::endl;

  if(geom->getGeometryTypeAsString() != "QuadGeom")
  {
    throw std::exception();
  }

  testGeom2D(geom);
}

void rectGridGeomTests()
{
  std::cout << "rectGridGeomTests()..." << std::endl;
  std::cout << "  Creating RectGridGeom..." << std::endl;

  DataStructure ds;
  auto geom = createGeom<RectGridGeom>(ds);

  std::cout << "  Checking Type as String..." << std::endl;

  if(geom->getGeometryTypeAsString() != "RectGridGeom")
  {
    throw std::exception();
  }

  testGeomGrid(geom);
}

void tetrahedralGeomTest()
{
  std::cout << "tetrahedralGeomTest()..." << std::endl;
  std::cout << "  Creating TetrahedralGeom..." << std::endl;

  DataStructure ds;
  auto geom = createGeom<TetrahedralGeom>(ds);

  std::cout << "  Checking Type as String..." << std::endl;

  if(geom->getGeometryTypeAsString() != "TetrahedralGeom")
  {
    throw std::exception();
  }

  testGeom3D(geom);
}

void triangleGeomTests()
{
  std::cout << "triangleGeomTests()..." << std::endl;
  std::cout << "  Creating TriangleGeom..." << std::endl;

  DataStructure ds;
  auto geom = createGeom<TriangleGeom>(ds);

  std::cout << "  Checking Type as String..." << std::endl;

  if(geom->getGeometryTypeAsString() != "TriangleGeom")
  {
    throw std::exception();
  }

  testGeom2D(geom);
}

void vertexGeomTests()
{
  std::cout << "vertexGeomTests()..." << std::endl;
  std::cout << "  Creating VertexGeom..." << std::endl;

  DataStructure ds;
  auto geom = createGeom<VertexGeom>(ds);

  std::cout << "  Checking Type as String..." << std::endl;

  if(geom->getGeometryTypeAsString() != "VertexGeom")
  {
    throw std::exception();
  }
}

int main(int argc, char** argv)
{
  edgeGeomTests();
  hexahedralGeomTests();
  imageGeomTests();
  quadGeomTests();
  rectGridGeomTests();
  tetrahedralGeomTest();
  triangleGeomTests();
  vertexGeomTests();
}
