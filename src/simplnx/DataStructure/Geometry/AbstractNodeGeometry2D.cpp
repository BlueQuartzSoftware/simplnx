#include "AbstractNodeGeometry2D.hpp"

#include "simplnx/Utilities/DataObjectUtilities.hpp"

namespace nx::core
{
AbstractNodeGeometry2D::AbstractNodeGeometry2D(DataStructure& dataStructure, std::string name)
: AbstractNodeGeometry1D(dataStructure, std::move(name))
{
}

AbstractNodeGeometry2D::AbstractNodeGeometry2D(DataStructure& dataStructure, std::string name, IdType importId)
: AbstractNodeGeometry1D(dataStructure, std::move(name), importId)
{
}

const std::optional<AbstractNodeGeometry2D::IdType>& AbstractNodeGeometry2D::getFaceListDataArrayId() const
{
  return m_FaceListId;
}

AbstractNodeGeometry2D::OptionalId AbstractNodeGeometry2D::getFaceListId() const
{
  return m_FaceListId;
}

void AbstractNodeGeometry2D::setFaceListId(const OptionalId& facesId)
{
  m_FaceListId = facesId;
}

AbstractNodeGeometry2D::SharedFaceList* AbstractNodeGeometry2D::getFaces()
{
  if(!m_FaceListId.has_value())
  {
    return nullptr;
  }
  return getDataStructureRef().getDataAs<SharedFaceList>(m_FaceListId);
}

const AbstractNodeGeometry2D::SharedFaceList* AbstractNodeGeometry2D::getFaces() const
{
  if(!m_FaceListId.has_value())
  {
    return nullptr;
  }
  return getDataStructureRef().getDataAs<SharedFaceList>(m_FaceListId);
}

AbstractNodeGeometry2D::SharedFaceList& AbstractNodeGeometry2D::getFacesRef()
{
  if(!m_FaceListId.has_value())
  {
    throw std::runtime_error(
        fmt::format("AbstractNodeGeometry1D::{} threw runtime exception. The geometry with name '{}' does not have a shared face list assigned.\n  {}:{}", __func__, getName(), __FILE__, __LINE__));
  }
  return getDataStructureRef().getDataRefAs<SharedFaceList>(m_FaceListId.value());
}

const AbstractNodeGeometry2D::SharedFaceList& AbstractNodeGeometry2D::getFacesRef() const
{
  if(!m_FaceListId.has_value())
  {
    throw std::runtime_error(
        fmt::format("AbstractNodeGeometry1D::{} threw runtime exception. The geometry with name '{}' does not have a shared face list assigned.\n  {}:{}", __func__, getName(), __FILE__, __LINE__));
  }
  return getDataStructureRef().getDataRefAs<SharedFaceList>(m_FaceListId.value());
}

void AbstractNodeGeometry2D::setFaceList(const SharedFaceList& faces)
{
  m_FaceListId = faces.getId();
}

void AbstractNodeGeometry2D::resizeFaceList(usize size)
{
  getFacesRef().getIDataStoreRef().resizeTuples({size});
}

usize AbstractNodeGeometry2D::getNumberOfFaces() const
{
  const auto* facesPtr = getFaces();
  return facesPtr == nullptr ? 0 : facesPtr->getNumberOfTuples();
}

void AbstractNodeGeometry2D::setFacePointIds(usize faceId, nonstd::span<usize> vertexIds)
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

void AbstractNodeGeometry2D::getFacePointIds(usize faceId, nonstd::span<usize> vertexIds) const
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

void AbstractNodeGeometry2D::getFaceCoordinates(usize faceId, nonstd::span<Point3Df> coords) const
{
  std::vector<usize> verts(getNumberOfVerticesPerFace());
  getFacePointIds(faceId, verts);
  for(usize index = 0; index < verts.size(); index++)
  {
    coords[index] = getVertexCoordinate(verts[index]);
  }
}

void AbstractNodeGeometry2D::deleteEdges()
{
  getDataStructureRef().removeData(m_EdgeDataArrayId);
  m_EdgeDataArrayId.reset();
}

const std::optional<AbstractNodeGeometry2D::IdType>& AbstractNodeGeometry2D::getUnsharedEdgesId() const
{
  return m_UnsharedEdgeListId;
}

void AbstractNodeGeometry2D::setUnsharedEdgesId(const OptionalId& unsharedEdgesId)
{
  m_UnsharedEdgeListId = unsharedEdgesId;
}

const AbstractNodeGeometry2D::SharedEdgeList* AbstractNodeGeometry2D::getUnsharedEdges() const
{
  return getDataStructureRef().getDataAs<SharedEdgeList>(m_UnsharedEdgeListId);
}

void AbstractNodeGeometry2D::deleteUnsharedEdges()
{
  getDataStructureRef().removeData(m_UnsharedEdgeListId);
  m_UnsharedEdgeListId.reset();
}

const std::optional<AbstractNodeGeometry2D::IdType>& AbstractNodeGeometry2D::getFaceAttributeMatrixId() const
{
  return m_FaceAttributeMatrixId;
}

void AbstractNodeGeometry2D::setFaceDataId(const OptionalId& faceDataId)
{
  m_FaceAttributeMatrixId = faceDataId;
}

AttributeMatrix* AbstractNodeGeometry2D::getFaceAttributeMatrix()
{
  if(!m_FaceAttributeMatrixId.has_value())
  {
    return nullptr;
  }
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_FaceAttributeMatrixId);
}

const AttributeMatrix* AbstractNodeGeometry2D::getFaceAttributeMatrix() const
{
  if(!m_FaceAttributeMatrixId.has_value())
  {
    return nullptr;
  }
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_FaceAttributeMatrixId);
}

AttributeMatrix& AbstractNodeGeometry2D::getFaceAttributeMatrixRef()
{
  if(!m_FaceAttributeMatrixId.has_value())
  {
    throw std::runtime_error(fmt::format("AbstractNodeGeometry2D::{} threw runtime exception. The geometry with name '{}' does not have a face attribute matrix assigned.\n  {}:{}", __func__,
                                         getName(), __FILE__, __LINE__));
  }
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_FaceAttributeMatrixId.value());
}

const AttributeMatrix& AbstractNodeGeometry2D::getFaceAttributeMatrixRef() const
{
  if(!m_FaceAttributeMatrixId.has_value())
  {
    throw std::runtime_error(fmt::format("AbstractNodeGeometry2D::{} threw runtime exception. The geometry with name '{}' does not have a face attribute matrix assigned.\n  {}:{}", __func__,
                                         getName(), __FILE__, __LINE__));
  }
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_FaceAttributeMatrixId.value());
}

DataPath AbstractNodeGeometry2D::getFaceAttributeMatrixDataPath() const
{
  return getFaceAttributeMatrixRef().getDataPaths().at(0);
}

void AbstractNodeGeometry2D::setFaceAttributeMatrix(const AttributeMatrix& attributeMatrix)
{
  m_FaceAttributeMatrixId = attributeMatrix.getId();
}
AbstractNodeGeometry2D::SharedEdgeList* AbstractNodeGeometry2D::createSharedEdgeList(usize numEdges)
{
  auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{numEdges}, std::vector<usize>{2}, 0);
  SharedEdgeList* edges = DataArray<MeshIndexType>::Create(*getDataStructure(), k_SharedEdgeListName, std::move(dataStore), getId());
  return edges;
}

void AbstractNodeGeometry2D::checkUpdatedIdsImpl(const std::unordered_map<AbstractDataObject::IdType, AbstractDataObject::IdType>& updatedIdsMap)
{
  AbstractNodeGeometry1D::checkUpdatedIdsImpl(updatedIdsMap);
  std::vector<bool> visited(3, false);
  for(const auto& updatedId : updatedIdsMap)
  {
    m_FaceListId = nx::core::VisitDataStructureId(m_FaceListId, updatedId, visited, 0);
    m_FaceAttributeMatrixId = nx::core::VisitDataStructureId(m_FaceAttributeMatrixId, updatedId, visited, 1);
    m_UnsharedEdgeListId = nx::core::VisitDataStructureId(m_UnsharedEdgeListId, updatedId, visited, 2);
  }
}

Result<> AbstractNodeGeometry2D::validate() const
{
  // Validate the next lower dimension geometry
  Result<> result = AbstractNodeGeometry1D::validate();

  usize numTuples = getNumberOfFaces();
  const AttributeMatrix* amPtr = getFaceAttributeMatrix();
  if(nullptr == amPtr)
  {
    return result;
  }
  usize amNumTuples = amPtr->getNumberOfTuples();

  if(numTuples != amNumTuples)
  {
    return MergeResults(result, MakeErrorResult(-4501, fmt::format("Triangle/Quad Geometry '{}' has {} faces but the face Attribute Matrix '{}' has {} total tuples.", getName(), numTuples,
                                                                   amPtr->getName(), amNumTuples)));
  }
  return result;
}

} // namespace nx::core
