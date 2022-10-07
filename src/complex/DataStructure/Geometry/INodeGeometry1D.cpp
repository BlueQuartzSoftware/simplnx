#include "INodeGeometry1D.hpp"

#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

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

const std::optional<INodeGeometry1D::IdType>& INodeGeometry1D::getEdgeListDataArrayId() const
{
  return m_EdgeDataArrayId;
}

INodeGeometry1D::SharedEdgeList* INodeGeometry1D::getEdges()
{
  return getDataStructureRef().getDataAs<SharedEdgeList>(m_EdgeDataArrayId);
}

const INodeGeometry1D::SharedEdgeList* INodeGeometry1D::getEdges() const
{
  return getDataStructureRef().getDataAs<SharedEdgeList>(m_EdgeDataArrayId);
}

INodeGeometry1D::SharedEdgeList& INodeGeometry1D::getEdgesRef()
{
  return getDataStructureRef().getDataRefAs<SharedEdgeList>(m_EdgeDataArrayId.value());
}

const INodeGeometry1D::SharedEdgeList& INodeGeometry1D::getEdgesRef() const
{
  return getDataStructureRef().getDataRefAs<SharedEdgeList>(m_EdgeDataArrayId.value());
}

void INodeGeometry1D::setEdgeList(const SharedEdgeList& edges)
{
  m_EdgeDataArrayId = edges.getId();
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
  getDataStructureRef().removeData(m_CellNeighborsDataArrayId);
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
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_EdgeAttributeMatrixId);
}

const AttributeMatrix* INodeGeometry1D::getEdgeAttributeMatrix() const
{
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_EdgeAttributeMatrixId);
}

AttributeMatrix& INodeGeometry1D::getEdgeAttributeMatrixRef()
{
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_EdgeAttributeMatrixId.value());
}

const AttributeMatrix& INodeGeometry1D::getEdgeAttributeMatrixRef() const
{
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

H5::ErrorType INodeGeometry1D::readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, bool preflight)
{
  H5::ErrorType error = INodeGeometry0D::readHdf5(dataStructureReader, groupReader, preflight);
  if(error < 0)
  {
    return error;
  }

  m_EdgeDataArrayId = ReadH5DataId(groupReader, H5Constants::k_EdgeListTag);
  m_EdgeAttributeMatrixId = ReadH5DataId(groupReader, H5Constants::k_EdgeDataTag);
  m_CellContainingVertDataArrayId = ReadH5DataId(groupReader, H5Constants::k_ElementContainingVertTag);
  m_CellNeighborsDataArrayId = ReadH5DataId(groupReader, H5Constants::k_ElementNeighborsTag);
  m_CellCentroidsDataArrayId = ReadH5DataId(groupReader, H5Constants::k_ElementCentroidTag);

  return error;
}

H5::ErrorType INodeGeometry1D::writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const
{
  H5::ErrorType error = INodeGeometry0D::writeHdf5(dataStructureWriter, parentGroupWriter, importable);
  if(error < 0)
  {
    return error;
  }

  H5::GroupWriter groupWriter = parentGroupWriter.createGroupWriter(getName());
  error = WriteH5DataId(groupWriter, m_EdgeDataArrayId, H5Constants::k_EdgeListTag);
  if(error < 0)
  {
    return error;
  }

  error = WriteH5DataId(groupWriter, m_EdgeAttributeMatrixId, H5Constants::k_EdgeDataTag);
  if(error < 0)
  {
    return error;
  }

  error = WriteH5DataId(groupWriter, m_CellContainingVertDataArrayId, H5Constants::k_ElementContainingVertTag);
  if(error < 0)
  {
    return error;
  }

  error = WriteH5DataId(groupWriter, m_CellNeighborsDataArrayId, H5Constants::k_ElementNeighborsTag);
  if(error < 0)
  {
    return error;
  }

  error = WriteH5DataId(groupWriter, m_CellCentroidsDataArrayId, H5Constants::k_ElementCentroidTag);
  if(error < 0)
  {
    return error;
  }

  return error;
}

void INodeGeometry1D::checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds)
{
  INodeGeometry0D::checkUpdatedIdsImpl(updatedIds);

  for(const auto& updatedId : updatedIds)
  {
    if(m_VertexDataArrayId == updatedId.first)
    {
      m_VertexDataArrayId = updatedId.second;
    }

    if(m_EdgeAttributeMatrixId == updatedId.first)
    {
      m_EdgeAttributeMatrixId = updatedId.second;
    }

    if(m_EdgeDataArrayId == updatedId.first)
    {
      m_EdgeDataArrayId = updatedId.second;
    }

    if(m_CellContainingVertDataArrayId == updatedId.first)
    {
      m_CellContainingVertDataArrayId = updatedId.second;
    }

    if(m_CellNeighborsDataArrayId == updatedId.first)
    {
      m_CellNeighborsDataArrayId = updatedId.second;
    }

    if(m_CellCentroidsDataArrayId == updatedId.first)
    {
      m_CellCentroidsDataArrayId = updatedId.second;
    }

    if(m_ElementSizesId == updatedId.first)
    {
      m_ElementSizesId = updatedId.second;
    }
  }
}
} // namespace complex
