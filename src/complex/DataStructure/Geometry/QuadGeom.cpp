#include "QuadGeom.hpp"

#include <stdexcept>

#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;

QuadGeom::QuadGeom(DataStructure& ds, std::string name)
: INodeGeometry2D(ds, std::move(name))
{
}

QuadGeom::QuadGeom(DataStructure& ds, std::string name, IdType importId)
: INodeGeometry2D(ds, std::move(name), importId)
{
}

QuadGeom::QuadGeom(const QuadGeom& other)
: INodeGeometry2D(other)
{
}

QuadGeom::QuadGeom(QuadGeom&& other) noexcept
: INodeGeometry2D(std::move(other))
{
}

QuadGeom::~QuadGeom() noexcept = default;

IGeometry::Type QuadGeom::getGeomType() const
{
  return IGeometry::Type::Quad;
}

DataObject::Type QuadGeom::getDataObjectType() const
{
  return DataObject::Type::QuadGeom;
}

QuadGeom* QuadGeom::Create(DataStructure& ds, std::string name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<QuadGeom>(new QuadGeom(ds, std::move(name)));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

QuadGeom* QuadGeom::Import(DataStructure& ds, std::string name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<QuadGeom>(new QuadGeom(ds, std::move(name), importId));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

std::string QuadGeom::getTypeName() const
{
  return "QuadGeom";
}

DataObject* QuadGeom::shallowCopy()
{
  return new QuadGeom(*this);
}

DataObject* QuadGeom::deepCopy()
{
  return new QuadGeom(*this);
}

void QuadGeom::setVertexIdsForFace(usize faceId, usize verts[4])
{
  auto& faces = getFacesRef();
  const usize offset = faceId * k_NumVerts;
  for(usize i = 0; i < k_NumVerts; i++)
  {
    faces[offset + i] = verts[i];
  }
}

void QuadGeom::getVertexIdsForFace(usize faceId, usize verts[k_NumVerts]) const
{
  auto& faces = getFacesRef();
  const usize offset = faceId * k_NumVerts;
  for(usize i = 0; i < k_NumVerts; i++)
  {
    verts[i] = faces.at(offset + i);
  }
}

void QuadGeom::getVertexCoordsForFace(usize faceId, complex::Point3D<float32>& vert1, complex::Point3D<float32>& vert2, complex::Point3D<float32>& vert3, complex::Point3D<float32>& vert4) const
{
  auto& faces = getFacesRef();
  usize verts[4];
  getVertexIdsForFace(faceId, verts);
  vert1 = getCoords(verts[0]);
  vert2 = getCoords(verts[1]);
  vert3 = getCoords(verts[2]);
  vert4 = getCoords(verts[3]);
}

usize QuadGeom::getNumberOfQuads() const
{
  auto& quads = getFacesRef();
  return quads.getNumberOfTuples();
}

usize QuadGeom::getNumberOfElements() const
{
  return getNumberOfQuads();
}

IGeometry::StatusCode QuadGeom::findElementSizes()
{
  auto dataStore = std::make_unique<DataStore<float32>>(getNumberOfQuads(), 0.0f);
  Float32Array* quadSizes = DataArray<float32>::Create(*getDataStructure(), "Quad Areas", std::move(dataStore), getId());
  GeometryHelpers::Topology::Find2DElementAreas(getFaces(), getVertices(), quadSizes);
  if(quadSizes == nullptr)
  {
    m_ElementSizesId.reset();
    return -1;
  }
  m_ElementSizesId = quadSizes->getId();
  return 1;
}

IGeometry::StatusCode QuadGeom::findElementsContainingVert()
{
  auto quadsContainingVert = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), "Quads Containing Vert", getId());
  GeometryHelpers::Connectivity::FindElementsContainingVert<uint16, MeshIndexType>(getFaces(), quadsContainingVert, getNumberOfVertices());
  if(quadsContainingVert == nullptr)
  {
    m_ElementContainingVertId.reset();
    return -1;
  }
  m_ElementContainingVertId = quadsContainingVert->getId();
  return 1;
}

IGeometry::StatusCode QuadGeom::findElementNeighbors()
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
  auto quadNeighbors = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), "Quad Neighbors", getId());
  err = GeometryHelpers::Connectivity::FindElementNeighbors<uint16, MeshIndexType>(getFaces(), getElementsContainingVert(), quadNeighbors, IGeometry::Type::Quad);
  if(quadNeighbors == nullptr)
  {
    m_ElementNeighborsId.reset();
    return -1;
  }
  m_ElementNeighborsId = quadNeighbors->getId();
  return err;
}

IGeometry::StatusCode QuadGeom::findElementCentroids()
{
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfQuads()}, std::vector<usize>{3}, 0.0f);
  auto quadCentroids = DataArray<float32>::Create(*getDataStructure(), "Quad Centroids", std::move(dataStore), getId());
  GeometryHelpers::Topology::FindElementCentroids(getFaces(), getVertices(), quadCentroids);
  if(quadCentroids == nullptr)
  {
    m_ElementCentroidsId.reset();
    return -1;
  }
  m_ElementCentroidsId = quadCentroids->getId();
  return 1;
}

Point3D<float64> QuadGeom::getParametricCenter() const
{
  return {0.5, 0.5, 0.0};
}

void QuadGeom::getShapeFunctions(const Point3D<float64>& pCoords, float64* shape) const
{
  float64 rm = 0.0;
  float64 sm = 0.0;

  rm = 1.0 - pCoords[0];
  sm = 1.0 - pCoords[1];

  shape[0] = -sm;
  shape[1] = sm;
  shape[2] = pCoords[1];
  shape[3] = -pCoords[1];
  shape[4] = -rm;
  shape[5] = -pCoords[0];
  shape[6] = pCoords[0];
  shape[7] = rm;
}

void QuadGeom::setCoords(usize vertId, const Point3D<float32>& coord)
{
  auto& verts = getVerticesRef();
  usize index = vertId * 4;
  for(usize i = 0; i < 3; i++)
  {
    verts[index + i] = coord[i];
  }
}

Point3D<float32> QuadGeom::getCoords(usize vertId) const
{
  auto& verts = getVerticesRef();
  usize index = vertId * 4;
  return {verts.at(index), verts.at(index + 1), verts.at(index + 2)};
}

void QuadGeom::getVertCoordsAtEdge(usize edgeId, Point3D<float32>& vert1, Point3D<float32>& vert2) const
{
  auto& vertices = getVerticesRef();

  usize verts[2];
  getVertsAtEdge(edgeId, verts);

  for(usize i = 0; i < 3; i++)
  {
    vert1[i] = vertices.at(verts[0] * 4 + i);
    vert2[i] = vertices.at(verts[1] * 4 + i);
  }
}

IGeometry::StatusCode QuadGeom::findEdges()
{
  auto* edgeList = createSharedEdgeList(0);
  GeometryHelpers::Connectivity::Find2DElementEdges(getFaces(), edgeList);
  if(edgeList == nullptr)
  {
    m_EdgeListId.reset();
    return -1;
  }
  m_EdgeListId = edgeList->getId();
  return 1;
}

IGeometry::StatusCode QuadGeom::findUnsharedEdges()
{
  auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{2}, 0);
  auto unsharedEdgeList = DataArray<MeshIndexType>::Create(*getDataStructure(), "Unshared Edge List", std::move(dataStore), getId());
  GeometryHelpers::Connectivity::Find2DUnsharedEdges(getFaces(), unsharedEdgeList);
  if(unsharedEdgeList == nullptr)
  {
    m_UnsharedEdgeListId.reset();
    return -1;
  }
  m_UnsharedEdgeListId = unsharedEdgeList->getId();
  return 1;
}
