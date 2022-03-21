#include "AbstractGeometry3D.hpp"

#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"

using namespace complex;

AbstractGeometry3D::AbstractGeometry3D(DataStructure& ds, std::string name)
: AbstractGeometry(ds, std::move(name))
{
}

AbstractGeometry3D::AbstractGeometry3D(DataStructure& ds, std::string name, IdType importId)
: AbstractGeometry(ds, std::move(name), importId)
{
}

AbstractGeometry3D::AbstractGeometry3D(const AbstractGeometry3D& other)
: AbstractGeometry(other)
, m_VertexListId(other.m_VertexListId)
, m_EdgeListId(other.m_EdgeListId)
, m_UnsharedEdgeListId(other.m_UnsharedEdgeListId)
, m_FaceListId(other.m_FaceListId)
, m_UnsharedFaceListId(other.m_UnsharedFaceListId)
{
}

AbstractGeometry3D::AbstractGeometry3D(AbstractGeometry3D&& other) noexcept
: AbstractGeometry(std::move(other))
, m_VertexListId(std::move(other.m_VertexListId))
, m_EdgeListId(std::move(other.m_EdgeListId))
, m_UnsharedEdgeListId(std::move(other.m_UnsharedEdgeListId))
, m_FaceListId(std::move(other.m_FaceListId))
, m_UnsharedFaceListId(std::move(other.m_UnsharedFaceListId))
{
}

AbstractGeometry3D::~AbstractGeometry3D() = default;
DataObject::Type AbstractGeometry3D::getDataObjectType() const
{
  return DataObject::Type::AbstractGeometry3D;
}

AbstractGeometry3D::SharedQuadList* AbstractGeometry3D::createSharedQuadList(usize numQuads)
{
  auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{numQuads}, std::vector<usize>{4}, 0);
  SharedQuadList* quads = DataArray<MeshIndexType>::Create(*getDataStructure(), "Shared Quad List", std::move(dataStore), getId());
  dataStore->fill(0);
  return quads;
}

AbstractGeometry3D::SharedTriList* AbstractGeometry3D::createSharedTriList(usize numTris)
{
  auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{numTris}, std::vector<usize>{3}, 0);
  SharedTriList* triangles = DataArray<MeshIndexType>::Create(*getDataStructure(), "Shared Tri List", std::move(dataStore), getId());
  triangles->getDataStore()->fill(0);
  return triangles;
}

void AbstractGeometry3D::resizeVertexList(usize numVertices)
{
  auto vertices = dynamic_cast<SharedVertexList*>(getDataStructure()->getData(m_VertexListId));
  if(!vertices)
  {
    return;
  }
  vertices->getDataStore()->reshapeTuples({numVertices});
}

void AbstractGeometry3D::setVertices(const SharedVertexList* vertices)
{
  if(vertices == nullptr)
  {
    m_VertexListId.reset();
    return;
  }

  m_VertexListId = vertices->getId();
}

AbstractGeometry::SharedVertexList* AbstractGeometry3D::getVertices()
{
  return dynamic_cast<SharedVertexList*>(getDataStructure()->getData(m_VertexListId));
}

const AbstractGeometry::SharedVertexList* AbstractGeometry3D::getVertices() const
{
  return dynamic_cast<const SharedVertexList*>(getDataStructure()->getData(m_VertexListId));
}

void AbstractGeometry3D::setCoords(usize vertId, const complex::Point3D<float32>& coords)
{
  auto vertices = dynamic_cast<Float32Array*>(getDataStructure()->getData(m_VertexListId));
  if(!vertices)
  {
    return;
  }

  usize index = vertId * 3;
  for(usize i = 0; i < 3; i++)
  {
    (*vertices)[index + i] = coords[i];
  }
}

complex::Point3D<float32> AbstractGeometry3D::getCoords(usize vertId) const
{
  auto vertices = dynamic_cast<const Float32Array*>(getDataStructure()->getData(m_VertexListId));
  if(!vertices)
  {
    return Point3D<float32>();
  }

  usize index = vertId * 3;
  auto x = (*vertices)[index];
  auto y = (*vertices)[index + 1];
  auto z = (*vertices)[index + 2];
  return Point3D<float32>(x, y, z);
}

usize AbstractGeometry3D::getNumberOfVertices() const
{
  auto vertices = dynamic_cast<const Float32Array*>(getDataStructure()->getData(m_VertexListId));
  if(!vertices)
  {
    return 0;
  }
  return vertices->getNumberOfTuples();
}

void AbstractGeometry3D::resizeEdgeList(usize numEdges)
{
  auto edges = getEdges();
  if(!edges)
  {
    return;
  }
  edges->getDataStore()->reshapeTuples({numEdges});
}

AbstractGeometry::SharedEdgeList* AbstractGeometry3D::getEdges()
{
  return dynamic_cast<SharedEdgeList*>(getDataStructure()->getData(m_EdgeListId));
}

const AbstractGeometry::SharedEdgeList* AbstractGeometry3D::getEdges() const
{
  return dynamic_cast<const SharedEdgeList*>(getDataStructure()->getData(m_EdgeListId));
}

void AbstractGeometry3D::setVertsAtEdge(usize edgeId, const usize verts[2])
{
  auto edges = dynamic_cast<SharedEdgeList*>(getDataStructure()->getData(m_EdgeListId));
  if(!edges)
  {
    return;
  }
  usize index = edgeId * 2;
  (*edges)[index] = verts[0];
  (*edges)[index + 1] = verts[1];
}

void AbstractGeometry3D::getVertsAtEdge(usize edgeId, usize verts[2]) const
{
  auto edges = getEdges();
  if(!edges)
  {
    return;
  }
  usize index = edgeId * 2;
  verts[0] = (*edges)[index];
  verts[1] = (*edges)[index + 1];
}

void AbstractGeometry3D::getVertCoordsAtEdge(usize edgeId, complex::Point3D<float32>& vert1, complex::Point3D<float32>& vert2) const
{
  auto edges = getEdges();
  if(!edges)
  {
    return;
  }

  auto vertices = getVertices();
  if(!vertices)
  {
    return;
  }

  usize verts[2];
  getVertsAtEdge(edgeId, verts);

  for(usize i = 0; i < 3; i++)
  {
    vert1[i] = (*vertices)[verts[0] * 3 + i];
    vert2[i] = (*vertices)[verts[1] * 3 + i];
  }
}

usize AbstractGeometry3D::getNumberOfEdges() const
{
  auto edges = getEdges();
  if(!edges)
  {
    return 0;
  }

  return edges->getNumberOfTuples();
}

void AbstractGeometry3D::deleteEdges()
{
  getDataStructure()->removeData(m_EdgeListId);
  m_EdgeListId.reset();
}

void AbstractGeometry3D::deleteFaces()
{
  getDataStructure()->removeData(m_FaceListId);
  m_FaceListId.reset();
}

const AbstractGeometry::SharedEdgeList* AbstractGeometry3D::getUnsharedEdges() const
{
  return dynamic_cast<const SharedEdgeList*>(getDataStructure()->getData(m_UnsharedEdgeListId));
}

void AbstractGeometry3D::deleteUnsharedEdges()
{
  getDataStructure()->removeData(m_UnsharedEdgeListId);
}

const AbstractGeometry::SharedEdgeList* AbstractGeometry3D::getUnsharedFaces() const
{
  return dynamic_cast<const SharedEdgeList*>(getDataStructure()->getData(m_UnsharedFaceListId));
}

void AbstractGeometry3D::deleteUnsharedFaces()
{
  getDataStructure()->removeData(m_UnsharedFaceListId);
  m_UnsharedFaceListId.reset();
}

usize AbstractGeometry3D::getNumberOfFaces() const
{
  auto faces = getFaces();
  if(!faces)
  {
    return 0;
  }
  return faces->getNumberOfTuples();
}

AbstractGeometry3D::SharedFaceList* AbstractGeometry3D::getFaces()
{
  return dynamic_cast<SharedFaceList*>(getDataStructure()->getData(m_FaceListId));
}

const AbstractGeometry3D::SharedFaceList* AbstractGeometry3D::getFaces() const
{
  return dynamic_cast<const SharedFaceList*>(getDataStructure()->getData(m_FaceListId));
}

void AbstractGeometry3D::setEdges(const SharedEdgeList* edges)
{
  if(!edges)
  {
    m_EdgeListId.reset();
    return;
  }
  m_EdgeListId = edges->getId();
}

void AbstractGeometry3D::setFaces(const SharedFaceList* faces)
{
  if(!faces)
  {
    m_FaceListId.reset();
    return;
  }
  m_FaceListId = faces->getId();
}

void AbstractGeometry3D::setUnsharedEdges(const SharedEdgeList* bEdgeList)
{
  if(!bEdgeList)
  {
    m_UnsharedEdgeListId.reset();
    return;
  }
  m_UnsharedEdgeListId = bEdgeList->getId();
}

void AbstractGeometry3D::setUnsharedFaces(const SharedFaceList* bFaceList)
{
  if(!bFaceList)
  {
    m_UnsharedFaceListId.reset();
    return;
  }
  m_UnsharedFaceListId = bFaceList->getId();
}

H5::ErrorType AbstractGeometry3D::writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const
{
  auto groupWriter = parentGroupWriter.createGroupWriter(getName());
  auto errorCode = writeH5ObjectAttributes(dataStructureWriter, groupWriter, importable);
  if(errorCode < 0)
  {
    return errorCode;
  }
  errorCode = WriteH5DataId(groupWriter, m_VertexListId, H5Constants::k_VertexListTag);
  if(errorCode < 0)
  {
    return errorCode;
  }
  errorCode = WriteH5DataId(groupWriter, m_EdgeListId, H5Constants::k_EdgeListTag);
  if(errorCode < 0)
  {
    return errorCode;
  }
  errorCode = WriteH5DataId(groupWriter, m_UnsharedEdgeListId, H5Constants::k_UnsharedEdgeListTag);
  if(errorCode < 0)
  {
    return errorCode;
  }
  errorCode = WriteH5DataId(groupWriter, m_FaceListId, H5Constants::k_FaceeListTag);
  if(errorCode < 0)
  {
    return errorCode;
  }
  errorCode = WriteH5DataId(groupWriter, m_UnsharedFaceListId, H5Constants::k_UnsharedFaceeListTag);
  if(errorCode < 0)
  {
    return errorCode;
  }
  return errorCode;
}

void AbstractGeometry3D::checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds)
{
  BaseGroup::checkUpdatedIdsImpl(updatedIds);

  for(const auto& updatedId : updatedIds)
  {
    if(m_VertexListId == updatedId.first)
    {
      m_VertexListId = updatedId.second;
    }

    if(m_EdgeListId == updatedId.first)
    {
      m_EdgeListId = updatedId.second;
    }

    if(m_UnsharedEdgeListId == updatedId.first)
    {
      m_UnsharedEdgeListId = updatedId.second;
    }

    if(m_FaceListId == updatedId.first)
    {
      m_FaceListId = updatedId.second;
    }

    if(m_UnsharedFaceListId == updatedId.first)
    {
      m_UnsharedFaceListId = updatedId.second;
    }
  }
}
