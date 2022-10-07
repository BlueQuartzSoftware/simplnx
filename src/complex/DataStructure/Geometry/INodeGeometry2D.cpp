#include "INodeGeometry2D.hpp"

namespace complex
{
INodeGeometry2D::INodeGeometry2D(DataStructure& ds, std::string name)
: INodeGeometry1D(ds, std::move(name))
{
}

INodeGeometry2D::INodeGeometry2D(DataStructure& ds, std::string name, IdType importId)
: INodeGeometry1D(ds, std::move(name), importId)
{
}

const std::optional<INodeGeometry2D::IdType>& INodeGeometry2D::getFaceListDataArrayId() const
{
  return m_FaceListId;
}

void INodeGeometry2D::setFaceListId(const OptionalId& facesId)
{
  m_FaceListId = facesId;
}

INodeGeometry2D::SharedFaceList* INodeGeometry2D::getFaces()
{
  return getDataStructureRef().getDataAs<SharedFaceList>(m_FaceListId);
}

const INodeGeometry2D::SharedFaceList* INodeGeometry2D::getFaces() const
{
  return getDataStructureRef().getDataAs<SharedFaceList>(m_FaceListId);
}

INodeGeometry2D::SharedFaceList& INodeGeometry2D::getFacesRef()
{
  return getDataStructureRef().getDataRefAs<SharedFaceList>(m_FaceListId.value());
}

const INodeGeometry2D::SharedFaceList& INodeGeometry2D::getFacesRef() const
{
  return getDataStructureRef().getDataRefAs<SharedFaceList>(m_FaceListId.value());
}

void INodeGeometry2D::setFaceList(const SharedFaceList& faces)
{
  m_FaceListId = faces.getId();
}

void INodeGeometry2D::resizeFaceList(usize size)
{
  getFacesRef().getIDataStoreRef().reshapeTuples({size});
}

usize INodeGeometry2D::getNumberOfFaces() const
{
  const auto& faces = getFacesRef();
  return faces.getNumberOfTuples();
}

void INodeGeometry2D::setFacePointIds(usize faceId, nonstd::span<usize> vertexIds)
{
  auto& faces = getFacesRef();
  const usize offset = faceId * getNumberOfVerticesPerFace();
  if(offset + getNumberOfVerticesPerFace() > faces.getSize())
  {
    return;
  }
  for(usize i = 0; i < getNumberOfVerticesPerFace(); i++)
  {
    faces[offset + i] = vertexIds[i];
  }
}

void INodeGeometry2D::getFacePointIds(usize faceId, nonstd::span<usize> vertexIds) const
{
  auto& cells = getFacesRef();
  const usize offset = faceId * getNumberOfVerticesPerFace();
  if(offset + getNumberOfVerticesPerFace() > cells.getSize())
  {
    return;
  }
  for(usize i = 0; i < getNumberOfVerticesPerFace(); i++)
  {
    vertexIds[i] = cells.at(offset + i);
  }
}

void INodeGeometry2D::getFaceCoordinates(usize faceId, nonstd::span<Point3Df> coords) const
{
  std::vector<usize> verts(getNumberOfVerticesPerFace());
  getFacePointIds(faceId, verts);
  for(usize index = 0; index < verts.size(); index++)
  {
    coords[index] = getVertexCoordinate(verts[index]);
  }
}

void INodeGeometry2D::deleteEdges()
{
  getDataStructureRef().removeData(m_EdgeDataArrayId);
  m_EdgeDataArrayId.reset();
}

const std::optional<INodeGeometry2D::IdType>& INodeGeometry2D::getUnsharedEdgesId() const
{
  return m_UnsharedEdgeListId;
}

void INodeGeometry2D::setUnsharedEdgesId(const OptionalId& unsharedEdgesId)
{
  m_UnsharedEdgeListId = unsharedEdgesId;
}

const INodeGeometry2D::SharedEdgeList* INodeGeometry2D::getUnsharedEdges() const
{
  return getDataStructureRef().getDataAs<SharedEdgeList>(m_UnsharedEdgeListId);
}

void INodeGeometry2D::deleteUnsharedEdges()
{
  getDataStructureRef().removeData(m_UnsharedEdgeListId);
  m_UnsharedEdgeListId.reset();
}

const std::optional<INodeGeometry2D::IdType>& INodeGeometry2D::getFaceAttributeMatrixId() const
{
  return m_FaceDataId;
}

void INodeGeometry2D::setFaceDataId(const OptionalId& faceDataId)
{
  m_FaceDataId = faceDataId;
}

AttributeMatrix* INodeGeometry2D::getFaceAttributeMatrix()
{
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_FaceDataId);
}

const AttributeMatrix* INodeGeometry2D::getFaceAttributeMatrix() const
{
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_FaceDataId);
}

AttributeMatrix& INodeGeometry2D::getFaceAttributeMatrixRef()
{
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_FaceDataId.value());
}

const AttributeMatrix& INodeGeometry2D::getFaceAttributeMatrixRef() const
{
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_FaceDataId.value());
}

DataPath INodeGeometry2D::getFaceAttributeMatrixDataPath() const
{
  return getFaceAttributeMatrixRef().getDataPaths().at(0);
}

void INodeGeometry2D::setFaceAttributeMatrix(const AttributeMatrix& attributeMatrix)
{
  m_FaceDataId = attributeMatrix.getId();
}

INodeGeometry2D::SharedEdgeList* INodeGeometry2D::createSharedEdgeList(usize numEdges)
{
  auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{numEdges}, std::vector<usize>{2}, 0);
  SharedEdgeList* edges = DataArray<MeshIndexType>::Create(*getDataStructure(), "Shared Edge List", std::move(dataStore), getId());
  return edges;
}

void INodeGeometry2D::checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds)
{
  INodeGeometry1D::checkUpdatedIdsImpl(updatedIds);

  for(const auto& updatedId : updatedIds)
  {
    if(m_FaceListId == updatedId.first)
    {
      m_FaceListId = updatedId.second;
    }

    if(m_FaceDataId == updatedId.first)
    {
      m_FaceDataId = updatedId.second;
    }

    if(m_UnsharedEdgeListId == updatedId.first)
    {
      m_UnsharedEdgeListId = updatedId.second;
    }
  }
}
} // namespace complex
