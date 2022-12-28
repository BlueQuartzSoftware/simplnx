#include "INodeGeometry2D.hpp"

#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

namespace complex
{
INodeGeometry2D::INodeGeometry2D(DataStructure& dataGraph, std::string name)
: INodeGeometry1D(dataGraph, std::move(name))
{
}

INodeGeometry2D::INodeGeometry2D(DataStructure& dataGraph, std::string name, IdType importId)
: INodeGeometry1D(dataGraph, std::move(name), importId)
{
}

const std::optional<INodeGeometry2D::IdType>& INodeGeometry2D::getFaceListDataArrayId() const
{
  return m_FaceListId;
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

H5::ErrorType INodeGeometry2D::readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, bool preflight)
{
  H5::ErrorType error = INodeGeometry1D::readHdf5(dataStructureReader, groupReader, preflight);
  if(error < 0)
  {
    return error;
  }

  m_FaceListId = ReadH5DataId(groupReader, H5Constants::k_FaceListTag);
  m_FaceAttributeMatrixId = ReadH5DataId(groupReader, H5Constants::k_FaceDataTag);
  m_UnsharedEdgeListId = ReadH5DataId(groupReader, H5Constants::k_UnsharedEdgeListTag);

  return error;
}

H5::ErrorType INodeGeometry2D::writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const
{
  H5::ErrorType error = INodeGeometry1D::writeHdf5(dataStructureWriter, parentGroupWriter, importable);
  if(error < 0)
  {
    return error;
  }

  H5::GroupWriter groupWriter = parentGroupWriter.createGroupWriter(getName());
  error = WriteH5DataId(groupWriter, m_FaceListId, H5Constants::k_FaceListTag);
  if(error < 0)
  {
    return error;
  }

  error = WriteH5DataId(groupWriter, m_FaceAttributeMatrixId, H5Constants::k_FaceDataTag);
  if(error < 0)
  {
    return error;
  }

  error = WriteH5DataId(groupWriter, m_UnsharedEdgeListId, H5Constants::k_UnsharedEdgeListTag);
  if(error < 0)
  {
    return error;
  }

  return error;
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

  for(const auto& updatedId : updatedIds)
  {
    if(m_FaceListId == updatedId.first)
    {
      m_FaceListId = updatedId.second;
    }

    if(m_FaceAttributeMatrixId == updatedId.first)
    {
      m_FaceAttributeMatrixId = updatedId.second;
    }

    if(m_UnsharedEdgeListId == updatedId.first)
    {
      m_UnsharedEdgeListId = updatedId.second;
    }
  }
}
} // namespace complex
