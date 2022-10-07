#include "INodeGeometry1D.hpp"

namespace complex
{
INodeGeometry1D::INodeGeometry1D(DataStructure& ds, std::string name)
: INodeGeometry0D(ds, std::move(name))
{
}

INodeGeometry1D::INodeGeometry1D(DataStructure& ds, std::string name, IdType importId)
: INodeGeometry0D(ds, std::move(name), importId)
{
}

const std::optional<INodeGeometry1D::IdType>& INodeGeometry1D::getEdgeListId() const
{
  return m_EdgeListId;
}

INodeGeometry1D::SharedEdgeList* INodeGeometry1D::getEdges()
{
  return getDataStructureRef().getDataAs<SharedEdgeList>(m_EdgeListId);
}

const INodeGeometry1D::SharedEdgeList* INodeGeometry1D::getEdges() const
{
  return getDataStructureRef().getDataAs<SharedEdgeList>(m_EdgeListId);
}

INodeGeometry1D::SharedEdgeList& INodeGeometry1D::getEdgesRef()
{
  return getDataStructureRef().getDataRefAs<SharedEdgeList>(m_EdgeListId.value());
}

const INodeGeometry1D::SharedEdgeList& INodeGeometry1D::getEdgesRef() const
{
  return getDataStructureRef().getDataRefAs<SharedEdgeList>(m_EdgeListId.value());
}

void INodeGeometry1D::setEdges(const SharedEdgeList& edges)
{
  m_EdgeListId = edges.getId();
}

void INodeGeometry1D::setEdgeListId(const std::optional<IdType>& edgeList)
{
  m_EdgeListId = edgeList;
}

void INodeGeometry1D::resizeEdgeList(usize size)
{
  getEdgesRef().getIDataStoreRef().reshapeTuples({size});
}

usize INodeGeometry1D::getNumberOfCells() const
{
  auto& edges = getEdgesRef();
  return edges.getNumberOfTuples();
}

usize INodeGeometry1D::getNumberOfEdges() const
{
  auto& edges = getEdgesRef();
  return edges.getNumberOfTuples();
}

void INodeGeometry1D::setEdgePointIds(usize edgeId, nonstd::span<usize> vertexIds)
{
  auto& edges = getEdgesRef();
  const usize offset = edgeId * k_NumEdgeVerts;
  if(offset + k_NumEdgeVerts > edges.getSize())
  {
    return;
  }
  for(usize i = 0; i < k_NumEdgeVerts; i++)
  {
    edges[offset + i] = vertexIds[i];
  }
}

void INodeGeometry1D::getEdgePointIds(usize edgeId, nonstd::span<usize> vertexIds) const
{
  auto& cells = getEdgesRef();
  const usize offset = edgeId * k_NumEdgeVerts;
  if(offset + k_NumEdgeVerts > cells.getSize())
  {
    return;
  }
  for(usize i = 0; i < k_NumEdgeVerts; i++)
  {
    vertexIds[i] = cells.at(offset + i);
  }
}

void INodeGeometry1D::getEdgeCoordinates(usize edgeId, nonstd::span<Point3Df> coords) const
{
  std::array<usize, k_NumEdgeVerts> verts = {0, 0};
  getEdgePointIds(edgeId, verts);
  coords[0] = getVertexCoordinate(verts[0]);
  coords[1] = getVertexCoordinate(verts[1]);
}

const INodeGeometry1D::ElementDynamicList* INodeGeometry1D::getElementsContainingVert() const
{
  return getDataStructureRef().getDataAs<ElementDynamicList>(m_CellContainingVertId);
}

void INodeGeometry1D::deleteElementsContainingVert()
{
  getDataStructureRef().removeData(m_CellContainingVertId);
  m_CellContainingVertId.reset();
}

const INodeGeometry1D::ElementDynamicList* INodeGeometry1D::getElementNeighbors() const
{
  return getDataStructureRef().getDataAs<ElementDynamicList>(m_CellNeighborsId);
}

void INodeGeometry1D::deleteElementNeighbors()
{
  getDataStructureRef().removeData(m_CellNeighborsId);
  m_CellNeighborsId.reset();
}

const Float32Array* INodeGeometry1D::getElementCentroids() const
{
  return getDataStructureRef().getDataAs<Float32Array>(m_CellCentroidsId);
}

void INodeGeometry1D::deleteElementCentroids()
{
  getDataStructureRef().removeData(m_CellCentroidsId);
  m_CellCentroidsId.reset();
}

const std::optional<INodeGeometry1D::IdType>& INodeGeometry1D::getEdgeDataId() const
{
  return m_EdgeDataId;
}

AttributeMatrix* INodeGeometry1D::getEdgeData()
{
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_EdgeDataId);
}

const AttributeMatrix* INodeGeometry1D::getEdgeData() const
{
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_EdgeDataId);
}

AttributeMatrix& INodeGeometry1D::getEdgeDataRef()
{
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_EdgeDataId.value());
}

const AttributeMatrix& INodeGeometry1D::getEdgeDataRef() const
{
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_EdgeDataId.value());
}

DataPath INodeGeometry1D::getEdgeDataPath() const
{
  return getEdgeDataRef().getDataPaths().at(0);
}

void INodeGeometry1D::setEdgeData(const AttributeMatrix& attributeMatrix)
{
  m_EdgeDataId = attributeMatrix.getId();
}

void INodeGeometry1D::setEdgeDataId(const std::optional<IdType>& edgeDataId)
{
  m_EdgeDataId = edgeDataId;
}

std::optional<DataObject::IdType> INodeGeometry1D::getElementContainingVertId() const
{
  return m_CellContainingVertId;
}

std::optional<DataObject::IdType> INodeGeometry1D::getElementNeighborsId() const
{
  return m_CellNeighborsId;
}

std::optional<DataObject::IdType> INodeGeometry1D::getElementCentroidsId() const
{
  return m_CellCentroidsId;
}

std::optional<DataObject::IdType> INodeGeometry1D::getElementSizesId() const
{
  return m_ElementSizesId;
}

void INodeGeometry1D::setElementContainingVertId(const std::optional<IdType>& elementsContainingVertId)
{
  m_CellContainingVertId = elementsContainingVertId;
}

void INodeGeometry1D::setElementNeighborsId(const std::optional<IdType>& elementNeighborsId)
{
  m_CellNeighborsId = elementNeighborsId;
}

void INodeGeometry1D::setElementCentroidsId(const std::optional<IdType>& centroidsId)
{
  m_CellCentroidsId = centroidsId;
}

void INodeGeometry1D::setElementSizesId(const std::optional<IdType>& sizesId)
{
  m_ElementSizesId = sizesId;
}

void INodeGeometry1D::checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds)
{
  INodeGeometry0D::checkUpdatedIdsImpl(updatedIds);

  for(const auto& updatedId : updatedIds)
  {
    if(m_VertexListId == updatedId.first)
    {
      m_VertexListId = updatedId.second;
    }

    if(m_EdgeDataId == updatedId.first)
    {
      m_EdgeDataId = updatedId.second;
    }

    if(m_EdgeListId == updatedId.first)
    {
      m_EdgeListId = updatedId.second;
    }

    if(m_CellContainingVertId == updatedId.first)
    {
      m_CellContainingVertId = updatedId.second;
    }

    if(m_CellNeighborsId == updatedId.first)
    {
      m_CellNeighborsId = updatedId.second;
    }

    if(m_CellCentroidsId == updatedId.first)
    {
      m_CellCentroidsId = updatedId.second;
    }

    if(m_ElementSizesId == updatedId.first)
    {
      m_ElementSizesId = updatedId.second;
    }
  }
}
} // namespace complex
