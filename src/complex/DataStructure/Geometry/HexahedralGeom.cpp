#include "HexahedralGeom.hpp"

#include <stdexcept>

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"

using namespace complex;

HexahedralGeom::HexahedralGeom(DataStructure* ds, const std::string& name)
: AbstractGeometry3D(ds, name)
{
}

HexahedralGeom::HexahedralGeom(DataStructure* ds, const std::string& name, size_t numHexas, const std::shared_ptr<SharedVertexList>& vertices, bool allocate)
: AbstractGeometry3D(ds, name)
{
}

HexahedralGeom::HexahedralGeom(DataStructure* ds, const std::string& name, const std::shared_ptr<SharedHexList>& hexas, const std::shared_ptr<SharedVertexList>& vertices)
: AbstractGeometry3D(ds, name)
{
}

HexahedralGeom::HexahedralGeom(const HexahedralGeom& other)
: AbstractGeometry3D(other)
, m_VertexListId(other.m_VertexListId)
, m_EdgeListId(other.m_EdgeListId)
, m_UnsharedEdgeListId(other.m_UnsharedEdgeListId)
, m_QuadListId(other.m_QuadListId)
, m_UnsharedQuadListId(other.m_UnsharedQuadListId)
, m_HexListId(other.m_HexListId)
, m_HexasContainingVertId(other.m_HexasContainingVertId)
, m_HexNeighborsId(other.m_HexNeighborsId)
, m_HexCentroidsId(other.m_HexCentroidsId)
, m_HexSizesId(other.m_HexSizesId)
{
}

HexahedralGeom::HexahedralGeom(HexahedralGeom&& other) noexcept
: AbstractGeometry3D(std::move(other))
, m_VertexListId(std::move(other.m_VertexListId))
, m_EdgeListId(std::move(other.m_EdgeListId))
, m_UnsharedEdgeListId(std::move(other.m_UnsharedEdgeListId))
, m_QuadListId(std::move(other.m_QuadListId))
, m_UnsharedQuadListId(std::move(other.m_UnsharedQuadListId))
, m_HexListId(std::move(other.m_HexListId))
, m_HexasContainingVertId(std::move(other.m_HexasContainingVertId))
, m_HexNeighborsId(std::move(other.m_HexNeighborsId))
, m_HexCentroidsId(std::move(other.m_HexCentroidsId))
, m_HexSizesId(std::move(other.m_HexSizesId))
{
}

HexahedralGeom::~HexahedralGeom() = default;

DataObject* HexahedralGeom::shallowCopy()
{
  return new HexahedralGeom(*this);
}

DataObject* HexahedralGeom::deepCopy()
{
  return new HexahedralGeom(*this);
}

std::string HexahedralGeom::getGeometryTypeAsString() const
{
  return "HexahedralGeom";
}

void HexahedralGeom::resizeQuadList(size_t numQuads)
{
  getQuads()->getDataStore()->resizeTuples(numQuads);
}

void HexahedralGeom::setQuads(const SharedQuadList* quads)
{
  if(!quads)
  {
    return;
  }
  m_QuadListId = quads->getId();
}

AbstractGeometry::SharedQuadList* HexahedralGeom::getQuads()
{
  if(!m_QuadListId)
  {
    return nullptr;
  }
  return dynamic_cast<SharedQuadList*>(getDataStructure()->getData(m_QuadListId.value()));
}

const AbstractGeometry::SharedQuadList* HexahedralGeom::getQuads() const
{
  if(!m_QuadListId)
  {
    return nullptr;
  }
  return dynamic_cast<const SharedQuadList*>(getDataStructure()->getData(m_QuadListId.value()));
}

void HexahedralGeom::setVertsAtQuad(size_t quadId, size_t verts[4])
{
  auto quads = getQuads();
  if(!quads)
  {
    return;
  }

  auto numQuads = quads->getTupleCount();
  if(quadId >= numQuads)
  {
    return;
  }

  for(size_t i = 0; i < 4; i++)
  {
    verts[i] = (*quads)[quadId + i];
  }
}

void HexahedralGeom::getVertsAtQuad(size_t quadId, size_t verts[4]) const
{
  auto quads = getQuads();
  if(!quads)
  {
    return;
  }
  size_t index = quadId * 4;
  for(size_t i = 0; i < 4; i++)
  {
    verts[i] = quads->at(index + i);
  }
}

void HexahedralGeom::getVertCoordsAtQuad(size_t quadId, complex::Point3D<float>& vert1, complex::Point3D<float>& vert2, complex::Point3D<float>& vert3, complex::Point3D<float>& vert4) const
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
  for(size_t i = 0; i < 3; i++)
  {
    vert1[i] = vertices->at(verts[0] * 3 + i);
    vert2[i] = vertices->at(verts[1] * 3 + i);
    vert3[i] = vertices->at(verts[2] * 3 + i);
    vert4[i] = vertices->at(verts[3] * 3 + i);
  }
}

size_t HexahedralGeom::getNumberOfQuads() const
{
  auto hex = getHexahedra();
  if(!hex)
  {
    return 0;
  }
  return hex->getTupleCount();
}

void HexahedralGeom::resizeHexList(size_t numHexas)
{
  getHexahedra()->getDataStore()->resizeTuples(numHexas);
}

void HexahedralGeom::setHexahedra(const SharedHexList* hexas)
{
  if(!hexas)
  {
    m_HexListId.reset();
    return;
  }
  m_HexListId = hexas->getId();
}

AbstractGeometry::SharedHexList* HexahedralGeom::getHexahedra()
{
  return dynamic_cast<SharedHexList*>(getDataStructure()->getData(m_HexListId));
}

const AbstractGeometry::SharedHexList* HexahedralGeom::getHexahedra() const
{
  return dynamic_cast<const SharedHexList*>(getDataStructure()->getData(m_HexListId));
}

void HexahedralGeom::setVertsAtHex(size_t hexId, size_t verts[8])
{
  auto hex = dynamic_cast<SharedHexList*>(getDataStructure()->getData(m_HexListId));
  if(!hex)
  {
    return;
  }
  size_t index = hexId * 8;
  for(size_t i = 0; i < 8; i++)
  {
    (*hex)[index + i] = verts[i];
  }
}

void HexahedralGeom::getVertsAtHex(size_t hexId, size_t verts[8]) const
{
  auto hex = getHexahedra();
  if(!hex)
  {
    return;
  }
  size_t index = hexId * 8;
  for(size_t i = 0; i < 8; i++)
  {
    verts[i] = (*hex)[index + i];
  }
}

void HexahedralGeom::getVertCoordsAtHex(size_t hexId, complex::Point3D<float>& vert1, complex::Point3D<float>& vert2, complex::Point3D<float>& vert3, complex::Point3D<float>& vert4,
                                        complex::Point3D<float>& vert5, complex::Point3D<float>& vert6, complex::Point3D<float>& vert7, complex::Point3D<float>& vert8) const
{
  if(!getHexahedra())
  {
    return;
  }
  auto vertices = dynamic_cast<const SharedVertexList*>(getDataStructure()->getData(m_VertexListId));
  if(!vertices)
  {
    return;
  }
  size_t vertIds[8];
  getVertsAtHex(hexId, vertIds);

  vert1 = Point3D<float>(&(*vertices)[vertIds[0]]);
  vert2 = Point3D<float>(&(*vertices)[vertIds[1]]);
  vert3 = Point3D<float>(&(*vertices)[vertIds[2]]);
  vert4 = Point3D<float>(&(*vertices)[vertIds[3]]);
  vert5 = Point3D<float>(&(*vertices)[vertIds[4]]);
  vert6 = Point3D<float>(&(*vertices)[vertIds[5]]);
  vert7 = Point3D<float>(&(*vertices)[vertIds[6]]);
  vert8 = Point3D<float>(&(*vertices)[vertIds[7]]);
}

size_t HexahedralGeom::getNumberOfHexas() const
{
  auto hexList = dynamic_cast<const SharedHexList*>(getDataStructure()->getData(m_HexListId));
  if(!hexList)
  {
    return 0;
  }
  return hexList->getTupleCount();
}

void HexahedralGeom::initializeWithZeros()
{
  auto vertices = getVertices();
  if(vertices)
  {
    vertices->getDataStore()->fill(0.0);
  }
  auto hexas = getHexahedra();
  if(hexas)
  {
    hexas->getDataStore()->fill(0.0);
  }
}

size_t HexahedralGeom::getNumberOfElements() const
{
  auto elements = dynamic_cast<const FloatArray*>(getDataStructure()->getData(m_HexSizesId));
  if(!elements)
  {
    return 0;
  }
  return elements->getTupleCount();
}

AbstractGeometry::StatusCode HexahedralGeom::findElementSizes()
{
  auto dataStore = new DataStore<float>(1, getNumberOfHexas());
  auto hexSizes = getDataStructure()->createDataArray<float>("Hex Volumes", dataStore, getId());
  m_HexSizesId = hexSizes->getId();
  GeometryHelpers::Topology::FindHexVolumes(getHexahedra(), getVertices(), hexSizes);
  if(getElementSizes() == nullptr)
  {
    m_HexSizesId.reset();
    return -1;
  }
  return 1;
}

const FloatArray* HexahedralGeom::getElementSizes() const
{
  return dynamic_cast<const FloatArray*>(getDataStructure()->getData(m_HexSizesId));
}

void HexahedralGeom::deleteElementSizes()
{
  getDataStructure()->removeData(m_HexSizesId);
  m_HexSizesId.reset();
}

AbstractGeometry::StatusCode HexahedralGeom::findElementsContainingVert()
{
  auto hexasControllingVert = getDataStructure()->createDynamicList<uint16_t, MeshIndexType>("Hex Containing Vertices", getId());
  m_HexasContainingVertId = hexasControllingVert->getId();
  GeometryHelpers::Connectivity::FindElementsContainingVert<uint16_t, MeshIndexType>(getHexahedra(), hexasControllingVert, getNumberOfVertices());
  if(getElementsContainingVert() == nullptr)
  {
    m_HexasContainingVertId.reset();
    return -1;
  }
  return 1;
}

const AbstractGeometry::ElementDynamicList* HexahedralGeom::getElementsContainingVert() const
{
  return dynamic_cast<const ElementDynamicList*>(getDataStructure()->getData(m_HexasContainingVertId));
}

void HexahedralGeom::deleteElementsContainingVert()
{
  getDataStructure()->removeData(m_HexasContainingVertId);
  m_HexasContainingVertId.reset();
}

AbstractGeometry::StatusCode HexahedralGeom::findElementNeighbors()
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
  auto hexNeighbors = getDataStructure()->createDynamicList<uint16_t, MeshIndexType>("Hex Neighbors", getId());
  m_HexNeighborsId = hexNeighbors->getId();
  err = GeometryHelpers::Connectivity::FindElementNeighbors<uint16_t, MeshIndexType>(getHexahedra(), getElementsContainingVert(), hexNeighbors, AbstractGeometry::Type::Hexahedral);
  if(getElementNeighbors() == nullptr)
  {
    m_HexNeighborsId.reset();
    return -1;
  }
  return err;
}

const AbstractGeometry::ElementDynamicList* HexahedralGeom::getElementNeighbors() const
{
  return dynamic_cast<const ElementDynamicList*>(getDataStructure()->getData(m_HexNeighborsId));
}

void HexahedralGeom::deleteElementNeighbors()
{
  getDataStructure()->removeData(m_HexNeighborsId);
  m_HexNeighborsId.reset();
}

AbstractGeometry::StatusCode HexahedralGeom::findElementCentroids()
{
  auto dataStore = new DataStore<float>(3, getNumberOfHexas());
  auto hexCentroids = getDataStructure()->createDataArray<float>("Hex Centroids", dataStore, getId());
  m_HexCentroidsId = hexCentroids->getId();
  GeometryHelpers::Topology::FindElementCentroids(getHexahedra(), getVertices(), hexCentroids);
  if(getElementCentroids() == nullptr)
  {
    m_HexCentroidsId.reset();
    return -1;
  }
  return 1;
}

const FloatArray* HexahedralGeom::getElementCentroids() const
{
  return dynamic_cast<const FloatArray*>(getDataStructure()->getData(m_HexCentroidsId));
}

void HexahedralGeom::deleteElementCentroids()
{
  getDataStructure()->removeData(m_HexCentroidsId);
  m_HexCentroidsId.reset();
}

complex::Point3D<double> HexahedralGeom::getParametricCenter() const
{
  return {0.5, 0.5, 0.5};
}

void HexahedralGeom::getShapeFunctions(const complex::Point3D<double>& pCoords, double* shape) const
{
  double rm = 0.0;
  double sm = 0.0;
  double tm = 0.0;

  rm = 1.0 - pCoords[0];
  sm = 1.0 - pCoords[1];
  tm = 1.0 - pCoords[2];

  // r-derivatives
  shape[0] = -sm * tm;
  shape[1] = sm * tm;
  shape[2] = pCoords[1] * tm;
  shape[3] = -pCoords[1] * tm;
  shape[4] = -sm * pCoords[2];
  shape[5] = sm * pCoords[2];
  shape[6] = pCoords[1] * pCoords[2];
  shape[7] = -pCoords[1] * pCoords[2];

  // s-derivatives
  shape[8] = -rm * tm;
  shape[9] = -pCoords[0] * tm;
  shape[10] = pCoords[0] * tm;
  shape[11] = rm * tm;
  shape[12] = -rm * pCoords[2];
  shape[13] = -pCoords[0] * pCoords[2];
  shape[14] = pCoords[0] * pCoords[2];
  shape[15] = rm * pCoords[2];

  // t-derivatives
  shape[16] = -rm * sm;
  shape[17] = -pCoords[0] * sm;
  shape[18] = -pCoords[0] * pCoords[1];
  shape[19] = -rm * pCoords[1];
  shape[20] = rm * sm;
  shape[21] = pCoords[0] * sm;
  shape[22] = pCoords[0] * pCoords[1];
  shape[23] = rm * pCoords[1];
}

void HexahedralGeom::findDerivatives(DoubleArray* field, DoubleArray* derivatives, Observable* observable) const
{
  throw std::runtime_error("");
}

complex::TooltipGenerator HexahedralGeom::getTooltipGenerator() const
{
  TooltipGenerator toolTipGen;
  toolTipGen.addTitle("Geometry Info");
  toolTipGen.addValue("Type", "HexahedralGeom");
  toolTipGen.addValue("Units", LengthUnitToString(getUnits()));
  toolTipGen.addValue("Number of Hexahedra", std::to_string(getNumberOfHexas()));
  toolTipGen.addValue("Number of Vertices", std::to_string(getNumberOfVertices()));
  return toolTipGen;
}

void HexahedralGeom::resizeVertexList(size_t numVertices)
{
  auto vertices = dynamic_cast<SharedVertexList*>(getDataStructure()->getData(m_VertexListId));
  if(!vertices)
  {
    return;
  }
  vertices->getDataStore()->resizeTuples(numVertices);
}

void HexahedralGeom::setVertices(const SharedVertexList* vertices)
{
  if(nullptr == vertices)
  {
    m_VertexListId.reset();
    return;
  }

  m_VertexListId = vertices->getId();
}

AbstractGeometry::SharedVertexList* HexahedralGeom::getVertices()
{
  return dynamic_cast<SharedVertexList*>(getDataStructure()->getData(m_VertexListId));
}

const AbstractGeometry::SharedVertexList* HexahedralGeom::getVertices() const
{
  return dynamic_cast<const SharedVertexList*>(getDataStructure()->getData(m_VertexListId));
}

void HexahedralGeom::setCoords(size_t vertId, const complex::Point3D<float>& coords)
{
  auto vertices = dynamic_cast<FloatArray*>(getDataStructure()->getData(m_VertexListId));
  if(!vertices)
  {
    return;
  }

  size_t index = vertId * 3;
  for(size_t i = 0; i < 3; i++)
  {
    (*vertices)[index + i] = coords[i];
  }
}

complex::Point3D<float> HexahedralGeom::getCoords(size_t vertId) const
{
  auto vertices = dynamic_cast<const FloatArray*>(getDataStructure()->getData(m_VertexListId));
  if(!vertices)
  {
    return Point3D<float>();
  }

  size_t index = vertId * 3;
  auto x = (*vertices)[index];
  auto y = (*vertices)[index + 1];
  auto z = (*vertices)[index + 2];
  return Point3D<float>(x, y, z);
}

size_t HexahedralGeom::getNumberOfVertices() const
{
  auto vertices = dynamic_cast<const FloatArray*>(getDataStructure()->getData(m_VertexListId));
  if(!vertices)
  {
    return 0;
  }
  return vertices->getTupleCount();
}

void HexahedralGeom::resizeEdgeList(size_t numEdges)
{
  auto edges = getEdges();
  if(!edges)
  {
    return;
  }
  edges->getDataStore()->resizeTuples(numEdges);
}

AbstractGeometry::SharedEdgeList* HexahedralGeom::getEdges()
{
  return dynamic_cast<SharedEdgeList*>(getDataStructure()->getData(m_EdgeListId));
}

const AbstractGeometry::SharedEdgeList* HexahedralGeom::getEdges() const
{
  return dynamic_cast<const SharedEdgeList*>(getDataStructure()->getData(m_EdgeListId));
}

void HexahedralGeom::setVertsAtEdge(size_t edgeId, const size_t verts[2])
{
  auto edges = dynamic_cast<SharedEdgeList*>(getDataStructure()->getData(m_EdgeListId));
  if(!edges)
  {
    return;
  }
  size_t index = edgeId * 2;
  (*edges)[index] = verts[0];
  (*edges)[index + 1] = verts[1];
}

void HexahedralGeom::getVertsAtEdge(size_t edgeId, size_t verts[2]) const
{
  auto edges = getEdges();
  if(!edges)
  {
    return;
  }
  size_t index = edgeId * 2;
  verts[0] = (*edges)[index];
  verts[1] = (*edges)[index + 1];
}

void HexahedralGeom::getVertCoordsAtEdge(size_t edgeId, complex::Point3D<float>& vert1, complex::Point3D<float>& vert2) const
{
  auto edges = getEdges();
  if(!edges)
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
    vert1[i] = (*vertices)[verts[0] * 3 + i];
    vert2[i] = (*vertices)[verts[1] * 3 + i];
  }
}

size_t HexahedralGeom::getNumberOfEdges() const
{
  auto edges = getEdges();
  if(!edges)
  {
    return 0;
  }

  return edges->getTupleCount();
}

AbstractGeometry::StatusCode HexahedralGeom::findEdges()
{
  auto edgeList = createSharedEdgeList(0);
  m_EdgeListId = edgeList->getId();
  GeometryHelpers::Connectivity::FindHexEdges(getHexahedra(), getEdges());
  if(getEdges() == nullptr)
  {
    m_EdgeListId.reset();
    return -1;
  }
  return 1;
}

void HexahedralGeom::deleteEdges()
{
  getDataStructure()->removeData(m_EdgeListId);
  m_EdgeListId.reset();
}

AbstractGeometry::StatusCode HexahedralGeom::findFaces()
{
  auto quadList = createSharedQuadList(0);
  m_QuadListId = quadList->getId();
  GeometryHelpers::Connectivity::FindHexFaces(getHexahedra(), quadList);
  if(quadList == nullptr)
  {
    m_QuadListId.reset();
    return -1;
  }
  return 1;
}

void HexahedralGeom::deleteFaces()
{
  getDataStructure()->removeData(m_QuadListId);
  m_QuadListId.reset();
}

AbstractGeometry::StatusCode HexahedralGeom::findUnsharedEdges()
{
  auto dataStore = new DataStore<MeshIndexType>(2, 0);
  auto unsharedEdgeList = getDataStructure()->createDataArray("Unshared Edge List", dataStore, getId());
  m_UnsharedEdgeListId = unsharedEdgeList->getId();
  GeometryHelpers::Connectivity::FindUnsharedHexEdges(getHexahedra(), unsharedEdgeList);
  if(unsharedEdgeList == nullptr)
  {
    m_UnsharedEdgeListId.reset();
    return -1;
  }
  return 1;
}

const AbstractGeometry::SharedEdgeList* HexahedralGeom::getUnsharedEdges() const
{
  return dynamic_cast<const SharedEdgeList*>(getDataStructure()->getData(m_UnsharedEdgeListId));
}

void HexahedralGeom::deleteUnsharedEdges()
{
  getDataStructure()->removeData(m_UnsharedEdgeListId);
}

AbstractGeometry::StatusCode HexahedralGeom::findUnsharedFaces()
{
  auto dataStore = new DataStore<MeshIndexType>(4, 0);
  auto unsharedQuadList = getDataStructure()->createDataArray<MeshIndexType>("Unshared Edge List", dataStore, getId());
  m_UnsharedQuadListId = unsharedQuadList->getId();
  GeometryHelpers::Connectivity::FindUnsharedHexFaces(getHexahedra(), unsharedQuadList);
  if(unsharedQuadList == nullptr)
  {
    m_UnsharedQuadListId.reset();
    return -1;
  }
  return 1;
}

const AbstractGeometry::SharedEdgeList* HexahedralGeom::getUnsharedFaces() const
{
  return dynamic_cast<const SharedEdgeList*>(getDataStructure()->getData(m_UnsharedQuadListId));
}

void HexahedralGeom::deleteUnsharedFaces()
{
  getDataStructure()->removeData(m_UnsharedQuadListId);
  m_UnsharedQuadListId.reset();
}

uint32_t HexahedralGeom::getXdmfGridType() const
{
  throw std::runtime_error("");
}

H5::ErrorType HexahedralGeom::generateXdmfText(std::ostream& out, const std::string& hdfFileName) const
{
  throw std::runtime_error("");
}

H5::ErrorType HexahedralGeom::readFromXdmfText(std::istream& in, const std::string& hdfFileName)
{
  throw std::runtime_error("");
}

void HexahedralGeom::setElementsContainingVert(const ElementDynamicList* elementsContainingVert)
{
  if(!elementsContainingVert)
  {
    m_HexasContainingVertId.reset();
    return;
  }
  m_HexasContainingVertId = elementsContainingVert->getId();
}

void HexahedralGeom::setElementNeighbors(const ElementDynamicList* elementNeighbors)
{
  if(!elementNeighbors)
  {
    m_HexNeighborsId.reset();
    return;
  }
  m_HexNeighborsId = elementNeighbors->getId();
}

void HexahedralGeom::setElementCentroids(const FloatArray* elementCentroids)
{
  if(!elementCentroids)
  {
    m_HexCentroidsId.reset();
    return;
  }
  m_HexCentroidsId = elementCentroids->getId();
}

void HexahedralGeom::setElementSizes(const FloatArray* elementSizes)
{
  if(!elementSizes)
  {
    m_HexSizesId.reset();
    return;
  }
  m_HexSizesId = elementSizes->getId();
}

void HexahedralGeom::setEdges(const SharedEdgeList* edges)
{
  if(!edges)
  {
    m_EdgeListId.reset();
  }
  m_EdgeListId = edges->getId();
}

void HexahedralGeom::setUnsharedEdges(const SharedEdgeList* bEdgeList)
{
  if(!bEdgeList)
  {
    m_UnsharedEdgeListId.reset();
  }
  m_UnsharedEdgeListId = bEdgeList->getId();
}

void HexahedralGeom::setUnsharedFaces(const SharedFaceList* bFaceList)
{
  if(!bFaceList)
  {
    m_UnsharedQuadListId.reset();
  }
  m_UnsharedQuadListId = bFaceList->getId();
}
