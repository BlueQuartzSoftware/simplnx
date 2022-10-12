#include "TetrahedralGeom.hpp"

#include <stdexcept>

#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;

TetrahedralGeom::TetrahedralGeom(DataStructure& ds, std::string name)
: INodeGeometry3D(ds, std::move(name))
{
}

TetrahedralGeom::TetrahedralGeom(DataStructure& ds, std::string name, IdType importId)
: INodeGeometry3D(ds, std::move(name), importId)
{
}

IGeometry::Type TetrahedralGeom::getGeomType() const
{
  return IGeometry::Type::Tetrahedral;
}

DataObject::Type TetrahedralGeom::getDataObjectType() const
{
  return DataObject::Type::TetrahedralGeom;
}

TetrahedralGeom* TetrahedralGeom::Create(DataStructure& ds, std::string name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<TetrahedralGeom>(new TetrahedralGeom(ds, std::move(name)));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

TetrahedralGeom* TetrahedralGeom::Import(DataStructure& ds, std::string name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<TetrahedralGeom>(new TetrahedralGeom(ds, std::move(name), importId));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

std::string TetrahedralGeom::getTypeName() const
{
  return "TetrahedralGeom";
}

DataObject* TetrahedralGeom::shallowCopy()
{
  return new TetrahedralGeom(*this);
}

std::shared_ptr<DataObject> TetrahedralGeom::deepCopy(const DataPath& copyPath)
{
  auto& dataStruct = *getDataStructure();
  auto copy = std::shared_ptr<TetrahedralGeom>(new TetrahedralGeom(dataStruct, copyPath.getTargetName(), getId()));
  if(dataStruct.insert(copy, copyPath.getParent()))
  {
    auto dataMapCopy = getDataMap().deepCopy(copyPath);
  }

  if(getElementSizes() != nullptr)
  {
    copy->findElementSizes();
  }
  if(getElementsContainingVert() != nullptr)
  {
    copy->findElementsContainingVert();
  }
  if(getElementNeighbors() != nullptr)
  {
    copy->findElementNeighbors();
  }
  if(getElementCentroids() != nullptr)
  {
    copy->findElementCentroids();
  }
  if(getUnsharedEdges() != nullptr)
  {
    copy->findUnsharedEdges();
  }
  if(getUnsharedFaces() != nullptr)
  {
    copy->findUnsharedFaces();
  }
  const DataPath copiedVertDataPath = copyPath.createChildPath(getVertexAttributeMatrix()->getName());
  copy->m_VertexAttributeMatrixId = dataStruct.getId(copiedVertDataPath);
  const DataPath copiedVertListPath = copyPath.createChildPath(getVertices()->getName());
  copy->m_VertexDataArrayId = dataStruct.getId(copiedVertListPath);
  const DataPath copiedEdgeDataPath = copyPath.createChildPath(getEdgeAttributeMatrix()->getName());
  copy->m_EdgeAttributeMatrixId = dataStruct.getId(copiedEdgeDataPath);
  const DataPath copiedEdgeListPath = copyPath.createChildPath(getEdges()->getName());
  copy->m_EdgeDataArrayId = dataStruct.getId(copiedEdgeListPath);
  const DataPath copiedFaceDataPath = copyPath.createChildPath(getFaceAttributeMatrix()->getName());
  copy->m_FaceDataId = dataStruct.getId(copiedFaceDataPath);
  const DataPath copiedFaceListPath = copyPath.createChildPath(getFaces()->getName());
  copy->m_FaceListId = dataStruct.getId(copiedFaceListPath);
  const DataPath copiedPolyDataPath = copyPath.createChildPath(getPolyhedraAttributeMatrix()->getName());
  copy->m_PolyhedronDataId = dataStruct.getId(copiedPolyDataPath);
  const DataPath copiedPolyListPath = copyPath.createChildPath(getPolyhedra()->getName());
  copy->m_PolyhedronListId = dataStruct.getId(copiedPolyListPath);

  return copy;
}

usize TetrahedralGeom::getNumberOfVerticesPerFace() const
{
  return k_NumFaceVerts;
}

usize TetrahedralGeom::getNumberOfVerticesPerCell() const
{
  return k_NumVerts;
}

usize TetrahedralGeom::getNumberOfCells() const
{
  auto& tets = getPolyhedraRef();
  return tets.getNumberOfTuples();
}

IGeometry::StatusCode TetrahedralGeom::findElementSizes()
{
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfCells()}, std::vector<usize>{1}, 0.0f);
  Float32Array* tetSizes = DataArray<float32>::Create(*getDataStructure(), "Tet Volumes", std::move(dataStore), getId());
  GeometryHelpers::Topology::FindTetVolumes(getPolyhedra(), getVertices(), tetSizes);
  if(tetSizes == nullptr)
  {
    m_ElementSizesId.reset();
    return -1;
  }
  m_ElementSizesId = tetSizes->getId();
  return 1;
}

IGeometry::StatusCode TetrahedralGeom::findElementsContainingVert()
{
  auto* tetsContainingVert = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), "Elements Containing Vert", getId());
  GeometryHelpers::Connectivity::FindElementsContainingVert<uint16, MeshIndexType>(getPolyhedra(), tetsContainingVert, getNumberOfVertices());
  if(tetsContainingVert == nullptr)
  {
    m_CellContainingVertDataArrayId.reset();
    return -1;
  }
  m_CellContainingVertDataArrayId = tetsContainingVert->getId();
  return 1;
}

IGeometry::StatusCode TetrahedralGeom::findElementNeighbors()
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
  auto* tetNeighbors = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), "Tet Neighbors", getId());
  err = GeometryHelpers::Connectivity::FindElementNeighbors<uint16, MeshIndexType>(getPolyhedra(), getElementsContainingVert(), tetNeighbors, IGeometry::Type::Tetrahedral);
  if(tetNeighbors == nullptr)
  {
    m_CellNeighborsDataArrayId.reset();
    return -1;
  }
  m_CellNeighborsDataArrayId = tetNeighbors->getId();
  return err;
}

IGeometry::StatusCode TetrahedralGeom::findElementCentroids()
{
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfCells()}, std::vector<usize>{3}, 0.0f);
  DataArray<float>* tetCentroids = DataArray<float32>::Create(*getDataStructure(), "Tet Centroids", std::move(dataStore), getId());
  GeometryHelpers::Topology::FindElementCentroids(getPolyhedra(), getVertices(), tetCentroids);
  if(tetCentroids == nullptr)
  {
    m_CellCentroidsDataArrayId.reset();
    return -1;
  }
  m_CellCentroidsDataArrayId = tetCentroids->getId();
  return 1;
}

Point3D<float64> TetrahedralGeom::getParametricCenter() const
{
  return {0.25, 0.25, 0.25};
}

void TetrahedralGeom::getShapeFunctions([[maybe_unused]] const Point3D<float64>& pCoords, double* shape) const
{
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

IGeometry::StatusCode TetrahedralGeom::findEdges()
{
  auto* edgeList = createSharedEdgeList(0);
  GeometryHelpers::Connectivity::FindTetEdges(getPolyhedra(), edgeList);
  if(edgeList == nullptr)
  {
    m_EdgeDataArrayId.reset();
    return -1;
  }
  m_EdgeDataArrayId = edgeList->getId();
  return 1;
}

IGeometry::StatusCode TetrahedralGeom::findFaces()
{
  auto* triList = createSharedTriList(0);
  GeometryHelpers::Connectivity::FindTetFaces(getPolyhedra(), triList);
  if(triList == nullptr)
  {
    m_FaceListId.reset();
    return -1;
  }
  m_FaceListId = triList->getId();
  return 1;
}

IGeometry::StatusCode TetrahedralGeom::findUnsharedEdges()
{
  auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{2}, 0);
  auto* unsharedEdgeList = DataArray<MeshIndexType>::Create(*getDataStructure(), "Unshared Edge List", std::move(dataStore), getId());
  GeometryHelpers::Connectivity::FindUnsharedTetEdges(getPolyhedra(), unsharedEdgeList);
  if(unsharedEdgeList == nullptr)
  {
    m_UnsharedEdgeListId.reset();
    return -1;
  }
  m_UnsharedEdgeListId = unsharedEdgeList->getId();
  return 1;
}

IGeometry::StatusCode TetrahedralGeom::findUnsharedFaces()
{
  auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{3}, 0);
  auto* unsharedTriList = DataArray<MeshIndexType>::Create(*getDataStructure(), "Unshared Face List", std::move(dataStore), getId());
  GeometryHelpers::Connectivity::FindUnsharedTetFaces(getPolyhedra(), unsharedTriList);
  if(unsharedTriList == nullptr)
  {
    m_UnsharedFaceListId.reset();
    return -1;
  }
  m_UnsharedFaceListId = unsharedTriList->getId();
  return 1;
}
