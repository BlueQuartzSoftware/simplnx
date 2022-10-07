#include "INodeGeometry2D.hpp"

#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

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

const std::optional<INodeGeometry2D::IdType>& INodeGeometry2D::getFaceListId() const
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

void INodeGeometry2D::setFaces(const SharedFaceList& faces)
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
  getDataStructureRef().removeData(m_EdgeListId);
  m_EdgeListId.reset();
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

const std::optional<INodeGeometry2D::IdType>& INodeGeometry2D::getFaceDataId() const
{
  return m_FaceDataId;
}

void INodeGeometry2D::setFaceDataId(const OptionalId& faceDataId)
{
  m_FaceDataId = faceDataId;
}

AttributeMatrix* INodeGeometry2D::getFaceData()
{
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_FaceDataId);
}

const AttributeMatrix* INodeGeometry2D::getFaceData() const
{
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_FaceDataId);
}

AttributeMatrix& INodeGeometry2D::getFaceDataRef()
{
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_FaceDataId.value());
}

const AttributeMatrix& INodeGeometry2D::getFaceDataRef() const
{
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_FaceDataId.value());
}

DataPath INodeGeometry2D::getFaceDataPath() const
{
  return getFaceDataRef().getDataPaths().at(0);
}

void INodeGeometry2D::setFaceData(const AttributeMatrix& attributeMatrix)
{
  m_FaceDataId = attributeMatrix.getId();
}

#if 0
H5::ErrorType INodeGeometry2D::readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, bool preflight)
{
  H5::ErrorType error = INodeGeometry1D::readHdf5(dataStructureReader, groupReader, preflight);
  if(error < 0)
  {
    return error;
  }

  m_FaceListId = ReadH5DataId(groupReader, H5Constants::k_FaceListTag);
  m_FaceDataId = ReadH5DataId(groupReader, H5Constants::k_FaceDataTag);
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

  error = WriteH5DataId(groupWriter, m_FaceDataId, H5Constants::k_FaceDataTag);
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

Zarr::ErrorType INodeGeometry2D::readZarr(Zarr::DataStructureReader& dataStructureReader, const FileVec::IGroup& collection, bool preflight)
{
  Zarr::ErrorType error = INodeGeometry1D::readZarr(dataStructureReader, collection, preflight);
  if(error < 0)
  {
    return error;
  }

  m_FaceListId = ReadZarrDataId(collection, H5Constants::k_FaceListTag);
  m_FaceDataId = ReadZarrDataId(collection, H5Constants::k_FaceDataTag);
  m_UnsharedEdgeListId = ReadZarrDataId(collection, H5Constants::k_UnsharedEdgeListTag);

  return error;
}

Zarr::ErrorType INodeGeometry2D::writeZarr(Zarr::DataStructureWriter& dataStructureWriter, FileVec::IGroup& parentGroupWriter, bool importable) const
{
  Zarr::ErrorType error = INodeGeometry1D::writeZarr(dataStructureWriter, parentGroupWriter, importable);
  if(error < 0)
  {
    return error;
  }

  auto groupWriter = parentGroupWriter.createOrFindGroup(getName());
  error = WriteZarrDataId(groupWriter, m_FaceListId, H5Constants::k_FaceListTag);
  if(error < 0)
  {
    return error;
  }

  error = WriteZarrDataId(groupWriter, m_FaceDataId, H5Constants::k_FaceDataTag);
  if(error < 0)
  {
    return error;
  }

  error = WriteZarrDataId(groupWriter, m_UnsharedEdgeListId, H5Constants::k_UnsharedEdgeListTag);
  if(error < 0)
  {
    return error;
  }

  return error;
}
#endif

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
