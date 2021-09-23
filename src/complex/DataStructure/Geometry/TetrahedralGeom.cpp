#include "TetrahedralGeom.hpp"

#include <stdexcept>

#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Writer.hpp"

using namespace complex;

TetrahedralGeom::TetrahedralGeom(DataStructure& ds, const std::string& name)
: AbstractGeometry3D(ds, name)
{
}

TetrahedralGeom::TetrahedralGeom(DataStructure& ds, const std::string& name, size_t numTets, const std::shared_ptr<SharedVertexList>& vertices, bool allocate)
: AbstractGeometry3D(ds, name)
{
}

TetrahedralGeom::TetrahedralGeom(DataStructure& ds, const std::string& name, const std::shared_ptr<SharedTetList>& tets, const std::shared_ptr<SharedVertexList>& vertices)
: AbstractGeometry3D(ds, name)
{
}

TetrahedralGeom::TetrahedralGeom(const TetrahedralGeom& other)
: AbstractGeometry3D(other)
, m_TriListId(other.m_TriListId)
, m_UnsharedTriListId(other.m_UnsharedTriListId)
, m_TetListId(other.m_TetListId)
, m_TetsContainingVertId(other.m_TetsContainingVertId)
, m_TetNeighborsId(other.m_TetNeighborsId)
, m_TetCentroidsId(other.m_TetCentroidsId)
, m_TetSizesId(other.m_TetSizesId)
{
}

TetrahedralGeom::TetrahedralGeom(TetrahedralGeom&& other) noexcept
: AbstractGeometry3D(std::move(other))
, m_TriListId(std::move(other.m_TriListId))
, m_UnsharedTriListId(std::move(other.m_UnsharedTriListId))
, m_TetListId(std::move(other.m_TetListId))
, m_TetsContainingVertId(std::move(other.m_TetsContainingVertId))
, m_TetNeighborsId(std::move(other.m_TetNeighborsId))
, m_TetCentroidsId(std::move(other.m_TetCentroidsId))
, m_TetSizesId(std::move(other.m_TetSizesId))
{
}

TetrahedralGeom::~TetrahedralGeom() = default;

TetrahedralGeom* TetrahedralGeom::Create(DataStructure& ds, const std::string& name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<TetrahedralGeom>(new TetrahedralGeom(ds, name));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

std::string TetrahedralGeom::getTypeName() const
{
  return getGeometryTypeAsString();
}

DataObject* TetrahedralGeom::shallowCopy()
{
  return new TetrahedralGeom(*this);
}

DataObject* TetrahedralGeom::deepCopy()
{
  throw std::runtime_error("");
}

std::string TetrahedralGeom::getGeometryTypeAsString() const
{
  return "TetrahedralGeom";
}

void TetrahedralGeom::resizeTriList(size_t numTris)
{
  getTriangles()->getDataStore()->reshapeTuples({numTris});
}

void TetrahedralGeom::setTriangles(const SharedTriList* triangles)
{
  if(!triangles)
  {
    m_TriListId.reset();
    return;
  }
  m_TriListId = triangles->getId();
}

AbstractGeometry::SharedTriList* TetrahedralGeom::getTriangles()
{
  return dynamic_cast<SharedTriList*>(getDataStructure()->getData(m_TriListId));
}

const AbstractGeometry::SharedTriList* TetrahedralGeom::getTriangles() const
{
  return dynamic_cast<const SharedTriList*>(getDataStructure()->getData(m_TriListId));
}

void TetrahedralGeom::setVertsAtTri(size_t triId, size_t verts[3])
{
  auto triangles = getTriangles();
  if(!triangles)
  {
    return;
  }

  for(size_t i = 0; i < 3; i++)
  {
    (*triangles)[triId * 3 + i] = verts[i];
  }
}

void TetrahedralGeom::getVertsAtTri(size_t triId, size_t verts[3]) const
{
  auto triangles = getTriangles();
  if(!triangles)
  {
    return;
  }

  for(size_t i = 0; i < 3; i++)
  {
    verts[i] = triangles->at(triId * 3 + i);
  }
}

size_t TetrahedralGeom::getNumberOfTris() const
{
  return getTriangles()->getNumberOfTuples();
}

void TetrahedralGeom::resizeTetList(size_t numTets)
{
  getTriangles()->getDataStore()->reshapeTuples({numTets});
}

void TetrahedralGeom::setTetrahedra(const SharedTetList* tets)
{
  if(tets != nullptr)
  {
    m_TetListId.reset();
    return;
  }
  m_TetListId = tets->getId();
}

AbstractGeometry::SharedTetList* TetrahedralGeom::getTetrahedra()
{
  return dynamic_cast<SharedTetList*>(getDataStructure()->getData(m_TetListId));
}

const AbstractGeometry::SharedTetList* TetrahedralGeom::getTetrahedra() const
{
  return dynamic_cast<const SharedTetList*>(getDataStructure()->getData(m_TetListId));
}

void TetrahedralGeom::setVertsAtTet(size_t tetId, size_t verts[4])
{
  auto* tets = getTetrahedra();
  if(nullptr == tets)
  {
    return;
  }
  for(size_t i = 0; i < 4; i++)
  {
    (*tets)[tetId * 4 + i] = verts[i];
  }
}

void TetrahedralGeom::getVertsAtTet(size_t tetId, size_t verts[4]) const
{
  auto tets = getTetrahedra();
  if(!tets)
  {
    return;
  }
  for(size_t i = 0; i < 4; i++)
  {
    verts[i] = (*tets)[tetId * 4 + i];
  }
}

void TetrahedralGeom::getVertCoordsAtTet(size_t tetId, complex::Point3D<float>& vert1, complex::Point3D<float>& vert2, complex::Point3D<float>& vert3, complex::Point3D<float>& vert4) const
{
  if(!getTetrahedra())
  {
    return;
  }
  auto vertices = getVertices();
  if(!vertices)
  {
    return;
  }
  size_t verts[4];
  getVertsAtTet(tetId, verts);
  vert1 = getCoords(verts[0]);
  vert2 = getCoords(verts[1]);
  vert3 = getCoords(verts[2]);
  vert4 = getCoords(verts[3]);
}

size_t TetrahedralGeom::getNumberOfTets() const
{
  auto tets = getTetrahedra();
  if(!tets)
  {
    return 0;
  }
  return tets->getNumberOfTuples();
}

void TetrahedralGeom::initializeWithZeros()
{
  auto vertices = getVertices();
  if(vertices)
  {
    vertices->getDataStore()->fill(0.0f);
  }
  auto tets = getTetrahedra();
  if(tets)
  {
    tets->getDataStore()->fill(0);
  }
}

size_t TetrahedralGeom::getNumberOfElements() const
{
  return getNumberOfTets();
}

AbstractGeometry::StatusCode TetrahedralGeom::findElementSizes()
{
  auto dataStore = new DataStore<float>( {getNumberOfTets()}, {1});
  FloatArray* tetSizes = DataArray<float>::Create(*getDataStructure(), "Tet Volumes", dataStore, getId());
  GeometryHelpers::Topology::FindTetVolumes(getTetrahedra(), getVertices(), tetSizes);
  if(tetSizes == nullptr)
  {
    m_TetSizesId.reset();
    return -1;
  }
  m_TetSizesId = tetSizes->getId();
  return 1;
}

const FloatArray* TetrahedralGeom::getElementSizes() const
{
  return dynamic_cast<const FloatArray*>(getDataStructure()->getData(m_TetSizesId));
}

void TetrahedralGeom::deleteElementSizes()
{
  getDataStructure()->removeData(m_TetSizesId);
  m_TetSizesId.reset();
}

AbstractGeometry::StatusCode TetrahedralGeom::findElementsContainingVert()
{
  auto tetsContainingVert = DynamicListArray<uint16_t, MeshIndexType>::Create(*getDataStructure(), "Elements Containing Vert", getId());
  GeometryHelpers::Connectivity::FindElementsContainingVert<uint16_t, MeshIndexType>(getTetrahedra(), tetsContainingVert, getNumberOfVertices());
  if(tetsContainingVert == nullptr)
  {
    m_TetsContainingVertId.reset();
    return -1;
  }
  m_TetsContainingVertId = tetsContainingVert->getId();
  return 1;
}

const AbstractGeometry::ElementDynamicList* TetrahedralGeom::getElementsContainingVert() const
{
  return dynamic_cast<const ElementDynamicList*>(getDataStructure()->getData(m_TetsContainingVertId));
}

void TetrahedralGeom::deleteElementsContainingVert()
{
  getDataStructure()->removeData(m_TetsContainingVertId);
  m_TetsContainingVertId.reset();
}

AbstractGeometry::StatusCode TetrahedralGeom::findElementNeighbors()
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
  auto tetNeighbors = DynamicListArray<uint16_t, MeshIndexType>::Create(*getDataStructure(), "Tet Neighbors", getId());
  err = GeometryHelpers::Connectivity::FindElementNeighbors<uint16_t, MeshIndexType>(getTetrahedra(), getElementsContainingVert(), tetNeighbors, AbstractGeometry::Type::Tetrahedral);
  if(tetNeighbors == nullptr)
  {
    m_TetNeighborsId.reset();
    return -1;
  }
  m_TetNeighborsId = tetNeighbors->getId();
  return err;
}

const AbstractGeometry::ElementDynamicList* TetrahedralGeom::getElementNeighbors() const
{
  return dynamic_cast<const ElementDynamicList*>(getDataStructure()->getData(m_TetNeighborsId));
}

void TetrahedralGeom::deleteElementNeighbors()
{
  getDataStructure()->removeData(m_TetNeighborsId);
  m_TetNeighborsId.reset();
}

AbstractGeometry::StatusCode TetrahedralGeom::findElementCentroids()
{
  auto dataStore = new DataStore<float>({getNumberOfTets()}, {3});
  DataArray<float>* tetCentroids = DataArray<float>::Create(*getDataStructure(), "Tet Centroids", dataStore, getId());
  GeometryHelpers::Topology::FindElementCentroids(getTetrahedra(), getVertices(), tetCentroids);
  if(tetCentroids == nullptr)
  {
    m_TetCentroidsId.reset();
    return -1;
  }
  m_TetCentroidsId = tetCentroids->getId();
  return 1;
}

const FloatArray* TetrahedralGeom::getElementCentroids() const
{
  return dynamic_cast<const FloatArray*>(getDataStructure()->getData(m_TetCentroidsId));
}

void TetrahedralGeom::deleteElementCentroids()
{
  getDataStructure()->removeData(m_TetCentroidsId);
  m_TetCentroidsId.reset();
}

complex::Point3D<double> TetrahedralGeom::getParametricCenter() const
{
  return {0.25, 0.25, 0.25};
}

void TetrahedralGeom::getShapeFunctions(const complex::Point3D<double>& pCoords, double* shape) const
{
  (void)pCoords;

  // r-derivatives
  shape[0] = -1.0;
  shape[1] = 1.0;
  shape[2] = 0.0;
  shape[3] = 0.0;

  // s-derivatives
  shape[4] = -1.0;
  shape[5] = 0.0;
  shape[6] = 1.0;
  shape[7] = 0.0;

  // t-derivatives
  shape[8] = -1.0;
  shape[9] = 0.0;
  shape[10] = 0.0;
  shape[11] = 1.0;
}

void TetrahedralGeom::findDerivatives(DoubleArray* field, DoubleArray* derivatives, Observable* observable) const
{
  throw std::runtime_error("");
}

complex::TooltipGenerator TetrahedralGeom::getTooltipGenerator() const
{
  TooltipGenerator toolTipGen;

  toolTipGen.addTitle("Geometry Info");
  toolTipGen.addValue("Type", "TetrahedralGeom");
  toolTipGen.addValue("Units", LengthUnitToString(getUnits()));
  toolTipGen.addValue("Number of Tetrahedra", std::to_string(getNumberOfTets()));
  toolTipGen.addValue("Number of Vertices", std::to_string(getNumberOfVertices()));

  return toolTipGen;
}

AbstractGeometry::StatusCode TetrahedralGeom::findEdges()
{
  auto edgeList = createSharedEdgeList(0);
  GeometryHelpers::Connectivity::FindTetEdges(getTetrahedra(), edgeList);
  if(edgeList == nullptr)
  {
    setEdges(nullptr);
    return -1;
  }
  setEdges(edgeList);
  return 1;
}

AbstractGeometry::StatusCode TetrahedralGeom::findFaces()
{
  auto triList = createSharedTriList(0);
  GeometryHelpers::Connectivity::FindTetFaces(getTetrahedra(), triList);
  if(triList == nullptr)
  {
    m_TriListId.reset();
    return -1;
  }
  m_TriListId = triList->getId();
  return 1;
}

AbstractGeometry::StatusCode TetrahedralGeom::findUnsharedEdges()
{
  auto dataStore = new DataStore<MeshIndexType>({0}, {2});
  auto unsharedEdgeList = DataArray<MeshIndexType>::Create(*getDataStructure(), "Unshared Edge List", dataStore, getId());
  GeometryHelpers::Connectivity::FindUnsharedTetEdges(getTetrahedra(), unsharedEdgeList);
  if(unsharedEdgeList == nullptr)
  {
    setUnsharedEdges(nullptr);
    return -1;
  }
  setUnsharedEdges(unsharedEdgeList);
  return 1;
}

AbstractGeometry::StatusCode TetrahedralGeom::findUnsharedFaces()
{
  auto dataStore = new DataStore<MeshIndexType>({0}, {3});
  auto unsharedTriList = DataArray<MeshIndexType>::Create(*getDataStructure(), "Unshared Face List", dataStore, getId());
  GeometryHelpers::Connectivity::FindUnsharedTetFaces(getTetrahedra(), unsharedTriList);
  if(unsharedTriList == nullptr)
  {
    m_UnsharedTriListId.reset();
    return -1;
  }
  m_UnsharedTriListId = unsharedTriList->getId();
  return 1;
}

void TetrahedralGeom::setElementSizes(const FloatArray* elementSizes)
{
  if(!elementSizes)
  {
    m_TetSizesId.reset();
    return;
  }
  m_TetSizesId = elementSizes->getId();
}

uint32_t TetrahedralGeom::getXdmfGridType() const
{
  throw std::runtime_error("");
}

void TetrahedralGeom::setElementsContainingVert(const ElementDynamicList* elementsContainingVert)
{
  if(!elementsContainingVert)
  {
    m_TetsContainingVertId.reset();
    return;
  }
  m_TetsContainingVertId = elementsContainingVert->getId();
}

void TetrahedralGeom::setElementNeighbors(const ElementDynamicList* elementNeighbors)
{
  if(!elementNeighbors)
  {
    m_TetNeighborsId.reset();
    return;
  }
  m_TetNeighborsId = elementNeighbors->getId();
}

void TetrahedralGeom::setElementCentroids(const FloatArray* elementCentroids)
{
  if(!elementCentroids)
  {
    m_TetCentroidsId.reset();
    return;
  }
  m_TetCentroidsId = elementCentroids->getId();
}

H5::ErrorType TetrahedralGeom::readHdf5(H5::IdType targetId, H5::IdType groupId)
{
  return getDataMap().readH5Group(*getDataStructure(), targetId);
}

H5::ErrorType TetrahedralGeom::writeHdf5_impl(H5::IdType parentId, H5::IdType groupId) const
{
  return getDataMap().writeH5Group(groupId);
}
