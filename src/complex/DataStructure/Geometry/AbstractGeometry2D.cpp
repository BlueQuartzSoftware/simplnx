#include "AbstractGeometry2D.hpp"
#include "complex/DataStructure/DataStructure.hpp"

using namespace complex;

AbstractGeometry2D::AbstractGeometry2D(DataStructure& ds, const std::string& name)
: AbstractGeometry(ds, name)
{
}

AbstractGeometry2D::AbstractGeometry2D(const AbstractGeometry2D& other)
: AbstractGeometry(other)
, m_VertexListId(other.m_VertexListId)
, m_EdgeListId(other.m_EdgeListId)
, m_UnsharedEdgeListId(other.m_UnsharedEdgeListId)
{
}

AbstractGeometry2D::AbstractGeometry2D(AbstractGeometry2D&& other) noexcept
: AbstractGeometry(std::move(other))
, m_VertexListId(std::move(other.m_VertexListId))
, m_EdgeListId(std::move(other.m_EdgeListId))
, m_UnsharedEdgeListId(std::move(other.m_UnsharedEdgeListId))
{
}

AbstractGeometry2D::~AbstractGeometry2D() = default;

void AbstractGeometry2D::resizeVertexList(size_t numVertices)
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
  auto data = getDataStructure()->getData(m_VertexListId);
  return dynamic_cast<SharedVertexList*>(data);
}

const AbstractGeometry::SharedVertexList* AbstractGeometry2D::getVertices() const
{
  auto data = getDataStructure()->getData(m_VertexListId);
  return dynamic_cast<const SharedVertexList*>(data);
}

AbstractGeometry::SharedEdgeList* AbstractGeometry2D::getEdges()
{
  return dynamic_cast<SharedEdgeList*>(getDataStructure()->getData(m_EdgeListId));
}

const AbstractGeometry::SharedEdgeList* AbstractGeometry2D::getEdges() const
{
  return dynamic_cast<const SharedEdgeList*>(getDataStructure()->getData(m_EdgeListId));
}

size_t AbstractGeometry2D::getNumberOfEdges() const
{
  auto edges = getEdges();
  if(!edges)
  {
    return 0;
  }
  return edges->getNumberOfTuples();
}

void AbstractGeometry2D::setVertsAtEdge(size_t edgeId, const size_t verts[2])
{
  auto edges = getEdges();
  if(!edges)
  {
    return;
  }
  (*edges)[edgeId * 2] = verts[0];
  (*edges)[edgeId * 2 + 1] = verts[1];
}

void AbstractGeometry2D::getVertsAtEdge(size_t edgeId, size_t verts[2]) const
{
  auto edges = getEdges();
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
