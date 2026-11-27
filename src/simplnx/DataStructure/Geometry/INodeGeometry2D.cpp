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

INodeGeometry2D::OptionalIdType INodeGeometry2D::getFaceListId() const
{
  return m_FaceListId;
}

void INodeGeometry2D::setFaceListId(const OptionalIdType& facesId)
{
  m_FaceListId = facesId;
}

INodeGeometry2D::SharedFaceListType* INodeGeometry2D::getFaces()
{
  if(!m_FaceListId.has_value())
  {
    return nullptr;
  }
  DataStructure* dataStructurePtr = getDataStructure();
  if(nullptr == dataStructurePtr)
  {
    return nullptr;
  }
  return dataStructurePtr->getDataAs<SharedFaceListType>(m_FaceListId);
}

const INodeGeometry2D::SharedFaceListType* INodeGeometry2D::getFaces() const
{
  if(!m_FaceListId.has_value())
  {
    return nullptr;
  }
  const DataStructure* dataStructurePtr = getDataStructure();
  if(nullptr == dataStructurePtr)
  {
    return nullptr;
  }
  return dataStructurePtr->getDataAs<SharedFaceListType>(m_FaceListId);
}

INodeGeometry2D::SharedFaceListType& INodeGeometry2D::getFacesRef()
{
  if(!m_FaceListId.has_value())
  {
    throw std::runtime_error(
        fmt::format("INodeGeometry1D::{} threw runtime exception. The geometry with name '{}' does not have a shared face list assigned.\n  {}:{}", __func__, getName(), __FILE__, __LINE__));
  }
  DataStructure* dataStructurePtr = getDataStructure();
  if(nullptr == dataStructurePtr)
  {
    throw std::runtime_error(
        fmt::format("INodeGeometry2D::{} threw runtime exception. The geometry with name '{}' does not have a Data Structure assigned.\n  {}:{}", __func__, getName(), __FILE__, __LINE__));
  }
  return dataStructurePtr->getDataRefAs<SharedFaceListType>(m_FaceListId.value());
}

const INodeGeometry2D::SharedFaceListType& INodeGeometry2D::getFacesRef() const
{
  if(!m_FaceListId.has_value())
  {
    throw std::runtime_error(
        fmt::format("INodeGeometry1D::{} threw runtime exception. The geometry with name '{}' does not have a shared face list assigned.\n  {}:{}", __func__, getName(), __FILE__, __LINE__));
  }
  const DataStructure* dataStructurePtr = getDataStructure();
  if(nullptr == dataStructurePtr)
  {
    throw std::runtime_error(
        fmt::format("INodeGeometry2D::{} threw runtime exception. The geometry with name '{}' does not have a Data Structure assigned.\n  {}:{}", __func__, getName(), __FILE__, __LINE__));
  }
  return dataStructurePtr->getDataRefAs<SharedFaceListType>(m_FaceListId.value());
}

void INodeGeometry2D::setFaceList(const SharedFaceListType& faces)
{
  m_FaceListId = faces.getId();
}

Result<> INodeGeometry2D::resizeFaceList(usize size)
{
  Result<> resizeResult = getFacesRef().getIDataStoreRef().resizeTuples({size});
  if(resizeResult.invalid())
  {
    resizeResult.errors()[0].message = fmt::format("Geometry '{}' failed to resize its face list to {} tuples: {}", getName(), size, resizeResult.errors()[0].message);
  }
  return resizeResult;
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
  const auto& cells = getFacesRef();
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

void INodeGeometry2D::getFaceCoordinates(usize faceId, nonstd::span<FloatVec3Type> coords) const
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

void INodeGeometry2D::setUnsharedEdgesId(const OptionalIdType& unsharedEdgesId)
{
  m_UnsharedEdgeListId = unsharedEdgesId;
}

const INodeGeometry2D::SharedEdgeListType* INodeGeometry2D::getUnsharedEdges() const
{
  const DataStructure* dataStructurePtr = getDataStructure();
  if(nullptr == dataStructurePtr)
  {
    return nullptr;
  }
  return dataStructurePtr->getDataAs<SharedEdgeListType>(m_UnsharedEdgeListId);
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

void INodeGeometry2D::setFaceDataId(const OptionalIdType& faceDataId)
{
  m_FaceAttributeMatrixId = faceDataId;
}

AttributeMatrix* INodeGeometry2D::getFaceAttributeMatrix()
{
  if(!m_FaceAttributeMatrixId.has_value())
  {
    return nullptr;
  }
  DataStructure* dataStructurePtr = getDataStructure();
  if(nullptr == dataStructurePtr)
  {
    return nullptr;
  }
  return dataStructurePtr->getDataAs<AttributeMatrix>(m_FaceAttributeMatrixId);
}

const AttributeMatrix* INodeGeometry2D::getFaceAttributeMatrix() const
{
  if(!m_FaceAttributeMatrixId.has_value())
  {
    return nullptr;
  }
  const DataStructure* dataStructurePtr = getDataStructure();
  if(nullptr == dataStructurePtr)
  {
    return nullptr;
  }
  return dataStructurePtr->getDataAs<AttributeMatrix>(m_FaceAttributeMatrixId);
}

AttributeMatrix& INodeGeometry2D::getFaceAttributeMatrixRef()
{
  if(!m_FaceAttributeMatrixId.has_value())
  {
    throw std::runtime_error(
        fmt::format("INodeGeometry2D::{} threw runtime exception. The geometry with name '{}' does not have a face attribute matrix assigned.\n  {}:{}", __func__, getName(), __FILE__, __LINE__));
  }
  DataStructure* dataStructurePtr = getDataStructure();
  if(nullptr == dataStructurePtr)
  {
    throw std::runtime_error(
        fmt::format("INodeGeometry2D::{} threw runtime exception. The geometry with name '{}' does not have a Data Structure assigned.\n  {}:{}", __func__, getName(), __FILE__, __LINE__));
  }
  return dataStructurePtr->getDataRefAs<AttributeMatrix>(m_FaceAttributeMatrixId.value());
}

const AttributeMatrix& INodeGeometry2D::getFaceAttributeMatrixRef() const
{
  if(!m_FaceAttributeMatrixId.has_value())
  {
    throw std::runtime_error(
        fmt::format("INodeGeometry2D::{} threw runtime exception. The geometry with name '{}' does not have a face attribute matrix assigned.\n  {}:{}", __func__, getName(), __FILE__, __LINE__));
  }
  const DataStructure* dataStructurePtr = getDataStructure();
  if(nullptr == dataStructurePtr)
  {
    throw std::runtime_error(
        fmt::format("INodeGeometry2D::{} threw runtime exception. The geometry with name '{}' does not have a Data Structure assigned.\n  {}:{}", __func__, getName(), __FILE__, __LINE__));
  }
  return dataStructurePtr->getDataRefAs<AttributeMatrix>(m_FaceAttributeMatrixId.value());
}

DataPath INodeGeometry2D::getFaceAttributeMatrixDataPath() const
{
  return getFaceAttributeMatrixRef().getDataPaths().at(0);
}

void INodeGeometry2D::setFaceAttributeMatrix(const AttributeMatrix& attributeMatrix)
{
  m_FaceAttributeMatrixId = attributeMatrix.getId();
}
INodeGeometry2D::SharedEdgeListType* INodeGeometry2D::createSharedEdgeList(usize numEdges)
{
  auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{numEdges}, std::vector<usize>{2}, 0);
  SharedEdgeListType* edgesPtr = DataArray<MeshIndexType>::Create(*getDataStructure(), k_SharedEdgeListName, std::move(dataStore), getId());
  return edgesPtr;
}

void INodeGeometry2D::copyMembersInto(INodeGeometry2D& copy, const DataPath& copyPath)
{
  INodeGeometry1D::copyMembersInto(copy, copyPath);

  copy.m_FaceAttributeMatrixId = deepCopyOwnedChild(copyPath, getFaceAttributeMatrix());
  copy.m_UnsharedEdgeListId = adoptCopiedChild<MeshIndexArrayType>(copyPath, k_UnsharedEdgesListName);
  // A 2D geometry owns its faces; its edges are derived by findEdges() and adopted under the shared name.
  copy.m_EdgeDataArrayId = adoptCopiedChild<MeshIndexArrayType>(copyPath, INodeGeometry2D::k_SharedEdgeListName);
}

void INodeGeometry2D::checkUpdatedIdsImpl(const std::unordered_map<DataObject::IdType, DataObject::IdType>& updatedIdsMap)
{
  INodeGeometry1D::checkUpdatedIdsImpl(updatedIdsMap);
  std::vector<bool> visited(3, false);
  for(const auto& updatedId : updatedIdsMap)
  {
    m_FaceListId = nx::core::VisitDataStructureId(m_FaceListId, updatedId, visited, 0);
    m_FaceAttributeMatrixId = nx::core::VisitDataStructureId(m_FaceAttributeMatrixId, updatedId, visited, 1);
    m_UnsharedEdgeListId = nx::core::VisitDataStructureId(m_UnsharedEdgeListId, updatedId, visited, 2);
  }
}

Result<> INodeGeometry2D::validate() const
{
  // Validate the next lower dimension geometry
  Result<> result = INodeGeometry1D::validate();

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
