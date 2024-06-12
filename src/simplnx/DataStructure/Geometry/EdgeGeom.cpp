#include "EdgeGeom.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"

#include <stdexcept>

using namespace nx::core;

EdgeGeom::EdgeGeom(DataStructure& dataStructure, std::string name)
: INodeGeometry1D(dataStructure, std::move(name))
{
  m_UnitDimensionality = 1;
}

EdgeGeom::EdgeGeom(DataStructure& dataStructure, std::string name, IdType importId)
: INodeGeometry1D(dataStructure, std::move(name), importId)
{
  m_UnitDimensionality = 1;
}

DataObject::Type EdgeGeom::getDataObjectType() const
{
  return DataObject::Type::EdgeGeom;
}

EdgeGeom* EdgeGeom::Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<EdgeGeom>(new EdgeGeom(dataStructure, std::move(name)));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

EdgeGeom* EdgeGeom::Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<EdgeGeom>(new EdgeGeom(dataStructure, std::move(name), importId));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

IGeometry::Type EdgeGeom::getGeomType() const
{
  return IGeometry::Type::Edge;
}

BaseGroup::GroupType EdgeGeom::getGroupType() const
{
  return GroupType::EdgeGeom;
}

std::string EdgeGeom::getTypeName() const
{
  return k_TypeName;
}

DataObject* EdgeGeom::shallowCopy()
{
  return new EdgeGeom(*this);
}

std::shared_ptr<DataObject> EdgeGeom::deepCopy(const DataPath& copyPath)
{
  auto& dataStruct = getDataStructureRef();
  // Don't construct with identifier since it will get created when inserting into data structure
  auto copy = std::shared_ptr<EdgeGeom>(new EdgeGeom(dataStruct, copyPath.getTargetName()));
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

    if(m_EdgeDataArrayId.has_value())
    {
      const DataPath copiedDataPath = copyPath.createChildPath(getEdges()->getName());
      // if this is not a parent of the data object, make a deep copy and insert it here
      if(!isParentOf(getEdges()))
      {
        const auto dataObjCopy = getEdges()->deepCopy(copiedDataPath);
      }
      copy->m_EdgeDataArrayId = dataStruct.getId(copiedDataPath);
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
    return copy;
  }
  return nullptr;
}

IGeometry::StatusCode EdgeGeom::findElementSizes(bool recalculate)
{
  auto* sizes = getDataStructureRef().getDataAsUnsafe<Float32Array>(m_ElementSizesId);
  if(sizes != nullptr && !recalculate)
  {
    return 0;
  }
  if(sizes == nullptr)
  {
    auto dataStore = std::make_unique<DataStore<float32>>(getNumberOfCells(), 0.0f);
    sizes = DataArray<float32>::Create(*getDataStructure(), k_VoxelSizes, std::move(dataStore), getId());
  }
  if(sizes == nullptr)
  {
    m_ElementSizesId.reset();
    return -1;
  }
  m_ElementSizesId = sizes->getId();

  std::array<Point3Df, 2> verts = {Point3Df(0.0f, 0.0f, 0.0f), Point3Df(0.0f, 0.0f, 0.0f)};

  for(usize i = 0; i < INodeGeometry1D::getNumberOfCells(); i++)
  {
    getEdgeCoordinates(i, verts);
    float32 length = 0.0f;
    for(usize j = 0; j < 3; j++)
    {
      length += (verts[0][j] - verts[1][j]) * (verts[0][j] - verts[1][j]);
    }
    (*sizes)[i] = std::sqrt(length);
  }

  return 1;
}

usize EdgeGeom::getNumberOfVerticesPerEdge() const
{
  return k_NumEdgeVerts;
}

IGeometry::StatusCode EdgeGeom::findElementsContainingVert(bool recalculate)
{
  auto* containsVert = getDataStructureRef().getDataAsUnsafe<ElementDynamicList>(m_CellContainingVertDataArrayId);
  if(containsVert != nullptr && !recalculate)
  {
    return 0;
  }
  if(containsVert == nullptr)
  {
    m_CellContainingVertDataArrayId.reset();
    return -1;
  }
  GeometryHelpers::Connectivity::FindElementsContainingVert<uint16, MeshIndexType>(getEdges(), containsVert, getNumberOfVertices());
  m_CellContainingVertDataArrayId = containsVert->getId();
  return 1;
}

IGeometry::StatusCode EdgeGeom::findElementNeighbors(bool recalculate)
{
  auto* edgeNeighbors = getDataStructureRef().getDataAsUnsafe<ElementDynamicList>(m_CellNeighborsDataArrayId);
  if(edgeNeighbors != nullptr && !recalculate)
  {
    return 0;
  }

  StatusCode err = findElementsContainingVert(recalculate);
  if(err < 0)
  {
    m_CellNeighborsDataArrayId.reset();
    return err;
  }
  if(edgeNeighbors == nullptr)
  {
    edgeNeighbors = ElementDynamicList::Create(*getDataStructure(), k_EltNeighbors, getId());
  }
  if(edgeNeighbors == nullptr)
  {
    m_CellNeighborsDataArrayId.reset();
    return -1;
  }
  m_CellNeighborsDataArrayId = edgeNeighbors->getId();
  err = GeometryHelpers::Connectivity::FindElementNeighbors<uint16, MeshIndexType>(getEdges(), getElementsContainingVert(), edgeNeighbors, IGeometry::Type::Edge);
  if(edgeNeighbors == nullptr)
  {
    m_CellNeighborsDataArrayId.reset();
    return -1;
  }
  return 1;
}

IGeometry::StatusCode EdgeGeom::findElementCentroids(bool recalculate)
{
  auto* edgeCentroids = getDataStructureRef().getDataAsUnsafe<Float32Array>(m_CellCentroidsDataArrayId);
  if(edgeCentroids != nullptr && !recalculate)
  {
    return 0;
  }
  if(edgeCentroids == nullptr)
  {
    auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfCells()}, std::vector<usize>{3}, 0.0f);
    edgeCentroids = DataArray<float32>::Create(*getDataStructure(), k_EltCentroids, std::move(dataStore), getId());
  }
  if(edgeCentroids == nullptr)
  {
    m_CellCentroidsDataArrayId.reset();
    return -1;
  }
  GeometryHelpers::Topology::FindElementCentroids(getEdges(), getVertices(), edgeCentroids);
  m_CellCentroidsDataArrayId = edgeCentroids->getId();
  return 1;
}

Point3D<float64> EdgeGeom::getParametricCenter() const
{
  return {0.5, 0.0, 0.0};
}

void EdgeGeom::getShapeFunctions([[maybe_unused]] const Point3D<float64>& pCoords, float64* shape) const
{
  shape[0] = -1.0;
  shape[1] = 1.0;
}
