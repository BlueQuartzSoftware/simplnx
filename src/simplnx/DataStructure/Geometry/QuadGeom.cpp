#include "QuadGeom.hpp"

#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"

#include <stdexcept>

using namespace nx::core;

QuadGeom::QuadGeom(DataStructure& dataStructure, std::string name)
: INodeGeometry2D(dataStructure, std::move(name))
{
}

QuadGeom::QuadGeom(DataStructure& dataStructure, std::string name, IdType importId)
: INodeGeometry2D(dataStructure, std::move(name), importId)
{
}

IGeometry::Type QuadGeom::getGeomType() const
{
  return IGeometry::Type::Quad;
}

DataObject::Type QuadGeom::getDataObjectType() const
{
  return DataObject::Type::QuadGeom;
}

BaseGroup::GroupType QuadGeom::getGroupType() const
{
  return GroupType::QuadGeom;
}

QuadGeom* QuadGeom::Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<QuadGeom>(new QuadGeom(dataStructure, std::move(name)));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

QuadGeom* QuadGeom::Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<QuadGeom>(new QuadGeom(dataStructure, std::move(name), importId));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

std::string QuadGeom::getTypeName() const
{
  return k_TypeName;
}

DataObject* QuadGeom::shallowCopy()
{
  return new QuadGeom(*this);
}

std::shared_ptr<DataObject> QuadGeom::deepCopy(const DataPath& copyPath)
{
  auto& dataStruct = getDataStructureRef();
  // Don't construct with identifier since it will get created when inserting into data structure
  auto copy = std::shared_ptr<QuadGeom>(new QuadGeom(dataStruct, copyPath.getTargetName()));
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

    if(m_FaceListId.has_value())
    {
      const DataPath copiedDataPath = copyPath.createChildPath(getFaces()->getName());
      // if this is not a parent of the data object, make a deep copy and insert it here
      if(!isParentOf(getFaces()))
      {
        const auto dataObjCopy = getFaces()->deepCopy(copiedDataPath);
      }
      copy->m_FaceListId = dataStruct.getId(copiedDataPath);
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
    return copy;
  }
  return nullptr;
}

usize QuadGeom::getNumberOfCells() const
{
  return getNumberOfFaces();
}

usize QuadGeom::getNumberOfVerticesPerFace() const
{
  return k_NumFaceVerts;
}

IGeometry::StatusCode QuadGeom::findElementSizes(bool recalculate)
{
  auto* quadSizes = getDataStructureRef().getDataAs<Float32Array>(m_ElementSizesId);
  if(quadSizes != nullptr && !recalculate)
  {
    return 0;
  }
  if(quadSizes == nullptr)
  {
    auto dataStore = std::make_unique<DataStore<float32>>(getNumberOfCells(), 0.0f);
    quadSizes = DataArray<float32>::Create(*getDataStructure(), k_VoxelSizes, std::move(dataStore), getId());
  }
  if(quadSizes == nullptr)
  {
    m_ElementSizesId.reset();
    return -1;
  }
  GeometryHelpers::Topology::Find2DElementAreas(getFaces(), getVertices(), quadSizes);
  m_ElementSizesId = quadSizes->getId();
  return 1;
}

IGeometry::StatusCode QuadGeom::findElementsContainingVert(bool recalculate)
{
  auto* quadsContainingVert = getDataStructureRef().getDataAs<ElementDynamicList>(m_CellContainingVertDataArrayId);
  if(quadsContainingVert != nullptr && !recalculate)
  {
    return 0;
  }
  if(quadsContainingVert == nullptr)
  {
    quadsContainingVert = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), k_EltsContainingVert, getId());
  }
  if(quadsContainingVert == nullptr)
  {
    m_CellContainingVertDataArrayId.reset();
    return -1;
  }
  GeometryHelpers::Connectivity::FindElementsContainingVert<uint16, MeshIndexType>(getFaces(), quadsContainingVert, getNumberOfVertices());
  m_CellContainingVertDataArrayId = quadsContainingVert->getId();
  return 1;
}

IGeometry::StatusCode QuadGeom::findElementNeighbors(bool recalculate)
{
  auto* quadNeighbors = getDataStructureRef().getDataAs<ElementDynamicList>(m_CellNeighborsDataArrayId);
  if(quadNeighbors != nullptr && !recalculate)
  {
    return 0;
  }

  StatusCode err = findElementsContainingVert(recalculate);
  if(err < 0)
  {
    m_CellNeighborsDataArrayId.reset();
    return err;
  }
  if(quadNeighbors == nullptr)
  {
    quadNeighbors = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), k_EltNeighbors, getId());
  }
  if(quadNeighbors == nullptr)
  {
    m_CellNeighborsDataArrayId.reset();
    return -1;
  }
  m_CellNeighborsDataArrayId = quadNeighbors->getId();
  err = GeometryHelpers::Connectivity::FindElementNeighbors<uint16, MeshIndexType>(getFaces(), getElementsContainingVert(), quadNeighbors, IGeometry::Type::Quad);
  if(err < 0)
  {
    return err;
  }
  return 1;
}

IGeometry::StatusCode QuadGeom::findElementCentroids(bool recalculate)
{
  auto* quadCentroids = getDataStructureRef().getDataAs<Float32Array>(m_CellCentroidsDataArrayId);
  if(quadCentroids != nullptr && !recalculate)
  {
    return 0;
  }
  if(quadCentroids == nullptr)
  {
    auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfCells()}, std::vector<usize>{3}, 0.0f);
    quadCentroids = DataArray<float32>::Create(*getDataStructure(), k_EltCentroids, std::move(dataStore), getId());
  }
  if(quadCentroids == nullptr)
  {
    m_CellCentroidsDataArrayId.reset();
    return -1;
  }
  GeometryHelpers::Topology::FindElementCentroids(getFaces(), getVertices(), quadCentroids);
  m_CellCentroidsDataArrayId = quadCentroids->getId();
  return 1;
}

Point3D<float64> QuadGeom::getParametricCenter() const
{
  return {0.5, 0.5, 0.0};
}

void QuadGeom::getShapeFunctions(const Point3D<float64>& pCoords, float64* shape) const
{
  float64 rm = 1.0 - pCoords[0];
  float64 sm = 1.0 - pCoords[1];

  shape[0] = -sm;
  shape[1] = sm;
  shape[2] = pCoords[1];
  shape[3] = -pCoords[1];
  shape[4] = -rm;
  shape[5] = -pCoords[0];
  shape[6] = pCoords[0];
  shape[7] = rm;
}

IGeometry::StatusCode QuadGeom::findEdges(bool recalculate)
{
  auto* edgeList = getDataStructureRef().getDataAs<DataArray<MeshIndexType>>(m_EdgeDataArrayId);
  if(edgeList != nullptr && !recalculate)
  {
    return 0;
  }
  if(edgeList == nullptr)
  {
    edgeList = createSharedEdgeList(0);
  }
  if(edgeList == nullptr)
  {
    m_EdgeDataArrayId.reset();
    return -1;
  }
  GeometryHelpers::Connectivity::Find2DElementEdges(getFaces(), edgeList);
  m_EdgeDataArrayId = edgeList->getId();
  return 1;
}

IGeometry::StatusCode QuadGeom::findUnsharedEdges(bool recalculate)
{
  auto* unsharedEdgeList = getDataStructureRef().getDataAs<DataArray<MeshIndexType>>(m_UnsharedEdgeListId);
  if(unsharedEdgeList != nullptr && !recalculate)
  {
    return 0;
  }
  if(unsharedEdgeList == nullptr)
  {
    auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{2}, 0);
    unsharedEdgeList = DataArray<MeshIndexType>::Create(*getDataStructure(), k_UnsharedEdges, std::move(dataStore), getId());
  }
  if(unsharedEdgeList == nullptr)
  {
    m_UnsharedEdgeListId.reset();
    return -1;
  }
  GeometryHelpers::Connectivity::Find2DUnsharedEdges<MeshIndexType>(getFaces(), unsharedEdgeList);
  m_UnsharedEdgeListId = unsharedEdgeList->getId();
  return 1;
}
