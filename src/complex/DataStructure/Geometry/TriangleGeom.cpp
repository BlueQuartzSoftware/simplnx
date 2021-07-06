#include "TriangleGeom.hpp"

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/DynamicListArray.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"

using namespace complex;

TriangleGeom::TriangleGeom(DataStructure* ds, const std::string& name)
: AbstractGeometry2D(ds, name)
{
}

TriangleGeom::TriangleGeom(DataStructure* ds, const std::string& name, size_t numTriangles, const SharedVertexList* vertices, bool allocate)
: AbstractGeometry2D(ds, name)
{
}

TriangleGeom::TriangleGeom(DataStructure* ds, const std::string& name, const SharedTriList* triangles, const SharedVertexList* vertices)
: AbstractGeometry2D(ds, name)
{
}

TriangleGeom::TriangleGeom(const TriangleGeom& other)
: AbstractGeometry2D(other)
, m_VertexListId(other.m_VertexListId)
, m_EdgeListId(other.m_EdgeListId)
, m_UnsharedEdgeListId(other.m_UnsharedEdgeListId)
, m_TriListId(other.m_TriListId)
, m_TrianglesContainingVertId(other.m_TrianglesContainingVertId)
, m_TriangleNeighborsId(other.m_TriangleNeighborsId)
, m_TriangleCentroidsId(other.m_TriangleCentroidsId)
, m_TriangleSizesId(other.m_TriangleSizesId)
{
}

TriangleGeom::TriangleGeom(TriangleGeom&& other) noexcept
: AbstractGeometry2D(std::move(other))
, m_VertexListId(std::move(other.m_VertexListId))
, m_EdgeListId(std::move(other.m_EdgeListId))
, m_UnsharedEdgeListId(std::move(other.m_UnsharedEdgeListId))
, m_TriListId(std::move(other.m_TriListId))
, m_TrianglesContainingVertId(std::move(other.m_TrianglesContainingVertId))
, m_TriangleNeighborsId(std::move(other.m_TriangleNeighborsId))
, m_TriangleCentroidsId(std::move(other.m_TriangleCentroidsId))
, m_TriangleSizesId(std::move(other.m_TriangleSizesId))
{
}

TriangleGeom::~TriangleGeom() = default;

DataObject* TriangleGeom::shallowCopy()
{
  return new TriangleGeom(*this);
}

DataObject* TriangleGeom::deepCopy()
{
  throw std::exception();
}

std::string TriangleGeom::getGeometryTypeAsString() const
{
  return "TriangleGeom";
}

void TriangleGeom::resizeTriList(size_t newNumTris)
{
  getTriangles()->getDataStore()->resizeTuples(newNumTris);
}

void TriangleGeom::setTriangles(const SharedTriList* triangles)
{
  if(!triangles)
  {
    m_TriListId.reset();
    return;
  }
  m_TriListId = triangles->getId();
}

AbstractGeometry::SharedTriList* TriangleGeom::getTriangles()
{
  return dynamic_cast<SharedTriList*>(getDataStructure()->getData(m_TriListId));
}

const AbstractGeometry::SharedTriList* TriangleGeom::getTriangles() const
{
  return dynamic_cast<const SharedTriList*>(getDataStructure()->getData(m_TriListId));
}

void TriangleGeom::setVertsAtTri(size_t triId, size_t verts[3])
{
  auto tris = getTriangles();
  if(!tris)
  {
    return;
  }
  for(size_t i = 0; i < 3; i++)
  {
    (*tris)[i] = verts[i];
  }
}

void TriangleGeom::getVertsAtTri(size_t triId, size_t verts[3]) const
{
  auto tris = getTriangles();
  if(!tris)
  {
    return;
  }
  for(size_t i = 0; i < 3; i++)
  {
    verts[i] = tris->at(i);
  }
}

void TriangleGeom::getVertCoordsAtTri(size_t triId, Point3D<float>& vert1, Point3D<float>& vert2, Point3D<float>& vert3) const
{
  if(!getTriangles())
  {
    return;
  }
  if(!getVertices())
  {
    return;
  }
  size_t verts[3];
  getVertsAtTri(triId, verts);
  vert1 = getCoords(verts[0]);
  vert2 = getCoords(verts[1]);
  vert3 = getCoords(verts[2]);
}

size_t TriangleGeom::getNumberOfTris() const
{
  auto tris = getTriangles();
  if(!tris)
  {
    return 0;
  }
  return tris->getTupleCount();
}

void TriangleGeom::initializeWithZeros()
{
  getVertices()->getDataStore()->fill(0.0f);
  getTriangles()->getDataStore()->fill(0);
}

size_t TriangleGeom::getNumberOfElements() const
{
  return getTriangles()->getTupleCount();
}

AbstractGeometry::StatusCode TriangleGeom::findElementSizes()
{
  auto dataStore = new DataStore<float>(1, getNumberOfTris());
  auto triangleSizes = getDataStructure()->createDataArray<float>("Triangle Areas", dataStore, getId());
  GeometryHelpers::Topology::Find2DElementAreas<size_t>(getTriangles(), getVertices(), triangleSizes);
  if(triangleSizes == nullptr)
  {
    m_TriangleSizesId.reset();
    return -1;
  }
  m_TriangleSizesId = triangleSizes->getId();
  return 1;
}

const FloatArray* TriangleGeom::getElementSizes() const
{
  return dynamic_cast<const FloatArray*>(getDataStructure()->getData(m_TriangleSizesId));
}

void TriangleGeom::deleteElementSizes()
{
  getDataStructure()->removeData(m_TriangleSizesId);
  m_TriangleSizesId.reset();
}

AbstractGeometry::StatusCode TriangleGeom::findElementsContainingVert()
{
  auto trianglesContainingVert = getDataStructure()->createDynamicList<uint16_t, MeshIndexType>("Triangles Containing Vert", getId());
  GeometryHelpers::Connectivity::FindElementsContainingVert<uint16_t, MeshIndexType>(getTriangles(), trianglesContainingVert, getNumberOfVertices());
  if(trianglesContainingVert == nullptr)
  {
    m_TrianglesContainingVertId.reset();
    return -1;
  }
  m_TrianglesContainingVertId = trianglesContainingVert->getId();
  return 1;
}

const AbstractGeometry::ElementDynamicList* TriangleGeom::getElementsContainingVert() const
{
  return dynamic_cast<const ElementDynamicList*>(getDataStructure()->getData(m_TrianglesContainingVertId));
}

void TriangleGeom::deleteElementsContainingVert()
{
  getDataStructure()->removeData(m_TrianglesContainingVertId);
  m_TrianglesContainingVertId.reset();
}

AbstractGeometry::StatusCode TriangleGeom::findElementNeighbors()
{
  StatusCode err = 0;
  if(getElementsContainingVert() == nullptr)
  {
    err = findElementsContainingVert();
    if(err < 0)
    {
      return err;
    }
  }
  auto triangleNeighbors = getDataStructure()->createDynamicList<uint16_t, MeshIndexType>("Triangle Neighbors", getId());
  err = GeometryHelpers::Connectivity::FindElementNeighbors<uint16_t, MeshIndexType>(getTriangles(), getElementsContainingVert(), triangleNeighbors, AbstractGeometry::Type::Triangle);
  if(triangleNeighbors == nullptr)
  {
    m_TriangleNeighborsId.reset();
    return -1;
  }
  m_TriangleNeighborsId = triangleNeighbors->getId();
  return err;
}

const AbstractGeometry::ElementDynamicList* TriangleGeom::getElementNeighbors() const
{
  return dynamic_cast<const ElementDynamicList*>(getDataStructure()->getData(m_TriangleNeighborsId));
}

void TriangleGeom::deleteElementNeighbors()
{
  getDataStructure()->removeData(m_TriangleNeighborsId);
  m_TriangleNeighborsId.reset();
}

AbstractGeometry::StatusCode TriangleGeom::findElementCentroids()
{
  auto dataStore = new DataStore<float>(3, getNumberOfTris());
  auto triangleCentroids = getDataStructure()->createDataArray<float>("Triangle Centroids", dataStore, getId());
  GeometryHelpers::Topology::FindElementCentroids<size_t>(getTriangles(), getVertices(), triangleCentroids);
  if(triangleCentroids == nullptr)
  {
    m_TriangleCentroidsId.reset();
    return -1;
  }
  m_TriangleCentroidsId = triangleCentroids->getId();
  return 1;
}

const FloatArray* TriangleGeom::getElementCentroids() const
{
  return dynamic_cast<const FloatArray*>(getDataStructure()->getData(m_TriangleCentroidsId));
}

void TriangleGeom::deleteElementCentroids()
{
  getDataStructure()->removeData(m_TriangleCentroidsId);
  m_TriangleCentroidsId.reset();
}

complex::Point3D<double> TriangleGeom::getParametricCenter() const
{
  return {1.0 / 3.0, 1.0 / 3.0, 0.0};
}

void TriangleGeom::getShapeFunctions(const complex::Point3D<double>& pCoords, double* shape) const
{
  (void)pCoords;

  // r derivatives
  shape[0] = -1.0;
  shape[1] = 1.0;
  shape[2] = 0.0;

  // s derivatives
  shape[3] = -1.0;
  shape[4] = 0.0;
  shape[5] = 1.0;
}

void TriangleGeom::findDerivatives(DoubleArray* field, DoubleArray* derivatives, Observable* observable) const
{
  throw std::exception();
}

complex::TooltipGenerator TriangleGeom::getTooltipGenerator() const
{
  TooltipGenerator toolTipGen;
  toolTipGen.addTitle("Geometry Info");
  toolTipGen.addValue("Type", "TriangleGeom");
  toolTipGen.addValue("Units", LengthUnitToString(getUnits()));
  toolTipGen.addValue("Number of Triangles", std::to_string(getNumberOfTris()));
  toolTipGen.addValue("Number of Vertices", std::to_string(getNumberOfVertices()));

  return toolTipGen;
}

void TriangleGeom::resizeVertexList(size_t newNumVertices)
{
  getVertices()->getDataStore()->resizeTuples(newNumVertices);
}

void TriangleGeom::setVertices(const SharedVertexList* vertices)
{
  if(!vertices)
  {
    m_VertexListId.reset();
    return;
  }
  m_VertexListId = vertices->getId();
}

AbstractGeometry::SharedVertexList* TriangleGeom::getVertices()
{
  return dynamic_cast<SharedVertexList*>(getDataStructure()->getData(m_VertexListId));
}

const AbstractGeometry::SharedVertexList* TriangleGeom::getVertices() const
{
  return dynamic_cast<const SharedVertexList*>(getDataStructure()->getData(m_VertexListId));
}

void TriangleGeom::setCoords(size_t vertId, const Point3D<float>& coords)
{
  auto vertices = getVertices();
  if(!vertices)
  {
    return;
  }
  const size_t offset = vertId * 3;
  for(size_t i = 0; i < 3; i++)
  {
    (*vertices)[offset + i] = coords[i];
  }
}

Point3D<float> TriangleGeom::getCoords(size_t vertId) const
{
  auto vertices = getVertices();
  if(!vertices)
  {
    return {};
  }
  const size_t offset = vertId * 3;
  Point3D<float> coords;
  for(size_t i = 0; i < 3; i++)
  {
    coords[i] = vertices->at(offset + i);
  }
  return coords;
}

size_t TriangleGeom::getNumberOfVertices() const
{
  auto vertices = getVertices();
  if(!vertices)
  {
    return 0;
  }
  return vertices->getTupleCount();
}

AbstractGeometry::StatusCode TriangleGeom::findEdges()
{
  auto dataStore = new DataStore<uint64_t>(2, 0);
  auto edgeList = getDataStructure()->createDataArray<uint64_t>("Edge List", dataStore, getId());
  GeometryHelpers::Connectivity::Find2DElementEdges<size_t>(getTriangles(), edgeList);
  if(edgeList == nullptr)
  {
    m_EdgeListId.reset();
    return -1;
  }
  m_EdgeListId = edgeList->getId();
  return 1;
}

void TriangleGeom::resizeEdgeList(size_t newNumEdges)
{
  getEdges()->getDataStore()->resizeTuples(newNumEdges);
}

AbstractGeometry::SharedEdgeList* TriangleGeom::getEdges()
{
  return dynamic_cast<SharedEdgeList*>(getDataStructure()->getData(m_EdgeListId));
}

const AbstractGeometry::SharedEdgeList* TriangleGeom::getEdges() const
{
  return dynamic_cast<const SharedEdgeList*>(getDataStructure()->getData(m_EdgeListId));
}

void TriangleGeom::deleteEdges()
{
  getDataStructure()->removeData(m_EdgeListId);
  m_EdgeListId.reset();
}

void TriangleGeom::setVertsAtEdge(size_t edgeId, size_t verts[2])
{
  auto edges = getEdges();
  if(!edges)
  {
    return;
  }
  for(size_t i = 0; i < 2; i++)
  {
    (*edges)[edgeId * 2 + i] = verts[i];
  }
}

void TriangleGeom::getVertsAtEdge(size_t edgeId, size_t verts[2]) const
{
  auto edges = getEdges();
  if(!edges)
  {
    return;
  }
  for(size_t i = 0; i < 2; i++)
  {
    verts[i] = edges->at(edgeId * 2 + i);
  }
}

void TriangleGeom::getVertCoordsAtEdge(size_t edgeId, Point3D<float>& vert1, Point3D<float>& vert2) const
{
  if(!getEdges())
  {
    return;
  }
  if(!getVertices())
  {
    return;
  }
  size_t verts[2];
  getVertsAtEdge(edgeId, verts);
  vert1 = getCoords(verts[0]);
  vert2 = getCoords(verts[1]);
}

size_t TriangleGeom::getNumberOfEdges() const
{
  auto edges = getEdges();
  if(!edges)
  {
    return 0;
  }
  return edges->getTupleCount();
}

AbstractGeometry::StatusCode TriangleGeom::findUnsharedEdges()
{
  auto dataStore = new DataStore<MeshIndexType>(2, 0);
  auto unsharedEdgeList = getDataStructure()->createDataArray("Unshared Edge List", dataStore, getId());
  GeometryHelpers::Connectivity::Find2DUnsharedEdges<size_t>(getTriangles(), unsharedEdgeList);
  if(unsharedEdgeList == nullptr)
  {
    m_UnsharedEdgeListId.reset();
    return -1;
  }
  m_UnsharedEdgeListId = unsharedEdgeList->getId();
  return 1;
}

const AbstractGeometry::SharedEdgeList* TriangleGeom::getUnsharedEdges() const
{
  return dynamic_cast<const SharedEdgeList*>(getDataStructure()->getData(m_UnsharedEdgeListId));
}

void TriangleGeom::deleteUnsharedEdges()
{
  getDataStructure()->removeData(m_UnsharedEdgeListId);
  m_UnsharedEdgeListId.reset();
}

uint32_t TriangleGeom::getXdmfGridType() const
{
  throw std::exception();
}

H5::ErrorType TriangleGeom::generateXdmfText(std::ostream& out, const std::string& hdfFileName) const
{
  throw std::exception();
}

H5::ErrorType TriangleGeom::readFromXdmfText(std::istream& in, const std::string& hdfFileName)
{
  throw std::exception();
}

void TriangleGeom::setElementsContainingVert(const ElementDynamicList* elementsContainingVert)
{
  if(!elementsContainingVert)
  {
    m_TrianglesContainingVertId.reset();
    return;
  }
  m_TrianglesContainingVertId = elementsContainingVert->getId();
}

void TriangleGeom::setElementNeighbors(const ElementDynamicList* elementNeighbors)
{
  if(!elementNeighbors)
  {
    m_TriangleNeighborsId.reset();
    return;
  }
  m_TriangleNeighborsId = elementNeighbors->getId();
}

void TriangleGeom::setElementCentroids(const FloatArray* elementCentroids)
{
  if(!elementCentroids)
  {
    m_TriangleCentroidsId.reset();
    return;
  }
  m_TriangleCentroidsId = elementCentroids->getId();
}

void TriangleGeom::setElementSizes(const FloatArray* elementSizes)
{
  if(!elementSizes)
  {
    m_TriangleSizesId.reset();
    return;
  }
  m_TriangleSizesId = elementSizes->getId();
}

void TriangleGeom::setEdges(const SharedEdgeList* edges)
{
  if(!edges)
  {
    m_EdgeListId.reset();
    return;
  }
  m_EdgeListId = edges->getId();
}

void TriangleGeom::setUnsharedEdges(const SharedEdgeList* bEdgeList)
{
  if(!bEdgeList)
  {
    m_UnsharedEdgeListId.reset();
    return;
  }
  m_UnsharedEdgeListId = bEdgeList->getId();
}
