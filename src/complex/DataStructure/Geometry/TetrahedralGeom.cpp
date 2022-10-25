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

BaseGroup::GroupType TetrahedralGeom::getGroupType() const
{
  return GroupType::TetrahedralGeom;
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
  auto& dataStruct = getDataStructureRef();
  // Don't construct with id since it will get created when inserting into data structure
  auto copy = std::shared_ptr<TetrahedralGeom>(new TetrahedralGeom(dataStruct, copyPath.getTargetName()));
  if(!dataStruct.containsData(copyPath) && dataStruct.insert(copy, copyPath.getParent()))
  {
    auto dataMapCopy = getDataMap().deepCopy(copyPath);

    if(m_VertexAttributeMatrixId.has_value())
    {
      const DataPath copiedDataPath = copyPath.createChildPath(getVertexAttributeMatrix()->getName());
      // if this is not a parent of the cell data object, make a deep copy and insert it here
      if(!isParentOf(getVertexAttributeMatrix()))
      {
        const auto dataObjCopy = getVertexAttributeMatrix()->deepCopy(copiedDataPath);
      }
      copy->m_VertexAttributeMatrixId = dataStruct.getId(copiedDataPath);
    }

    if(m_VertexDataArrayId.has_value())
    {
      const DataPath copiedDataPath = copyPath.createChildPath(getVertices()->getName());
      // if this is not a parent of the data object, make a deep copy and insert it here
      if(!isParentOf(getVertices()))
      {
        const auto dataObjCopy = getVertices()->deepCopy(copiedDataPath);
      }
      copy->m_VertexDataArrayId = dataStruct.getId(copiedDataPath);
    }

    if(m_EdgeAttributeMatrixId.has_value())
    {
      const DataPath copiedDataPath = copyPath.createChildPath(getEdgeAttributeMatrix()->getName());
      // if this is not a parent of the cell data object, make a deep copy and insert it here
      if(!isParentOf(getEdgeAttributeMatrix()))
      {
        const auto dataObjCopy = getEdgeAttributeMatrix()->deepCopy(copiedDataPath);
      }
      copy->m_EdgeAttributeMatrixId = dataStruct.getId(copiedDataPath);
    }

    if(m_FaceAttributeMatrixId.has_value())
    {
      const DataPath copiedDataPath = copyPath.createChildPath(getFaceAttributeMatrix()->getName());
      // if this is not a parent of the cell data object, make a deep copy and insert it here
      if(!isParentOf(getFaceAttributeMatrix()))
      {
        const auto dataObjCopy = getFaceAttributeMatrix()->deepCopy(copiedDataPath);
      }
      copy->m_FaceAttributeMatrixId = dataStruct.getId(copiedDataPath);
    }

    if(m_PolyhedronAttributeMatrixId.has_value())
    {
      const DataPath copiedDataPath = copyPath.createChildPath(getPolyhedraAttributeMatrix()->getName());
      // if this is not a parent of the cell data object, make a deep copy and insert it here
      if(!isParentOf(getPolyhedraAttributeMatrix()))
      {
        const auto dataObjCopy = getPolyhedraAttributeMatrix()->deepCopy(copiedDataPath);
      }
      copy->m_PolyhedronAttributeMatrixId = dataStruct.getId(copiedDataPath);
    }

    if(m_PolyhedronListId.has_value())
    {
      const DataPath copiedDataPath = copyPath.createChildPath(getPolyhedra()->getName());
      // if this is not a parent of the data object, make a deep copy and insert it here
      if(!isParentOf(getPolyhedra()))
      {
        const auto dataObjCopy = getPolyhedra()->deepCopy(copiedDataPath);
      }
      copy->m_PolyhedronListId = dataStruct.getId(copiedDataPath);
    }

    if(const auto voxelSizesCopy = dataStruct.getDataAs<Float32Array>(copyPath.createChildPath(k_VoxelSizes)); voxelSizesCopy != nullptr)
    {
      copy->m_ElementSizesId = voxelSizesCopy->getId();
    }
    if(const auto eltContVertCopy = dataStruct.getDataAs<ElementDynamicList>(copyPath.createChildPath(k_EltsContainingVert)); eltContVertCopy != nullptr)
    {
      copy->m_CellContainingVertDataArrayId = eltContVertCopy->getId();
    }
    if(const auto eltNeighborsCopy = dataStruct.getDataAs<ElementDynamicList>(copyPath.createChildPath(k_EltNeighbors)); eltNeighborsCopy != nullptr)
    {
      copy->m_CellNeighborsDataArrayId = eltNeighborsCopy->getId();
    }
    if(const auto eltCentroidsCopy = dataStruct.getDataAs<Float32Array>(copyPath.createChildPath(k_EltCentroids)); eltCentroidsCopy != nullptr)
    {
      copy->m_CellCentroidsDataArrayId = eltCentroidsCopy->getId();
    }
    if(const auto unsharedEdgesCopy = dataStruct.getDataAs<DataArray<MeshIndexType>>(copyPath.createChildPath(k_UnsharedEdges)); unsharedEdgesCopy != nullptr)
    {
      copy->m_UnsharedEdgeListId = unsharedEdgesCopy->getId();
    }
    if(const auto edgesCopy = dataStruct.getDataAs<DataArray<MeshIndexType>>(copyPath.createChildPath(INodeGeometry2D::k_Edges)); edgesCopy != nullptr)
    {
      copy->m_EdgeDataArrayId = edgesCopy->getId();
    }
    if(const auto unsharedFacesCopy = dataStruct.getDataAs<DataArray<MeshIndexType>>(copyPath.createChildPath(k_UnsharedFaces)); unsharedFacesCopy != nullptr)
    {
      copy->m_UnsharedFaceListId = unsharedFacesCopy->getId();
    }
    if(const auto facesCopy = dataStruct.getDataAs<DataArray<MeshIndexType>>(copyPath.createChildPath(INodeGeometry3D::k_TriangleFaceList)); facesCopy != nullptr)
    {
      copy->m_FaceListId = facesCopy->getId();
    }
    return copy;
  }
  return nullptr;
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
  Float32Array* tetSizes = DataArray<float32>::Create(*getDataStructure(), k_VoxelSizes, std::move(dataStore), getId());
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
  auto* tetsContainingVert = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), k_EltsContainingVert, getId());
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
  auto* tetNeighbors = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), k_EltNeighbors, getId());
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
  DataArray<float>* tetCentroids = DataArray<float32>::Create(*getDataStructure(), k_EltCentroids, std::move(dataStore), getId());
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
  auto* unsharedEdgeList = DataArray<MeshIndexType>::Create(*getDataStructure(), k_UnsharedEdges, std::move(dataStore), getId());
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
  auto* unsharedTriList = DataArray<MeshIndexType>::Create(*getDataStructure(), k_UnsharedFaces, std::move(dataStore), getId());
  GeometryHelpers::Connectivity::FindUnsharedTetFaces(getPolyhedra(), unsharedTriList);
  if(unsharedTriList == nullptr)
  {
    m_UnsharedFaceListId.reset();
    return -1;
  }
  m_UnsharedFaceListId = unsharedTriList->getId();
  return 1;
}
