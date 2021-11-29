#include "TriangleGeom.hpp"

#include <stdexcept>

#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/DynamicListArray.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;

TriangleGeom::TriangleGeom(DataStructure& ds, std::string name)
: AbstractGeometry2D(ds, std::move(name))
{
}

TriangleGeom::TriangleGeom(DataStructure& ds, std::string name, IdType importId)
: AbstractGeometry2D(ds, std::move(name), importId)
{
}

TriangleGeom::TriangleGeom(DataStructure& ds, std::string name, usize numTriangles, const SharedVertexList* vertices, bool allocate)
: AbstractGeometry2D(ds, std::move(name))
{
}

TriangleGeom::TriangleGeom(DataStructure& ds, std::string name, const SharedTriList* triangles, const SharedVertexList* vertices)
: AbstractGeometry2D(ds, std::move(name))
{
}

TriangleGeom::TriangleGeom(const TriangleGeom& other)
: AbstractGeometry2D(other)
, m_TriListId(other.m_TriListId)
, m_TrianglesContainingVertId(other.m_TrianglesContainingVertId)
, m_TriangleNeighborsId(other.m_TriangleNeighborsId)
, m_TriangleCentroidsId(other.m_TriangleCentroidsId)
, m_TriangleSizesId(other.m_TriangleSizesId)
{
}

TriangleGeom::TriangleGeom(TriangleGeom&& other) noexcept
: AbstractGeometry2D(std::move(other))
, m_TriListId(std::move(other.m_TriListId))
, m_TrianglesContainingVertId(std::move(other.m_TrianglesContainingVertId))
, m_TriangleNeighborsId(std::move(other.m_TriangleNeighborsId))
, m_TriangleCentroidsId(std::move(other.m_TriangleCentroidsId))
, m_TriangleSizesId(std::move(other.m_TriangleSizesId))
{
}

TriangleGeom::~TriangleGeom() = default;

TriangleGeom* TriangleGeom::Create(DataStructure& ds, std::string name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<TriangleGeom>(new TriangleGeom(ds, std::move(name)));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

TriangleGeom* TriangleGeom::Import(DataStructure& ds, std::string name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<TriangleGeom>(new TriangleGeom(ds, std::move(name), importId));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

std::string TriangleGeom::getTypeName() const
{
  return getGeometryTypeAsString();
}

DataObject* TriangleGeom::shallowCopy()
{
  return new TriangleGeom(*this);
}

DataObject* TriangleGeom::deepCopy()
{
  auto copy = new TriangleGeom(*getDataStructure(), getName(), getId());

  copy->m_TriListId = m_TriListId;
  copy->m_TrianglesContainingVertId = m_TrianglesContainingVertId;
  copy->m_TriangleNeighborsId = m_TriangleNeighborsId;
  copy->m_TriangleCentroidsId = m_TriangleCentroidsId;
  copy->m_TriangleSizesId = m_TriangleSizesId;

  for(auto& [id, childPtr] : getDataMap())
  {
    copy->insert(childPtr);
  }
  return copy;
}

std::string TriangleGeom::getGeometryTypeAsString() const
{
  return "TriangleGeom";
}

void TriangleGeom::resizeFaceList(usize newNumTris)
{
  getFaces()->getDataStore()->reshapeTuples({newNumTris});
}

void TriangleGeom::setFaces(const SharedTriList* triangles)
{
  if(!triangles)
  {
    m_TriListId.reset();
    return;
  }
  m_TriListId = triangles->getId();
}

AbstractGeometry::SharedTriList* TriangleGeom::getFaces()
{
  return dynamic_cast<SharedTriList*>(getDataStructure()->getData(m_TriListId));
}

const AbstractGeometry::SharedTriList* TriangleGeom::getFaces() const
{
  return dynamic_cast<const SharedTriList*>(getDataStructure()->getData(m_TriListId));
}

void TriangleGeom::setVertexIdsForFace(usize triId, usize verts[3])
{
  auto tris = getFaces();
  if(!tris)
  {
    return;
  }
  for(usize i = 0; i < 3; i++)
  {
    (*tris)[i] = verts[i];
  }
}

void TriangleGeom::getVertexIdsForFace(usize triId, usize verts[3]) const
{
  auto tris = getFaces();
  if(!tris)
  {
    return;
  }
  for(usize i = 0; i < 3; i++)
  {
    verts[i] = tris->at(i);
  }
}

void TriangleGeom::getVertexCoordsForFace(usize triId, Point3D<float32>& vert1, Point3D<float32>& vert2, Point3D<float32>& vert3) const
{
  if(!getFaces())
  {
    return;
  }
  if(!getVertices())
  {
    return;
  }
  usize verts[3];
  getVertexIdsForFace(triId, verts);
  vert1 = getCoords(verts[0]);
  vert2 = getCoords(verts[1]);
  vert3 = getCoords(verts[2]);
}

usize TriangleGeom::getNumberOfFaces() const
{
  auto tris = getFaces();
  if(!tris)
  {
    return 0;
  }
  return tris->getNumberOfTuples();
}

void TriangleGeom::initializeWithZeros()
{
  getVertices()->getDataStore()->fill(0.0f);
  getFaces()->getDataStore()->fill(0);
}

usize TriangleGeom::getNumberOfElements() const
{
  return getFaces()->getNumberOfTuples();
}

AbstractGeometry::StatusCode TriangleGeom::findElementSizes()
{
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfFaces()}, std::vector<usize>{1});
  Float32Array* triangleSizes = DataArray<float32>::Create(*getDataStructure(), "Triangle Areas", std::move(dataStore), getId());
  GeometryHelpers::Topology::Find2DElementAreas(getFaces(), getVertices(), triangleSizes);
  if(triangleSizes == nullptr)
  {
    m_TriangleSizesId.reset();
    return -1;
  }
  m_TriangleSizesId = triangleSizes->getId();
  return 1;
}

const Float32Array* TriangleGeom::getElementSizes() const
{
  return dynamic_cast<const Float32Array*>(getDataStructure()->getData(m_TriangleSizesId));
}

void TriangleGeom::deleteElementSizes()
{
  getDataStructure()->removeData(m_TriangleSizesId);
  m_TriangleSizesId.reset();
}

AbstractGeometry::StatusCode TriangleGeom::findElementsContainingVert()
{
  auto trianglesContainingVert = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), "Triangles Containing Vert", getId());
  GeometryHelpers::Connectivity::FindElementsContainingVert<uint16, MeshIndexType>(getFaces(), trianglesContainingVert, getNumberOfVertices());
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
  auto triangleNeighbors = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), "Triangle Neighbors", getId());
  err = GeometryHelpers::Connectivity::FindElementNeighbors<uint16, MeshIndexType>(getFaces(), getElementsContainingVert(), triangleNeighbors, AbstractGeometry::Type::Triangle);
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
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfFaces()}, std::vector<usize>{3});
  auto triangleCentroids = DataArray<float32>::Create(*getDataStructure(), "Triangle Centroids", std::move(dataStore), getId());
  GeometryHelpers::Topology::FindElementCentroids(getFaces(), getVertices(), triangleCentroids);
  if(triangleCentroids == nullptr)
  {
    m_TriangleCentroidsId.reset();
    return -1;
  }
  m_TriangleCentroidsId = triangleCentroids->getId();
  return 1;
}

const Float32Array* TriangleGeom::getElementCentroids() const
{
  return dynamic_cast<const Float32Array*>(getDataStructure()->getData(m_TriangleCentroidsId));
}

void TriangleGeom::deleteElementCentroids()
{
  getDataStructure()->removeData(m_TriangleCentroidsId);
  m_TriangleCentroidsId.reset();
}

complex::Point3D<float64> TriangleGeom::getParametricCenter() const
{
  return {1.0 / 3.0, 1.0 / 3.0, 0.0};
}

void TriangleGeom::getShapeFunctions(const complex::Point3D<float64>& pCoords, double* shape) const
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

void TriangleGeom::findDerivatives(Float64Array* field, Float64Array* derivatives, Observable* observable) const
{
  throw std::runtime_error("");
}

complex::TooltipGenerator TriangleGeom::getTooltipGenerator() const
{
  TooltipGenerator toolTipGen;
  toolTipGen.addTitle("Geometry Info");
  toolTipGen.addValue("Type", "TriangleGeom");
  toolTipGen.addValue("Units", LengthUnitToString(getUnits()));
  toolTipGen.addValue("Number of Triangles", std::to_string(getNumberOfFaces()));
  toolTipGen.addValue("Number of Vertices", std::to_string(getNumberOfVertices()));

  return toolTipGen;
}

void TriangleGeom::setCoords(usize vertId, const Point3D<float32>& coords)
{
  auto vertices = getVertices();
  if(!vertices)
  {
    return;
  }
  const usize offset = vertId * 3;
  for(usize i = 0; i < 3; i++)
  {
    (*vertices)[offset + i] = coords[i];
  }
}

Point3D<float32> TriangleGeom::getCoords(usize vertId) const
{
  auto vertices = getVertices();
  if(!vertices)
  {
    return {};
  }
  const usize offset = vertId * 3;
  Point3D<float32> coords;
  for(usize i = 0; i < 3; i++)
  {
    coords[i] = vertices->at(offset + i);
  }
  return coords;
}

usize TriangleGeom::getNumberOfVertices() const
{
  auto vertices = getVertices();
  if(!vertices)
  {
    return 0;
  }
  return vertices->getNumberOfTuples();
}

AbstractGeometry::StatusCode TriangleGeom::findEdges()
{
  auto dataStore = std::make_unique<DataStore<uint64>>(std::vector<usize>{0}, std::vector<usize>{2});
  DataArray<uint64>* edgeList = DataArray<uint64>::Create(*getDataStructure(), "Edge List", std::move(dataStore), getId());
  GeometryHelpers::Connectivity::Find2DElementEdges(getFaces(), edgeList);
  if(edgeList == nullptr)
  {
    setEdges(nullptr);
    return -1;
  }
  setEdges(edgeList);
  return 1;
}

void TriangleGeom::resizeEdgeList(usize newNumEdges)
{
  getEdges()->getDataStore()->reshapeTuples({newNumEdges});
}

void TriangleGeom::getVertCoordsAtEdge(usize edgeId, Point3D<float32>& vert1, Point3D<float32>& vert2) const
{
  if(!getEdges())
  {
    return;
  }
  if(!getVertices())
  {
    return;
  }
  usize verts[2];
  getVertsAtEdge(edgeId, verts);
  vert1 = getCoords(verts[0]);
  vert2 = getCoords(verts[1]);
}

AbstractGeometry::StatusCode TriangleGeom::findUnsharedEdges()
{
  auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{2});
  auto unsharedEdgeList = DataArray<MeshIndexType>::Create(*getDataStructure(), "Unshared Edge List", std::move(dataStore), getId());
  GeometryHelpers::Connectivity::Find2DUnsharedEdges(getFaces(), unsharedEdgeList);
  if(unsharedEdgeList == nullptr)
  {
    setUnsharedEdges(nullptr);
    return -1;
  }
  setUnsharedEdges(unsharedEdgeList);
  return 1;
}

uint32 TriangleGeom::getXdmfGridType() const
{
  throw std::runtime_error("");
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

void TriangleGeom::setElementCentroids(const Float32Array* elementCentroids)
{
  if(!elementCentroids)
  {
    m_TriangleCentroidsId.reset();
    return;
  }
  m_TriangleCentroidsId = elementCentroids->getId();
}

void TriangleGeom::setElementSizes(const Float32Array* elementSizes)
{
  if(!elementSizes)
  {
    m_TriangleSizesId.reset();
    return;
  }
  m_TriangleSizesId = elementSizes->getId();
}

H5::ErrorType TriangleGeom::readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader)
{
  m_TriListId = ReadH5DataId(groupReader, H5Constants::k_TriangleListTag);
  m_TrianglesContainingVertId = ReadH5DataId(groupReader, H5Constants::k_TrianglesContainingVertTag);
  m_TriangleNeighborsId = ReadH5DataId(groupReader, H5Constants::k_TriangleNeighborsTag);
  m_TriangleCentroidsId = ReadH5DataId(groupReader, H5Constants::k_TriangleCentroidsTag);
  m_TriangleSizesId = ReadH5DataId(groupReader, H5Constants::k_TriangleSizesTag);

  return getDataMap().readH5Group(dataStructureReader, groupReader, getId());
}

H5::ErrorType TriangleGeom::writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter) const
{
  auto errorCode = AbstractGeometry2D::writeHdf5(dataStructureWriter, parentGroupWriter);
  if(errorCode < 0)
  {
    return errorCode;
  }

  auto groupWriter = parentGroupWriter.createGroupWriter(getName());
  errorCode = writeH5ObjectAttributes(dataStructureWriter, groupWriter);
  if(errorCode < 0)
  {
    return errorCode;
  }

  // Write DataObject IDs
  errorCode = WriteH5DataId(groupWriter, m_TriListId, H5Constants::k_TriangleListTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_TrianglesContainingVertId, H5Constants::k_TrianglesContainingVertTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_TriangleNeighborsId, H5Constants::k_TriangleNeighborsTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_TriangleCentroidsId, H5Constants::k_TriangleCentroidsTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_TriangleSizesId, H5Constants::k_TriangleSizesTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  return getDataMap().writeH5Group(dataStructureWriter, groupWriter);
}
