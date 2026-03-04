#include "AbstractNodeGeometry1D.hpp"

#include "simplnx/Utilities/DataObjectUtilities.hpp"

namespace nx::core
{
AbstractNodeGeometry1D::AbstractNodeGeometry1D(DataStructure& dataStructure, std::string name)
: AbstractNodeGeometry0D(dataStructure, std::move(name))
{
}

AbstractNodeGeometry1D::AbstractNodeGeometry1D(DataStructure& dataStructure, std::string name, IdType importId)
: AbstractNodeGeometry0D(dataStructure, std::move(name), importId)
{
}

const std::optional<AbstractNodeGeometry1D::IdType>& AbstractNodeGeometry1D::getEdgeListDataArrayId() const
{
  return m_EdgeDataArrayId;
}

AbstractNodeGeometry1D::SharedEdgeList* AbstractNodeGeometry1D::getEdges()
{
  if(!m_EdgeDataArrayId.has_value())
  {
    return nullptr;
  }
  return getDataStructureRef().getDataAs<SharedEdgeList>(m_EdgeDataArrayId);
}

const AbstractNodeGeometry1D::SharedEdgeList* AbstractNodeGeometry1D::getEdges() const
{
  if(!m_EdgeDataArrayId.has_value())
  {
    return nullptr;
  }
  return getDataStructureRef().getDataAs<SharedEdgeList>(m_EdgeDataArrayId);
}

AbstractNodeGeometry1D::SharedEdgeList& AbstractNodeGeometry1D::getEdgesRef()
{
  if(!m_EdgeDataArrayId.has_value())
  {
    throw std::runtime_error(
        fmt::format("AbstractNodeGeometry1D::{} threw runtime exception. The geometry with name '{}' does not have a shared edge list assigned.\n  {}:{}", __func__, getName(), __FILE__, __LINE__));
  }
  return getDataStructureRef().getDataRefAs<SharedEdgeList>(m_EdgeDataArrayId.value());
}

const AbstractNodeGeometry1D::SharedEdgeList& AbstractNodeGeometry1D::getEdgesRef() const
{
  if(!m_EdgeDataArrayId.has_value())
  {
    throw std::runtime_error(
        fmt::format("AbstractNodeGeometry1D::{} threw runtime exception. The geometry with name '{}' does not have a shared edge list assigned.\n  {}:{}", __func__, getName(), __FILE__, __LINE__));
  }
  return getDataStructureRef().getDataRefAs<SharedEdgeList>(m_EdgeDataArrayId.value());
}

void AbstractNodeGeometry1D::setEdgeList(const SharedEdgeList& edges)
{
  m_EdgeDataArrayId = edges.getId();
}

std::optional<AbstractDataObject::IdType> AbstractNodeGeometry1D::getEdgeListId() const
{
  return m_EdgeDataArrayId;
}

void AbstractNodeGeometry1D::setEdgeListId(const std::optional<IdType>& edgeList)
{
  m_EdgeDataArrayId = edgeList;
}

void AbstractNodeGeometry1D::resizeEdgeList(usize size)
{
  getEdgesRef().getIDataStoreRef().resizeTuples({size});
}

usize AbstractNodeGeometry1D::getNumberOfCells() const
{
  return getNumberOfEdges();
}

usize AbstractNodeGeometry1D::getNumberOfEdges() const
{
  const auto* edgesPtr = getEdges();
  return edgesPtr == nullptr ? 0 : edgesPtr->getNumberOfTuples();
}

void AbstractNodeGeometry1D::setEdgePointIds(usize edgeId, nonstd::span<usize> vertexIds)
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

void AbstractNodeGeometry1D::getEdgePointIds(usize edgeId, nonstd::span<usize> vertexIds) const
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

void AbstractNodeGeometry1D::getEdgeCoordinates(usize edgeId, nonstd::span<Point3Df> coords) const
{
  std::array<usize, k_NumEdgeVerts> verts = {0, 0};
  getEdgePointIds(edgeId, verts);
  coords[0] = getVertexCoordinate(verts[0]);
  coords[1] = getVertexCoordinate(verts[1]);
}

const AbstractNodeGeometry1D::ElementDynamicList* AbstractNodeGeometry1D::getElementsContainingVert() const
{
  return getDataStructureRef().getDataAs<ElementDynamicList>(m_CellContainingVertDataArrayId);
}

void AbstractNodeGeometry1D::deleteElementsContainingVert()
{
  getDataStructureRef().removeData(m_CellContainingVertDataArrayId);
  m_CellContainingVertDataArrayId.reset();
}

const AbstractNodeGeometry1D::ElementDynamicList* AbstractNodeGeometry1D::getElementNeighbors() const
{
  return getDataStructureRef().getDataAs<ElementDynamicList>(m_CellNeighborsDataArrayId);
}

void AbstractNodeGeometry1D::deleteElementNeighbors()
{
  if(!getDataStructureRef().removeData(m_CellNeighborsDataArrayId))
  {
    throw std::runtime_error(fmt::format("{}({}): Function {}: Unable to remove Element Neighbors", "deleteElementNeighbors()", __FILE__, __LINE__));
  }
  m_CellNeighborsDataArrayId.reset();
}

const Float32Array* AbstractNodeGeometry1D::getElementCentroids() const
{
  return getDataStructureRef().getDataAs<Float32Array>(m_CellCentroidsDataArrayId);
}

void AbstractNodeGeometry1D::deleteElementCentroids()
{
  getDataStructureRef().removeData(m_CellCentroidsDataArrayId);
  m_CellCentroidsDataArrayId.reset();
}

const std::optional<AbstractNodeGeometry1D::IdType>& AbstractNodeGeometry1D::getEdgeAttributeMatrixId() const
{
  return m_EdgeAttributeMatrixId;
}

AttributeMatrix* AbstractNodeGeometry1D::getEdgeAttributeMatrix()
{
  if(!m_EdgeAttributeMatrixId.has_value())
  {
    return nullptr;
  }
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_EdgeAttributeMatrixId);
}

const AttributeMatrix* AbstractNodeGeometry1D::getEdgeAttributeMatrix() const
{
  if(!m_EdgeAttributeMatrixId.has_value())
  {
    return nullptr;
  }
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_EdgeAttributeMatrixId);
}

AttributeMatrix& AbstractNodeGeometry1D::getEdgeAttributeMatrixRef()
{
  if(!m_EdgeAttributeMatrixId.has_value())
  {
    throw std::runtime_error(fmt::format("AbstractNodeGeometry1D::{} threw runtime exception. The geometry with name '{}' does not have an edge attribute matrix assigned.\n  {}:{}", __func__,
                                         getName(), __FILE__, __LINE__));
  }
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_EdgeAttributeMatrixId.value());
}

const AttributeMatrix& AbstractNodeGeometry1D::getEdgeAttributeMatrixRef() const
{
  if(!m_EdgeAttributeMatrixId.has_value())
  {
    throw std::runtime_error(fmt::format("AbstractNodeGeometry1D::{} threw runtime exception. The geometry with name '{}' does not have an edge attribute matrix assigned.\n  {}:{}", __func__,
                                         getName(), __FILE__, __LINE__));
  }
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_EdgeAttributeMatrixId.value());
}

DataPath AbstractNodeGeometry1D::getEdgeAttributeMatrixDataPath() const
{
  return getEdgeAttributeMatrixRef().getDataPaths().at(0);
}

void AbstractNodeGeometry1D::setEdgeAttributeMatrix(const AttributeMatrix& attributeMatrix)
{
  m_EdgeAttributeMatrixId = attributeMatrix.getId();
}

void AbstractNodeGeometry1D::setEdgeDataId(const std::optional<IdType>& edgeDataId)
{
  m_EdgeAttributeMatrixId = edgeDataId;
}

std::optional<AbstractDataObject::IdType> AbstractNodeGeometry1D::getElementContainingVertId() const
{
  return m_CellContainingVertDataArrayId;
}

std::optional<AbstractDataObject::IdType> AbstractNodeGeometry1D::getElementNeighborsId() const
{
  return m_CellNeighborsDataArrayId;
}

std::optional<AbstractDataObject::IdType> AbstractNodeGeometry1D::getElementCentroidsId() const
{
  return m_CellCentroidsDataArrayId;
}

std::optional<AbstractDataObject::IdType> AbstractNodeGeometry1D::getElementSizesId() const
{
  return m_ElementSizesId;
}

void AbstractNodeGeometry1D::setElementContainingVertId(const std::optional<IdType>& elementsContainingVertId)
{
  m_CellContainingVertDataArrayId = elementsContainingVertId;
}

void AbstractNodeGeometry1D::setElementNeighborsId(const std::optional<IdType>& elementNeighborsId)
{
  m_CellNeighborsDataArrayId = elementNeighborsId;
}

void AbstractNodeGeometry1D::setElementCentroidsId(const std::optional<IdType>& centroidsId)
{
  m_CellCentroidsDataArrayId = centroidsId;
}

void AbstractNodeGeometry1D::setElementSizesId(const std::optional<IdType>& sizesId)
{
  m_ElementSizesId = sizesId;
}

void AbstractNodeGeometry1D::checkUpdatedIdsImpl(const std::unordered_map<AbstractDataObject::IdType, AbstractDataObject::IdType>& updatedIdsMap)
{
  AbstractNodeGeometry0D::checkUpdatedIdsImpl(updatedIdsMap);

  std::vector<bool> visited(7, false);

  for(const auto& updatedId : updatedIdsMap)
  {
    m_EdgeAttributeMatrixId = nx::core::VisitDataStructureId(m_EdgeAttributeMatrixId, updatedId, visited, 0);
    m_EdgeDataArrayId = nx::core::VisitDataStructureId(m_EdgeDataArrayId, updatedId, visited, 1);
    m_CellContainingVertDataArrayId = nx::core::VisitDataStructureId(m_CellContainingVertDataArrayId, updatedId, visited, 2);
    m_CellNeighborsDataArrayId = nx::core::VisitDataStructureId(m_CellNeighborsDataArrayId, updatedId, visited, 3);
    m_CellCentroidsDataArrayId = nx::core::VisitDataStructureId(m_CellCentroidsDataArrayId, updatedId, visited, 4);
    m_ElementSizesId = nx::core::VisitDataStructureId(m_ElementSizesId, updatedId, visited, 5);
  }
}

Result<> AbstractNodeGeometry1D::validate() const
{
  // Validate the next lower dimension geometry
  Result<> result = AbstractNodeGeometry0D::validate();

  usize numTuples = getNumberOfEdges();
  const AttributeMatrix* amPtr = getEdgeAttributeMatrix();
  if(nullptr == amPtr)
  {
    return result;
  }
  usize amNumTuples = amPtr->getNumberOfTuples();

  if(numTuples != amNumTuples)
  {
    return MergeResults(
        result, MakeErrorResult(-4501, fmt::format("Edge Geometry '{}' has {} edges but the edge Attribute Matrix '{}' has {} total tuples.", getName(), numTuples, amPtr->getName(), amNumTuples)));
  }
  return result;
}

} // namespace nx::core
