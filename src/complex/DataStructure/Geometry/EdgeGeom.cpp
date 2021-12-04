#include "EdgeGeom.hpp"

#include <stdexcept>

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;

EdgeGeom::EdgeGeom(DataStructure& ds, std::string name)
: AbstractGeometry(ds, std::move(name))
{
}

EdgeGeom::EdgeGeom(DataStructure& ds, std::string name, IdType importId)
: AbstractGeometry(ds, std::move(name), importId)
{
}

EdgeGeom::EdgeGeom(DataStructure& ds, std::string name, const SharedEdgeList* edges, const SharedVertexList* vertices)
: AbstractGeometry(ds, std::move(name))
{
  setEdges(edges);
  setVertices(vertices);
}

EdgeGeom::EdgeGeom(const EdgeGeom& other) = default;

EdgeGeom::EdgeGeom(EdgeGeom&& other) noexcept
: AbstractGeometry(std::move(other))
, m_VertexListId(other.m_VertexListId)
, m_EdgeListId(other.m_EdgeListId)
, m_EdgesContainingVertId(other.m_EdgesContainingVertId)
, m_EdgeNeighborsId(other.m_EdgeNeighborsId)
, m_EdgeCentroidsId(other.m_EdgeCentroidsId)
, m_EdgeSizesId(other.m_EdgeSizesId)
{
}

EdgeGeom::~EdgeGeom() = default;
DataObject::DataObjectType EdgeGeom::getDataObjectType() const
{
  return DataObjectType::EdgeGeom;
}

EdgeGeom* EdgeGeom::Create(DataStructure& ds, std::string name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<EdgeGeom>(new EdgeGeom(ds, std::move(name)));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

EdgeGeom* EdgeGeom::Import(DataStructure& ds, std::string name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<EdgeGeom>(new EdgeGeom(ds, std::move(name), importId));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

std::string EdgeGeom::getTypeName() const
{
  return "EdgeGeom";
}

DataObject* EdgeGeom::shallowCopy()
{
  return new EdgeGeom(*this);
}

DataObject* EdgeGeom::deepCopy()
{
  auto copy = new EdgeGeom(*getDataStructure(), getName(), getId());

  copy->m_VertexListId = m_VertexListId;
  copy->m_EdgeListId = m_EdgeListId;
  copy->m_EdgesContainingVertId = m_EdgesContainingVertId;
  copy->m_EdgeNeighborsId = m_EdgeNeighborsId;
  copy->m_EdgeCentroidsId = m_EdgeCentroidsId;
  copy->m_EdgeSizesId = m_EdgeSizesId;

  for(auto& [id, childPtr] : getDataMap())
  {
    copy->insert(childPtr);
  }
  return copy;
}

std::string EdgeGeom::getGeometryTypeAsString() const
{
  return "EdgeGeom";
}

void EdgeGeom::resizeVertexList(usize newNumVertices)
{
  if(getVertices() == nullptr)
  {
    return;
  }
  getVertices()->getDataStore()->reshapeTuples({newNumVertices});
}

void EdgeGeom::setVertices(const SharedVertexList* vertices)
{
  if(vertices == nullptr)
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

void EdgeGeom::setCoords(usize vertId, const complex::Point3D<float32>& coords)
{
  auto vertices = getVertices();
  if(vertices == nullptr)
  {
    return;
  }

  for(usize i = 0; i < 3; i++)
  {
    (*vertices)[vertId * 3 + i] = coords[i];
  }
}

complex::Point3D<float32> EdgeGeom::getCoords(usize vertId) const
{
  const auto* vertices = getVertices();
  if(vertices == nullptr)
  {
    return {};
  }

  Point3D<float32> coord;
  for(usize i = 0; i < 3; i++)
  {
    coord[i] = (*vertices)[vertId * 3 + i];
  }
  return coord;
}

usize EdgeGeom::getNumberOfVertices() const
{
  auto vertices = getVertices();
  if(vertices == nullptr)
  {
    return 0;
  }
  return vertices->getNumberOfTuples();
}

void EdgeGeom::resizeEdgeList(usize newNumEdges)
{
  auto edges = getEdges();
  if(edges == nullptr)
  {
    return;
  }
  edges->getDataStore()->reshapeTuples({newNumEdges});
}

void EdgeGeom::setEdges(const SharedEdgeList* edges)
{
  if(edges == nullptr)
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

void EdgeGeom::setVertsAtEdge(usize edgeId, usize verts[2])
{
  auto edges = getEdges();
  if(edges == nullptr)
  {
    return;
  }

  auto numEdges = edges->getNumberOfTuples();
  if(edgeId >= numEdges)
  {
    return;
  }

  for(usize i = 0; i < 2; i++)
  {
    (*edges)[edgeId + i] = verts[i];
  }
}

void EdgeGeom::getVertsAtEdge(usize edgeId, usize verts[2])
{
  auto edges = getEdges();
  if(edges == nullptr)
  {
    return;
  }

  auto numEdges = edges->getNumberOfTuples();
  if(edgeId >= numEdges)
  {
    return;
  }

  for(usize i = 0; i < 2; i++)
  {
    verts[i] = (*edges)[edgeId + i];
  }
}

void EdgeGeom::getVertCoordsAtEdge(usize edgeId, complex::Point3D<float32>& vert1, complex::Point3D<float32>& vert2) const
{
  auto edges = getEdges();
  if(edges == nullptr)
  {
    return;
  }

  auto numEdges = edges->getNumberOfTuples();
  if(edgeId >= numEdges)
  {
    return;
  }

  auto vertices = getVertices();
  if(vertices == nullptr)
  {
    return;
  }

  auto vert1Id = (*edges)[edgeId];
  auto vert2Id = (*edges)[edgeId + 1];

  vert1 = &(*vertices)[vert1Id];
  vert2 = &(*vertices)[vert2Id];
}

usize EdgeGeom::getNumberOfEdges() const
{
  auto edges = getEdges();
  if(edges == nullptr)
  {
    return 0;
  }
  return edges->getNumberOfTuples();
}

void EdgeGeom::initializeWithZeros()
{
  auto vertices = getVertices();
  if(vertices != nullptr)
  {
    getVertices()->getDataStore()->fill(0.0f);
  }
  auto edges = getEdges();
  if(edges == nullptr)
  {
    getEdges()->getDataStore()->fill(0.0f);
  }
}

usize EdgeGeom::getNumberOfElements() const
{
  auto edges = getEdges();
  if(edges == nullptr)
  {
    return 0;
  }
  return edges->getNumberOfTuples();
}

AbstractGeometry::StatusCode EdgeGeom::findElementSizes()
{
  auto dataStore = std::make_unique<DataStore<float32>>(getNumberOfElements());
  auto sizes = DataArray<float32>::Create(*getDataStructure(), "Edge Lengths", std::move(dataStore), getId());
  m_EdgeSizesId = sizes->getId();

  Point3D<float32> vert0 = {0.0f, 0.0f, 0.0f};
  Point3D<float32> vert1 = {0.0f, 0.0f, 0.0f};

  for(usize i = 0; i < getNumberOfEdges(); i++)
  {
    getVertCoordsAtEdge(i, vert0, vert1);
    float32 length = 0.0f;
    for(usize j = 0; j < 3; j++)
    {
      length += (vert0[j] - vert1[j]) * (vert0[j] - vert1[j]);
    }
    (*sizes)[i] = sqrtf(length);
  }

  return 1;
}

const Float32Array* EdgeGeom::getElementSizes() const
{
  return dynamic_cast<const Float32Array*>(getDataStructure()->getData(m_EdgeSizesId));
}

void EdgeGeom::deleteElementSizes()
{
  getDataStructure()->removeData(m_EdgeSizesId.value());
  m_EdgeSizesId.reset();
}

AbstractGeometry::StatusCode EdgeGeom::findElementsContainingVert()
{
  auto containsVert = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), "Edges Containing Vert", getId());
  GeometryHelpers::Connectivity::FindElementsContainingVert<uint16, MeshIndexType>(getEdges(), containsVert, getNumberOfVertices());
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
  auto edgeNeighbors = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), "Edge Neighbors", getId());
  if(edgeNeighbors == nullptr)
  {
    err = -1;
    return err;
  }
  m_EdgeNeighborsId = edgeNeighbors->getId();
  err = GeometryHelpers::Connectivity::FindElementNeighbors<uint16, MeshIndexType>(getEdges(), getElementsContainingVert(), edgeNeighbors, AbstractGeometry::Type::Edge);
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
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfElements()}, std::vector<usize>{3});
  Float32Array* edgeCentroids = DataArray<float32>::Create(*getDataStructure(), "Edge Centroids", std::move(dataStore), getId());
  GeometryHelpers::Topology::FindElementCentroids(getEdges(), getVertices(), edgeCentroids);
  if(getElementCentroids() == nullptr)
  {
    return -1;
  }
  return 1;
}

const Float32Array* EdgeGeom::getElementCentroids() const
{
  if(!m_EdgeCentroidsId)
  {
    return nullptr;
  }
  return dynamic_cast<const Float32Array*>(getDataStructure()->getData(m_EdgeCentroidsId.value()));
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

complex::Point3D<float64> EdgeGeom::getParametricCenter() const
{
  return {0.5, 0.0, 0.0};
}

void EdgeGeom::getShapeFunctions(const complex::Point3D<float64>& pCoords, double* shape) const
{
  (void)pCoords;

  shape[0] = -1.0;
  shape[1] = 1.0;
}

void EdgeGeom::findDerivatives(Float64Array* field, Float64Array* derivatives, Observable* observable) const
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

uint32 EdgeGeom::getXdmfGridType() const
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

void EdgeGeom::setElementCentroids(const Float32Array* elementCentroids)
{
  deleteElementCentroids();
  if(!elementCentroids)
  {
    return;
  }
  m_EdgeCentroidsId = elementCentroids->getId();
}

void EdgeGeom::setElementSizes(const Float32Array* elementSizes)
{
  deleteElementSizes();
  if(!elementSizes)
  {
    return;
  }
  m_EdgeSizesId = elementSizes->getId();
}

H5::ErrorType EdgeGeom::readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader)
{
  m_VertexListId = ReadH5DataId(groupReader, H5Constants::k_VertexListTag);
  m_EdgeListId = ReadH5DataId(groupReader, H5Constants::k_EdgeListTag);
  m_EdgesContainingVertId = ReadH5DataId(groupReader, H5Constants::k_EdgesContainingVertTag);
  m_EdgeNeighborsId = ReadH5DataId(groupReader, H5Constants::k_EdgeNeighborsTag);
  m_EdgeCentroidsId = ReadH5DataId(groupReader, H5Constants::k_EdgeCentroidTag);
  m_EdgeSizesId = ReadH5DataId(groupReader, H5Constants::k_EdgeSizesTag);

  return getDataMap().readH5Group(dataStructureReader, groupReader, getId());
}

H5::ErrorType EdgeGeom::writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter) const
{
  auto groupWriter = parentGroupWriter.createGroupWriter(getName());
  auto err = writeH5ObjectAttributes(dataStructureWriter, groupWriter);
  if(err < 0)
  {
    return err;
  }

  auto errorCode = WriteH5DataId(groupWriter, m_VertexListId, H5Constants::k_VertexListTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_EdgeListId, H5Constants::k_EdgeListTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_EdgesContainingVertId, H5Constants::k_EdgesContainingVertTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_EdgeNeighborsId, H5Constants::k_EdgeNeighborsTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_EdgeCentroidsId, H5Constants::k_EdgeCentroidTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_EdgeSizesId, H5Constants::k_EdgeSizesTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  return getDataMap().writeH5Group(dataStructureWriter, groupWriter);
}
