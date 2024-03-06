#include "INodeGeometry2D.hpp"

#include "simplnx/Utilities/DataObjectUtilities.hpp"

namespace nx::core
{
INodeGeometry2D::INodeGeometry2D(DataStructure& dataStructure, std::string name)
: INodeGeometry1D(dataStructure, std::move(name))
{
}

INodeGeometry2D::INodeGeometry2D(DataStructure& dataStructure, std::string name, IdType importId)
: INodeGeometry1D(dataStructure, std::move(name), importId)
{
}

const std::optional<INodeGeometry2D::IdType>& INodeGeometry2D::getFaceListDataArrayId() const
{
  return m_FaceListId;
}

INodeGeometry2D::OptionalId INodeGeometry2D::getFaceListId() const
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
  getFacesRef().getIDataStoreRef().resizeTuples({size});
}

usize INodeGeometry2D::getNumberOfFaces() const
{
  const auto* facesPtr = getFaces();
  return facesPtr == nullptr ? 0 : facesPtr->getNumberOfTuples();
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
  return m_FaceAttributeMatrixId;
}

void INodeGeometry2D::setFaceDataId(const OptionalId& faceDataId)
{
  m_FaceAttributeMatrixId = faceDataId;
}

AttributeMatrix* INodeGeometry2D::getFaceAttributeMatrix()
{
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_FaceAttributeMatrixId);
}

const AttributeMatrix* INodeGeometry2D::getFaceAttributeMatrix() const
{
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_FaceAttributeMatrixId);
}

AttributeMatrix& INodeGeometry2D::getFaceAttributeMatrixRef()
{
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_FaceAttributeMatrixId.value());
}

const AttributeMatrix& INodeGeometry2D::getFaceAttributeMatrixRef() const
{
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_FaceAttributeMatrixId.value());
}

DataPath INodeGeometry2D::getFaceAttributeMatrixDataPath() const
{
  return getFaceAttributeMatrixRef().getDataPaths().at(0);
}

void INodeGeometry2D::setFaceAttributeMatrix(const AttributeMatrix& attributeMatrix)
{
  m_FaceAttributeMatrixId = attributeMatrix.getId();
}
INodeGeometry2D::SharedEdgeList* INodeGeometry2D::createSharedEdgeList(usize numEdges)
{
  auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{numEdges}, std::vector<usize>{2}, 0);
  SharedEdgeList* edges = DataArray<MeshIndexType>::Create(*getDataStructure(), k_Edges, std::move(dataStore), getId());
  return edges;
}

void INodeGeometry2D::checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds)
{
  INodeGeometry1D::checkUpdatedIdsImpl(updatedIds);
  std::vector<bool> visited(3, false);
  for(const auto& updatedId : updatedIds)
  {
    m_FaceListId = nx::core::VisitDataStructureId(m_FaceListId, updatedId, visited, 0);
    m_FaceAttributeMatrixId = nx::core::VisitDataStructureId(m_FaceAttributeMatrixId, updatedId, visited, 1);
    m_UnsharedEdgeListId = nx::core::VisitDataStructureId(m_UnsharedEdgeListId, updatedId, visited, 2);
  }
}
} // namespace nx::core
