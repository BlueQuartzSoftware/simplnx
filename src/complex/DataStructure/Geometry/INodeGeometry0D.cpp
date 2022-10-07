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

const std::optional<INodeGeometry0D::IdType>& INodeGeometry0D::getVertexListId() const
{
  return m_VertexListId;
}

INodeGeometry0D::SharedVertexList* INodeGeometry0D::getVertices()
{
  return getDataStructureRef().getDataAs<SharedVertexList>(m_VertexListId);
}

const INodeGeometry0D::SharedVertexList* INodeGeometry0D::getVertices() const
{
  return getDataStructureRef().getDataAs<SharedVertexList>(m_VertexListId);
}

INodeGeometry0D::SharedVertexList& INodeGeometry0D::getVerticesRef()
{
  return getDataStructureRef().getDataRefAs<SharedVertexList>(m_VertexListId.value());
}

const INodeGeometry0D::SharedVertexList& INodeGeometry0D::getVerticesRef() const
{
  return getDataStructureRef().getDataRefAs<SharedVertexList>(m_VertexListId.value());
}

void INodeGeometry0D::setVertices(const INodeGeometry0D::SharedVertexList& vertices)
{
  m_VertexListId = vertices.getId();
}

void INodeGeometry0D::setVertexListId(const std::optional<IdType>& vertices)
{
  m_VertexListId = vertices;
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

const std::optional<INodeGeometry0D::IdType>& INodeGeometry0D::getVertexDataId() const
{
  return m_VertexDataId;
}

void INodeGeometry0D::setVertexDataId(const OptionalId& vertexDataId)
{
  m_VertexDataId = vertexDataId;
}

AttributeMatrix* INodeGeometry0D::getVertexData()
{
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_VertexDataId);
}

const AttributeMatrix* INodeGeometry0D::getVertexData() const
{
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_VertexDataId);
}

AttributeMatrix& INodeGeometry0D::getVertexDataRef()
{
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_VertexDataId.value());
}

const AttributeMatrix& INodeGeometry0D::getVertexDataRef() const
{
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_VertexDataId.value());
}

DataPath INodeGeometry0D::getVertexDataPath() const
{
  return getVertexDataRef().getDataPaths().at(0);
}

void INodeGeometry0D::setVertexData(const AttributeMatrix& attributeMatrix)
{
  m_VertexDataId = attributeMatrix.getId();
}

void INodeGeometry0D::checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds)
{
  IGeometry::checkUpdatedIdsImpl(updatedIds);

  for(const auto& updatedId : updatedIds)
  {
    if(m_VertexListId == updatedId.first)
    {
      m_VertexListId = updatedId.second;
    }
    if(m_VertexDataId == updatedId.first)
    {
      m_VertexDataId = updatedId.second;
    }
  }
}
} // namespace complex
