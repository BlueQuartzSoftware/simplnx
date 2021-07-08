#include "QuadGeom.hpp"

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"

using namespace complex;

QuadGeom::QuadGeom(DataStructure* ds, const std::string& name)
: AbstractGeometry2D(ds, name)
{
}

QuadGeom::QuadGeom(DataStructure* ds, const std::string& name, size_t numQuads, const std::shared_ptr<SharedVertexList>& vertices, bool allocate)
: AbstractGeometry2D(ds, name)
{
}

QuadGeom::QuadGeom(DataStructure* ds, const std::string& name, const std::shared_ptr<SharedQuadList>& quads, const std::shared_ptr<SharedVertexList>& vertices)
: AbstractGeometry2D(ds, name)
{
}

QuadGeom::QuadGeom(const QuadGeom& other)
: AbstractGeometry2D(other)
, m_VertexListId(other.m_VertexListId)
, m_EdgeListId(other.m_EdgeListId)
, m_UnsharedEdgeListId(other.m_UnsharedEdgeListId)
, m_QuadListId(other.m_QuadListId)
, m_QuadsContainingVertId(other.m_QuadsContainingVertId)
, m_QuadNeighborsId(other.m_QuadNeighborsId)
, m_QuadCentroidsId(other.m_QuadCentroidsId)
, m_QuadSizesId(other.m_QuadSizesId)
{
}

QuadGeom::QuadGeom(QuadGeom&& other) noexcept
: AbstractGeometry2D(std::move(other))
, m_VertexListId(std::move(other.m_VertexListId))
, m_EdgeListId(std::move(other.m_EdgeListId))
, m_UnsharedEdgeListId(std::move(other.m_UnsharedEdgeListId))
, m_QuadListId(std::move(other.m_QuadListId))
, m_QuadsContainingVertId(std::move(other.m_QuadsContainingVertId))
, m_QuadNeighborsId(std::move(other.m_QuadNeighborsId))
, m_QuadCentroidsId(std::move(other.m_QuadCentroidsId))
, m_QuadSizesId(std::move(other.m_QuadSizesId))
{
}

QuadGeom::~QuadGeom() = default;

DataObject* QuadGeom::shallowCopy()
{
  return new QuadGeom(*this);
}

DataObject* QuadGeom::deepCopy()
{
  throw std::exception();
}

std::string QuadGeom::getGeometryTypeAsString() const
{
  return "QuadGeom";
}

void QuadGeom::resizeQuadList(size_t numQuads)
{
  getQuads()->getDataStore()->resizeTuples(numQuads);
}

void QuadGeom::setQuads(const SharedQuadList* quads)
{
  if(!quads)
  {
    m_QuadListId.reset();
    return;
  }
  m_QuadListId = quads->getId();
}

AbstractGeometry::SharedQuadList* QuadGeom::getQuads()
{
  return dynamic_cast<SharedQuadList*>(getDataStructure()->getData(m_QuadListId));
}

const AbstractGeometry::SharedQuadList* QuadGeom::getQuads() const
{
  return dynamic_cast<const SharedQuadList*>(getDataStructure()->getData(m_QuadListId));
}

void QuadGeom::setVertsAtQuad(size_t quadId, size_t verts[4])
{
  auto quads = getQuads();
  if(!quads)
  {
    return;
  }
  const size_t offset = quadId * 4;
  for(size_t i = 0; i < 4; i++)
  {
    (*quads)[offset + i] = verts[i];
  }
}

void QuadGeom::getVertsAtQuad(size_t quadId, size_t verts[4]) const
{
  auto quads = getQuads();
  if(!quads)
  {
    return;
  }
  for(size_t i = 0; i < 4; i++)
  {
    verts[i] = quads->at(quadId * 4 + i);
  }
}

void QuadGeom::getVertCoordsAtQuad(size_t quadId, complex::Point3D<float>& vert1, complex::Point3D<float>& vert2, complex::Point3D<float>& vert3, complex::Point3D<float>& vert4) const
{
  if(!getQuads())
  {
    return;
  }
  auto vertices = getVertices();
  if(!vertices)
  {
    return;
  }
  size_t verts[4];
  getVertsAtQuad(quadId, verts);
  for(size_t i = 0; i < 4; i++)
  {
    vert1[i] = vertices->at(verts[0] * 4 + i);
    vert2[i] = vertices->at(verts[1] * 4 + i);
    vert3[i] = vertices->at(verts[2] * 4 + i);
    vert4[i] = vertices->at(verts[3] * 4 + i);
  }
}

void QuadGeom::initializeWithZeros()
{
  auto vertices = getVertices();
  if(vertices)
  {
    vertices->getDataStore()->fill(0.0f);
  }
  auto quads = getQuads();
  if(quads)
  {
    quads->getDataStore()->fill(0.0);
  }
}

size_t QuadGeom::getNumberOfQuads() const
{
  auto quads = getQuads();
  if(!quads)
  {
    return 0;
  }
  return quads->getTupleCount();
}

size_t QuadGeom::getNumberOfElements() const
{
  return getNumberOfQuads();
}

AbstractGeometry::StatusCode QuadGeom::findElementSizes()
{
  auto dataStore = new DataStore<float>(1, getNumberOfQuads());
  auto quadSizes = getDataStructure()->createDataArray<float>("Quad Areas", dataStore, getId());
  GeometryHelpers::Topology::Find2DElementAreas<size_t>(getQuads(), getVertices(), quadSizes);
  if(quadSizes == nullptr)
  {
    m_QuadSizesId.reset();
    return -1;
  }
  m_QuadSizesId = quadSizes->getId();
  return 1;
}

const FloatArray* QuadGeom::getElementSizes() const
{
  return dynamic_cast<const FloatArray*>(getDataStructure()->getData(m_QuadSizesId));
}

void QuadGeom::deleteElementSizes()
{
  getDataStructure()->removeData(m_QuadSizesId);
  m_QuadSizesId.reset();
}

AbstractGeometry::StatusCode QuadGeom::findElementsContainingVert()
{
  auto quadsContainingVert = getDataStructure()->createDynamicList<uint16_t, MeshIndexType>("Quads Containing Vert", getId());
  GeometryHelpers::Connectivity::FindElementsContainingVert<uint16_t, MeshIndexType>(getQuads(), quadsContainingVert, getNumberOfVertices());
  if(quadsContainingVert == nullptr)
  {
    m_QuadsContainingVertId.reset();
    return -1;
  }
  m_QuadsContainingVertId = quadsContainingVert->getId();
  return 1;
}

const AbstractGeometry::ElementDynamicList* QuadGeom::getElementsContainingVert() const
{
  return dynamic_cast<const ElementDynamicList*>(getDataStructure()->getData(m_QuadsContainingVertId));
}

void QuadGeom::deleteElementsContainingVert()
{
  getDataStructure()->removeData(m_QuadsContainingVertId);
  m_QuadsContainingVertId.reset();
}

AbstractGeometry::StatusCode QuadGeom::findElementNeighbors()
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
  auto quadNeighbors = getDataStructure()->createDynamicList<uint16_t, MeshIndexType>("Quad Neighbors", getId());
  err = GeometryHelpers::Connectivity::FindElementNeighbors<uint16_t, MeshIndexType>(getQuads(), getElementsContainingVert(), quadNeighbors, AbstractGeometry::Type::Quad);
  if(quadNeighbors == nullptr)
  {
    m_QuadNeighborsId.reset();
    return -1;
  }
  m_QuadNeighborsId = quadNeighbors->getId();
  return err;
}

const AbstractGeometry::ElementDynamicList* QuadGeom::getElementNeighbors() const
{
  return dynamic_cast<const ElementDynamicList*>(getDataStructure()->getData(m_QuadNeighborsId));
}

void QuadGeom::deleteElementNeighbors()
{
  getDataStructure()->removeData(m_QuadNeighborsId);
  m_QuadNeighborsId.reset();
}

AbstractGeometry::StatusCode QuadGeom::findElementCentroids()
{
  auto dataStore = new DataStore<float>(3, getNumberOfQuads());
  auto quadCentroids = getDataStructure()->createDataArray<float>("Quad Centroids", dataStore, getId());
  GeometryHelpers::Topology::FindElementCentroids<size_t>(getQuads(), getVertices(), quadCentroids);
  if(quadCentroids == nullptr)
  {
    m_QuadCentroidsId.reset();
    return -1;
  }
  m_QuadCentroidsId = quadCentroids->getId();
  return 1;
}

const FloatArray* QuadGeom::getElementCentroids() const
{
  return dynamic_cast<const FloatArray*>(getDataStructure()->getData(m_QuadCentroidsId));
}

void QuadGeom::deleteElementCentroids()
{
  getDataStructure()->removeData(m_QuadCentroidsId);
  m_QuadCentroidsId.reset();
}

complex::Point3D<double> QuadGeom::getParametricCenter() const
{
  return {0.5, 0.5, 0.0};
}

void QuadGeom::getShapeFunctions(const complex::Point3D<double>& pCoords, double* shape) const
{
  double rm = 0.0;
  double sm = 0.0;

  rm = 1.0 - pCoords[0];
  sm = 1.0 - pCoords[1];

  shape[0] = -sm;
  shape[1] = sm;
  shape[2] = pCoords[1];
  shape[3] = -pCoords[1];
  shape[4] = -rm;
  shape[5] = -pCoords[0];
  shape[6] = pCoords[0];
  shape[7] = rm;
}

void QuadGeom::findDerivatives(DoubleArray* field, DoubleArray* derivatives, Observable* observable) const
{
  throw std::exception();
}

complex::TooltipGenerator QuadGeom::getTooltipGenerator() const
{
  TooltipGenerator toolTipGen;
  toolTipGen.addTitle("Geometry Info");
  toolTipGen.addValue("Type", "QuadGeom");
  toolTipGen.addValue("Units", LengthUnitToString(getUnits()));
  toolTipGen.addValue("Number of Quads", std::to_string(getNumberOfQuads()));
  toolTipGen.addValue("Number of Vertices", std::to_string(getNumberOfVertices()));

  return toolTipGen;
}

void QuadGeom::resizeVertexList(size_t numVertices)
{
  getVertices()->getDataStore()->resizeTuples(numVertices);
}

void QuadGeom::setVertices(const SharedVertexList* vertices)
{
  if(!vertices)
  {
    m_VertexListId.reset();
    return;
  }
  m_VertexListId = vertices->getId();
}

AbstractGeometry::SharedVertexList* QuadGeom::getVertices()
{
  return dynamic_cast<SharedVertexList*>(getDataStructure()->getData(m_VertexListId));
}

const AbstractGeometry::SharedVertexList* QuadGeom::getVertices() const
{
  return dynamic_cast<const SharedVertexList*>(getDataStructure()->getData(m_VertexListId));
}

void QuadGeom::setCoords(size_t vertId, const Point3D<float>& coord)
{
  auto verts = getVertices();
  if(!verts)
  {
    return;
  }
  size_t index = vertId * 4;
  for(size_t i = 0; i < 3; i++)
  {
    (*verts)[index + i] = coord[i];
  }
}

Point3D<float> QuadGeom::getCoords(size_t vertId) const
{
  auto verts = getVertices();
  if(!verts)
  {
    return Point3D<float>();
  }
  size_t index = vertId * 4;
  return {verts->at(index), verts->at(index + 1), verts->at(index + 2)};
}

size_t QuadGeom::getNumberOfVertices() const
{
  auto vertices = getVertices();
  if(!vertices)
  {
    return 0;
  }
  return vertices->getTupleCount();
}

void QuadGeom::resizeEdgeList(size_t numEdges)
{
  getEdges()->getDataStore()->resizeTuples(numEdges);
}

AbstractGeometry::SharedEdgeList* QuadGeom::getEdges()
{
  return dynamic_cast<SharedEdgeList*>(getDataStructure()->getData(m_EdgeListId));
}

const AbstractGeometry::SharedEdgeList* QuadGeom::getEdges() const
{
  return dynamic_cast<const SharedEdgeList*>(getDataStructure()->getData(m_EdgeListId));
}

void QuadGeom::setVertsAtEdge(size_t edgeId, const size_t verts[2])
{
  auto edges = getEdges();
  if(!edges)
  {
    return;
  }
  (*edges)[edgeId * 2] = verts[0];
  (*edges)[edgeId * 2 + 1] = verts[1];
}

void QuadGeom::getVertsAtEdge(size_t edgeId, size_t verts[2]) const
{
  auto edges = getEdges();
  if(!edges)
  {
    return;
  }
  verts[0] = edges->at(edgeId * 2);
  verts[1] = edges->at(edgeId * 2 + 1);
}

void QuadGeom::getVertCoordsAtEdge(size_t edgeId, complex::Point3D<float>& vert1, complex::Point3D<float>& vert2) const
{
  if(!getEdges())
  {
    return;
  }
  auto vertices = getVertices();
  if(!vertices)
  {
    return;
  }

  size_t verts[2];
  getVertsAtEdge(edgeId, verts);

  for(size_t i = 0; i < 3; i++)
  {
    vert1[i] = vertices->at(verts[0] * 4 + i);
    vert2[i] = vertices->at(verts[1] * 4 + i);
  }
}

size_t QuadGeom::getNumberOfEdges() const
{
  auto edges = getEdges();
  if(!edges)
  {
    return 0;
  }
  return edges->getTupleCount();
}

AbstractGeometry::StatusCode QuadGeom::findEdges()
{
  auto edgeList = createSharedEdgeList(0);
  GeometryHelpers::Connectivity::Find2DElementEdges<size_t>(getQuads(), edgeList);
  if(edgeList == nullptr)
  {
    m_EdgeListId.reset();
    return -1;
  }
  m_EdgeListId = edgeList->getId();
  return 1;
}

void QuadGeom::deleteEdges()
{
  getDataStructure()->removeData(m_EdgeListId);
  m_EdgeListId.reset();
}

AbstractGeometry::StatusCode QuadGeom::findUnsharedEdges()
{
  auto dataStore = new DataStore<MeshIndexType>(2, 0);
  auto unsharedEdgeList = getDataStructure()->createDataArray<MeshIndexType>("Unshared Edge List", dataStore, getId());
  GeometryHelpers::Connectivity::Find2DUnsharedEdges<size_t>(getQuads(), unsharedEdgeList);
  if(unsharedEdgeList == nullptr)
  {
    m_UnsharedEdgeListId.reset();
    return -1;
  }
  m_UnsharedEdgeListId = unsharedEdgeList->getId();
  return 1;
}

const AbstractGeometry::SharedEdgeList* QuadGeom::getUnsharedEdges() const
{
  return dynamic_cast<const SharedEdgeList*>(getDataStructure()->getData(m_UnsharedEdgeListId));
}

void QuadGeom::deleteUnsharedEdges()
{
  getDataStructure()->removeData(m_UnsharedEdgeListId);
  m_UnsharedEdgeListId.reset();
}

void QuadGeom::setEdges(const SharedEdgeList* edges)
{
  if(!edges)
  {
    m_EdgeListId.reset();
    return;
  }
  m_EdgeListId = edges->getId();
}

void QuadGeom::setUnsharedEdges(const SharedEdgeList* bEdgeList)
{
  if(!bEdgeList)
  {
    m_UnsharedEdgeListId.reset();
    return;
  }
  m_UnsharedEdgeListId = bEdgeList->getId();
}

uint32_t QuadGeom::getXdmfGridType() const
{
  throw std::exception();
}

H5::ErrorType QuadGeom::generateXdmfText(std::ostream& out, const std::string& hdfFileName) const
{
  throw std::exception();
}

H5::ErrorType QuadGeom::readFromXdmfText(std::istream& in, const std::string& hdfFileName)
{
  throw std::exception();
}

void QuadGeom::setElementsContainingVert(const ElementDynamicList* elementsContainingVert)
{
  if(!elementsContainingVert)
  {
    m_QuadsContainingVertId.reset();
    return;
  }
  m_QuadsContainingVertId = elementsContainingVert->getId();
}

void QuadGeom::setElementNeighbors(const ElementDynamicList* elementNeighbors)
{
  if(!elementNeighbors)
  {
    m_QuadNeighborsId.reset();
    return;
  }
  m_QuadNeighborsId = elementNeighbors->getId();
}

void QuadGeom::setElementCentroids(const FloatArray* elementCentroids)
{
  if(!elementCentroids)
  {
    m_QuadCentroidsId.reset();
    return;
  }
  m_QuadCentroidsId = elementCentroids->getId();
}

void QuadGeom::setElementSizes(const FloatArray* elementSizes)
{
  if(!elementSizes)
  {
    m_QuadSizesId.reset();
    return;
  }
  m_QuadSizesId = elementSizes->getId();
}
