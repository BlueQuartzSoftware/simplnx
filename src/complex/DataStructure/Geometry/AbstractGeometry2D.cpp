#include "AbstractGeometry2D.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"

using namespace complex;

AbstractGeometry2D::AbstractGeometry2D(DataStructure& ds, std::string name)
: AbstractGeometry(ds, std::move(name))
{
}

AbstractGeometry2D::AbstractGeometry2D(DataStructure& ds, std::string name, IdType importId)
: AbstractGeometry(ds, std::move(name), importId)
{
}

AbstractGeometry2D::AbstractGeometry2D(const AbstractGeometry2D& other) = default;

AbstractGeometry2D::AbstractGeometry2D(AbstractGeometry2D&& other) noexcept
: AbstractGeometry(std::move(other))
, m_VertexListId(other.m_VertexListId)
, m_EdgeListId(other.m_EdgeListId)
, m_UnsharedEdgeListId(other.m_UnsharedEdgeListId)
{
}

AbstractGeometry2D::~AbstractGeometry2D() = default;
DataObject::DataObjectType AbstractGeometry2D::getDataObjectType() const
{
  return DataObjectType::AbstractGeometry2D;
}

void AbstractGeometry2D::resizeVertexList(usize numVertices)
{
  getVertices()->getDataStore()->reshapeTuples({numVertices});
}

void AbstractGeometry2D::setVertices(const SharedVertexList* vertices)
{
  if(vertices == nullptr)
  {
    m_VertexListId.reset();
    return;
  }
  m_VertexListId = vertices->getId();
}

AbstractGeometry::SharedVertexList* AbstractGeometry2D::getVertices()
{
  DataObject* data = getDataStructure()->getData(m_VertexListId);
  return dynamic_cast<SharedVertexList*>(data);
}

const AbstractGeometry::SharedVertexList* AbstractGeometry2D::getVertices() const
{
  const DataObject* data = getDataStructure()->getData(m_VertexListId);
  return dynamic_cast<const SharedVertexList*>(data);
}

DataObject::IdType AbstractGeometry2D::getVertListId() const
{
  return m_VertexListId.value();
}

AbstractGeometry::SharedEdgeList* AbstractGeometry2D::getEdges()
{
  return dynamic_cast<SharedEdgeList*>(getDataStructure()->getData(m_EdgeListId));
}

const AbstractGeometry::SharedEdgeList* AbstractGeometry2D::getEdges() const
{
  return dynamic_cast<const SharedEdgeList*>(getDataStructure()->getData(m_EdgeListId));
}

usize AbstractGeometry2D::getNumberOfEdges() const
{
  const SharedEdgeList* edges = getEdges();
  if(!edges)
  {
    return 0;
  }
  return edges->getNumberOfTuples();
}

void AbstractGeometry2D::setVertsAtEdge(usize edgeId, const usize verts[2])
{
  SharedEdgeList* edges = getEdges();
  if(!edges)
  {
    return;
  }
  (*edges)[edgeId * 2] = verts[0];
  (*edges)[edgeId * 2 + 1] = verts[1];
}

void AbstractGeometry2D::getVertsAtEdge(usize edgeId, usize verts[2]) const
{
  const SharedEdgeList* edges = getEdges();
  if(!edges)
  {
    return;
  }
  verts[0] = edges->at(edgeId * 2);
  verts[1] = edges->at(edgeId * 2 + 1);
}

void AbstractGeometry2D::deleteEdges()
{
  getDataStructure()->removeData(m_EdgeListId);
  m_EdgeListId.reset();
}

const AbstractGeometry::SharedEdgeList* AbstractGeometry2D::getUnsharedEdges() const
{
  return dynamic_cast<const SharedEdgeList*>(getDataStructure()->getData(m_UnsharedEdgeListId));
}

void AbstractGeometry2D::deleteUnsharedEdges()
{
  getDataStructure()->removeData(m_UnsharedEdgeListId);
  m_UnsharedEdgeListId.reset();
}

void AbstractGeometry2D::setEdges(const SharedEdgeList* edges)
{
  if(!edges)
  {
    m_EdgeListId.reset();
    return;
  }
  m_EdgeListId = edges->getId();
}

void AbstractGeometry2D::setUnsharedEdges(const SharedEdgeList* bEdgeList)
{
  if(!bEdgeList)
  {
    m_UnsharedEdgeListId.reset();
    return;
  }
  m_UnsharedEdgeListId = bEdgeList->getId();
}

H5::ErrorType AbstractGeometry2D::writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter) const
{
  auto groupWriter = parentGroupWriter.createGroupWriter(getName());
  auto errorCode = writeH5ObjectAttributes(dataStructureWriter, groupWriter);
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

  return errorCode;
}
