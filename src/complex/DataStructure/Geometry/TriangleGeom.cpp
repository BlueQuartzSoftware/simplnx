#include "TriangleGeom.hpp"

#include <stdexcept>

#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/DynamicListArray.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;

TriangleGeom::TriangleGeom(DataStructure& ds, std::string name)
: INodeGeometry2D(ds, std::move(name))
{
}

TriangleGeom::TriangleGeom(DataStructure& ds, std::string name, IdType importId)
: INodeGeometry2D(ds, std::move(name), importId)
{
}

TriangleGeom::TriangleGeom(const TriangleGeom& other)
: INodeGeometry2D(other)
{
}

TriangleGeom::TriangleGeom(TriangleGeom&& other) noexcept
: INodeGeometry2D(std::move(other))
{
}

TriangleGeom::~TriangleGeom() noexcept = default;

IGeometry::Type TriangleGeom::getGeomType() const
{
  return IGeometry::Type::Triangle;
}

DataObject::Type TriangleGeom::getDataObjectType() const
{
  return DataObject::Type::TriangleGeom;
}

TriangleGeom* TriangleGeom::Create(DataStructure& ds, std::string name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<TriangleGeom>(new TriangleGeom(ds, std::move(name)));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

TriangleGeom* TriangleGeom::Import(DataStructure& ds, std::string name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<TriangleGeom>(new TriangleGeom(ds, std::move(name), importId));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

std::string TriangleGeom::getTypeName() const
{
  return "TriangleGeom";
}

DataObject* TriangleGeom::shallowCopy()
{
  return new TriangleGeom(*this);
}

DataObject* TriangleGeom::deepCopy()
{
  return new TriangleGeom(*this);
}

void TriangleGeom::setVertexIdsForFace(usize faceId, usize verts[3])
{
  auto& faces = getFacesRef();
  const usize offset = faceId * k_NumVerts;
  for(usize i = 0; i < k_NumVerts; i++)
  {
    faces[offset + i] = verts[i];
  }
}

void TriangleGeom::getVertexIdsForFace(usize faceId, usize verts[3]) const
{
  auto& faces = getFacesRef();
  const usize offset = faceId * k_NumVerts;
  for(usize i = 0; i < k_NumVerts; i++)
  {
    verts[i] = faces.at(offset + i);
  }
}

void TriangleGeom::getVertexCoordsForFace(usize faceId, Point3D<float32>& vert1, Point3D<float32>& vert2, Point3D<float32>& vert3) const
{
  usize verts[k_NumVerts];
  getVertexIdsForFace(faceId, verts);
  vert1 = getCoords(verts[0]);
  vert2 = getCoords(verts[1]);
  vert3 = getCoords(verts[2]);
}

usize TriangleGeom::getNumberOfFaces() const
{
  auto& faces = getFacesRef();
  return faces.getNumberOfTuples();
}

usize TriangleGeom::getNumberOfElements() const
{
  return getFacesRef().getNumberOfTuples();
}

IGeometry::StatusCode TriangleGeom::findElementSizes()
{
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfFaces()}, std::vector<usize>{1}, 0.0f);
  Float32Array* triangleSizes = DataArray<float32>::Create(*getDataStructure(), "Triangle Areas", std::move(dataStore), getId());
  GeometryHelpers::Topology::Find2DElementAreas(getFaces(), getVertices(), triangleSizes);
  if(triangleSizes == nullptr)
  {
    m_ElementSizesId.reset();
    return -1;
  }
  m_ElementSizesId = triangleSizes->getId();
  return 1;
}

IGeometry::StatusCode TriangleGeom::findElementsContainingVert()
{
  auto trianglesContainingVert = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), "Triangles Containing Vert", getId());
  GeometryHelpers::Connectivity::FindElementsContainingVert<uint16, MeshIndexType>(getFaces(), trianglesContainingVert, getNumberOfVertices());
  if(trianglesContainingVert == nullptr)
  {
    m_ElementContainingVertId.reset();
    return -1;
  }
  m_ElementContainingVertId = trianglesContainingVert->getId();
  return 1;
}

IGeometry::StatusCode TriangleGeom::findElementNeighbors()
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
  auto triangleNeighbors = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), "Triangle Neighbors", getId());
  err = GeometryHelpers::Connectivity::FindElementNeighbors<uint16, MeshIndexType>(getFaces(), getElementsContainingVert(), triangleNeighbors, IGeometry::Type::Triangle);
  if(triangleNeighbors == nullptr)
  {
    m_ElementNeighborsId.reset();
    return -1;
  }
  m_ElementNeighborsId = triangleNeighbors->getId();
  return err;
}

IGeometry::StatusCode TriangleGeom::findElementCentroids()
{
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfFaces()}, std::vector<usize>{3}, 0.0f);
  auto triangleCentroids = DataArray<float32>::Create(*getDataStructure(), "Triangle Centroids", std::move(dataStore), getId());
  GeometryHelpers::Topology::FindElementCentroids(getFaces(), getVertices(), triangleCentroids);
  if(triangleCentroids == nullptr)
  {
    m_ElementCentroidsId.reset();
    return -1;
  }
  m_ElementCentroidsId = triangleCentroids->getId();
  return 1;
}

complex::Point3D<float64> TriangleGeom::getParametricCenter() const
{
  return {1.0 / 3.0, 1.0 / 3.0, 0.0};
}

void TriangleGeom::getShapeFunctions([[maybe_unused]] const Point3D<float64>& pCoords, float64* shape) const
{
  // r derivatives
  shape[0] = -1.0;
  shape[1] = 1.0;
  shape[2] = 0.0;

  // s derivatives
  shape[3] = -1.0;
  shape[4] = 0.0;
  shape[5] = 1.0;
}

void TriangleGeom::setCoords(usize vertId, const Point3D<float32>& coords)
{
  auto& vertices = getVerticesRef();
  const usize offset = vertId * 3;
  for(usize i = 0; i < 3; i++)
  {
    vertices[offset + i] = coords[i];
  }
}

Point3D<float32> TriangleGeom::getCoords(usize vertId) const
{
  auto& vertices = getVerticesRef();
  const usize offset = vertId * 3;
  Point3D<float32> coords;
  for(usize i = 0; i < 3; i++)
  {
    coords[i] = vertices.at(offset + i);
  }
  return coords;
}

IGeometry::StatusCode TriangleGeom::findEdges()
{
  auto dataStore = std::make_unique<DataStore<uint64>>(std::vector<usize>{0}, std::vector<usize>{2}, 0);
  DataArray<uint64>* edgeList = DataArray<uint64>::Create(*getDataStructure(), "Edge List", std::move(dataStore), getId());
  GeometryHelpers::Connectivity::Find2DElementEdges(getFaces(), edgeList);
  if(edgeList == nullptr)
  {
    m_EdgeListId.reset();
    return -1;
  }
  m_EdgeListId = edgeList->getId();
  return 1;
}

void TriangleGeom::getVertCoordsAtEdge(usize edgeId, Point3D<float32>& vert1, Point3D<float32>& vert2) const
{
  usize verts[2];
  getVertsAtEdge(edgeId, verts);
  vert1 = getCoords(verts[0]);
  vert2 = getCoords(verts[1]);
}

IGeometry::StatusCode TriangleGeom::findUnsharedEdges()
{
  auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{2}, 0);
  auto* unsharedEdgeList = DataArray<MeshIndexType>::Create(*getDataStructure(), "Unshared Edge List", std::move(dataStore), getId());
  GeometryHelpers::Connectivity::Find2DUnsharedEdges(getFaces(), unsharedEdgeList);
  if(unsharedEdgeList == nullptr)
  {
    m_UnsharedEdgeListId.reset();
    return -1;
  }
  m_UnsharedEdgeListId = unsharedEdgeList->getId();
  return 1;
}

H5::ErrorType TriangleGeom::readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, bool preflight)
{
  m_FaceListId = ReadH5DataId(groupReader, H5Constants::k_TriangleListTag);
  m_ElementContainingVertId = ReadH5DataId(groupReader, H5Constants::k_TrianglesContainingVertTag);
  m_ElementNeighborsId = ReadH5DataId(groupReader, H5Constants::k_TriangleNeighborsTag);
  m_ElementCentroidsId = ReadH5DataId(groupReader, H5Constants::k_TriangleCentroidsTag);
  m_ElementSizesId = ReadH5DataId(groupReader, H5Constants::k_TriangleSizesTag);

  return INodeGeometry2D::readHdf5(dataStructureReader, groupReader, preflight);
}

H5::ErrorType TriangleGeom::writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const
{
  auto errorCode = INodeGeometry2D::writeHdf5(dataStructureWriter, parentGroupWriter, importable);
  if(errorCode < 0)
  {
    return errorCode;
  }

  auto groupWriter = parentGroupWriter.createGroupWriter(getName());
  errorCode = writeH5ObjectAttributes(dataStructureWriter, groupWriter, importable);
  if(errorCode < 0)
  {
    return errorCode;
  }

  // Write DataObject IDs
  errorCode = WriteH5DataId(groupWriter, m_FaceListId, H5Constants::k_TriangleListTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_ElementContainingVertId, H5Constants::k_TrianglesContainingVertTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_ElementNeighborsId, H5Constants::k_TriangleNeighborsTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_ElementCentroidsId, H5Constants::k_TriangleCentroidsTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_ElementSizesId, H5Constants::k_TriangleSizesTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  return errorCode;
}
