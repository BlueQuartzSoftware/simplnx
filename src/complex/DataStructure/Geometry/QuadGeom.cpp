#include "QuadGeom.hpp"

#include <stdexcept>

#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;

namespace H5Constants
{
const std::string QuadListTag = "Quad List ID";
const std::string QuadsContainingVertTag = "Quads Containing Vertex ID";
const std::string QuadNeighborsTag = "Quad Neighbors ID";
const std::string QuadCentroidsTag = "Quad Centroids ID";
const std::string QuadSizesTag = "Quad Sizes ID";
} // namespace H5Constants

QuadGeom::QuadGeom(DataStructure& ds, const std::string& name)
: AbstractGeometry2D(ds, name)
{
}

QuadGeom::QuadGeom(DataStructure& ds, const std::string& name, IdType importId)
: AbstractGeometry2D(ds, name, importId)
{
}

QuadGeom::QuadGeom(DataStructure& ds, const std::string& name, usize numQuads, const std::shared_ptr<SharedVertexList>& vertices, bool allocate)
: AbstractGeometry2D(ds, name)
{
}

QuadGeom::QuadGeom(DataStructure& ds, const std::string& name, const std::shared_ptr<SharedQuadList>& quads, const std::shared_ptr<SharedVertexList>& vertices)
: AbstractGeometry2D(ds, name)
{
}

QuadGeom::QuadGeom(const QuadGeom& other)
: AbstractGeometry2D(other)
, m_QuadListId(other.m_QuadListId)
, m_QuadsContainingVertId(other.m_QuadsContainingVertId)
, m_QuadNeighborsId(other.m_QuadNeighborsId)
, m_QuadCentroidsId(other.m_QuadCentroidsId)
, m_QuadSizesId(other.m_QuadSizesId)
{
}

QuadGeom::QuadGeom(QuadGeom&& other) noexcept
: AbstractGeometry2D(std::move(other))
, m_QuadListId(std::move(other.m_QuadListId))
, m_QuadsContainingVertId(std::move(other.m_QuadsContainingVertId))
, m_QuadNeighborsId(std::move(other.m_QuadNeighborsId))
, m_QuadCentroidsId(std::move(other.m_QuadCentroidsId))
, m_QuadSizesId(std::move(other.m_QuadSizesId))
{
}

QuadGeom::~QuadGeom() = default;

QuadGeom* QuadGeom::Create(DataStructure& ds, const std::string& name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<QuadGeom>(new QuadGeom(ds, name));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

QuadGeom* QuadGeom::Import(DataStructure& ds, const std::string& name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<QuadGeom>(new QuadGeom(ds, name, importId));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

std::string QuadGeom::getTypeName() const
{
  return getGeometryTypeAsString();
}

DataObject* QuadGeom::shallowCopy()
{
  return new QuadGeom(*this);
}

DataObject* QuadGeom::deepCopy()
{
  throw std::runtime_error("");
}

std::string QuadGeom::getGeometryTypeAsString() const
{
  return "QuadGeom";
}

void QuadGeom::resizeQuadList(usize numQuads)
{
  getQuads()->getDataStore()->reshapeTuples({numQuads});
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

void QuadGeom::setVertsAtQuad(usize quadId, usize verts[4])
{
  auto quads = getQuads();
  if(!quads)
  {
    return;
  }
  const usize offset = quadId * 4;
  for(usize i = 0; i < 4; i++)
  {
    (*quads)[offset + i] = verts[i];
  }
}

void QuadGeom::getVertsAtQuad(usize quadId, usize verts[4]) const
{
  auto quads = getQuads();
  if(!quads)
  {
    return;
  }
  for(usize i = 0; i < 4; i++)
  {
    verts[i] = quads->at(quadId * 4 + i);
  }
}

void QuadGeom::getVertCoordsAtQuad(usize quadId, complex::Point3D<float32>& vert1, complex::Point3D<float32>& vert2, complex::Point3D<float32>& vert3, complex::Point3D<float32>& vert4) const
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
  usize verts[4];
  getVertsAtQuad(quadId, verts);
  for(usize i = 0; i < 4; i++)
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

usize QuadGeom::getNumberOfQuads() const
{
  auto quads = getQuads();
  if(!quads)
  {
    return 0;
  }
  return quads->getNumberOfTuples();
}

usize QuadGeom::getNumberOfElements() const
{
  return getNumberOfQuads();
}

AbstractGeometry::StatusCode QuadGeom::findElementSizes()
{
  auto dataStore = new DataStore<float32>(getNumberOfQuads());
  Float32Array* quadSizes = DataArray<float32>::Create(*getDataStructure(), "Quad Areas", dataStore, getId());
  GeometryHelpers::Topology::Find2DElementAreas(getQuads(), getVertices(), quadSizes);
  if(quadSizes == nullptr)
  {
    m_QuadSizesId.reset();
    return -1;
  }
  m_QuadSizesId = quadSizes->getId();
  return 1;
}

const Float32Array* QuadGeom::getElementSizes() const
{
  return dynamic_cast<const Float32Array*>(getDataStructure()->getData(m_QuadSizesId));
}

void QuadGeom::deleteElementSizes()
{
  getDataStructure()->removeData(m_QuadSizesId);
  m_QuadSizesId.reset();
}

AbstractGeometry::StatusCode QuadGeom::findElementsContainingVert()
{
  auto quadsContainingVert = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), "Quads Containing Vert", getId());
  GeometryHelpers::Connectivity::FindElementsContainingVert<uint16, MeshIndexType>(getQuads(), quadsContainingVert, getNumberOfVertices());
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
  auto quadNeighbors = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), "Quad Neighbors", getId());
  err = GeometryHelpers::Connectivity::FindElementNeighbors<uint16, MeshIndexType>(getQuads(), getElementsContainingVert(), quadNeighbors, AbstractGeometry::Type::Quad);
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
  auto dataStore = new DataStore<float32>({getNumberOfQuads()}, {3});
  auto quadCentroids = DataArray<float32>::Create(*getDataStructure(), "Quad Centroids", dataStore, getId());
  GeometryHelpers::Topology::FindElementCentroids(getQuads(), getVertices(), quadCentroids);
  if(quadCentroids == nullptr)
  {
    m_QuadCentroidsId.reset();
    return -1;
  }
  m_QuadCentroidsId = quadCentroids->getId();
  return 1;
}

const Float32Array* QuadGeom::getElementCentroids() const
{
  return dynamic_cast<const Float32Array*>(getDataStructure()->getData(m_QuadCentroidsId));
}

void QuadGeom::deleteElementCentroids()
{
  getDataStructure()->removeData(m_QuadCentroidsId);
  m_QuadCentroidsId.reset();
}

complex::Point3D<float64> QuadGeom::getParametricCenter() const
{
  return {0.5, 0.5, 0.0};
}

void QuadGeom::getShapeFunctions(const complex::Point3D<float64>& pCoords, double* shape) const
{
  float64 rm = 0.0;
  float64 sm = 0.0;

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

void QuadGeom::findDerivatives(Float64Array* field, Float64Array* derivatives, Observable* observable) const
{
  throw std::runtime_error("");
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

void QuadGeom::setCoords(usize vertId, const Point3D<float32>& coord)
{
  auto verts = getVertices();
  if(!verts)
  {
    return;
  }
  usize index = vertId * 4;
  for(usize i = 0; i < 3; i++)
  {
    (*verts)[index + i] = coord[i];
  }
}

Point3D<float32> QuadGeom::getCoords(usize vertId) const
{
  auto verts = getVertices();
  if(!verts)
  {
    return Point3D<float32>();
  }
  usize index = vertId * 4;
  return {verts->at(index), verts->at(index + 1), verts->at(index + 2)};
}

usize QuadGeom::getNumberOfVertices() const
{
  auto vertices = getVertices();
  if(!vertices)
  {
    return 0;
  }
  return vertices->getNumberOfTuples();
}

void QuadGeom::resizeEdgeList(usize numEdges)
{
  getEdges()->getDataStore()->reshapeTuples({numEdges});
}

void QuadGeom::getVertCoordsAtEdge(usize edgeId, complex::Point3D<float32>& vert1, complex::Point3D<float32>& vert2) const
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

  usize verts[2];
  getVertsAtEdge(edgeId, verts);

  for(usize i = 0; i < 3; i++)
  {
    vert1[i] = vertices->at(verts[0] * 4 + i);
    vert2[i] = vertices->at(verts[1] * 4 + i);
  }
}

AbstractGeometry::StatusCode QuadGeom::findEdges()
{
  auto edgeList = createSharedEdgeList(0);
  GeometryHelpers::Connectivity::Find2DElementEdges(getQuads(), edgeList);
  if(edgeList == nullptr)
  {
    setEdges(nullptr);
    return -1;
  }
  setEdges(edgeList);
  return 1;
}

AbstractGeometry::StatusCode QuadGeom::findUnsharedEdges()
{
  auto dataStore = new DataStore<MeshIndexType>({0}, {2});
  auto unsharedEdgeList = DataArray<MeshIndexType>::Create(*getDataStructure(), "Unshared Edge List", dataStore, getId());
  GeometryHelpers::Connectivity::Find2DUnsharedEdges(getQuads(), unsharedEdgeList);
  if(unsharedEdgeList == nullptr)
  {
    setUnsharedEdges(nullptr);
    return -1;
  }
  setUnsharedEdges(unsharedEdgeList);
  return 1;
}

uint32 QuadGeom::getXdmfGridType() const
{
  throw std::runtime_error("");
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

void QuadGeom::setElementCentroids(const Float32Array* elementCentroids)
{
  if(!elementCentroids)
  {
    m_QuadCentroidsId.reset();
    return;
  }
  m_QuadCentroidsId = elementCentroids->getId();
}

void QuadGeom::setElementSizes(const Float32Array* elementSizes)
{
  if(!elementSizes)
  {
    m_QuadSizesId.reset();
    return;
  }
  m_QuadSizesId = elementSizes->getId();
}

H5::ErrorType QuadGeom::readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader)
{
  m_QuadListId = ReadH5DataId(groupReader, H5Constants::QuadListTag);
  m_QuadsContainingVertId = ReadH5DataId(groupReader, H5Constants::QuadsContainingVertTag);
  m_QuadNeighborsId = ReadH5DataId(groupReader, H5Constants::QuadNeighborsTag);
  m_QuadCentroidsId = ReadH5DataId(groupReader, H5Constants::QuadCentroidsTag);
  m_QuadSizesId = ReadH5DataId(groupReader, H5Constants::QuadSizesTag);

  return getDataMap().readH5Group(dataStructureReader, groupReader, getId());
}

H5::ErrorType QuadGeom::writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter) const
{
  auto groupWriter = parentGroupWriter.createGroupWriter(getName());
  auto errorCode = writeH5ObjectAttributes(dataStructureWriter, groupWriter);
  if(errorCode < 0)
  {
    return errorCode;
  }

  // Write DataObject IDs
  errorCode = WriteH5DataId(groupWriter, m_QuadListId, H5Constants::QuadListTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_QuadsContainingVertId, H5Constants::QuadsContainingVertTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_QuadNeighborsId, H5Constants::QuadNeighborsTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_QuadCentroidsId, H5Constants::QuadCentroidsTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_QuadSizesId, H5Constants::QuadSizesTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  return getDataMap().writeH5Group(dataStructureWriter, groupWriter);
}
