#include "INodeGeometry1D.hpp"

#include "simplnx/Utilities/DataObjectUtilities.hpp"

namespace nx::core
{
INodeGeometry1D::INodeGeometry1D(DataStructure& dataStructure, std::string name)
: INodeGeometry0D(dataStructure, std::move(name))
{
}

INodeGeometry1D::INodeGeometry1D(DataStructure& dataStructure, std::string name, IdType importId)
: INodeGeometry0D(dataStructure, std::move(name), importId)
{
}

const std::optional<INodeGeometry1D::IdType>& INodeGeometry1D::getEdgeListDataArrayId() const
{
  return m_EdgeDataArrayId;
}

INodeGeometry1D::SharedEdgeList* INodeGeometry1D::getEdges()
{
  if(!m_EdgeDataArrayId.has_value())
  {
    return nullptr;
  }
  return getDataStructureRef().getDataAs<SharedEdgeList>(m_EdgeDataArrayId);
}

const INodeGeometry1D::SharedEdgeList* INodeGeometry1D::getEdges() const
{
  if(!m_EdgeDataArrayId.has_value())
  {
    return nullptr;
  }
  return getDataStructureRef().getDataAs<SharedEdgeList>(m_EdgeDataArrayId);
}

INodeGeometry1D::SharedEdgeList& INodeGeometry1D::getEdgesRef()
{
  if(!m_EdgeDataArrayId.has_value())
  {
    throw std::runtime_error(
        fmt::format("INodeGeometry1D::{} threw runtime exception. The geometry with name '{}' does not have a shared edge list assigned.\n  {}:{}", __func__, getName(), __FILE__, __LINE__));
  }
  return getDataStructureRef().getDataRefAs<SharedEdgeList>(m_EdgeDataArrayId.value());
}

const INodeGeometry1D::SharedEdgeList& INodeGeometry1D::getEdgesRef() const
{
  if(!m_EdgeDataArrayId.has_value())
  {
    throw std::runtime_error(
        fmt::format("INodeGeometry1D::{} threw runtime exception. The geometry with name '{}' does not have a shared edge list assigned.\n  {}:{}", __func__, getName(), __FILE__, __LINE__));
  }
  return getDataStructureRef().getDataRefAs<SharedEdgeList>(m_EdgeDataArrayId.value());
}

void INodeGeometry1D::setEdgeList(const SharedEdgeList& edges)
{
  m_EdgeDataArrayId = edges.getId();
}

std::optional<DataObject::IdType> INodeGeometry1D::getEdgeListId() const
{
  return m_EdgeDataArrayId;
}

void INodeGeometry1D::setEdgeListId(const std::optional<IdType>& edgeList)
{
  m_EdgeDataArrayId = edgeList;
}

void INodeGeometry1D::resizeEdgeList(usize size)
{
  getEdgesRef().getIDataStoreRef().resizeTuples({size});
}

usize INodeGeometry1D::getNumberOfCells() const
{
  return getNumberOfEdges();
}

usize INodeGeometry1D::getNumberOfEdges() const
{
  const auto* edgesPtr = getEdges();
  return edgesPtr == nullptr ? 0 : edgesPtr->getNumberOfTuples();
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
  return getDataStructureRef().getDataAs<ElementDynamicList>(m_CellContainingVertDataArrayId);
}

void INodeGeometry1D::deleteElementsContainingVert()
{
  getDataStructureRef().removeData(m_CellContainingVertDataArrayId);
  m_CellContainingVertDataArrayId.reset();
}

const INodeGeometry1D::ElementDynamicList* INodeGeometry1D::getElementNeighbors() const
{
  return getDataStructureRef().getDataAs<ElementDynamicList>(m_CellNeighborsDataArrayId);
}

void INodeGeometry1D::deleteElementNeighbors()
{
  if(!getDataStructureRef().removeData(m_CellNeighborsDataArrayId))
  {
    throw std::runtime_error(fmt::format("{}({}): Function {}: Unable to remove Element Neighbors", "deleteElementNeighbors()", __FILE__, __LINE__));
  }
  m_CellNeighborsDataArrayId.reset();
}

const Float32Array* INodeGeometry1D::getElementCentroids() const
{
  return getDataStructureRef().getDataAs<Float32Array>(m_CellCentroidsDataArrayId);
}

void INodeGeometry1D::deleteElementCentroids()
{
  getDataStructureRef().removeData(m_CellCentroidsDataArrayId);
  m_CellCentroidsDataArrayId.reset();
}

const std::optional<INodeGeometry1D::IdType>& INodeGeometry1D::getEdgeAttributeMatrixId() const
{
  return m_EdgeAttributeMatrixId;
}

AttributeMatrix* INodeGeometry1D::getEdgeAttributeMatrix()
{
  if(!m_EdgeAttributeMatrixId.has_value())
  {
    return nullptr;
  }
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_EdgeAttributeMatrixId);
}

const AttributeMatrix* INodeGeometry1D::getEdgeAttributeMatrix() const
{
  if(!m_EdgeAttributeMatrixId.has_value())
  {
    return nullptr;
  }
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_EdgeAttributeMatrixId);
}

AttributeMatrix& INodeGeometry1D::getEdgeAttributeMatrixRef()
{
  if(!m_EdgeAttributeMatrixId.has_value())
  {
    throw std::runtime_error(
        fmt::format("INodeGeometry1D::{} threw runtime exception. The geometry with name '{}' does not have an edge attribute matrix assigned.\n  {}:{}", __func__, getName(), __FILE__, __LINE__));
  }
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_EdgeAttributeMatrixId.value());
}

const AttributeMatrix& INodeGeometry1D::getEdgeAttributeMatrixRef() const
{
  if(!m_EdgeAttributeMatrixId.has_value())
  {
    throw std::runtime_error(
        fmt::format("INodeGeometry1D::{} threw runtime exception. The geometry with name '{}' does not have an edge attribute matrix assigned.\n  {}:{}", __func__, getName(), __FILE__, __LINE__));
  }
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_EdgeAttributeMatrixId.value());
}

DataPath INodeGeometry1D::getEdgeAttributeMatrixDataPath() const
{
  return getEdgeAttributeMatrixRef().getDataPaths().at(0);
}

void INodeGeometry1D::setEdgeAttributeMatrix(const AttributeMatrix& attributeMatrix)
{
  m_EdgeAttributeMatrixId = attributeMatrix.getId();
}

void INodeGeometry1D::setEdgeDataId(const std::optional<IdType>& edgeDataId)
{
  m_EdgeAttributeMatrixId = edgeDataId;
}

std::optional<DataObject::IdType> INodeGeometry1D::getElementContainingVertId() const
{
  return m_CellContainingVertDataArrayId;
}

std::optional<DataObject::IdType> INodeGeometry1D::getElementNeighborsId() const
{
  return m_CellNeighborsDataArrayId;
}

std::optional<DataObject::IdType> INodeGeometry1D::getElementCentroidsId() const
{
  return m_CellCentroidsDataArrayId;
}

std::optional<DataObject::IdType> INodeGeometry1D::getElementSizesId() const
{
  return m_ElementSizesId;
}

void INodeGeometry1D::setElementContainingVertId(const std::optional<IdType>& elementsContainingVertId)
{
  m_CellContainingVertDataArrayId = elementsContainingVertId;
}

void INodeGeometry1D::setElementNeighborsId(const std::optional<IdType>& elementNeighborsId)
{
  m_CellNeighborsDataArrayId = elementNeighborsId;
}

void INodeGeometry1D::setElementCentroidsId(const std::optional<IdType>& centroidsId)
{
  m_CellCentroidsDataArrayId = centroidsId;
}

void INodeGeometry1D::setElementSizesId(const std::optional<IdType>& sizesId)
{
  m_ElementSizesId = sizesId;
}

void INodeGeometry1D::checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds)
{
  INodeGeometry0D::checkUpdatedIdsImpl(updatedIds);

  std::vector<bool> visited(7, false);

  for(const auto& updatedId : updatedIds)
  {
    m_EdgeAttributeMatrixId = nx::core::VisitDataStructureId(m_EdgeAttributeMatrixId, updatedId, visited, 0);
    m_EdgeDataArrayId = nx::core::VisitDataStructureId(m_EdgeDataArrayId, updatedId, visited, 1);
    m_CellContainingVertDataArrayId = nx::core::VisitDataStructureId(m_CellContainingVertDataArrayId, updatedId, visited, 2);
    m_CellNeighborsDataArrayId = nx::core::VisitDataStructureId(m_CellNeighborsDataArrayId, updatedId, visited, 3);
    m_CellCentroidsDataArrayId = nx::core::VisitDataStructureId(m_CellCentroidsDataArrayId, updatedId, visited, 4);
    m_ElementSizesId = nx::core::VisitDataStructureId(m_ElementSizesId, updatedId, visited, 5);
  }
}

Result<> INodeGeometry1D::validateGeometry() const
{
  // Validate the next lower dimension geometry
  Result<> result = INodeGeometry0D::validateGeometry();

  usize numTuples = getNumberOfEdges();
  const AttributeMatrix* amPtr = getEdgeAttributeMatrix();
  if(nullptr == amPtr)
  {
    return result;
  }
  usize amNumTuples = amPtr->getNumTuples();

  if(numTuples != amNumTuples)
  {
    return MergeResults(
        result, MakeErrorResult(-4501, fmt::format("Edge Geometry '{}' has {} edges but the edge Attribute Matrix '{}' has {} total tuples.", getName(), numTuples, amPtr->getName(), amNumTuples)));
  }
  return result;
}

} // namespace nx::core
