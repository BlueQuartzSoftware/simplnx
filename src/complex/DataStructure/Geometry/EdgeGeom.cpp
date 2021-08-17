#include "EdgeGeom.hpp"

#include <stdexcept>

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"

using namespace complex;

EdgeGeom::EdgeGeom(DataStructure* ds, const std::string& name)
: AbstractGeometry(ds, name)
{
}

EdgeGeom::EdgeGeom(DataStructure* ds, const std::string& name, const SharedEdgeList* edges, const SharedVertexList* vertices)
: AbstractGeometry(ds, name)
{
  setEdges(edges);
  setVertices(vertices);
}

EdgeGeom::EdgeGeom(const EdgeGeom& other)
: AbstractGeometry(other)
, m_VertexListId(other.m_VertexListId)
, m_EdgeListId(other.m_EdgeListId)
, m_EdgesContainingVertId(other.m_EdgesContainingVertId)
, m_EdgeNeighborsId(other.m_EdgeNeighborsId)
, m_EdgeCentroidsId(other.m_EdgeCentroidsId)
, m_EdgeSizesId(other.m_EdgeSizesId)
{
}

EdgeGeom::EdgeGeom(EdgeGeom&& other) noexcept
: AbstractGeometry(std::move(other))
, m_VertexListId(std::move(other.m_VertexListId))
, m_EdgeListId(std::move(other.m_EdgeListId))
, m_EdgesContainingVertId(std::move(other.m_EdgesContainingVertId))
, m_EdgeNeighborsId(std::move(other.m_EdgeNeighborsId))
, m_EdgeCentroidsId(std::move(other.m_EdgeCentroidsId))
, m_EdgeSizesId(std::move(other.m_EdgeSizesId))
{
}

EdgeGeom::~EdgeGeom() = default;

DataObject* EdgeGeom::shallowCopy()
{
  return new EdgeGeom(*this);
}

DataObject* EdgeGeom::deepCopy()
{
  throw std::runtime_error("");
}

std::string EdgeGeom::getGeometryTypeAsString() const
{
  return "EdgeGeom";
}

void EdgeGeom::resizeVertexList(size_t newNumVertices)
{
  if(!getVertices())
  {
    return;
  }
  getVertices()->getDataStore()->resizeTuples(newNumVertices);
}

void EdgeGeom::setVertices(const SharedVertexList* vertices)
{
  if(!vertices)
  {
    return;
  }
  m_VertexListId = vertices->getId();
}

AbstractGeometry::SharedVertexList* EdgeGeom::getVertices()
{
  return dynamic_cast<SharedVertexList*>(getDataStructure()->getData(m_VertexListId));
}

const AbstractGeometry::SharedVertexList* EdgeGeom::getVertices() const
{
  return dynamic_cast<const SharedVertexList*>(getDataStructure()->getData(m_VertexListId));
}

void EdgeGeom::setCoords(size_t vertId, const complex::Point3D<float>& coords)
{
  auto vertices = getVertices();
  if(!vertices)
  {
    return;
  }

  for(size_t i = 0; i < 3; i++)
  {
    (*vertices)[vertId * 3 + i] = coords[i];
  }
}

complex::Point3D<float> EdgeGeom::getCoords(size_t vertId) const
{
  auto vertices = getVertices();
  if(!vertices)
  {
    return Point3D<float>();
  }

  Point3D<float> coord;
  for(size_t i = 0; i < 3; i++)
  {
    coord[i] = (*vertices)[vertId * 3 + i];
  }
  return coord;
}

size_t EdgeGeom::getNumberOfVertices() const
{
  auto vertices = getVertices();
  if(!vertices)
  {
    return 0;
  }
  return vertices->getTupleCount();
}

void EdgeGeom::resizeEdgeList(size_t newNumEdges)
{
  auto edges = getEdges();
  if(!edges)
  {
    return;
  }
  edges->getDataStore()->resizeTuples(newNumEdges);
}

void EdgeGeom::setEdges(const SharedEdgeList* edges)
{
  if(!edges)
  {
    m_EdgeListId.reset();
    return;
  }
  m_EdgeListId = edges->getId();
}

AbstractGeometry::SharedEdgeList* EdgeGeom::getEdges()
{
  return dynamic_cast<SharedEdgeList*>(getDataStructure()->getData(m_EdgeListId));
}

const AbstractGeometry::SharedEdgeList* EdgeGeom::getEdges() const
{
  return dynamic_cast<const SharedEdgeList*>(getDataStructure()->getData(m_EdgeListId));
}

void EdgeGeom::setVertsAtEdge(size_t edgeId, size_t verts[2])
{
  auto edges = getEdges();
  if(!edges)
  {
    return;
  }

  auto numEdges = edges->getTupleCount();
  if(edgeId >= numEdges)
  {
    return;
  }

  for(size_t i = 0; i < 2; i++)
  {
    (*edges)[edgeId + i] = verts[i];
  }
}

void EdgeGeom::getVertsAtEdge(size_t edgeId, size_t verts[2])
{
  auto edges = getEdges();
  if(!edges)
  {
    return;
  }

  auto numEdges = edges->getTupleCount();
  if(edgeId >= numEdges)
  {
    return;
  }

  for(size_t i = 0; i < 2; i++)
  {
    verts[i] = (*edges)[edgeId + i];
  }
}

void EdgeGeom::getVertCoordsAtEdge(size_t edgeId, complex::Point3D<float>& vert1, complex::Point3D<float>& vert2) const
{
  auto edges = getEdges();
  if(!edges)
  {
    return;
  }

  auto numEdges = edges->getTupleCount();
  if(edgeId >= numEdges)
  {
    return;
  }

  auto vertices = getVertices();
  if(!vertices)
  {
    return;
  }

  auto vert1Id = (*edges)[edgeId];
  auto vert2Id = (*edges)[edgeId + 1];

  vert1 = &(*vertices)[vert1Id];
  vert2 = &(*vertices)[vert2Id];
}

size_t EdgeGeom::getNumberOfEdges() const
{
  auto edges = getEdges();
  if(!edges)
  {
    return 0;
  }
  return edges->getTupleCount();
}

void EdgeGeom::initializeWithZeros()
{
  auto vertices = getVertices();
  if(vertices)
  {
    getVertices()->getDataStore()->fill(0.0f);
  }
  auto edges = getEdges();
  if(!edges)
  {
    getEdges()->getDataStore()->fill(0.0f);
  }
}

size_t EdgeGeom::getNumberOfElements() const
{
  auto edges = getEdges();
  if(!edges)
  {
    return 0;
  }
  return edges->getTupleCount();
}

AbstractGeometry::StatusCode EdgeGeom::findElementSizes()
{
  auto dataStore = new DataStore<float>(1, getNumberOfElements());
  auto sizes = getDataStructure()->createDataArray<float>("Edge Lengths", dataStore, getId());
  m_EdgeSizesId = sizes->getId();

  Point3D<float> vert0 = {0.0f, 0.0f, 0.0f};
  Point3D<float> vert1 = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < getNumberOfEdges(); i++)
  {
    getVertCoordsAtEdge(i, vert0, vert1);
    float length = 0.0f;
    for(size_t j = 0; j < 3; j++)
    {
      length += (vert0[j] - vert1[j]) * (vert0[j] - vert1[j]);
    }
    (*sizes)[i] = sqrtf(length);
  }

  return 1;
}

const FloatArray* EdgeGeom::getElementSizes() const
{
  return dynamic_cast<const FloatArray*>(getDataStructure()->getData(m_EdgeSizesId));
}

void EdgeGeom::deleteElementSizes()
{
  getDataStructure()->removeData(m_EdgeSizesId.value());
  m_EdgeSizesId.reset();
}

AbstractGeometry::StatusCode EdgeGeom::findElementsContainingVert()
{
  auto containsVert = getDataStructure()->createDynamicList<uint16_t, MeshIndexType>("Edges Containing Vert", getId());
  GeometryHelpers::Connectivity::FindElementsContainingVert<uint16_t, MeshIndexType>(getEdges(), containsVert, getNumberOfVertices());
  if(getElementsContainingVert() == nullptr)
  {
    return -1;
  }
  return 1;
}

const AbstractGeometry::ElementDynamicList* EdgeGeom::getElementsContainingVert() const
{
  return dynamic_cast<const ElementDynamicList*>(getDataStructure()->getData(m_EdgesContainingVertId.value()));
}

void EdgeGeom::deleteElementsContainingVert()
{
  if(!m_EdgesContainingVertId)
  {
    return;
  }
  getDataStructure()->removeData(m_EdgesContainingVertId.value());
  m_EdgesContainingVertId.reset();
}

AbstractGeometry::StatusCode EdgeGeom::findElementNeighbors()
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
  auto edgeNeighbors = getDataStructure()->createDynamicList<uint16_t, MeshIndexType>("Edge Neighbors", getId());
  if(edgeNeighbors == nullptr)
  {
    err = -1;
  }
  m_EdgeNeighborsId = edgeNeighbors->getId();
  err = GeometryHelpers::Connectivity::FindElementNeighbors<uint16_t, MeshIndexType>(getEdges(), getElementsContainingVert(), edgeNeighbors, AbstractGeometry::Type::Edge);
  if(getElementNeighbors() == nullptr)
  {
    m_EdgeNeighborsId.reset();
    err = -1;
  }
  return err;
}

const AbstractGeometry::ElementDynamicList* EdgeGeom::getElementNeighbors() const
{
  return dynamic_cast<const ElementDynamicList*>(getDataStructure()->getData(m_EdgeNeighborsId));
}

void EdgeGeom::deleteElementNeighbors()
{
  if(!m_EdgeNeighborsId)
  {
    return;
  }
  getDataStructure()->removeData(m_EdgeNeighborsId.value());
  m_EdgeNeighborsId.reset();
}

AbstractGeometry::StatusCode EdgeGeom::findElementCentroids()
{
  auto dataStore = new DataStore<float>(3, getNumberOfElements());
  auto edgeCentroids = getDataStructure()->createDataArray<float>("Edge Centroids", dataStore, getId());
  GeometryHelpers::Topology::FindElementCentroids(getEdges(), getVertices(), edgeCentroids);
  if(getElementCentroids() == nullptr)
  {
    return -1;
  }
  return 1;
}

const FloatArray* EdgeGeom::getElementCentroids() const
{
  if(!m_EdgeCentroidsId)
  {
    return nullptr;
  }
  return dynamic_cast<const FloatArray*>(getDataStructure()->getData(m_EdgeCentroidsId.value()));
}

void EdgeGeom::deleteElementCentroids()
{
  if(!m_EdgeCentroidsId)
  {
    return;
  }
  getDataStructure()->removeData(m_EdgeCentroidsId.value());
  m_EdgeCentroidsId.reset();
}

complex::Point3D<double> EdgeGeom::getParametricCenter() const
{
  return {0.5, 0.0, 0.0};
}

void EdgeGeom::getShapeFunctions(const complex::Point3D<double>& pCoords, double* shape) const
{
  (void)pCoords;

  shape[0] = -1.0;
  shape[1] = 1.0;
}

void EdgeGeom::findDerivatives(DoubleArray* field, DoubleArray* derivatives, Observable* observable) const
{
  throw std::runtime_error("");
}

complex::TooltipGenerator EdgeGeom::getTooltipGenerator() const
{
  TooltipGenerator toolTipGen;
  toolTipGen.addTitle("Geometry Info");
  toolTipGen.addValue("Type", "EdgeGeom");
  toolTipGen.addValue("Units", LengthUnitToString(getUnits()));
  toolTipGen.addValue("Number of Edges", std::to_string(getNumberOfEdges()));
  toolTipGen.addValue("Number of Vertices", std::to_string(getNumberOfVertices()));
  return toolTipGen;
}

uint32_t EdgeGeom::getXdmfGridType() const
{
  throw std::runtime_error("");
}

void EdgeGeom::setElementsContainingVert(const ElementDynamicList* elementsContainingVert)
{
  deleteElementsContainingVert();
  if(!elementsContainingVert)
  {
    return;
  }
  m_EdgesContainingVertId = elementsContainingVert->getId();
}

void EdgeGeom::setElementNeighbors(const ElementDynamicList* elementNeighbors)
{
  deleteElementNeighbors();
  if(!elementNeighbors)
  {
    return;
  }
  m_EdgeNeighborsId = elementNeighbors->getId();
}

void EdgeGeom::setElementCentroids(const FloatArray* elementCentroids)
{
  deleteElementCentroids();
  if(!elementCentroids)
  {
    return;
  }
  m_EdgeCentroidsId = elementCentroids->getId();
}

void EdgeGeom::setElementSizes(const FloatArray* elementSizes)
{
  deleteElementSizes();
  if(!elementSizes)
  {
    return;
  }
  m_EdgeSizesId = elementSizes->getId();
}
