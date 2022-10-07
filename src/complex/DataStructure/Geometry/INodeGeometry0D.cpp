#include "INodeGeometry0D.hpp"

namespace complex
{
INodeGeometry0D::INodeGeometry0D(DataStructure& ds, std::string name)
: IGeometry(ds, std::move(name))
{
}

INodeGeometry0D::INodeGeometry0D(DataStructure& ds, std::string name, IdType importId)
: IGeometry(ds, std::move(name), importId)
{
}

const std::optional<INodeGeometry0D::IdType>& INodeGeometry0D::getSharedVertexDataArrayId() const
{
  return m_VertexDataArrayId;
}

INodeGeometry0D::SharedVertexList* INodeGeometry0D::getVertices()
{
  return getDataStructureRef().getDataAs<SharedVertexList>(m_VertexDataArrayId);
}

const INodeGeometry0D::SharedVertexList* INodeGeometry0D::getVertices() const
{
  return getDataStructureRef().getDataAs<SharedVertexList>(m_VertexDataArrayId);
}

INodeGeometry0D::SharedVertexList& INodeGeometry0D::getVerticesRef()
{
  return getDataStructureRef().getDataRefAs<SharedVertexList>(m_VertexDataArrayId.value());
}

const INodeGeometry0D::SharedVertexList& INodeGeometry0D::getVerticesRef() const
{
  return getDataStructureRef().getDataRefAs<SharedVertexList>(m_VertexDataArrayId.value());
}

void INodeGeometry0D::setVertices(const INodeGeometry0D::SharedVertexList& vertices)
{
  m_VertexDataArrayId = vertices.getId();
}

std::optional<DataObject::IdType> INodeGeometry0D::getVertexListId() const
{
  return m_VertexDataArrayId;
}

void INodeGeometry0D::setVertexListId(const std::optional<IdType>& vertices)
{
  m_VertexDataArrayId = vertices;
}

void INodeGeometry0D::resizeVertexList(usize size)
{
  getVerticesRef().getIDataStoreRef().reshapeTuples({size});
}

usize INodeGeometry0D::getNumberOfVertices() const
{
  const auto& vertices = getVerticesRef();
  return vertices.getNumberOfTuples();
}

usize INodeGeometry0D::getNumberOfCells() const
{
  const auto& vertices = getVerticesRef();
  return vertices.getNumberOfTuples();
}

void INodeGeometry0D::setVertexCoordinate(usize vertId, const Point3D<float32>& coordinate)
{
  auto& vertices = getVerticesRef();
  const usize offset = vertId * 3;
  for(usize i = 0; i < 3; i++)
  {
    vertices[offset + i] = coordinate[i];
  }
}

Point3D<float32> INodeGeometry0D::getVertexCoordinate(usize vertId) const
{
  auto& vertices = getVerticesRef();
  const usize offset = vertId * 3;
  Point3D<float32> coordinate;
  for(usize i = 0; i < 3; i++)
  {
    coordinate[i] = vertices.at(offset + i);
  }
  return coordinate;
}

const std::optional<INodeGeometry0D::IdType>& INodeGeometry0D::getVertexAttributeMatrixId() const
{
  return m_VertexAttributeMatrixId;
}

void INodeGeometry0D::setVertexDataId(const OptionalId& vertexDataId)
{
  m_VertexDataArrayId = vertexDataId;
}

AttributeMatrix* INodeGeometry0D::getVertexAttributeMatrix()
{
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_VertexAttributeMatrixId);
}

const AttributeMatrix* INodeGeometry0D::getVertexAttributeMatrix() const
{
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_VertexAttributeMatrixId);
}

AttributeMatrix& INodeGeometry0D::getVertexAttributeMatrixRef()
{
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_VertexAttributeMatrixId.value());
}

const AttributeMatrix& INodeGeometry0D::getVertexAttributeMatrixRef() const
{
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_VertexAttributeMatrixId.value());
}

DataPath INodeGeometry0D::getVertexAttributeMatrixDataPath() const
{
  return getVertexAttributeMatrixRef().getDataPaths().at(0);
}

void INodeGeometry0D::setVertexAttributeMatrix(const AttributeMatrix& attributeMatrix)
{
  m_VertexAttributeMatrixId = attributeMatrix.getId();
}

void INodeGeometry0D::checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds)
{
  IGeometry::checkUpdatedIdsImpl(updatedIds);

  for(const auto& updatedId : updatedIds)
  {
    if(m_VertexDataArrayId == updatedId.first)
    {
      m_VertexDataArrayId = updatedId.second;
    }
    if(m_VertexAttributeMatrixId == updatedId.first)
    {
      m_VertexAttributeMatrixId = updatedId.second;
    }
  }
}
} // namespace complex
