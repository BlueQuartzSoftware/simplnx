#include "EdgeGeom.hpp"

#include <stdexcept>

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;

EdgeGeom::EdgeGeom(DataStructure& ds, std::string name)
: INodeGeometry1D(ds, std::move(name))
{
}

EdgeGeom::EdgeGeom(DataStructure& ds, std::string name, IdType importId)
: INodeGeometry1D(ds, std::move(name), importId)
{
}

EdgeGeom::EdgeGeom(const EdgeGeom&) = default;

EdgeGeom::EdgeGeom(EdgeGeom&&) noexcept = default;

EdgeGeom::~EdgeGeom() noexcept = default;

DataObject::Type EdgeGeom::getDataObjectType() const
{
  return DataObject::Type::EdgeGeom;
}

EdgeGeom* EdgeGeom::Create(DataStructure& ds, std::string name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<EdgeGeom>(new EdgeGeom(ds, std::move(name)));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

EdgeGeom* EdgeGeom::Import(DataStructure& ds, std::string name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<EdgeGeom>(new EdgeGeom(ds, std::move(name), importId));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

IGeometry::Type EdgeGeom::getGeomType() const
{
  return IGeometry::Type::Edge;
}

std::string EdgeGeom::getTypeName() const
{
  return "EdgeGeom";
}

DataObject* EdgeGeom::shallowCopy()
{
  return new EdgeGeom(*this);
}

DataObject* EdgeGeom::deepCopy()
{
  return new EdgeGeom(*this);
}

void EdgeGeom::setCoords(usize vertId, const Point3D<float32>& coords)
{
  auto& vertices = getVerticesRef();
  for(usize i = 0; i < 3; i++)
  {
    vertices[vertId * 3 + i] = coords[i];
  }
}

Point3D<float32> EdgeGeom::getCoords(usize vertId) const
{
  auto& vertices = getVerticesRef();
  Point3D<float32> coord;
  for(usize i = 0; i < 3; i++)
  {
    coord[i] = vertices[vertId * 3 + i];
  }
  return coord;
}

void EdgeGeom::setVertsAtEdge(usize edgeId, const usize verts[2])
{
  auto& edges = getEdgesRef();
  usize numEdges = edges.getNumberOfTuples();
  if(edgeId >= numEdges)
  {
    return;
  }

  for(usize i = 0; i < 2; i++)
  {
    edges[edgeId + i] = verts[i];
  }
}

void EdgeGeom::getVertsAtEdge(usize edgeId, usize verts[2]) const
{
  auto& edges = getEdgesRef();
  usize numEdges = edges.getNumberOfTuples();
  if(edgeId >= numEdges)
  {
    return;
  }

  for(usize i = 0; i < 2; i++)
  {
    verts[i] = edges[edgeId + i];
  }
}

void EdgeGeom::getVertCoordsAtEdge(usize edgeId, Point3D<float32>& vert1, Point3D<float32>& vert2) const
{
  auto& edges = getEdgesRef();
  usize numEdges = edges.getNumberOfTuples();
  if(edgeId >= numEdges)
  {
    return;
  }

  auto& vertices = getVerticesRef();

  MeshIndexType vert1Id = edges[edgeId];
  MeshIndexType vert2Id = edges[edgeId + 1];

  vert1 = &vertices[vert1Id];
  vert2 = &vertices[vert2Id];
}

usize EdgeGeom::getNumberOfElements() const
{
  auto& edges = getEdgesRef();
  return edges.getNumberOfTuples();
}

IGeometry::StatusCode EdgeGeom::findElementSizes()
{
  auto dataStore = std::make_unique<DataStore<float32>>(getNumberOfElements(), 0.0f);
  auto* sizes = DataArray<float32>::Create(*getDataStructure(), "Edge Lengths", std::move(dataStore), getId());
  if(sizes == nullptr)
  {
    return -1;
  }
  m_ElementSizesId = sizes->getId();

  Point3D<float32> vert0 = {0.0f, 0.0f, 0.0f};
  Point3D<float32> vert1 = {0.0f, 0.0f, 0.0f};

  for(usize i = 0; i < getNumberOfEdges(); i++)
  {
    getVertCoordsAtEdge(i, vert0, vert1);
    float32 length = 0.0f;
    for(usize j = 0; j < 3; j++)
    {
      length += (vert0[j] - vert1[j]) * (vert0[j] - vert1[j]);
    }
    (*sizes)[i] = std::sqrt(length);
  }

  return 1;
}

IGeometry::StatusCode EdgeGeom::findElementsContainingVert()
{
  auto* containsVert = ElementDynamicList::Create(*getDataStructure(), "Edges Containing Vert", getId());
  if(containsVert == nullptr)
  {
    return -1;
  }
  GeometryHelpers::Connectivity::FindElementsContainingVert<uint16, MeshIndexType>(getEdges(), containsVert, getNumberOfVertices());
  if(getElementsContainingVert() == nullptr)
  {
    return -1;
  }
  m_ElementContainingVertId = containsVert->getId();
  return 1;
}

IGeometry::StatusCode EdgeGeom::findElementNeighbors()
{
  StatusCode err = 0;
  if(getElementsContainingVert() == nullptr)
  {
    err = findElementsContainingVert();
    if(err < 0)
    {
      return err;
    }
  }
  auto* edgeNeighbors = ElementDynamicList::Create(*getDataStructure(), "Edge Neighbors", getId());
  if(edgeNeighbors == nullptr)
  {
    err = -1;
    return err;
  }
  m_ElementNeighborsId = edgeNeighbors->getId();
  err = GeometryHelpers::Connectivity::FindElementNeighbors<uint16, MeshIndexType>(getEdges(), getElementsContainingVert(), edgeNeighbors, IGeometry::Type::Edge);
  if(getElementNeighbors() == nullptr)
  {
    m_ElementNeighborsId.reset();
    err = -1;
  }
  return err;
}

IGeometry::StatusCode EdgeGeom::findElementCentroids()
{
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfElements()}, std::vector<usize>{3}, 0.0f);
  auto* edgeCentroids = DataArray<float32>::Create(*getDataStructure(), "Edge Centroids", std::move(dataStore), getId());
  GeometryHelpers::Topology::FindElementCentroids(getEdges(), getVertices(), edgeCentroids);
  if(getElementCentroids() == nullptr)
  {
    return -1;
  }
  m_ElementCentroidsId = edgeCentroids->getId();
  return 1;
}

Point3D<float64> EdgeGeom::getParametricCenter() const
{
  return {0.5, 0.0, 0.0};
}

void EdgeGeom::getShapeFunctions([[maybe_unused]] const Point3D<float64>& pCoords, float64* shape) const
{
  shape[0] = -1.0;
  shape[1] = 1.0;
}

H5::ErrorType EdgeGeom::readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, bool preflight)
{
  m_VertexListId = ReadH5DataId(groupReader, H5Constants::k_VertexListTag);
  m_EdgeListId = ReadH5DataId(groupReader, H5Constants::k_EdgeListTag);
  m_ElementContainingVertId = ReadH5DataId(groupReader, H5Constants::k_EdgesContainingVertTag);
  m_ElementNeighborsId = ReadH5DataId(groupReader, H5Constants::k_EdgeNeighborsTag);
  m_ElementCentroidsId = ReadH5DataId(groupReader, H5Constants::k_EdgeCentroidTag);
  m_ElementSizesId = ReadH5DataId(groupReader, H5Constants::k_EdgeSizesTag);

  return BaseGroup::readHdf5(dataStructureReader, groupReader, preflight);
}

H5::ErrorType EdgeGeom::writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const
{
  auto groupWriter = parentGroupWriter.createGroupWriter(getName());
  auto err = writeH5ObjectAttributes(dataStructureWriter, groupWriter, importable);
  if(err < 0)
  {
    return err;
  }

  auto errorCode = WriteH5DataId(groupWriter, m_VertexListId, H5Constants::k_VertexListTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_EdgeListId, H5Constants::k_EdgeListTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_ElementContainingVertId, H5Constants::k_EdgesContainingVertTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_ElementNeighborsId, H5Constants::k_EdgeNeighborsTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_ElementCentroidsId, H5Constants::k_EdgeCentroidTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_ElementSizesId, H5Constants::k_EdgeSizesTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  return errorCode;
}
